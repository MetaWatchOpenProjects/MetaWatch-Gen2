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
/*! \file CommandTask.c
*
*/
/******************************************************************************/


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "Messages.h"

#include "hal_lpm.h"
#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_vibe.h"

#include "Background.h"
#include "BufferPool.h"
#include "Buttons.h"
#include "DebugUart.h"
#include "SerialProfile.h"
#include "Utilities.h"
#include "MessageQueues.h"
#include "Vibration.h"
#include "OneSecondTimers.h"
#include "CommandTask.h"
#include "Statistics.h"


#define COMMAND_QUEUE_LEN         32    
#define COMMAND_TASK_STACK_DEPTH	(configMINIMAL_STACK_DEPTH + 30)
#define COMMAND_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)

static xQueueHandle CommandQueueHandle;
static xTaskHandle CommandTaskHandle;


static void CommandTask(void *pvParameters);

static void CommandHandler(eCmdType CmdType);

static eCmdType CmdType;

static void BlasterCmdHandler(void);

void InitializeCommandTask(void)
{
  CommandQueueHandle = 
    xQueueCreate(COMMAND_QUEUE_LEN,(unsigned int) sizeof(eCmdType) );

  xTaskCreate(CommandTask, 
              "COMMMAND", 
              COMMAND_TASK_STACK_DEPTH, 
              NULL, 
              COMMAND_TASK_PRIORITY, 
              &CommandTaskHandle);
}



/*! Handle the buttons, vibration, and one second timers */
static void CommandTask(void *pvParameters)
{
  if ( CommandQueueHandle == 0 )
  {
    PrintString("CommandQueueHandle not created!");  
  }

  InitializeButtons();
  InitializeVibration();
  InitializeOneSecondTimers();
  
#ifdef BLASTER
  PrintString("^^ Blaster mode enabled ^^ ");
  EnableRtcPrescaleInterruptUser(RTC_TIMER_USER_BLASTER);
#endif
  
  for(;;)
  { 
    if ( xQueueReceive(CommandQueueHandle,&CmdType, portMAX_DELAY ) == pdTRUE )
    {
      CommandHandler(CmdType); 
    }
  }

}

/* the command task has its own queue type */
unsigned char CommandTaskReadyToSleep(void)
{
  return ( CommandQueueHandle->uxMessagesWaiting == 0 );  
}


static void CommandHandler(eCmdType CmdType)
{
  tHostMsg* pMsg;

  switch ( CmdType )
  {
    
  case ButtonDebounce:
    /* Enable the RTC prescale one interrupt to time the button debounce. */
    EnableRtcPrescaleInterruptUser(RTC_TIMER_BUTTON); 
    break;
  
  case ButtonState:
    ButtonStateHandler(); 
    break;
   
  case VibrationState:
    VibrationMotorStateMachine();
    break;
    
  case ExpiredTimerCmd:
    ExpiredOneSecondTimerHandler();
    break;
 
  case SniffModeEntryCmd:
    BPL_AllocMessageBuffer(&pMsg);
    pMsg->Type = EnterSniffModeMsg;
    RouteMsg(&pMsg);  
    break;
 
  case BlasterCmd:
    BlasterCmdHandler();
    break;
   
  case ScrollModeCmd:
    BPL_AllocMessageBuffer(&pMsg);
    pMsg->Type = OledScrollMsg;
    RouteMsg(&pMsg);  
    break;
    
  default:
    PrintStringAndHex("Unhandled command type",(unsigned int)CmdType);
  
    break;
  }
  
}

/* The only task that can handle eCmdType is the command task */
void RouteCommand(eCmdType CmdType)
{
  /* if the queue is full, don't wait */
  if ( xQueueSend(CommandQueueHandle, &CmdType, 0) == errQUEUE_FULL )
  { 
    PrintString("Command task queue is full\r\n");
  }
  
}


void RouteCommandFromIsr(eCmdType CmdType)
{
  if ( xQueueSendFromISR(CommandQueueHandle, &CmdType, 0) == errQUEUE_FULL )
  {
    PrintString("Command task queue is full\r\n");
  }

}

/******************************************************************************/


#if 0
static void RtcPrescale0Handler(void)
{

#if 0
  /* if another frequency is desired then it must be done in hal_rtc.h */
  if( QueryRtcUserActive(RTC_TIMER_PEDOMETER) )
  {
    RouteCommand(PedometerState);    
  }
#endif
  


}
#endif

static tHostMsg* pTestMsg;
    
static void BlasterCmdHandler(void)
{
  /* message blaster */
  if (   QueryPhoneConnected() 
      && gBtStats.TxDataStalled != 1 
      && ( QueueHandles[SPP_TASK_QINDEX]->uxMessagesWaiting < (QueueHandles[SPP_TASK_QINDEX])->uxLength) )
  {
    /* repeatedly send a message */
    BPL_AllocMessageBuffer(&pTestMsg);
    unsigned char pData[HOST_MSG_MAX_PAYLOAD_LENGTH];
    
    pData[0] = (unsigned char)eScReserved;
    
    UTL_BuildHstMsg(pTestMsg, 
                    StatusChangeEvent, 
                    0x00, 
                    pData, 
                    HOST_MSG_MAX_PAYLOAD_LENGTH);
    
    RouteMsg(&pTestMsg);
  }
}

/* set watchdog for 16 second timeout */
void SetWatchdogReset(void)
{
  /* write password, select aclk, divide by 512*1024 = 16 ms */
  WDTCTL = WDTPW + WDTSSEL__ACLK + WDTIS_3;
}

/* write the inverse of the password and force reset */
void ForceWatchdogReset(void)
{
  WDTCTL = ~WDTPW; 
}


#if 0
/* set this up to run every 8 seconds */
static unsigned char TaskResponses;

static void SendTaskQueries(void)
{
  TaskReponses = 0;
  
  tHostMsg* pQueryMsg;
  
  for ( unsigned char i = BACKGROUND_QINDEX; i < TOTAL_QUEUES; i++)
  {
    BPL_AllocMessageBuffer(&pQueryMsg);
    pQueryMsg->Type = TaskQueryMsg;
    pQueryMsg->Options = i;
    RouteMsg(&pQueryMsg);
  }
  
}

void SetTaskReponse(unsigned char TaskIndex)
{
  ENTER_CRITICAL_REGION();
  TaskReponses |= (1 << TaskIndex);  
  LEAVE_CRITICAL_REGION();
  
}

static void CheckTaskResponses(void)
{
  if ( TaskReponses == ALL_TASKS_HAVE_RESPONDED )
  {
    SetWatchdogReset();  
  }
}
#endif