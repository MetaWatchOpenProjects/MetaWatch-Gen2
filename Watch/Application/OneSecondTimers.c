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
  unsigned char Allocated;
  unsigned char Running;
  unsigned char RepeatCount;
  unsigned char Qindex;
  eMessageType CallbackMsgType;
  unsigned char CallbackMsgOptions;

} tOneSecondTimer;


static tOneSecondTimer OneSecondTimers[TOTAL_ONE_SECOND_TIMERS];

static xSemaphoreHandle OneSecondTimerMutex;
  
void InitializeOneSecondTimers(void)
{
  /* clear information for all of the timers */
  unsigned char i;
  for ( i = 0; i < TOTAL_ONE_SECOND_TIMERS; i++ )
  {
    OneSecondTimers[i].Timeout = 0;
    OneSecondTimers[i].DownCounter = 0;
    OneSecondTimers[i].Allocated = 0;
    OneSecondTimers[i].Running = 0;
    OneSecondTimers[i].RepeatCount = 0;
    OneSecondTimers[i].Qindex = 0;
    OneSecondTimers[i].CallbackMsgType = InvalidMessage;
    OneSecondTimers[i].CallbackMsgOptions = 0;
  
  }

  OneSecondTimerMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(OneSecondTimerMutex);
  
}


tTimerId AllocateOneSecondTimer(void)
{
  signed char result = -1;

  xSemaphoreTake(OneSecondTimerMutex,portMAX_DELAY);

  unsigned char i;
  for( i = 0; i < TOTAL_ONE_SECOND_TIMERS; i++)
  {
    if ( OneSecondTimers[i].Allocated == 0 )
    {
      OneSecondTimers[i].Allocated = 1;
      result = i;
      break;
    }

  }

  if ( result < 0 )
  {
    PrintString("Unable to allocate Timer\r\n");
  }
  
  xSemaphoreGive(OneSecondTimerMutex);
  
  return result;
}


signed char DeallocateOneSecondTimer(tTimerId TimerId)
{
  signed char result = -1;

  if ( TimerId < 0 )
  {
    PrintString("Invalid Timer Id\r\n");  
  }
  
  portENTER_CRITICAL();

  if ( OneSecondTimers[TimerId].Allocated == 1 )
  {
    OneSecondTimers[TimerId].Allocated = 0;
    OneSecondTimers[TimerId].Running = 0;
    result = TimerId;
  }

  portEXIT_CRITICAL();

  if ( result < 0 )
  {
    PrintString("Unable to deallocate timer!\r\n");
  }

  return result;
}



void StartOneSecondTimer(tTimerId TimerId)
{
  if (  OneSecondTimers[TimerId].Allocated == 0 ||
        OneSecondTimers[TimerId].CallbackMsgType == InvalidMessage )
  {
    PrintString("Cannot start timer with invalid parameters\r\n");  
    return;
  }
  
  portENTER_CRITICAL();

  OneSecondTimers[TimerId].Running = 1;
  OneSecondTimers[TimerId].DownCounter = OneSecondTimers[TimerId].Timeout;
  
  portEXIT_CRITICAL();
}

void StopOneSecondTimer(tTimerId TimerId)
{
  portENTER_CRITICAL();

  OneSecondTimers[TimerId].Running = 0;
  
  portEXIT_CRITICAL();
}

/* setup a timer so the Restart timer function can be used */
void SetupOneSecondTimer(tTimerId TimerId,
                         unsigned int Timeout,
                         unsigned char RepeatCount,
                         unsigned char Qindex,
                         eMessageType CallbackMsgType,
                         unsigned char MsgOptions)
{
  
  if (   OneSecondTimers[TimerId].Allocated == 0 
      || TimerId < 0 )
  {
    PrintString("Timer not Allocated\r\n");
    return;
  }
  
  portENTER_CRITICAL();
    
  OneSecondTimers[TimerId].RepeatCount = RepeatCount;
  OneSecondTimers[TimerId].Timeout = Timeout;
  OneSecondTimers[TimerId].Qindex = Qindex;
  OneSecondTimers[TimerId].CallbackMsgType = CallbackMsgType;
  OneSecondTimers[TimerId].CallbackMsgOptions = MsgOptions;
    
  portEXIT_CRITICAL();
}

#if 0
void ChangeOneSecondTimerTimeout(tTimerId TimerId,
                                 unsigned int Timeout)
{
  OneSecondTimers[TimerId].Timeout = Timeout;
}

void ChangeOneSecondTimerMsgOptions(tTimerId TimerId,
                                    unsigned char MsgOptions)
{
  OneSecondTimers[TimerId].CallbackMsgOptions = MsgOptions;
}

void ChangeOneSecondTimerMsgCallback(tTimerId TimerId,
                                     eMessageType CallbackMsgType;
                                     unsigned char MsgOptions)
{
  OneSecondTimers[TimerId].CallbackMsgType = CallbackMsgType;
  OneSecondTimers[TimerId].CallbackMsgOptions = MsgOptions;
}
#endif

/* this should be as fast as possible because it happens in interrupt context
 * and it also often occurs when the part is sleeping
 */
unsigned char OneSecondTimerHandlerIsr(void)
{
  unsigned char ExitLpm = 0;
  
  unsigned char i;
  for ( i = 0; i < TOTAL_ONE_SECOND_TIMERS; i++ )
  {
    if ( OneSecondTimers[i].Running == 1 )
    {        
      /* decrement the counter first */
      if ( OneSecondTimers[i].DownCounter > 0 )
      {
        OneSecondTimers[i].DownCounter--;
      }
      
      /* has the counter reached 0 ?*/
      if ( OneSecondTimers[i].DownCounter == 0 )
      {
        /* should the counter be reloaded or stopped */
        if ( OneSecondTimers[i].RepeatCount == 0xFF )
        {
          OneSecondTimers[i].DownCounter = OneSecondTimers[i].Timeout;  
        }
        else if ( OneSecondTimers[i].RepeatCount > 0 )
        {
          OneSecondTimers[i].DownCounter = OneSecondTimers[i].Timeout;
          OneSecondTimers[i].RepeatCount--;
        }
        else
        {
          OneSecondTimers[i].Running = 0;  
        }
        
        tMessage OneSecondMsg;
        SetupMessage(&OneSecondMsg,
                     OneSecondTimers[i].CallbackMsgType,
                     OneSecondTimers[i].CallbackMsgOptions);
        
        SendMessageToQueueFromIsr(OneSecondTimers[i].Qindex,&OneSecondMsg);
        ExitLpm = 1;
      }
    }
  }
  
  return ExitLpm;
  
}

