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
/*! \file BufferPool.h
 *
 * The buffer pool is block of memory used for message allocation.  All of the 
 * free buffers are put into the free queue.
 *
 */
/******************************************************************************/

#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#ifndef MESSAGES_H
  #error "Messages.h must be included before BufferPool.h"
#endif


/*! Initialize memory used by the buffer pool.  The buffer pool is a queue
 * that holds free memory buffers that are used for tHostMsg messages.
 */
void InitializeBufferPool(void);

/*! Remove a message from the free buffer pool 
 * 
 * \return a pointer to a buffer
 */
unsigned char* BPL_AllocMessageBuffer(void);

/*! Add a message to the free buffer pool.  This function performs basic memory
 * range checking.
 * 
 * \param pBuffer points to the message buffer to be freed (added
 * to the free queue).
 */
void BPL_FreeMessageBuffer(unsigned char* pBuffer);

/*! Add a message to the free buffer pool.  This function performs basic memory
 * range checking.
 * 
 * \param pBuffer points to the message buffer to be freed (added
 * to the free queue).
 */
void BPL_FreeMessageBufferFromIsr(unsigned char* pBuffer);

#endif /* BUFFER_POOL_H */
