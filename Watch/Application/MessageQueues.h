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
/*! \file MessageQueues.h
 * 
 * This file contains the handles for all of the message queues.  
 * 
 * When a message is passed to the RouteMsg function it determines which queue 
 * to place a message based on the Type.
 */
/******************************************************************************/

#ifndef MESSAGE_QUEUES_H
#define MESSAGE_QUEUES_H

#ifndef QUEUE_H
  #error "queue.h must be included before MessageQueues.h"
#endif

#define MESSAGE_QUEUE_ITEM_SIZE ( ( unsigned int ) sizeof( tMessage ) )

#define FREE_QINDEX        ( 0 )
#define BACKGROUND_QINDEX  ( 1 )
#define DISPLAY_QINDEX     ( 2 )
#define SPP_TASK_QINDEX    ( 3 )
#define TOTAL_QUEUES       ( 4 )


/*! Array of all of the queue handles */
extern xQueueHandle QueueHandles[TOTAL_QUEUES];


/*! \return 1 when all task queues are empty and the part can go into sleep 
 * mode 
 */
unsigned char AllTaskQueuesEmpty(void);


/*! Send a message to the free queue.  If this is a short message that 
 * did not allocate a buffer then no action is taken.
 *
 * \param pMsg A pointer to message buffer
 */
void SendToFreeQueue(tMessage* pMsg);


/*! Send a message to the free queue.  If this is a short message that 
 * did not allocate a buffer then no action is taken.
 *
 * \param pMsg A pointer to a message buffer
 */
void SendToFreeQueueIsr(tMessage* pMsg);

/*! Route a message to the appropriate task (queue). This operation is a copy.
 *
 * \param pMsg A pointer to a message buffer
 */
void RouteMsg(tMessage* pMsg);

/*! Send a message to a queue from an ISR.  This requires 1/2 the time of 
 * RouteMsgFromIsr.
 *
 * \param Qindex is the queue index to place the message in
 * \param pMsg A pointer to a message buffer
 */
void SendMessageToQueueFromIsr(unsigned char Qindex,tMessage* pMsg);

/*! Let routing know handle of wrapper queue.  This function is used so that the
 * number of queues can change without affecting the stack.
 *
 * \param WrapperQueueHandle - handle for wrapper queue (spp or ble)
 */
void AssignWrapperQueueHandle(xQueueHandle WrapperHandle);

/*! Set the message parameters.  Safe to call when in interrupt context.
 *
 * \param pMsg is a pointer to a message
 * \param Type is the message type
 * \param Options are the message options
 *
 * \note This does not allocate a buffer from the buffer pool (It is for a 
 * "short" message ).
 *
 */
void SetupMessage(tMessage* pMsg,
                  unsigned char Type,
                  unsigned char Options);

/*! Set the message parameters.
 *
 * The pMsg->pBuffer pMsg will point to a message buffer after a call to 
 * this function (unless there aren't any available).  A buffer allocation
 * failure will result in a watchdog reset.
 *
 * \param pMsg is a pointer to a message
 * \param Type is the message type
 * \param Options are the message options
 *
 * \note This cannot be called from an ISR.  It was chosen to not allow buffers
 * to be allocated from ISRs because of the amount of time it can take
 *
 */
void SetupMessageAndAllocateBuffer(tMessage* pMsg,
                                   unsigned char Type,
                                   unsigned char Options);


/*! Print the message type
 *
 * \param pMsg is a pointer to a message 
 *
 */
void PrintMessageType(tMessage* pMsg);

#endif /* MESSAGE_QUEUES_H */

 

