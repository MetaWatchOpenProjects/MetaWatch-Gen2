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

#include "hal_board_type.h"
#include "hal_lpm.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "BufferPool.h" 
#include "Statistics.h"
#include "DebugUart.h"

/*! Allocate two header bytes at the start of each message buffer.  These are
 * not visible outside this module, which is why the struct is defined here.
 *
 * \param msgHeader that is not externally accessable
 * \param msgOptions byte that is not externally accessable
 * \param buffer is a buffer of HOST_MSG_BUFFER_LENGTH whose address goes
 * onto a queue
*/
typedef struct
{
  unsigned char msgHeader;                
  unsigned char msgOptions;               
  unsigned char buffer[HOST_MSG_BUFFER_LENGTH];  

} tBufferPoolBuffer;    


// This memory is never accessed directly, pointers to the buffers are put on
// a queue at startup
static tBufferPoolBuffer BufferPool[NUM_MSG_BUFFERS];

// all pointers on/off the queue must be in this range, use these to check the
// buffer free to make sure we don't trash the pool
static const unsigned char* LOW_BUFFER_ADDRESS   = &(BufferPool[0].buffer[0]);
static const unsigned char* HIGH_BUFFER_ADDRESS  = &(BufferPool[NUM_MSG_BUFFERS - 1].buffer[0]);



void InitializeBufferPool( void )
{
  unsigned int  ii;              // loop counter
  unsigned char* pMsgBuffer;      // holds the address of the msg buffer to add to the queue
  
  ii = sizeof(unsigned char*);  // use this to check the compilers size for a byte *
  
  // create the queue to hold the free message buffers
  QueueHandles[FREE_QINDEX] =
    xQueueCreate( ( unsigned portBASE_TYPE ) NUM_MSG_BUFFERS,
                  ( unsigned portBASE_TYPE ) sizeof(unsigned char*) );
  
  // Add the queue to the registry so we can tack it in StateViewer
  // vQueueAddToRegistry( m_xMessagedQueue, ( signed portCHAR * ) "Message_Buffer_Q" );
  
  // Add the address of each buffer's data section to the queue
  for(ii = 0; ii < NUM_MSG_BUFFERS; ii++)
  {
      // use the address of the data section so that the header is only accessed
      // as needed
      pMsgBuffer = & (BufferPool[ii].buffer[0]);
  
      // We only call this at initialization so it should never fail
      // params: queue handle, ptr to item to queue, ticks to wait if full
      if ( xQueueSend( QueueHandles[FREE_QINDEX], &pMsgBuffer, 0 ) != pdTRUE )
      {
        PrintString("Unable to build Free Queue\r\n");
        gAppStats.BufferPoolFailure = 1;
      }
  }
}





void BPL_AllocMessageBuffer(tHostMsg** ppMsgBuffer)
{
  // params are: queue handle, ptr to the msg buffer, ticks to wait
  if( pdTRUE != xQueueReceive( QueueHandles[FREE_QINDEX], (unsigned char**) ppMsgBuffer, 0 ) )
  {
    *ppMsgBuffer = NULL;
    
    PrintString("Unable to Allocate Buffer\r\n");
    gAppStats.BufferPoolFailure = 1;
  }

  /* set default values
   * this only makes sense since the buffers are all of the same type 
   */
  (*ppMsgBuffer)->Type = InvalidMessage;
  (*ppMsgBuffer)->Length = 0;
  (*ppMsgBuffer)->Options = NO_MSG_OPTIONS;
    
}




void BPL_AllocMessageBufferFromISR(tHostMsg** ppMsgBuffer)
{  
  // params are: queue handle, ptr to the msg buffer, ticks to wait
  if( pdTRUE != xQueueReceiveFromISR( QueueHandles[FREE_QINDEX], (unsigned char**) ppMsgBuffer, 0 ))
  {
    *ppMsgBuffer = NULL;
    
    PrintString("Unable to Allocate Buffer from Isr\r\n");
    gAppStats.BufferPoolFailure = 1;
  }
}


/* checks that address is in range */
void BPL_FreeMessageBuffer( tHostMsg** ppMsgBuffer )
{
  unsigned char* pMsgBuffer = (unsigned char*) (*ppMsgBuffer);

  // make sure the returned pointer is in range
  if (   pMsgBuffer < LOW_BUFFER_ADDRESS 
      || pMsgBuffer > HIGH_BUFFER_ADDRESS )
  {
    PrintString("Free Buffer Corruption\r\n");
    gAppStats.BufferPoolFailure = 1;
  }

  // params are: queue handle, ptr to item to queue, tick to wait if full
  // the queue can't be full unless there is a bug, so don't wait on full
  if( pdTRUE != xQueueSend(QueueHandles[FREE_QINDEX], ppMsgBuffer, 0) )
  {
    PrintString("Unable to add buffer to Free Queue\r\n");
    gAppStats.BufferPoolFailure = 1;
  }

}


