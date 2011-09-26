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
/*! \file hal_battery.h
 *
 * Battery charging and power good detection.
 */
/******************************************************************************/

#ifndef HAL_BATTERY_H
#define HAL_BATTERY_H

/*! The state of the battery charging circuit */
typedef enum
{
  BATTERY_STATE_PRECHARGE = 0x00,
  BATTERY_STATE_CHARGE_DONE = 0x01,
  BATTERY_STATE_FAST_CHARGE = 0x02,
  BATTERY_STATE_CHARGE_OFF  = 0x03,

} tBatteryState;  


/*! Decodes the bits on port 6 to determine the battery state
 *
 *   Stat2    Stat1      Status                  Hex value
 *    0          0       Precharge                   0x00
 *    1          0       Fast charge                 0x10
 *    0          1       Charge Done                 0x08
 *    1          1       Timer Fault or Sleep        0x18
 *
 * \param port6Value is the is the status inputs from the BQ24080
 *
 * \return state of the battery charging circuit based on input pins.
 */
tBatteryState DecodeBatteryState(unsigned char port6Value);

/*! Configure the pins used for reading the battery state and 
 * charging the battery.
 */
void ConfigureBatteryPins(void);

/*! Read battery state and update charge output 
 *
 * \return 1 if the state of power good has changed, 0 otherwise
 *
 * \note Status can be used to update the LCD screen of a possible change
 * in battery charging state
 */
unsigned char BatteryChargingControl(void);


/*! Query whether or not the battery is charging
 * If powergood isn't valid then neither are the charge bits 
 *
 * \return 1 if the battery is charging else 0
 */
unsigned char QueryBatteryCharging(void);

/*!
 * \return 1 if the power is good and 0 otherwise.
 */
unsigned char QueryPowerGood(void);

#endif // HAL_BATTERY_H