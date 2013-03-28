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
/*! \file BufferPool.c
*
* This file does not conform to the coding standard.
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "queue.h"         
#include "task.h"

#include "hal_board_type.h"
#include "hal_lpm.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "BufferPool.h" 
#include "Statistics.h"
#include "DebugUart.h"
#include "IdleTask.h"

/*! \param buffer is a buffer of MSG_BUFFER_LENGTH whose address goes
 * onto a queue
*/
typedef struct
{
  /* ~hack
   * Reserve header bytes to avoid mem-copy in _HostMsg_write
   */
  unsigned char header[MSG_HEADER_LENGTH];
  unsigned char buffer[MSG_BUFFER_LENGTH - MSG_HEADER_LENGTH];  

} tBufferPoolBuffer;    


/*! number of buffers in the message buffer pool */
#define MSG_BUF_NUM 20 //15

// This memory is never accessed directly, pointers to the buffers are put on
// a queue at startup
static tBufferPoolBuffer BufferPool[MSG_BUF_NUM];

// all pointers on/off the queue must be in this range, use these to check the
// buffer free to make sure we don't trash the pool
static const unsigned char* LOW_BUFFER_ADDRESS   = &(BufferPool[0].buffer[0]);
static const unsigned char* HIGH_BUFFER_ADDRESS  = &(BufferPool[MSG_BUF_NUM - 1].buffer[0]);

static void SetBufferPoolFailureBit(void)
{
  PrintS("@ BufPool Fail");
  SoftwareReset();
}

void InitBufferPool(void)
{
  unsigned int  i;              // loop counter
  unsigned char* pMsgBuffer;    // holds the address of the msg buffer to add to the queue
  
  // create the queue to hold the free message buffers
  QueueHandles[FREE_QINDEX] =
    xQueueCreate( ( unsigned portBASE_TYPE ) MSG_BUF_NUM,
                  ( unsigned portBASE_TYPE ) sizeof(unsigned char*) );
  
  // Add the address of each buffer's data section to the queue
  for (i = 0; i < MSG_BUF_NUM; i++)
  {
    // use the address of the data section so that the header is only accessed
    // as needed
    pMsgBuffer = &(BufferPool[i].buffer[0]);

    // We only call this at initialization so it should never fail
    // params: queue handle, ptr to item to queue, ticks to wait if full
    if (xQueueSend( QueueHandles[FREE_QINDEX], &pMsgBuffer, DONT_WAIT) != pdTRUE)
    {
      PrintS("@ Init BufPool");
      SetBufferPoolFailureBit();
    }
  }
}

unsigned char *BPL_AllocMessageBuffer(void)
{
  unsigned char *pBuffer = NULL;

  // params are: queue handle, ptr to the msg buffer, ticks to wait
  if (pdTRUE != xQueueReceive(QueueHandles[FREE_QINDEX], &pBuffer, DONT_WAIT))
  {
    PrintS("@ Alloc");
    SetBufferPoolFailureBit();

  }
  else if (pBuffer < LOW_BUFFER_ADDRESS || pBuffer > HIGH_BUFFER_ADDRESS)
  {
    PrintF("@ Alloc invalid pBuf 0x%04X", (unsigned int)pBuffer);
    SetBufferPoolFailureBit();
  }

  return pBuffer;
}

void BPL_FreeMessageBuffer(unsigned char *pBuffer)
{
  // make sure the returned pointer is in range
  if (pBuffer < LOW_BUFFER_ADDRESS || pBuffer > HIGH_BUFFER_ADDRESS)
  {
    PrintF("@ Delloc invalid pBuf 0x%04X", (unsigned int)pBuffer);
    SetBufferPoolFailureBit();
  }

  // params are: queue handle, ptr to item to queue, tick to wait if full
  // the queue can't be full unless there is a bug, so don't wait on full
  if(pdTRUE != xQueueSend(QueueHandles[FREE_QINDEX], &pBuffer, DONT_WAIT))
  {
    PrintS("@ Delloc");
    SetBufferPoolFailureBit();
  }
}

void BPL_FreeMessageBufferFromIsr(unsigned char *pBuffer)
{
  // make sure the returned pointer is in range
  if (pBuffer < LOW_BUFFER_ADDRESS || pBuffer > HIGH_BUFFER_ADDRESS)
  {
    PrintS("@ Delloc ISR invalid pBuf");
    SetBufferPoolFailureBit();
  }

  signed portBASE_TYPE HigherPriorityTaskWoken;
  
  // params are: queue handle, ptr to item to queue, tick to wait if full
  // the queue can't be full unless there is a bug, so don't wait on full
  if(pdTRUE != xQueueSendFromISR(QueueHandles[FREE_QINDEX], &pBuffer, &HigherPriorityTaskWoken))
  {
    PrintS("@ Delloc ISR");
    SetBufferPoolFailureBit();
  }

#if 0
  /* this should never be true when freeing a message buffer */
  if ( HigherPriorityTaskWoken == pdTRUE )
  {
    portYIELD();
  }
#endif
  
}

#if 0
/* This is too slow to use */
unsigned char* BPL_AllocMessageBufferFromISR(void)
{  
  unsigned char * pBuffer = NULL;

  signed portBASE_TYPE HigherPriorityTaskWoken;
  
  // params are: queue handle, ptr to the msg buffer, ticks to wait
  if( pdTRUE != xQueueReceiveFromISR(QueueHandles[FREE_QINDEX], 
                                     &pBuffer, 
                                     &HigherPriorityTaskWoken ))
  {
    PrintS("@ Alloc Buf frm Isr");
    SetBufferPoolFailureBit();
  }
  
  if ( HigherPriorityTaskWoken == pdTRUE )
  {
    portYIELD();
  }
  
  return pBuffer;
}
#endif

