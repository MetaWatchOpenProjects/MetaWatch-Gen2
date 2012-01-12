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
#include "queue.h"

#include "hal_board_type.h"

#include "Messages.h"
#include "MessageQueues.h"

#include "DebugUart.h"
#include "hal_crc.h"
#include "Messages.h"
#include "Utilities.h"

#ifdef TASK_DEBUG
static unsigned char TaskIndex = 0;
static tTaskInfo taskInfoArray[MAX_NUM_TASK_LOG_ENTRIES];
#endif

#ifdef TASK_DEBUG
/*! Keep track of a Task.
 *
 * \return TRUE if task could be registered
 *         FALSE if the task did not fit into the list
 */
unsigned char UTL_RegisterFreeRtosTask(void * TaskHandle,
                                       unsigned int StackDepth)
{
  unsigned char retValue = 1;
  
  if(TaskIndex >= MAX_NUM_TASK_LOG_ENTRIES)
  {
    PrintString("Task Log is Full \r\n");
    retValue = 0;  
  }
  else
  {
    taskInfoArray[TaskIndex].taskHandle = TaskHandle;
    taskInfoArray[TaskIndex].Depth = StackDepth;
    
	/* if this is true then there are big problems */
    if ( StackDepth > 1000 )
    {
      __no_operation();  
    }
    
    TaskIndex++;
  
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
  PrintStringSpaceAndThreeDecimals((char*)pcTaskGetTaskName( taskInfoArray[PrintTaskIndex].taskHandle ),
                                   FreeEntries,
                                   taskInfoArray[PrintTaskIndex].Depth - FreeEntries,
                                   taskInfoArray[PrintTaskIndex].Depth);
                                 
               
  PrintTaskIndex++;
  if ( PrintTaskIndex >= TaskIndex )
  {
    PrintTaskIndex = 0;
    
    tMessage OutgoingMsg;
    SetupMessage(&OutgoingMsg,QueryMemoryMsg,NO_MSG_OPTIONS);
    RouteMsg(&OutgoingMsg);
  }

}

#endif /* TASK_DEBUG */


void GenerateHostMsgCrc(tMessage* pMsg,unsigned int PayloadLength)
{
  tWordByteUnion Crc;
  
  halCrcInit();
  halCrcAddByte(HOST_MSG_START_FLAG);
  halCrcAddByte(pMsg->Length);
  halCrcAddByte(pMsg->Type);
  halCrcAddByte(pMsg->Options);
  Crc.word = halCrcCalculate(pMsg->pBuffer,PayloadLength);
  halCrcGiveMutex();
  
  /* now put the crc into the message buffer */
  pMsg->pBuffer[pMsg->Length-1] = Crc.byte1;
  pMsg->pBuffer[pMsg->Length-2] = Crc.byte0;
  
}


void CopyHostMsgPayload(unsigned char* pBuffer, 
                        unsigned char* pSource, 
                        unsigned char Size)
{
  for(unsigned char i = 0; i < Size; i++)
  {
    pBuffer[i] = pSource[i];
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
#ifdef CHECK_STACK_USAGE

  portBASE_TYPE HighWater = uxTaskGetStackHighWaterMark(TaskHandle);
  
  PrintStringAndDecimal(TaskName,HighWater);

#endif
  
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
  /* try to print task name */
  PrintString2("Stack overflow for ",(char*)pcTaskName);
}

/******************************************************************************/
/* 
 * from TI 
 */
unsigned char PMM15Check (void)
{
  // First check if SVSL/SVML is configured for fast wake-up
  if ( (!(SVSMLCTL & SVSLE)) || ((SVSMLCTL & SVSLE) && (SVSMLCTL & SVSLFP)) ||
       (!(SVSMLCTL & SVMLE)) || ((SVSMLCTL & SVMLE) && (SVSMLCTL & SVMLFP)) )
  { 
    // Next Check SVSH/SVMH settings to see if settings are affected by PMM15
    if ((SVSMHCTL & SVSHE) && (!(SVSMHCTL & SVSHFP)))
    {
      if ( (!(SVSMHCTL & SVSHMD)) ||
           ((SVSMHCTL & SVSHMD) && (SVSMHCTL & SVSMHACE)) )
        return 1; // SVSH affected configurations
    }

    if ((SVSMHCTL & SVMHE) && (!(SVSMHCTL & SVMHFP)) && (SVSMHCTL & SVSMHACE))
      return 1; // SVMH affected configurations
  }

  return 0; // SVS/M settings not affected by PMM15

}

/******************************************************************************/

void CheckQueueUsage(xQueueHandle Qhandle)
{
#ifdef CHECK_QUEUE_USAGE
  portBASE_TYPE waiting = Qhandle->uxMessagesWaiting + 1;
  
  if (  waiting > Qhandle->MaxWaiting )
  {
    Qhandle->MaxWaiting = waiting;    
  }
  
#endif
}
     