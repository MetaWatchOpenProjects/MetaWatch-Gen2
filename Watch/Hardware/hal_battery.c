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

// used to monitor battery status
static unsigned char BatteryChargeBits;

// shows the current charge state
static volatile tBatteryState BatteryState;


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

}

/******************************************************************************/

static unsigned char SavedPowerGood = 0xff;

/*! Update the power good status 
 *
 * \return 1 if power is good status has changed
 *
 */
static unsigned char UpdatePowerGood(unsigned char Value)
{
  unsigned char PowerGoodChanged = 0;
  unsigned char PowerGood = 0;
  
  Value &= BAT_CHARGE_PWR_GOOD;

  /* Bit 5 is low, which means we have input power == good */
  if ( Value == 0 )
  {
    PowerGood = 1;
  }
  else
  {
    PowerGood = 0;
  }
  
  /* only print an update if the value changes */
  if ( SavedPowerGood != PowerGood )
  {
    PowerGoodChanged = 1;
    if ( Value == 0 )
    {
      PrintString("PowerGood ");  
    }
    else
    {
      PrintString("PowerNotGood ");
    }
  }
  SavedPowerGood = PowerGood;
  
  return PowerGoodChanged;
  
}


unsigned char BatteryChargingControl(void)
{
  /* prepare for measurment */
  BAT_CHARGE_OUT |= BAT_CHARGE_OPEN_DRAIN_BITS;

  /* a wait is required between the two steps */
  TaskDelayLpmDisable();
  vTaskDelay(10);
  TaskDelayLpmEnable();
  
  /* take reading */
  BatteryChargeBits = BAT_CHARGE_IN;

  /* measurement is complete */
  BAT_CHARGE_OUT &= ~BAT_CHARGE_OPEN_DRAIN_BITS;

  /* 
   * Update Status Bits
   */
  
  unsigned char BatteryChargeChange = UpdatePowerGood(BatteryChargeBits);
                   
  if( SavedPowerGood )
  {
    BATTERY_CHARGE_ENABLE();
  }
  else
  {
    BATTERY_CHARGE_DISABLE();
  }

  /* This function decodes the port 6 bits to get the battery charger state */
  BatteryState = DecodeBatteryState(BatteryChargeBits);

  return BatteryChargeChange;
  
}


tBatteryState DecodeBatteryState(unsigned char port6Value)
{
  static unsigned char SavedValue;
  
  // Mask to leave the status bits
  port6Value &= (BAT_CHARGE_STAT1 | BAT_CHARGE_STAT2);
  
  // the enum uses the two lsbs in the byte
  port6Value = port6Value >> 3;
  
  port6Value &= 0x03;

  // there isn't a good way to determine if the battery is connected
#if 1
    
  if ( port6Value != SavedValue )
  {
  switch (port6Value)
  {
    case 0x00: PrintString("[Pre] ");   break;
    case 0x02: PrintString("[Fast] ");  break;
    case 0x01: PrintString("[Done] ");  break; 
    case 0x03: PrintString("[Sleep] "); break;
  
  default:
    PrintString("[Bat?]");
    break;
  
  }
  }
  SavedValue = port6Value;
   
#endif
  
  // The port 6 value converted to the typedef enum
  return (tBatteryState)port6Value;
}


unsigned char QueryBatteryCharging(void)
{
  if (   ( BatteryState == BATTERY_STATE_PRECHARGE 
      ||   BatteryState == BATTERY_STATE_FAST_CHARGE )
      && SavedPowerGood  )
  {
    return 1; 
  }
  else
  {
    return 0;  
  }
  
}

unsigned char QueryPowerGood(void)
{ 
  return SavedPowerGood;
}
