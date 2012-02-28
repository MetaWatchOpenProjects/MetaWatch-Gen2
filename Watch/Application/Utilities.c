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
    taskInfoArray[TaskIndex].FreeEntries =  0xffff;
    
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

  taskInfoArray[PrintTaskIndex].FreeEntries = 
    uxTaskGetStackHighWaterMark( taskInfoArray[PrintTaskIndex].taskHandle );
  
  /* free, used, total */
  PrintStringSpaceAndThreeDecimals
    ((char*)pcTaskGetTaskName( taskInfoArray[PrintTaskIndex].taskHandle ),
     taskInfoArray[PrintTaskIndex].FreeEntries,
     taskInfoArray[PrintTaskIndex].Depth - taskInfoArray[PrintTaskIndex].FreeEntries,
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

void CopyHostMsgPayload(unsigned char* pBuffer, 
                        unsigned char* pSource, 
                        unsigned char Size)
{
  unsigned char i;
  for(i = 0; i < Size; i++)
  {
    pBuffer[i] = pSource[i];
  }
}

unsigned char * GetDeviceNameString(void)
{
  return (unsigned char *)SPP_DEVICE_NAME;  
}

unsigned char * GetSoftwareVersionString(void)
{
  return (unsigned char *)VERSION_STRING;  
}



/* called in each task but currently disabled */
void CheckStackUsage(xTaskHandle TaskHandle,tString * TaskName)
{
#ifdef CHECK_STACK_USAGE

  portBASE_TYPE HighWater = uxTaskGetStackHighWaterMark(TaskHandle);
  
  PrintStringAndDecimal(TaskName,HighWater);

#endif
  
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, char *pcTaskName )
{
  /* try to print task name */
  PrintString2("Stack overflow for ",(tString*)pcTaskName);
  ForceWatchdogReset();
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
     
/******************************************************************************/

#if 0
/* set watchdog for 16 second timeout */
static void SetWatchdogReset(void)
{
  /* write password, select aclk, divide by 512*1024 = 16 ms */
  WDTCTL = WDTPW + WDTSSEL__ACLK + WDTIS_3;
}
#endif

/* write the inverse of the password and force reset */
void ForceWatchdogReset(void)
{
  __disable_interrupt();
  __delay_cycles(100000);
  
#ifdef DEBUG_WATCHDOG_RESET

  /* wait here forever */
  ENABLE_LCD_LED();
  while(1);

#else

  WDTCTL = ~WDTPW; 

#endif

}
