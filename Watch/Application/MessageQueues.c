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

void SetupMessage(tMessage *pMsg, unsigned char Type, unsigned char Options)
{
  pMsg->Length = 0;
  pMsg->Type = Type;
  pMsg->Options = Options;
  pMsg->pBuffer = NULL;  
}

void SetupMessageWithBuffer(tMessage *pMsg, unsigned char Type, unsigned char Options)
{
//  pMsg->Length = 0;
  pMsg->Type = Type;
  pMsg->Options = Options;
  pMsg->pBuffer = BPL_AllocMessageBuffer();
}

unsigned char *CreateMessage(tMessage *pMsg)
{
  pMsg->pBuffer = BPL_AllocMessageBuffer();
  return pMsg->pBuffer;
}

void SendMessage(tMessage *pMsg, unsigned char Type, unsigned char Options)
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
  if (MsgInfo[pMsg->Type].MsgQueue != FREE_QINDEX)
  {
    portBASE_TYPE result = xQueueSend(QueueHandles[MsgInfo[pMsg->Type].MsgQueue], pMsg, DONT_WAIT);
    
    if (result == errQUEUE_FULL)
    {
      PrintF("@ Q Full:%s", MsgInfo[pMsg->Type].MsgQueue == WRAPPER_QINDEX ? "Wrp" : "Disp");
      SendToFreeQueue(pMsg);
    }
  }
  else SendToFreeQueue(pMsg);
  
  UpdateWatchdogInfo();
}

/* most messages do not have a buffer so there really isn't anything to free */
void SendToFreeQueue(tMessage *pMsg)
{
  if (pMsg->pBuffer != NULL)
  {
    /* The free queue holds pointers to buffers, not messages */
    BPL_FreeMessageBuffer(pMsg->pBuffer);
  }
}
  
/* send a message to a specific queue from an isr 
 * routing requires 25 us
 * putting a message into a queue (in interrupt context) requires 28 us
 * this requires 1/2 the time of using route
 */
void SendMessageToQueueFromIsr(unsigned char Qindex, tMessage *pMsg)
{
  signed portBASE_TYPE HigherPriorityTaskWoken;
  
  if (errQUEUE_FULL == xQueueSendFromISR(QueueHandles[Qindex], pMsg, &HigherPriorityTaskWoken))
  {
    PrintF("@ Q Full Isr:%s", Qindex == WRAPPER_QINDEX ? "Wrp" : "Disp");
    
    if (pMsg->pBuffer != NULL)
    {
      BPL_FreeMessageBufferFromIsr(pMsg->pBuffer);
    }
  }
}

void PrintMessageType(tMessage *pMsg)
{
  if (MsgInfo[pMsg->Type].Log)
  {
#if PRINT_MESSAGE_OPTIONS
    PrintF("%s 0x%02x", MsgInfo[pMsg->Type].MsgStr, pMsg->Options);
#else
    PrintS(MsgInfo[pMsg->Type].MsgStr);
#endif
  }
}

