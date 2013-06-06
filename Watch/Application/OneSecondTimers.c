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

/******************************************************************************/
/*! \file OneSecondTimers.c
*
*/
/******************************************************************************/

#include <String.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "hal_board_type.h"
#include "hal_clock_control.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Utilities.h"

#include "OneSecondTimers.h"

/*! One Second Timer Structure */
typedef struct
{
  unsigned int Timeout;
  unsigned int DownCounter;
  unsigned char Running;
  unsigned char Repeat;
  unsigned char Qindex;
  eMessageType MsgType;
  unsigned char MsgOpt;
} tOneSecondTimer;

typedef struct
{
  unsigned int Timeout;
  unsigned char Repeat;
  unsigned char Qindex;
  eMessageType MsgType;
  unsigned char MsgOpt;
} tTimerSettings;

static const tTimerSettings TimerSettings[] =
{
  {TOUT_MONITOR_BATTERY, REPEAT_FOREVER, DISPLAY_QINDEX, MonitorBatteryMsg, MSG_OPT_NONE},
  {TOUT_NOTIF_MODE, TOUT_ONCE, DISPLAY_QINDEX, ModeTimeoutMsg, NOTIF_MODE},
  {TOUT_CALL_NOTIF, TOUT_ONCE, DISPLAY_QINDEX, CallerNameMsg, SHOW_NOTIF_END},
  {TOUT_BACKLIGHT, TOUT_ONCE, DISPLAY_QINDEX, SetBacklightMsg, LED_OFF_OPTION},
  {TOUT_CONN_HFP_MAP_SHORT, TOUT_ONCE, WRAPPER_QINDEX, ConnTimeoutMsg, MSG_OPT_NONE},
  {TOUT_TUNNEL_LONG, TOUT_ONCE, WRAPPER_QINDEX, TunnelTimeoutMsg, MSG_OPT_NONE},
  {TOUT_HEARTBEAT, TOUT_ONCE, WRAPPER_QINDEX, HeartbeatTimeoutMsg, MSG_OPT_NONE},
};
#define TOTAL_TIMERS    (sizeof(TimerSettings) / sizeof(tTimerSettings))

static tOneSecondTimer Timer[TOTAL_TIMERS];
  
/* start the timer if not started already; restart otherwise */
void StartTimer(eTimerId Id)
{
  portENTER_CRITICAL();
  if (!Timer[Id].MsgType)
  {
    Timer[Id].Qindex = TimerSettings[Id].Qindex;
    Timer[Id].MsgType = TimerSettings[Id].MsgType;
    Timer[Id].MsgOpt = TimerSettings[Id].MsgOpt;
  }

  if (!Timer[Id].Timeout) Timer[Id].Timeout = TimerSettings[Id].Timeout;
  if (!Timer[Id].Repeat) Timer[Id].Repeat = TimerSettings[Id].Repeat;
  
  Timer[Id].DownCounter = Timer[Id].Timeout;
  Timer[Id].Running = pdTRUE;
  portEXIT_CRITICAL();
}

void SetTimer(eTimerId Id, unsigned int Timeout, unsigned char Repeat)
{
  if (!Timeout || !Repeat) StopTimer(Id);
  else
  {
    Timer[Id].Timeout = Timeout;
    Timer[Id].Repeat = Repeat;
    StartTimer(Id);
  }
}

void StopTimer(eTimerId Id)
{
  Timer[Id].Running = pdFALSE;
  Timer[Id].Timeout = TimerSettings[Id].Timeout;
  Timer[Id].Repeat = TimerSettings[Id].Repeat;
}

/* this should be as fast as possible because it happens in interrupt context
 * and it also often occurs when the part is sleeping
 */
unsigned char OneSecondTimerHandlerIsr(void)
{
  unsigned char ExitLpm = 0;
  
  unsigned char i;
  for (i = 0; i < TOTAL_TIMERS; ++i)
  {
    if (Timer[i].Running)
    {        
      /* decrement the counter first, then check if the counter == 0 */
      if (--Timer[i].DownCounter == 0)
      {
        if (Timer[i].Repeat != REPEAT_FOREVER) Timer[i].Repeat --;
        
        if (Timer[i].Repeat == 0) Timer[i].Running = pdFALSE;
        else Timer[i].DownCounter = Timer[i].Timeout;

        tMessage Msg;
        SetupMessage(&Msg, Timer[i].MsgType, Timer[i].MsgOpt);
        SendMessageToQueueFromIsr(Timer[i].Qindex, &Msg);
        ExitLpm = 1;
      }
    }
  }
  
  return ExitLpm;
}

