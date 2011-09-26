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

#define MESSAGE_QUEUE_ITEM_SIZE ( ( unsigned int ) sizeof( unsigned char* ) )

#define FREE_QINDEX        ( 0 )
#define BACKGROUND_QINDEX  ( 1 )
#define DISPLAY_QINDEX     ( 2 )
#define LCD_TASK_QINDEX    ( 3 )
#define SRAM_QINDEX        ( 4 )
#define SPP_TASK_QINDEX    ( 5 ) 
#define TOTAL_QUEUES       ( 6 )

/*! Array of all of the queue handles */
extern xQueueHandle QueueHandles[TOTAL_QUEUES];

/*! send a buffer to the free queue
*
* \param ppMsg A pointer to message buffer pointer
*/
void SendToFreeQueue(tHostMsg** ppMsg);


/*! send a buffer to the free queue from an ISR
*
* \param ppMsg A pointer to a message buffer pointer
*/
void SendToFreeQueueIsr(tHostMsg** ppMsg);

/*! Route a message to the appropriate task (queue)
*
* \param ppMsg A pointer to a message buffer pointer
*/
void RouteMsg(tHostMsg** ppMsg);

/*! Route a message to the appropriate task (queue) from an ISR
*
* \param ppMsg A pointer to a message buffer pointer
*/
void RouteMsgFromIsr(tHostMsg** ppMsg);


#endif /* MESSAGE_QUEUES_H */

 

