//==============================================================================
//  Copyright Meta Watch Ltd. - http://www.MetaWatch.org/
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
/*! \file IdleTask.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "portmacro.h"

#include "Messages.h"
#include "MessageQueues.h"

#include "hal_lpm.h"
#include "hal_board_type.h"
#include "hal_miscellaneous.h"
#include "hal_calibration.h"
#include "hal_boot.h"

#include "DebugUart.h"
#include "Wrapper.h"
#include "Utilities.h"
#include "IdleTask.h"

#include "OSAL_Nv.h"
#include "NvIds.h"

/******************************************************************************/

__no_init __root static tWatchdogInfo WatchdogInfo @ WATCHDOG_INFO_ADDR;

/******************************************************************************/

static unsigned char TaskCheckInFlags;

/******************************************************************************/

void vApplicationIdleHook(void)
{

  /* Put the processor to sleep if the serial port indicates it is OK and
   * all of the queues are empty.
   *
   * This will stop the OS scheduler.
   */ 
  DEBUG5_HIGH();
 
  /* enter a critical section so that the flags can be checked */
  __disable_interrupt();
  __no_operation();
  
  /* the watchdog is set at 16 seconds.
   * the battery interval rate is set a 8 seconds
   * each task checks in at the battery interval rate
   */
  UpdateWatchdogInfo();
 
  if (   GetShippingModeFlag() 
      || (   WatchdogInfo.SppReadyToSleep
          && WatchdogInfo.TaskDelayLockCount == 0
          && WatchdogInfo.DisplayMessagesWaiting == 0
          && WatchdogInfo.SppMessagesWaiting == 0 ) )

  {

    RestartWatchdog();
    
    /* Call MSP430 Utility function to enable low power mode 3.     */
    /* Put OS and Processor to sleep. Will need an interrupt        */
    /* to wake us up from here.   */
    MSP430_LPM_ENTER();

    /* If we get here then interrupts are enabled */
    extern xTaskHandle IdleTaskHandle;
//    CheckStackUsage(IdleTaskHandle,"~IdlTsk ");

  }
  else
  {
    /* we aren't going to sleep so enable interrupts */
    __enable_interrupt();
    __no_operation();
  }

  DEBUG5_LOW();
  
}

/* 8 us */
void UpdateWatchdogInfo(void)
{
  WatchdogInfo.SppReadyToSleep = SerialPortReadyToSleep();

  WatchdogInfo.TaskDelayLockCount = GetTaskDelayLockCount();

  WatchdogInfo.DisplayMessagesWaiting =
    QueueHandles[DISPLAY_QINDEX]->uxMessagesWaiting;

  WatchdogInfo.SppMessagesWaiting = 
    QueueHandles[SPP_TASK_QINDEX]->uxMessagesWaiting;
}


void WatchdogTimeoutHandler(unsigned char ResetSource)
{
  if ( ResetSource == SYSRSTIV_WDTTO || ResetSource == SYSRSTIV_WDTKEY )
  {
    if ( ResetSource == SYSRSTIV_WDTTO )
    {
      PrintString("## Watchdog Failsafe\r\n");
    }
    else
    {
      PrintString("## Forced Watchdog Timeout\r\n");
    }
    
    PrintStringAndDecimal("SppReadyToSleep ",WatchdogInfo.SppReadyToSleep);
    
    PrintStringAndDecimal("TaskDelayLockCount ",
                          WatchdogInfo.TaskDelayLockCount);
    
    PrintStringAndDecimal("DisplayMessagesWaiting ",
                          WatchdogInfo.DisplayMessagesWaiting);
    
    PrintStringAndDecimal("SppMessagesWaiting ",
                          WatchdogInfo.SppMessagesWaiting);
    
    /* 
     * now save the information
     */
    
    unsigned char nvWatchdogCount = 0;
    OsalNvItemInit(NVID_WATCHDOG_RESET_COUNT,
                   sizeof(nvWatchdogCount),
                   &nvWatchdogCount);
    
    PrintStringAndDecimal("Total Watchdogs: ",nvWatchdogCount);
     
    /* If a watchdog is occurring over and over we don't want to ruin the flash.
     * Limit to 255 writes until code is reflashed or phone clears value
     */
    if ( nvWatchdogCount != 0xFF )
    {
      nvWatchdogCount++;
      OsalNvWrite(NVID_WATCHDOG_RESET_COUNT,
                  NV_ZERO_OFFSET,
                  sizeof(nvWatchdogCount),
                  &nvWatchdogCount);
    
      OsalNvItemInit(NVID_WATCHDOG_INFORMATION,
                     sizeof(WatchdogInfo),
                     &WatchdogInfo);
    
      OsalNvWrite(NVID_WATCHDOG_INFORMATION,
                  NV_ZERO_OFFSET,
                  sizeof(WatchdogInfo),
                  &WatchdogInfo);
    }
  }
  
}

/******************************************************************************/
/******************************************************************************/

void CheckQueueUsage(xQueueHandle Qhandle)
{
#if CHECK_QUEUE_USAGE
  portBASE_TYPE waiting = Qhandle->uxMessagesWaiting + 1;
  
  if (  waiting > Qhandle->MaxWaiting )
  {
    Qhandle->MaxWaiting = waiting;    
  }
#endif
}
     
/******************************************************************************/

void RestartWatchdog(void)
{
#if ENABLE_WATCHDOG

  /* set watchdog for 16 second timeout 
   * write password, select aclk, WDTIS_3 means divide by 512*1024 = 16 s;
   * WDTIS_2: 4 mins 
   */
#if USE_FAILSAFE_WATCHDOG
  
  WDTCTL = WDTPW + WDTCNTCL + WDTSSEL__ACLK + WDTIS_3;
  SFRIE1 &= ~WDTIE;
  
#else
  
  WDTCTL = WDTPW + WDTCNTCL + WDTSSEL__ACLK + WDTIS_3 + WDTTMSEL;

  /* enable watchdog timer interrupt */
  SFRIE1 |= WDTIE;

#endif

#endif /* ENABLE_WATCHDOG */
  
}

#define WATCHDOG_LED_DELAY() { __delay_us(2000000); }

/* this is for unrecoverable errors */
void ForceWatchdogReset(void)
{
  __disable_interrupt();

#if USE_LED_FOR_WATCHDOG_DEBUG
  ENABLE_LCD_LED();
  WATCHDOG_LED_DELAY();
#endif
  
#if USE_FAILSAFE_WATCHDOG
  while(1);
#else
  /* write the inverse of the password and cause a reset */
  WDTCTL = ~WDTPW;
#endif
  
}

/******************************************************************************/

/* the timer mode is used when the option USE_FAILSAFE_WATCHDOG == 0 
 * this is for debugging only
 */

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(WatchdogTimerIsr,".text:_isr");
#endif

#ifndef BOOTLOADER
#pragma vector=WDT_VECTOR
__interrupt void WatchdogTimerIsr(void)
#else
void WatchdogTimerIsr(void)
#endif
{
  /* add your debug code here */
  __no_operation();
  
#if USE_LED_FOR_WATCHDOG_DEBUG
  ENABLE_LCD_LED();
  WATCHDOG_LED_DELAY();
#endif

  /* write the inverse of the password and cause a reset */
  WDTCTL = ~WDTPW; 
}

/******************************************************************************/

/* the clearing of the flags cannot be done in the idle loop because
 * it may be interrupted
 */
void TaskCheckIn(etTaskCheckInId TaskId)
{
  portENTER_CRITICAL();  
  
  TaskCheckInFlags |= (1 << (unsigned char)TaskId);

#if WATCHDOG_TEST_MODE == 0
  if ( TaskCheckInFlags == ALL_TASKS_HAVE_CHECKED_IN )
  {
    /* all tasks have checked in - so the flags can be cleared
     * and the watchdog can be kicked
     */
    TaskCheckInFlags = 0;
    RestartWatchdog();
  }
#endif
  
  portEXIT_CRITICAL();
//  PrintStringAndHexByte("- ChkIn:0x", TaskCheckInFlags);
}

/******************************************************************************/

void PrintResetSource(void)
{
  unsigned int source = GetResetSource();
  
#if 1

  PrintStringAndHexByte("ResetSource 0x",(unsigned char)source);

#else
  
  switch(source)
  {
    
  case 0x0000: PrintString("ResetSource 0x0000 - No interrupt pending\r\n"); break;
  case 0x0002: PrintString("ResetSource 0x0002 - Brownout (BOR) (highest priority)\r\n"); break;
  case 0x0004: PrintString("ResetSource 0x0004 - RST/NMI (BOR)\r\n"); break;
  case 0x0006: PrintString("ResetSource 0x0006 - PMMSWBOR (BOR)\r\n"); break;
  case 0x0008: PrintString("ResetSource 0x0008 - Wakeup from LPMx.5 (BOR)\r\n"); break;
  case 0x000A: PrintString("ResetSource 0x000A - Security violation (BOR)\r\n"); break;
  case 0x000C: PrintString("ResetSource 0x000C - SVSL (POR)\r\n"); break;
  case 0x000E: PrintString("ResetSource 0x000E - SVSH (POR)\r\n"); break;
  case 0x0010: PrintString("ResetSource 0x0010 - SVML_OVP (POR)\r\n"); break;
  case 0x0012: PrintString("ResetSource 0x0012 - SVMH_OVP (POR)\r\n"); break;
  case 0x0014: PrintString("ResetSource 0x0014 - PMMSWPOR (POR)\r\n"); break;
  case 0x0016: PrintString("ResetSource 0x0016 - WDT time out (PUC)\r\n"); break;
  case 0x0018: PrintString("ResetSource 0x0018 - WDT password violation (PUC)\r\n"); break;
  case 0x001A: PrintString("ResetSource 0x001A - Flash password violation (PUC)\r\n"); break;
  case 0x001C: PrintString("ResetSource 0x001C - PLL unlock (PUC)\r\n"); break;
  case 0x001E: PrintString("ResetSource 0x001E - PERF peripheral/configuration area fetch (PUC)\r\n"); break;
  case 0x0020: PrintString("ResetSource 0x0020 - PMM password violation (PUC)\r\n"); break;
  default:     PrintString("ResetSource 0x???? - Unknown\r\n"); break;
    
  }
#endif
  
  
}

/******************************************************************************/
