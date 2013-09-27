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

#define CLIP_OFF        0
#define CLIP_ON         1
#define CLIP_INIT       0xFF

void InitBattery(void);

/*! Query whether or not the battery is charging */
unsigned char Charging(void);

/*! Return whether the power from the charger is present */
unsigned char ClipOn(void);

/*! Return whether clip state is changed */
unsigned char CheckClip(void);

/*! read battery, charging or checkbatterylow */
void CheckBattery(void);

/*! Returns the average of the last 4 sensor Sense ADC cycles
 *\return battery reading in percentage
 */
unsigned char BatteryPercentage(void);

unsigned char ChargeEnabled(void);
void ToggleCharging(void);

#endif // HAL_BATTERY_H
