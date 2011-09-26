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
/*! \file Utilities.c
 *
 */
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"

#include "hal_board_type.h"

#include "DebugUart.h"
#include "hal_crc.h"
#include "Messages.h"
#include "Utilities.h"

#ifdef TASK_DEBUG
static unsigned char TaskIndex = 0;
static unsigned char NumberOfTasksInLog  = 0;
static tTaskInfo taskInfoArray[MAX_NUM_TASK_LOG_ENTRIES];
#endif

#ifdef TASK_DEBUG
/*! Keep track of a Task.
 *
 * \return TRUE if task could be registered
 *         FALSE if the task did not fit into the list
 */
unsigned char UTL_RegisterFreeRtosTask(pdTASK_CODE pxTaskCode, 
                                       const signed char * const pcName, 
                                       void *xTaskHandle,
                                       unsigned int StackDepth)
{
  unsigned char retValue = 1;
  
  if(NumberOfTasksInLog >= MAX_NUM_TASK_LOG_ENTRIES)
  {
    PrintString("Task Log is Full \r\n");
    retValue = 0;  
  }
  else
  {
    //taskInfoArray[TaskIndex].taskCode = pxTaskCode;
    taskInfoArray[TaskIndex].name = (signed char*)pcName;
    taskInfoArray[TaskIndex].taskHandle = xTaskHandle;
    taskInfoArray[TaskIndex].Depth = StackDepth;
    
    if ( StackDepth > 1000 )
    {
      __no_operation();  
    }
    
    TaskIndex++;
    NumberOfTasksInLog++;
  
  }
  
  return retValue;
}



/*! Check if tasks are running out of stack space.
 *
*/
static unsigned char PrintTaskIndex = 0;

void UTL_FreeRtosTaskStackCheck( void )
{
  unsigned int FreeEntries = 0;

  FreeEntries = uxTaskGetStackHighWaterMark( taskInfoArray[PrintTaskIndex].taskHandle );
  
  /* free, used, total */
  PrintStringSpaceAndThreeDecimals(taskInfoArray[PrintTaskIndex].name,
                                   FreeEntries,
                                   taskInfoArray[PrintTaskIndex].Depth - FreeEntries,
                                   taskInfoArray[PrintTaskIndex].Depth);
                                 
               
  PrintTaskIndex++;
  if ( PrintTaskIndex >= TaskIndex )
  {
    PrintTaskIndex = 0;  
  }

}

#endif


/*! Builds a host message including the header and CRC
 *
 * The message type, options and data must be provided by the user. The function
 * then creates the message header and CRC. The result is a packet that can be
 * sent directly to the host that meets the protocol requirements.
 *
 * \param pMsg Message allocated from the buffer pool
 * \param msgType
 *
 */
void UTL_BuildHstMsg(tHostMsg* pMsg, 
                     eMessageType msgType, 
                     unsigned char msgOptions, 
                     unsigned char *pData, 
                     unsigned char dataLen )
{
  unsigned char ii;
  tWordByteUnion crc;
  
  if ( dataLen > HOST_MSG_MAX_PAYLOAD_LENGTH )
  {
    PrintString("Data is too large for message\r\n");  
  }
  
  // fill in the header
  pMsg->startByte  = HOST_MSG_START_FLAG;
  pMsg->Length  = dataLen + (HOST_MSG_HEADER_LENGTH + HOST_MSG_CRC_LENGTH);
  pMsg->Type    = msgType;
  pMsg->Options = msgOptions;
  
  // copy the message data
  for(ii = 0; ii < dataLen; ii++)
  {
    pMsg->pPayload[ii] = pData[ii];
  }
  
  // exclude the crc bytes in the length since they aren't part of the crc
  crc.word = halCrcCalculate( (unsigned char*) pMsg, pMsg->Length - HOST_MSG_CRC_LENGTH);
  
  // The CRC is in the two bytes that follow the data
  pMsg->pPayload[dataLen] = crc.byte0;
  pMsg->pPayload[dataLen + 1] = crc.byte1;

  // for debugging
  pMsg->crcLsb = pMsg->pPayload[dataLen];
  pMsg->crcMsb = pMsg->pPayload[dataLen + 1];
  
}

/* udpate header bytes and calculate the crc */
void UTL_PrepareHstMsg(tHostMsg* pMsg)
{
  tWordByteUnion crc;
  unsigned char CrcIndex = pMsg->Length;
  
  // fill in the header
  pMsg->startByte  = HOST_MSG_START_FLAG;
  pMsg->Length += (HOST_MSG_HEADER_LENGTH + HOST_MSG_CRC_LENGTH);
  
  // exclude the crc bytes in the length since they aren't part of the crc
  crc.word = halCrcCalculate( (unsigned char*) pMsg, pMsg->Length - HOST_MSG_CRC_LENGTH);
  
  // The CRC is in the two bytes that follow the data
  pMsg->pPayload[CrcIndex] = crc.byte0;
  pMsg->pPayload[CrcIndex + 1] = crc.byte1;

  // for debugging
  pMsg->crcLsb = pMsg->pPayload[CrcIndex];
  pMsg->crcMsb = pMsg->pPayload[CrcIndex + 1];
  
}







void CopyBytes(unsigned char* pDest, unsigned char* pSource, unsigned char Size)
{
  for(unsigned char i = 0; i < Size; i++)
  {
    pDest[i] = pSource[i];
  }
}

unsigned char * GetDeviceNameString(void)
{
  return SPP_DEVICE_NAME;  
}

unsigned char * GetSoftwareVersionString(void)
{
  return VERSION_STRING;  
}



/* called in each task but currently disabled */
void CheckStackUsage(xTaskHandle TaskHandle,signed char* TaskName)
{
#if 0
  portBASE_TYPE HighWater = uxTaskGetStackHighWaterMark(TaskHandle);
  
#if 1
  if ( HighWater <  16 )
  {
    PrintStringAndDecimal((signed char*)TaskName,HighWater);
  }
#else
  PrintStringAndDecimal(TaskName,HighWater);
#endif

#endif
  
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
  /* try to print task name */
  PrintString2("Stack overflow for ",pcTaskName);
}
