//==============================================================================
//  Copyright 2011-2013 Meta Watch Ltd. - http://www.MetaWatch.org/
// 
//  Licensed under the Meta Watch License, Version 1.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//      http://www.MetaWatch.org/licenses/license-1.0.html
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//==============================================================================

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_clock_control.h"
#include "Messages.h"
#include "Countdown.h"
#include "DebugUart.h"
#include "Wrapper.h"
#include "OneSecondTimers.h"
#include "Vibration.h"

typedef struct
{
  unsigned int Timeout;
  unsigned char Repeat;
  unsigned char MsgType;
  unsigned char MsgOpt;
} TimerSetting_t;

static TimerSetting_t const TimerSettings[] =
{
  {TOUT_IDLE_MODE, TOUT_ONCE, ModeTimeoutMsg, NOTIF_MODE},
  {TOUT_APP_MODE, TOUT_ONCE, ModeTimeoutMsg, NOTIF_MODE},
  {TOUT_NOTIF_MODE, TOUT_ONCE, ModeTimeoutMsg, NOTIF_MODE},
  {TOUT_MUSIC_MODE, TOUT_ONCE, ModeTimeoutMsg, NOTIF_MODE},
  {TOUT_MONITOR_BATTERY, REPEAT_FOREVER, MonitorBatteryMsg, MSG_OPT_NONE},
  {TOUT_BACKLIGHT, TOUT_ONCE, SetBacklightMsg, LED_OFF},
  {TOUT_CONN_HFP_MAP_SHORT, TOUT_ONCE, ConnTimeoutMsg, MSG_OPT_NONE},
  {TOUT_CONN_HFP_MAP_LONG, TOUT_ONCE, ConnTimeoutMsg, MSG_OPT_NONE},
  {TOUT_INTERVAL_LONG, TOUT_ONCE, UpdConnParamMsg, LONG},
  {TOUT_HEARTBEAT, TOUT_ONCE, HeartbeatMsg, MSG_OPT_NONE},
  {TOUT_TO_SNIFF, TOUT_ONCE, SniffControlMsg, MSG_OPT_ENTER_SNIFF},
  {TOUT_FIELD_TEST, REPEAT_FOREVER, FieldTestMsg, FIELD_TEST_TIMEOUT},
  {TOUT_COUNT_DOWN, REPEAT_FOREVER, CountdownMsg, CDT_COUNT},
  {TOUT_DISCONNECT, TOUT_ONCE, ConnTimeoutMsg, MSG_OPT_DISCONN},
};

static char const TimerName[][5] =
{
  "Idle", "App", "Ntf", "Musc", "Batt", "Led", "HfpS", "HfpL", "Intv", "Beat",
  "Ring", "Snif", "Ftm", "Ctdn",
};

struct TimerItem
{
  unsigned char Id;
  unsigned char Repeat;
  unsigned int DownCounter;
  struct TimerItem *Next;
};

typedef struct TimerItem Timer_t;
#define TIMER_SIZE    (sizeof(Timer_t))

static Timer_t *TimerList = NULL;
  
void StartTimer(eTimerId Id)
{
  portENTER_CRITICAL();

  Timer_t *pTimer = TimerList;

  while (pTimer)
  {
    if (pTimer->Id == Id)
    {
      pTimer->DownCounter = TimerSettings[Id].Timeout;
      PrintF("RstTmr:%s", TimerName[Id]);
      portEXIT_CRITICAL();
      return;
    }
    else if (pTimer->Next == NULL) break;

    pTimer = pTimer->Next;
  }

  // create a timer
  Timer_t *pNext = (Timer_t *)pvPortMalloc(TIMER_SIZE);
  PrintF("CrtTmr:%s", TimerName[Id]);

  pNext->Id = Id;
  pNext->Repeat = TimerSettings[Id].Repeat;
  pNext->DownCounter = TimerSettings[Id].Timeout;
  pNext->Next = NULL;

  if (TimerList) pTimer->Next = pNext;
  else TimerList = pNext;

  portEXIT_CRITICAL();
}

void StopTimer(eTimerId Id)
{
  Timer_t *pTimer = TimerList;
  Timer_t *pPrev = NULL;

  while (pTimer)
  {
    if (pTimer->Id == Id)
    {
      if (pPrev) pPrev->Next = pTimer->Next;
      else TimerList = pTimer->Next;

      PrintF("StpTmr:%s", TimerName[Id]);
      vPortFree(pTimer);
      break;
    }
    else
    {
      pPrev = pTimer;
      pTimer = pTimer->Next;
    }
  }
}

/* this should be as fast as possible because it happens in interrupt context
 * and it also often occurs when the part is sleeping
 */
unsigned char OneSecondTimerHandlerIsr(void)
{
  unsigned char ExitLpm = 0;
  Timer_t *pTimer = TimerList;
//  Timer_t *pPrev = NULL;

  while (pTimer)
  {
    /* decrement the counter first, then check if the counter == 0 */
    if (--pTimer->DownCounter == 0)
    {
      SendMessageIsr(TimerSettings[pTimer->Id].MsgType, TimerSettings[pTimer->Id].MsgOpt);

      if (pTimer->Repeat != REPEAT_FOREVER) pTimer->Repeat --;

      if (pTimer->Repeat == 0) SendMessageIsr(StopTimerMsg, pTimer->Id);
      else pTimer->DownCounter = TimerSettings[pTimer->Id].Timeout;

      ExitLpm = 1;
    }

//    if (Stop)
//    {
//      if (pPrev) pPrev->Next = pTimer->Next;
//      else TimerList = pTimer->Next;
//
//      vPortFree(pTimer);
//      pTimer = pPrev ? pPrev->Next : TimerList;
//    }
//    else
    {
//      pPrev = pTimer;
      pTimer = pTimer->Next;
    }
  }
  
  return ExitLpm;
}
