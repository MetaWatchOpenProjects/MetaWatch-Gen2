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
#include "MessageInfo.h"
#include "DebugUart.h"
#include "IdleTask.h"
#include "Log.h"

#define QUEUE_NUM         2
xQueueHandle QueueHandles[QUEUE_NUM];

#define HEAP_MIN_ADDR     (0x2000)

unsigned char *CreateMessage(tMessage *pMsg)
{
  pMsg->pBuffer = (unsigned char *)pvPortMalloc(pMsg->Length + MSG_OVERHEAD_LENGTH);
  
  if (pMsg->pBuffer < (volatile unsigned char *)HEAP_MIN_ADDR)
  {
    PrintF("@^@ %04X:%u", pMsg->pBuffer, pMsg->Length + MSG_OVERHEAD_LENGTH);
    pMsg->pBuffer = NULL;
  }
//  else PrintF("+%04X:%u", pMsg->pBuffer, pMsg->Length + MSG_OVERHEAD_LENGTH);

  if (pMsg->pBuffer) pMsg->pBuffer += MSG_HEADER_LENGTH;
  return pMsg->pBuffer;
}

void FreeMessageBuffer(unsigned char *pBuffer)
{
  vPortFree(pBuffer - MSG_HEADER_LENGTH);
//  PrintF("-%04X", pBuffer - MSG_HEADER_LENGTH);
}

void SendMessage(unsigned char Type, unsigned char Options)
{
  tMessage Msg = {0, Type, Options, NULL};
  RouteMsg(&Msg);
}

void RouteMsg(tMessage *pMsg)
{
  if (pMsg->Length == 0 && pMsg->pBuffer || pMsg->Length && pMsg->pBuffer == NULL)
  {
    if (pMsg->pBuffer) FreeMessageBuffer(pMsg->pBuffer);
    PrintF("# Memleak:%s", MsgInfo[pMsg->Type].MsgStr);
    return;
  }

  portBASE_TYPE Result = xQueueSend(QueueHandles[MsgInfo[pMsg->Type].MsgQueue], pMsg, DONT_WAIT);
  
  if (Result == errQUEUE_FULL)
  {
    PrintF("#%c Q Full", MsgInfo[pMsg->Type].MsgQueue == WRAPPER_QINDEX ? 'W' : 'D');
    if (pMsg->pBuffer) FreeMessageBuffer(pMsg->pBuffer);
  }
}

/* send a message to a specific queue from an isr
 * routing requires 25 us
 * putting a message into a queue (in interrupt context) requires 28 us
 * this requires 1/2 the time of using route
 */
void SendMessageIsr(unsigned char Type, unsigned char Options)
{
  signed portBASE_TYPE HigherPriorityTaskWoken;
  
  tMessage Msg = {0, Type, Options, NULL};

  if (errQUEUE_FULL == xQueueSendFromISR(QueueHandles[MsgInfo[Type].MsgQueue], &Msg, &HigherPriorityTaskWoken))
    PrintF("#%c Q Full", MsgInfo[Type].MsgQueue == WRAPPER_QINDEX ? 'W' : 'D');
}

void ShowMessageInfo(tMessage *pMsg)
{
  if (MsgInfo[pMsg->Type].Log) PrintF("%s x%02X", MsgInfo[pMsg->Type].MsgStr, pMsg->Options);

  UpdateLog(pMsg->Type, pMsg->Options);
  UpdateQueueInfo();
}
