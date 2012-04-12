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
/*! \file hal_battery.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"

#include "hal_board_type.h"
#include "hal_battery.h"

#include "DebugUart.h"
#include "Display.h"
#include "Adc.h"

// used to monitor battery status
static unsigned char CurrentBatteryState;
static unsigned char LastBatteryState;
static unsigned char BatteryChargeEnabled;
static unsigned char PowerGood;

#define BAT_CHARGE_OPEN_DRAIN_BITS \
  ( BAT_CHARGE_STAT1 + BAT_CHARGE_STAT2 + BAT_CHARGE_PWR_GOOD )

#define BAT_CHARGE_STATUS_OPEN_DRAIN_BITS \
  ( BAT_CHARGE_STAT1 + BAT_CHARGE_STAT2 )

void ConfigureBatteryPins(void)
{
  // enable the resistor on the interface pins to the battery charger
  BAT_CHARGE_REN |= BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // Start with these as pull downs so we use less current
  BAT_CHARGE_OUT &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // Set these bits as inputs
  BAT_CHARGE_DIR &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // charge enable output
  BAT_CHARGE_DIR |= BAT_CHARGE_ENABLE_PIN;
  
  BATTERY_CHARGE_DISABLE();
  BatteryChargeEnabled = 0;
  CurrentBatteryState = BATTERY_CHARGE_OFF_FAULT_SLEEP;
  
  CurrentBatteryState = 0x00;
  LastBatteryState = 0xff;
  
}

/******************************************************************************/

unsigned char BatteryChargingControl(void)
{
  unsigned char BatteryChargeChange = 0;

  /* 
   * first find out if the charger is connected
   */
  
  /* turn on the pullup in the MSP430 for the open drain bit of the charger */
  BAT_CHARGE_OUT |= BAT_CHARGE_PWR_GOOD;
    
  TaskDelayLpmDisable();
  vTaskDelay(1);
  TaskDelayLpmEnable();
    
  /* take reading */
  unsigned char BatteryChargeBits = BAT_CHARGE_IN;
  unsigned char PowerGoodN = BatteryChargeBits & BAT_CHARGE_PWR_GOOD;
 
  /* Bit 5 is low, which means we have input power == good */
  if ( PowerGoodN == 0 )
  {
    PowerGood = 1;
  }
  else
  {
    PowerGood = 0;
  }
  
  /* turn off the pullup */
  BAT_CHARGE_OUT &= ~BAT_CHARGE_PWR_GOOD;
  
  if ( PowerGood == 0 )
  {
    /* the charger automatically shuts down when power is not good
     * need to make sure pullups aren't enabled.
     */
    if ( BatteryChargeEnabled == 1 )
    {
      BatteryChargeEnabled = 0;
      BatteryChargeChange = 1;
    }
    
    BATTERY_CHARGE_DISABLE();
    CurrentBatteryState = BATTERY_CHARGE_OFF_FAULT_SLEEP; 
    BAT_CHARGE_OUT &= ~BAT_CHARGE_STATUS_OPEN_DRAIN_BITS;
      
  }
  else
  {
    /* prepare to read status bits */
    BAT_CHARGE_OUT |= BAT_CHARGE_STATUS_OPEN_DRAIN_BITS;
  
    /* always enable the charger (it may already be enabled)
     * status bits are not valid unless charger is enabled 
     */
    BATTERY_CHARGE_ENABLE();
    BatteryChargeEnabled = 1;
  
    /* wait until signals are valid - measured 400 us */
    TaskDelayLpmDisable();
    vTaskDelay(1);
    TaskDelayLpmEnable();
    
    /* take reading */
    BatteryChargeBits = BAT_CHARGE_IN;
    
    /*
     * Decode the battery state
     */
    
    /* mask and shift to get the current battery charge status */
    BatteryChargeBits &= (BAT_CHARGE_STAT1 | BAT_CHARGE_STAT2);
    CurrentBatteryState = BatteryChargeBits >> 3;
    
    if ( LastBatteryState != CurrentBatteryState )
    {
      BatteryChargeChange = 1;
    }
    LastBatteryState = CurrentBatteryState;
   
    if ( QueryBatteryDebug() )
    {
      if ( BatteryChargeChange )
      {
        switch (CurrentBatteryState)
        {
        case BATTERY_PRECHARGE:              PrintString("[Pre] ");   break;
        case BATTERY_FAST_CHARGE:            PrintString("[Fast] ");  break;
        case BATTERY_CHARGE_DONE:            PrintString("[Done] ");  break; 
        case BATTERY_CHARGE_OFF_FAULT_SLEEP: PrintString("[Sleep] "); break;
        default:                             PrintString("[Bat?]");   break;
        }
        
      }
    }
    
    /* if the part is in sleep mode or there was a fault then
     * disable the charge control and disable the pullups 
     *
     * any faults will be cleared by the high-to-low transition on the enable pin
     */
    switch (CurrentBatteryState)
    {
    case BATTERY_PRECHARGE:
    case BATTERY_FAST_CHARGE:
    case BATTERY_CHARGE_DONE: 
      /* do nothing */ 
      break; 
    
    case BATTERY_CHARGE_OFF_FAULT_SLEEP: 
      BATTERY_CHARGE_DISABLE();
      BatteryChargeEnabled = 0;
      BAT_CHARGE_OUT &= ~BAT_CHARGE_STATUS_OPEN_DRAIN_BITS;
      break;
    
    default:   
      break;
    }

  }
  
  return BatteryChargeChange;
  
}


unsigned char QueryBatteryCharging(void)
{
  if (   CurrentBatteryState == BATTERY_PRECHARGE 
      || CurrentBatteryState == BATTERY_FAST_CHARGE )
  {
    return 1; 
  }
  else
  {
    return 0;  
  }
  
}

/* is 5V connected? */
unsigned char QueryPowerGood(void)
{
  return PowerGood;  
}

/* the charge enable bit is used to get status information so
 * knowing its value cannot be used to determine if the battery 
 * is being charged
 */
unsigned char QueryBatteryChargeEnabled(void)
{
  return BatteryChargeEnabled;  
}
