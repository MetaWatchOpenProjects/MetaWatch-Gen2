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
#include "hal_battery.h"
#include "hal_lpm.h"

#include "Messages.h"
#include "MessageQueues.h"

#include "DebugUart.h"
#include "Messages.h"
#include "Utilities.h"
#include "IdleTask.h"

#include "NvIds.h"
#include "OSAL_Nv.h"

#if TASK_DEBUG
static unsigned char TaskIndex = 0;
static tTaskInfo taskInfoArray[MAX_NUM_TASK_LOG_ENTRIES];
#endif

#if TASK_DEBUG
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
    taskInfoArray[TaskIndex].FreeEntries =  StackDepth;
    
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
static unsigned char PrintTaskIndex = 3;

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
    PrintTaskIndex = 3;    
    CreateAndSendMessage(&Msg, QueryMemoryMsg, MSG_OPT_NONE);
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

/* called in each task but currently disabled */
void CheckStackUsage(xTaskHandle TaskHandle, tString *TaskName)
{
#if CHECK_STACK_USAGE

  portBASE_TYPE HighWater = uxTaskGetStackHighWaterMark(TaskHandle);
  PrintStringAndDecimal(TaskName, HighWater);

#endif
  
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask, char *pcTaskName)
{
  /* try to print task name */
  PrintString2("# Stack overflow:",(tString*)pcTaskName);
  SoftwareReset();
}

/******************************************************************************/
/* 
 * from TI 
 */
unsigned char PMM15Check (void)
{
  // First check if SVSL/SVML is configured for fast wake-up
  if ((!(SVSMLCTL & SVSLE)) || ((SVSMLCTL & SVSLE) && (SVSMLCTL & SVSLFP)) ||
      (!(SVSMLCTL & SVMLE)) || ((SVSMLCTL & SVMLE) && (SVSMLCTL & SVMLFP)))
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

void InitializeMuxMode(void)
{
  unsigned char nvMuxMode = MUX_MODE_DEFAULT_5V; //SERIAL
  OsalNvItemInit(NVID_MUX_CONTROL_5V, sizeof(nvMuxMode), &nvMuxMode);
  
  nvMuxMode = MUX_MODE_DEFAULT_NORMAL; //GROUND
  OsalNvItemInit(NVID_MUX_CONTROL_NORMAL, sizeof(nvMuxMode), &nvMuxMode);
}

/* read the value from flash (as opposed to shadowing it in ram) */
unsigned char GetMuxMode(unsigned char PowerGood)
{
  unsigned char nvMuxMode;

  OsalNvRead(PowerGood ? NVID_MUX_CONTROL_5V : NVID_MUX_CONTROL_NORMAL,
    NV_ZERO_OFFSET, sizeof(nvMuxMode), &nvMuxMode);
  
  return nvMuxMode;   
}

void SetMuxMode(unsigned char MuxMode, unsigned char PowerGood)
{
  PrintString2("- SetMux: ", MuxMode == MUX_MODE_SERIAL ? "Serial" :
    (MuxMode == MUX_MODE_SPY_BI_WIRE ? "SBW" : "GND"));

  OsalNvWrite(PowerGood? NVID_MUX_CONTROL_5V : NVID_MUX_CONTROL_NORMAL,
    NV_ZERO_OFFSET, sizeof(MuxMode), &MuxMode);
  
  ChangeMuxMode();
}

/******************************************************************************/
/* This only applies to board configuration 5 and later
 * Since these pins were previously unused the board configuration is not checked
 * This function does not modify the value stored in flash that is used 
 * during start-up
 * The RST/NMI pin is handled separately
 * This is used by the bootloader so don't use Print().
 */
void ChangeMuxMode(void)
{
#ifdef DIGITAL
#ifndef HW_DEVBOARD_V2
  
  static unsigned char MuxMode = MUX_MODE_OFF;
  
  if (GetMuxMode(ExtPower()) == MuxMode) return;
  
  MuxMode = GetMuxMode(ExtPower());
  
  /* setup default state for mux */
  ENABLE_MUX_OUTPUT_CONTROL();

  switch (MuxMode)
  {
  case MUX_MODE_OFF:
    /* this mode should not be used but is included for completeness */
    MUX_OUTPUT_OFF();
    break;
  
  case MUX_MODE_SERIAL: // with bootloader for flashing
    MUX_OUTPUT_SELECTS_SERIAL();
    break;
  
  case MUX_MODE_GND: // normal mode
    /* this mode prevents shorting of the case back pins due to sweat,
     * screwdrivers, etc
     */
    MUX_OUTPUT_SELECTS_GND();
    break;
  
  case MUX_MODE_SPY_BI_WIRE: // TI jtag in-circuit debugging
    MUX_OUTPUT_SELECTS_SPY();
    break;
    
  default:
    MUX_OUTPUT_SELECTS_GND();
    break;
  }
  
#endif
#endif
}
  
/******************************************************************************/
