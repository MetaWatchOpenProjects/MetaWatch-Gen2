//==============================================================================
//  Copyright 2011 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#define INVALID_ID(_x)    (_x == UNASSIGNED || _x >= TOTAL_TIMERS)

/*! One Second Timer Structure */
typedef struct
{
  unsigned int Timeout;
  unsigned int DownCounter;
  unsigned char Allocated;
  unsigned char Running;
  unsigned char RepeatCount;
  unsigned char Qindex;
  eMessageType CallbackMsgType;
  unsigned char CallbackMsgOptions;

} tOneSecondTimer;

static tOneSecondTimer Timers[TOTAL_TIMERS];
static xSemaphoreHandle OneSecondTimerMutex;
  
static void InitOneSecondTimers(void);

static void InitOneSecondTimers(void)
{
  /* clear information for all of the timers */
  memset(Timers, 0, sizeof(tOneSecondTimer) * TOTAL_TIMERS);

  OneSecondTimerMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(OneSecondTimerMutex);
}

/* setup a timer so the Restart timer function can be used */
tTimerId StartTimer(unsigned int Timeout,
                    unsigned char RepeatCount,
                    unsigned char Qindex,
                    eMessageType CallbackMsgType,
                    unsigned char MsgOptions)
{
  if (!OneSecondTimerMutex) InitOneSecondTimers();
  xSemaphoreTake(OneSecondTimerMutex, portMAX_DELAY);

  unsigned char i;
  for (i = 0; i < TOTAL_TIMERS; ++i)
  {
    if (!Timers[i].Allocated)
    {
      Timers[i].Allocated = pdTRUE;
      break;
    }
  }
  xSemaphoreGive(OneSecondTimerMutex);

  if (i == TOTAL_TIMERS)
  {
    PrintS("# AllocTimer");
    return UNASSIGNED;
  }
  
  portENTER_CRITICAL();
  Timers[i].RepeatCount = RepeatCount;
  Timers[i].Timeout = Timeout;
  Timers[i].Qindex = Qindex;
  Timers[i].CallbackMsgType = CallbackMsgType;
  Timers[i].CallbackMsgOptions = MsgOptions;
    
  /* start the timer */
  Timers[i].Running = pdTRUE;
  Timers[i].DownCounter = Timeout;
  portEXIT_CRITICAL();

  return i;
}

void StopTimer(tTimerId Id)
{
  if (INVALID_ID(Id)) return;

  if (Timers[Id].Allocated)
  {
    portENTER_CRITICAL();
    Timers[Id].Running = pdFALSE;
    Timers[Id].Allocated = pdFALSE;
    portEXIT_CRITICAL();
  }
}

void ResetTimer(tTimerId Id)
{
  if (INVALID_ID(Id)) return;

  if (Timers[Id].Allocated)
  {
    portENTER_CRITICAL();
    Timers[Id].DownCounter = Timers[Id].Timeout;
    portEXIT_CRITICAL();
  }
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
    if (Timers[i].Running)
    {        
      /* decrement the counter first, then check if the counter == 0 */
      if (--Timers[i].DownCounter == 0)
      {
        if (Timers[i].RepeatCount == 0)
        {
          Timers[i].Running = pdFALSE;
          Timers[i].Allocated = pdFALSE;
        }
        else
        {
          Timers[i].DownCounter = Timers[i].Timeout;
          if (Timers[i].RepeatCount != REPEAT_FOREVER) Timers[i].RepeatCount --;
        }

        tMessage Msg;
        SetupMessage(&Msg, Timers[i].CallbackMsgType, Timers[i].CallbackMsgOptions);
        SendMessageToQueueFromIsr(Timers[i].Qindex, &Msg);
        ExitLpm = 1;
      }
    }
  }
  
  return ExitLpm;
}

