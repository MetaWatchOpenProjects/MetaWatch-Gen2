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
/*! \file hal_lpm.c
 *
 */
/******************************************************************************/
#include "portmacro.h"
#include "hal_board_type.h"
#include "hal_rtos_timer.h"
#include "hal_lpm.h"
#include "HAL_UCS.h"

static void EnterLpm3(void);
static void EnterShippingMode(void);

static unsigned char EnterShippingModeFlag = 0;

static unsigned char TaskDelayLpmLockCount = 0;


void MSP430_LPM_ENTER(void)
{
  if ( EnterShippingModeFlag )
  {
    EnterShippingMode();  
  }
  else
  {
    EnterLpm3();  
  }
  
}


static void EnterLpm3(void)
{
#ifdef LPM_ENABLED
    
  /* Turn off the watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  /*
   * Enter a critical section to do so that we do not get switched out by the
   * OS in the middle of stopping the OS Scheduler.
   */
  __disable_interrupt();
  __no_operation();
  DisableRtosTick();
  
  /* errata PMM11 divide MCLK by two before going to sleep */
  MCLK_DIV(2);
  DEBUG1_HIGH();
  
  __enable_interrupt();
  LPM3;
  __no_operation();
  DEBUG1_LOW();

  /* errata PMM11 - wait to put MCLK into normal mode */
  __delay_cycles(100);
  MCLK_DIV(1);
  
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



void SetShippingModeFlag(void)
{
  EnterShippingModeFlag = 1;  
}

void ClearShippingModeFlag(void)
{
  EnterShippingModeFlag = 0;  
}

static void EnterShippingMode(void)
{
  /* Turn off the watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;
  
  EnableRstPin();
  
  __delay_cycles(100000);
  
  __disable_interrupt();
  __no_operation();
  
  /* 
   * the radio draws more current in reset than it does after 
   * the patch is loaded
   */
  
  DISABLE_DISPLAY_POWER();
  DISABLE_LCD_ENABLE();
  BATTERY_CHARGE_DISABLE();
  LIGHT_SENSOR_SHUTDOWN();
  BATTERY_SENSE_DISABLE();
  HARDWARE_CFG_SENSE_DISABLE();
  APPLE_POWER_DISABLE();
  ACCELEROMETER_INT_DISABLE();
  DISABLE_BUTTONS();

  SELECT_ACLK(SELA__REFOCLK);                
  SELECT_FLLREF(SELREF__REFOCLK); 
  UCSCTL8 &= ~SMCLKREQEN;
  UCSCTL6 |= SMCLKOFF;
  /* disable aclk */
  P11SEL &= ~BIT0;
  XT1_Stop();
  
  /* turn off the regulator */
  unsigned char temp = PMMCTL0_L;
  PMMCTL0_H = PMMPW_H;
  PMMCTL0_L = PMMREGOFF | temp;
  LPM4;
  __no_operation();
  __no_operation();
  
  /* should not get here without a power event */
  SoftwareReset();
}

void SoftwareReset(void)
{
  /* let the uart drain */
  __delay_cycles(100000);
  
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

/******************************************************************************/

extern unsigned char nvRstNmiConfiguration;

unsigned char QueryRstPinEnabled(void)
{
  return ( nvRstNmiConfiguration == RST_PIN_ENABLED );  
}

void ConfigureResetPinFunction(unsigned char Control)
{
  switch (Control)
  {
  case RST_PIN_ENABLED:
    nvRstNmiConfiguration = RST_PIN_ENABLED;
    SFRRPCR |= SYSRSTRE;
    SFRRPCR &= ~SYSNMI;
    break;
    
  case RST_PIN_DISABLED:
  default:
    /* enable nmi functionality but don't enable the interrupt */
    nvRstNmiConfiguration = RST_PIN_DISABLED;
    SFRRPCR &= ~SYSRSTRE;
    SFRRPCR |= SYSNMI;
    break;
  
  }
}

void EnableRstPin(void)
{
  ConfigureResetPinFunction(RST_PIN_ENABLED);  
}

void DisableRstPin(void)
{
  ConfigureResetPinFunction(RST_PIN_DISABLED);  
}
