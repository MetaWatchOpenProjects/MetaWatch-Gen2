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

#define BATTERY         (0)
#define LIGHT_SENSOR    (1)

#define CRITICAL_BT_OFF  (0)
#define CRITICAL_WARNING (1)

#define BATTERY_FULL_LEVEL        (4175) //4210 4140
#define BATTERY_CRITICAL_LEVEL    (3300)
#define BATTERY_LEVEL_RANGE       (BATTERY_FULL_LEVEL - BATTERY_CRITICAL_LEVEL)
#define BATTERY_LEVEL_NUMBER      (7)
#define BATTERY_LEVEL_INTERVAL    (BATTERY_LEVEL_RANGE / BATTERY_LEVEL_NUMBER)

#define DARK_LEVEL              (3)
#define BATTERY_PERCENT_UNKNOWN (101)

void InitAdc(void);

/*! Returns the average of the last 8 sensor Sense ADC cycles
 *\return sensor voltage in millivolts
 */
unsigned int Read(unsigned char Sensor);

/*! Start an ADC cycle to read battery voltage.  This function must
 * be called from a task.  It will return when finished.  The result can be 
 * read using the command ReadBatterySense.
 */
void BatterySenseCycle(void);

/*! Set the values of the low battery warning message and the value at which
 * the bluetooth radio should be turned off
 *
 * \param pData first character is LowBatteryWarningLevel in 10ths of a volt
 * and second byte is LowBatteryBtOffLevel in 10ths of a volt (37 = 3.7 volts)
 */
void SetBatteryLevels(unsigned char *pData);

unsigned int BatteryCriticalLevel(unsigned char Type);

unsigned char BatteryPercentage(void);

/*! Reads battery sense value and takes the appropriate action.
 *\note This function is meant to be called from a task.  When the low battery
 * warning level is reached a message is sent to the host.  When the low battery
 * bluetooth off message is reached then watch will vibrate, a message will be
 * sent to the phone, and the bluetooth radio will be turned off.
 * 
 */
void CheckBatteryLow(void);

/*! Start an ADC cycle to read the light sensor.  This function must
 * be called from a task.  It will return when finished.  It waits until the
 * light sensor is ready to send data. The result can be read using the calling
 * ReadLightSense.
 */
unsigned int LightSenseCycle(void);

void ReadLightSensorHandler(void);

#endif // ADC_H
