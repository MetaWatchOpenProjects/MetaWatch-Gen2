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
/*! \file Adc.h
 *
 * Control the Analog to Digital Conversion function in the MSP430.  The 
 * functions must be called from a task (they block).
 *
 * The hardware configuration cycle may be deprecated because a board version 
 * is programmed during production.
 */
/******************************************************************************/

#ifndef ADC_H
#define ADC_H

#define BATTERY           0
#define LIGHT_SENSOR      1

#define CRITICAL_BT_OFF   0
#define CRITICAL_WARNING  1

#define BATTERY_FULL_LEVEL        (4140) //4210 4140
#define BATTERY_CRITICAL_LEVEL    (3300)
#define BATTERY_LEVEL_RANGE       (BATTERY_FULL_LEVEL - BATTERY_CRITICAL_LEVEL)
#define BATTERY_LEVEL_NUMBER      (7)
#define BATTERY_LEVEL_INTERVAL    14 //(BATTERY_LEVEL_RANGE / BATTERY_LEVEL_NUMBER)
#define LEVEL_RANGE_ONETENTH      (BATTERY_LEVEL_RANGE / 10) //* 205) >> 11)
#define LEVEL_RANGE_ONEPERCENT    (BATTERY_LEVEL_RANGE / 100) // * 41) >> 12)


#define WARNING_LEVEL             20 //3468 //20% * (4140 - 3300) + 3300
#define RADIO_OFF_LEVEL           0

#define DARK_LEVEL                3
#define BATTERY_UNKNOWN           101

void InitAdc(void);

/*! Start an ADC cycle to read battery voltage.  This function must
 * be called from a task.  It will return when finished.  The result can be 
 * read using the command ReadBatterySense.
 */
unsigned char ReadBattery(void);

/*! Set the values of the low battery warning message and the value at which
 * the bluetooth radio should be turned off
 *
 * \param pData first character is LowBatteryWarningLevel in 10ths of a volt
 * and second byte is LowBatteryBtOffLevel in 10ths of a volt (37 = 3.7 volts)
 */
void SetBatteryLevels(unsigned char *pData);

/*! Start an ADC cycle to read the light sensor.  This function must
 * be called from a task.  It will return when finished.  It waits until the
 * light sensor is ready to send data. The result can be read using the calling
 * ReadLightSense.
 */
unsigned int LightSenseCycle(void);

void ReadLightSensorHandler(void);

#endif // ADC_H
