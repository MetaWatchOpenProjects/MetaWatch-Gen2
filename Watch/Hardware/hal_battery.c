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
static unsigned char AllowCharge;

void ConfigureBatteryPins(void)
{
  // enable the resistor on the interface pins to the battery charger
  BAT_CHARGE_REN |= BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // Start with these are pull downs so we use less current
  BAT_CHARGE_OUT &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // Set these bits a inputs
  BAT_CHARGE_DIR &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // charge enable output
  BAT_CHARGE_DIR |= BAT_CHARGE_ENABLE_PIN;
  
  BATTERY_CHARGE_DISABLE();
  BatteryChargeEnabled = 0;
  
  CurrentBatteryState = 0x00;
  LastBatteryState = 0xff;
  AllowCharge = 1;
  
}

/******************************************************************************/

unsigned char BatteryChargingControl(void)
{
  unsigned char BatteryChargeChange = 0;

  /* put some hysteresis on charge */
  unsigned int bV = ReadBatterySenseAverage();
  if ( bV > 0 && bV < 4000 )
  {
    AllowCharge = 1;  
  }
  
  if ( AllowCharge )
  {
    /* prepare for measurement */
    BAT_CHARGE_OUT |= BAT_CHARGE_OPEN_DRAIN_BITS;
  
    /* always enable the charger (it may already be enabled) */
    BATTERY_CHARGE_ENABLE();
    BatteryChargeEnabled = 1;
    
    /* a wait is required between the two steps */
    TaskDelayLpmDisable();
    vTaskDelay(10);
    TaskDelayLpmEnable();
    
    /* take reading */
    unsigned char BatteryChargeBits = BAT_CHARGE_IN;
  
#if 0
    /* Bit 5 is low, which means we have input power == good */
    unsigned char PowerGoodN = BatteryChargeBits & BAT_CHARGE_PWR_GOOD;
    if ( PowerGoodN == 0 )
    {
      PowerGood = 1;
    }
    else
    {
      PowerGood = 0;
    }
#endif
    
    /*
     * Decode the battery state
     */
    
    // mask and shift to get the current battery charge status
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
      /* do nothing */ 
      break; 
    
    case BATTERY_CHARGE_DONE: 
    case BATTERY_CHARGE_OFF_FAULT_SLEEP: 
      BATTERY_CHARGE_DISABLE();
      BatteryChargeEnabled = 0;
      AllowCharge = 0;
      BAT_CHARGE_OUT &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
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

unsigned char QueryBatteryChargeEnabled(void)
{
  return BatteryChargeEnabled;  
}