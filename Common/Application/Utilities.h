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
/*! \file Utilities.h
 *
 * Includes functions to check on task memory usage and prepare buffers
 * for transmission to the host
 *
 */
/******************************************************************************/

#ifndef UTILITIES_H
#define UTILITIES_H

#ifndef TASK_H
  #error "Task.h must be included before Utilities.h"
#endif

/*! This determines how many tasks can be kept track of in the utility functions */
#define MAX_NUM_TASK_LOG_ENTRIES  ( TOTAL_TASKS )

/*! Task Information Structure */
typedef struct
{
  signed char * name;
  void * taskHandle;
  unsigned int Depth;

} tTaskInfo;


#ifdef TASK_DEBUG
/*! Cycle through all of the tasks and display how much stack they are using 
 * (Prints information for one task each call)
 */
void UTL_FreeRtosTaskStackCheck(void);
#endif

/*! Check how much statck a task is currently using
 *
 * \param TaskHandle points to the task
 * \param TaskName is a string representation of the task
 */
void CheckStackUsage(xTaskHandle TaskHandle,signed char * TaskName);

/*! Prepare a message for transmission to the host by filling in the header
 * bytes, setting message type, copying payload, and generating checksum
 *
 * \param pMsg is a pointer to the message
 * \param msgType is the type of message
 * \param msgOption is the options for the message
 * \param pData is a pointer to payload data to be copied into message
 * \param dataLen is the size of the payload
 */
void UTL_BuildHstMsg(tHostMsg* pMsg, 
                     eMessageType msgType, 
                     unsigned char msgOption, 
                     unsigned char *pData, 
                     unsigned char dataLen );

/*! Prepare a message for transmission to the host by filling in the header
 * bytes and calculating the checksum
 *
 * \param pMsg is a pointer to the message
 */
void UTL_PrepareHstMsg(tHostMsg* pMsg);

/*void CopyBytes(unsigned char* pDest, unsigned char* pSource, unsigned char Size); */

/*! \return a pointer to the device name string */
unsigned char * GetDeviceNameString(void);

/*! return a pointer to the software version string */
unsigned char * GetSoftwareVersionString(void);

#endif /* UTILITIES_H */