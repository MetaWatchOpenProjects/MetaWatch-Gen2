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

#include <string.h>
#include "FreeRTOS.h"
#include "portmacro.h"
#include "hal_lpm.h"
#include "hal_battery.h"
#include "hal_board_type.h"
#include "IdleTask.h"
#include "hal_boot.h"
#include "Log.h"
#include "queue.h"
#include "Wrapper.h"
#include "Messages.h"

#if __IAR_SYSTEMS_ICC__
__no_init __root unsigned char niRadioReadyToSleep @ RADIO_SLEEP_ADDR;
__no_init __root unsigned char niDisplayQueue @ DISPLAY_QUEUE_ADDR;
__no_init __root unsigned char niWrapperQueue @ WRAPPER_QUEUE_ADDR;
__no_init __root unsigned char niWdtNum @ WDT_NUM_ADDR;
#else
extern unsigned char niRadioReadyToSleep;
extern unsigned char niDisplayQueue;
extern unsigned char niWrapperQueue;
extern unsigned char niWdtNum;
#endif

extern unsigned char niResetCode;
extern xQueueHandle QueueHandles[];

static unsigned char Interval = WDT_SHORT;
static unsigned char LpmChecking = FALSE;

void SetWatchdogInterval(unsigned char Intvl)
{
  Interval = Intvl;
}

void ResetWatchdog(void)
{
  /* set watchdog for 16 second timeout
   * write password, select aclk, WDTIS_3 means divide by 512*1024 = 16s;
   * WDTIS_2: 4 mins 
   */
#if USE_FAILSAFE_WATCHDOG
  
  WDTCTL = WDTPW + WDTCNTCL + WDTSSEL__ACLK + Interval;
  SFRIE1 &= ~WDTIE;
  
#else
  
  WDTCTL = WDTPW + WDTCNTCL + WDTSSEL__ACLK + Interval + WDTTMSEL;

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

  niWdtNum ++;
  niResetCode = RESET_WDT;

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

  niWdtNum ++;
  niResetCode = RESET_WDT;
  /* write the inverse of the password and cause a PUC reset */
  WDTCTL = ~WDTPW; 
}

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
    ResetWatchdog();
    TaskCheckInFlags = 0;
  }
  
  portEXIT_CRITICAL();
}

/* 8 us */
void UpdateQueueInfo(void)
{
  niRadioReadyToSleep = ReadyToSleep();
  niDisplayQueue = QueueHandles[DISPLAY_QINDEX]->uxMessagesWaiting;
  niWrapperQueue = QueueHandles[WRAPPER_QINDEX]->uxMessagesWaiting;
}

void vApplicationIdleHook(void)
{
  /* Put the processor to sleep if the serial port indicates it is OK and
   * all of the queues are empty.
   * This will stop the OS scheduler.
   */ 

  /* enter a critical section so that the flags can be checked */
  __disable_interrupt();
  __no_operation();

#if LOGGING
  /* the watchdog is set at 16 seconds.
   * the battery interval rate is set a 10 seconds
   * each task checks in at the battery interval rate
   */
  UpdateQueueInfo();
#endif

#if SUPPORT_LPM
  if (niRadioReadyToSleep && niDisplayQueue == 0 && niWrapperQueue == 0)
  {
    /* Call MSP430 Utility function to enable low power mode 3.     */
    /* Put OS and Processor to sleep. Will need an interrupt        */
    /* to wake us up from here.   */
    if (LpmChecking) DISABLE_LCD_LED();
    EnterLpm3();
    if (LpmChecking) ENABLE_LCD_LED();

//  __enable_interrupt();
//  __no_operation();
//  
//    /* If we get here then interrupts are enabled */
//    return;
  }
#endif

  /* we aren't going to sleep so enable interrupts */
  __enable_interrupt();
  __no_operation();
}

void CheckLpm(void)
{
  LpmChecking = !LpmChecking;
}
