//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

__no_init __root static tWatchdogInfo WatchdogInfo @ WATCHDOG_INFO_ADDR;
__no_init __root static unsigned int niWdtCounter @ WATCHDOG_COUNTER_ADDR;
extern unsigned int niReset;

static void PrintResetSource(unsigned int Source);

void vApplicationIdleHook(void)
{

  /* Put the processor to sleep if the serial port indicates it is OK and
   * all of the queues are empty.
   *
   * This will stop the OS scheduler.
   */ 
#if 0
  DEBUG5_HIGH();
#endif

  /* enter a critical section so that the flags can be checked */
  __disable_interrupt();
  __no_operation();
  
  /* the watchdog is set at 16 seconds.
   * the battery interval rate is set a 10 seconds
   * each task checks in at the battery interval rate
   */
  UpdateWatchdogInfo();

  if ((WatchdogInfo.SppReadyToSleep &&
      WatchdogInfo.TaskDelayLockCount == 0 &&
      WatchdogInfo.DisplayMessagesWaiting == 0 &&
      WatchdogInfo.SppMessagesWaiting == 0) ||
      ShippingMode())
  {
    /* Call MSP430 Utility function to enable low power mode 3.     */
    /* Put OS and Processor to sleep. Will need an interrupt        */
    /* to wake us up from here.   */
    MSP430_LPM_ENTER();

    /* If we get here then interrupts are enabled */
//    extern xTaskHandle IdleTaskHandle;
//    CheckStackUsage(IdleTaskHandle,"~IdlTsk ");
  }
  else
  {
    /* we aren't going to sleep so enable interrupts */
    __enable_interrupt();
    __no_operation();
  }

#if 0
  DEBUG5_LOW();
#endif
}

void CheckQueueUsage(xQueueHandle Qhandle)
{
#if CHECK_QUEUE_USAGE
  portBASE_TYPE waiting = Qhandle->uxMessagesWaiting + 1;
  
  if (waiting > Qhandle->MaxWaiting)
  {
    Qhandle->MaxWaiting = waiting;    
  }
#endif
}
     
/******************************************************************************/

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

void ShowWatchdogInfo(void)
{
  if (niReset == FLASH_RESET_CODE) niWdtCounter = 0;

  unsigned int ResetSource = GetResetSource();
  PrintResetSource(ResetSource);

  if (ResetSource == SYSRSTIV_WDTTO || ResetSource == SYSRSTIV_WDTKEY)
  {
    PrintString3("# WDT ", ResetSource == SYSRSTIV_WDTTO ? "Failsafe" : "Forced", CR);
    PrintStringAndDecimal("SppReadyToSleep ", WatchdogInfo.SppReadyToSleep);
    PrintStringAndDecimal("TaskDelayLockCount ", WatchdogInfo.TaskDelayLockCount);
    PrintStringAndDecimal("DisplayMsgWaiting ", WatchdogInfo.DisplayMessagesWaiting);
    PrintStringAndDecimal("SppMsgWaiting ", WatchdogInfo.SppMessagesWaiting);
    niWdtCounter ++;
  }
  
  PrintStringAndDecimal("Total Watchdogs: ", niWdtCounter);
}

void ResetWatchdog(void)
{
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
}

#define WATCHDOG_LED_DELAY() { __delay_us(2000000); }

/* this is for unrecoverable errors */
void WatchdogReset(void)
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

  // BOR reset
//  PMMCTL0 = PMMPW | PMMSWBOR;
  /* write the inverse of the password and cause a PUC reset */
  WDTCTL = ~WDTPW; 
}

/******************************************************************************/

/* the clearing of the flags cannot be done in the idle loop because
 * it may be interrupted
 */
void TaskCheckIn(etTaskCheckInId TaskId)
{
  static unsigned char TaskCheckInFlags = 0;

  portENTER_CRITICAL();
  
  TaskCheckInFlags |= (1 << TaskId);

  if (TaskCheckInFlags == ALL_TASKS_HAVE_CHECKED_IN)
  {
    /* all tasks have checked in - so the flags can be cleared
     * and the watchdog can be kicked
     */
    TaskCheckInFlags = 0;
    ResetWatchdog();
  }
  
  portEXIT_CRITICAL();
}

/* Prints reset code and the interrupt type */
static void PrintResetSource(unsigned int Source)
{  
  PrintString("ResetSource 0x");
  PrintHex(Source);

#if 0
  PrintString(" - ");
  switch (Source)
  {
  case 0x0000: PrintString("No interrupt pending"); break;
  case 0x0002: PrintString("Brownout (BOR) (highest priority)"); break;
  case 0x0004: PrintString("RST/NMI (BOR)"); break;
  case 0x0006: PrintString("PMMSWBOR (BOR)"); break;
  case 0x0008: PrintString("Wakeup from LPMx.5 (BOR)"); break;
  case 0x000A: PrintString("Security violation (BOR)"); break;
  case 0x000C: PrintString("SVSL (POR)"); break;
  case 0x000E: PrintString("SVSH (POR)"); break;
  case 0x0010: PrintString("SVML_OVP (POR)"); break;
  case 0x0012: PrintString("SVMH_OVP (POR)"); break;
  case 0x0014: PrintString("PMMSWPOR (POR)"); break;
  case 0x0016: PrintString("WDT time out (PUC)"); break;
  case 0x0018: PrintString("WDT password violation (PUC)"); break;
  case 0x001A: PrintString("Flash password violation (PUC)"); break;
  case 0x001C: PrintString("PLL unlock (PUC)"); break;
  case 0x001E: PrintString("PERF peripheral/configuration area fetch (PUC)"); break;
  case 0x0020: PrintString("PMM password violation (PUC)"); break;
  default:     PrintString("Unknown"); break;
  }
#endif
  PrintString(CR);
}
