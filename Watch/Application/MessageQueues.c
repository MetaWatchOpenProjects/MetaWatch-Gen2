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
/*! \file MessageQueues.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"           
#include "queue.h"              
#include "task.h"

#include "hal_board_type.h"

#include "Messages.h"
#include "BufferPool.h"
#include "MessageQueues.h"
#include "MessageInfo.h"
#include "DebugUart.h"
#include "Statistics.h"
#include "IdleTask.h"

xQueueHandle QueueHandles[TOTAL_QUEUES];

static void SendMsgToQ(unsigned char Qindex, tMessage* pMsg);
static void PrintQueueNameIsFull(unsigned char Qindex);

void SetupMessage(tMessage* pMsg, unsigned char Type, unsigned char Options)
{
  pMsg->Length = 0;
  pMsg->Type = Type;
  pMsg->Options = Options;
  pMsg->pBuffer = NULL;  
}

void SetupMessageAndAllocateBuffer(tMessage* pMsg, unsigned char Type, unsigned char Options)
{
  pMsg->Length = 0;
  pMsg->Type = Type;
  pMsg->Options = Options;
  pMsg->pBuffer = BPL_AllocMessageBuffer();
}

void SendMessage(tMessage* pMsg, unsigned char Type, unsigned char Options)
{
  SetupMessage(pMsg, Type, Options);
  RouteMsg(pMsg);
}

void CreateAndSendMessage(unsigned char Type, unsigned char Options)
{
  tMessage Msg;
  SendMessage(&Msg, Type, Options);
}

void RouteMsg(tMessage *pMsg)
{
  SendMsgToQ(MsgInfo[pMsg->Type].MsgQueue, pMsg);
  UpdateWatchdogInfo();
}

void PrintMessageType(tMessage *pMsg)
{
  if (MsgInfo[pMsg->Type].Log)
  { 
    
#if DONT_PRINT_MESSAGE_TYPE
    PrintString2(MsgInfo[pMsg->Type].MsgStr, CR);
#else
    PrintStringAndHexByte(MsgInfo[pMsg->Type].MsgStr, pMsg->Type);
#endif
  }
}

/* if the queue is full, don't wait */
static void SendMsgToQ(unsigned char Qindex, tMessage* pMsg)
{
  if ( Qindex == FREE_QINDEX )
  {
    SendToFreeQueue(pMsg);  
  }
  else if (errQUEUE_FULL == xQueueSend(QueueHandles[Qindex], pMsg, DONT_WAIT))
  {
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(pMsg);
  }
}

/* most messages do not have a buffer so there really isn't anything to free */
void SendToFreeQueue(tMessage* pMsg)
{
  if (pMsg->pBuffer != NULL)
  {
    /* The free queue holds pointers to buffers, not messages */
    BPL_FreeMessageBuffer(pMsg->pBuffer);
  }
}
  
void SendToFreeQueueIsr(tMessage* pMsg)
{
  if (pMsg->pBuffer != NULL)
  {
    BPL_FreeMessageBufferFromIsr(pMsg->pBuffer);
  }
}

static void PrintQueueNameIsFull(unsigned char Qindex)
{
  gAppStats.QueueOverflow = 1;
  
  switch(Qindex)
  {
  case FREE_QINDEX:        PrintString("Shoud not get here\r\n");   break;
  case DISPLAY_QINDEX:     PrintString("Display Q is full\r\n");    break;
  case SPP_TASK_QINDEX:    PrintString("Spp Task Q is full\r\n");   break;
  default:                 PrintString("Unknown Q is full\r\n");    break;
  }
}

/* send a message to a specific queue from an isr 
 * routing requires 25 us
 * putting a message into a queue (in interrupt context) requires 28 us
 * this requires 1/2 the time of using route
 */
void SendMessageToQueueFromIsr(unsigned char Qindex, tMessage* pMsg)
{
  signed portBASE_TYPE HigherPriorityTaskWoken;
  
  if ( Qindex == FREE_QINDEX )
  {
    SendToFreeQueueIsr(pMsg);  
  }
  else if ( errQUEUE_FULL == xQueueSendFromISR(QueueHandles[Qindex],
                                               pMsg,
                                               &HigherPriorityTaskWoken))
  {
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueueIsr(pMsg);
  }
  
  
#if 0
  /* This is not used
   *
   * we don't want to task switch when in sleep mode
   * The ISR will exit sleep mode and then the RTOS will run
   * 
   */
  if ( HigherPriorityTaskWoken == pdTRUE )
  {
    portYIELD();
  }
#endif
}

#if 0
/* this was kept for reference */
static void SendMessageToQueueWait(unsigned char Qindex, tMessage* pMsg)
{
  /* wait */
  if ( xQueueSend(QueueHandles[Qindex], pMsg, portMAX_DELAY) == errQUEUE_FULL )
  { 
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(pMsg);
  }
}
#endif

