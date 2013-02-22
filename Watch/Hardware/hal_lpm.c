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

/******************************************************************************/
/*! \file hal_lpm.c
 *
 */
/******************************************************************************/
#include "portmacro.h"
#include "intrinsics.h"
#include "hal_board_type.h"
#include "hal_rtos_timer.h"
#include "hal_lpm.h"
#include "hal_miscellaneous.h"
#include "HAL_UCS.h"
#include "Messages.h"
#include "LcdDriver.h"
#include "DebugUart.h"

static unsigned char ShippingModeEnabled = 0;
static unsigned char TaskDelayLpmLockCount = 0;

static void EnterLpm3(void);

void MSP430_LPM_ENTER(void)
{
#if SHIPPING_MODE
  if (ShippingModeEnabled == 1)
  {
    /* Turn off the watchdog timer */
    WDTCTL = WDTPW + WDTHOLD;
#ifdef DIGITAL
    ClearLcd();
#endif
    ConfigResetPin(RST_PIN_ENABLED);
    
    __delay_cycles(100000);
    
    __disable_interrupt();
    __no_operation();
    
    DisableRtosTick();
    
    /* 
     * the radio draws more current in reset than it does after 
     * the patch is loaded
     */
    
    DISABLE_LCD_POWER();
    DISABLE_LCD_ENABLE();
    BATTERY_CHARGE_DISABLE();
    LIGHT_SENSOR_SHUTDOWN();
    BATTERY_SENSE_DISABLE();
    HARDWARE_CFG_SENSE_DISABLE();
    APPLE_POWER_DISABLE();
    ACCELEROMETER_INT_DISABLE();
    DISABLE_BUTTONS();
    
#ifdef DIGITAL
    /* SHIPPING */
    ENABLE_SHIPPING_WAKEUP();
#endif
    
    SELECT_ACLK(SELA__REFOCLK);                
    SELECT_FLLREF(SELREF__REFOCLK); 
    UCSCTL8 &= ~SMCLKREQEN;
    UCSCTL6 |= SMCLKOFF;
    /* disable aclk */
    P11SEL &= ~BIT0;
    XT1_Stop();
    
    /* turn off the regulator */
    PMMCTL0_H = PMMPW_H;
    PMMCTL0_L = PMMREGOFF;
    __low_power_mode_4();
    __no_operation();
    __no_operation();
    
    /* should not get here without a power event */
    SoftwareReset();
  }
  else EnterLpm3(); 
#else
    EnterLpm3();  
#endif
}

static void EnterLpm3(void)
{
#if LPM_ENABLED
//  PrintString2("- EnterLpm", CR);
  /*
   * we are already in critical section so that we do not get switched out by the
   * OS in the middle of stopping the OS Scheduler.
   */
  DisableRtosTick();
  
  /* errata PMM11 + PMM12 divide MCLK by two before going to sleep */
  if ( QueryErrataGroup1() )
  {
    MCLK_DIV(2);
  }
  
  DEBUG1_HIGH();

  /* leave fll control alone SCG0 (ucs7) */
  _BIS_SR(SCG1 + CPUOFF + GIE);
  __no_operation();
  DEBUG1_LOW();

  /* errata PMM11 + PMM12 - wait to put MCLK into normal mode */
  if ( QueryErrataGroup1() )
  {
    __delay_cycles(100);
    MCLK_DIV(1);
  }
  
  /* Generate a vTickIsr by setting the flag to trigger an interrupt
   * You can't call vTaskIncrementTick and vTaskSwitchContext from within a
   * task so do it with an ISR.  We need to cause an OS tick here so that tasks
   * blocked on an event sent by an ISR will run.  FreeRTOS queues them for
   * the next system tick.
   */
  EnableRtosTick();
  RTOS_TICK_SET_IFG();
  
  __no_operation();

#endif  
}

void EnableShippingMode(void)
{
  ShippingModeEnabled = 1;
}

unsigned char ShippingMode(void)
{
  return ShippingModeEnabled;
}

void SoftwareReset(void)
{
  PMMCTL0 = PMMPW | PMMSWBOR;
}

void TaskDelayLpmDisable(void)
{
  portENTER_CRITICAL();
  TaskDelayLpmLockCount++;
  portEXIT_CRITICAL();
}

void TaskDelayLpmEnable(void)
{
  portENTER_CRITICAL();
  if ( TaskDelayLpmLockCount )
  {
    TaskDelayLpmLockCount--;
  }
  portEXIT_CRITICAL();
}

unsigned char GetTaskDelayLockCount(void)
{
  return TaskDelayLpmLockCount;  
}

void ConfigResetPin(unsigned char Set)
{
  if (Set)
  {
    /* enable nmi functionality but don't enable the interrupt */
    SFRRPCR &= ~SYSRSTRE;
    SFRRPCR |= SYSNMI;
  }
  else
  {
    SFRRPCR |= SYSRSTRE;
    SFRRPCR &= ~SYSNMI;
  }
}

