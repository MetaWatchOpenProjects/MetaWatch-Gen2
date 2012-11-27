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

#define CRITICAL_BT_OFF  (0)
#define CRITICAL_WARNING (1)

/*! Initialize the Analog-to-Digital Conversion peripheral.  Set the outputs from
 * the micro to the correct state.  The ADC is used to read the 
 * hardware configuration registers, the battery voltage, and the value from the
 * light sensor.
*/
void InitializeAdc(void);

/*! Start an ADC cycle to read battery voltage.  This function must
 * be called from a task.  It will return when finished.  The result can be 
 * read using the command ReadBatterySense.
 */
void BatterySenseCycle(void);

/*! Start an ADC cycle to read the light sensor.  This function must
 * be called from a task.  It will return when finished.  It waits until the
 * light sensor is ready to send data. The result can be read using the calling
 * ReadLightSense.
 */
void LightSenseCycle(void);

/*! Returns the last Battery Sense value
 *
 *\return Battery Voltage in millivolts
 */
unsigned int ReadBatterySense(void);

/*! Returns the last Light Sense value
 *
 *\return Light Sense in millivolts
 */
unsigned int ReadLightSense(void);

/*! Returns the average of the last 10 Battery Sense ADC cycles 
 *
 *\return Battery Voltage in millivolts
 */
unsigned int ReadBatterySenseAverage(void);

/*! Returns the average of the last 10 Light Sensor analog to digital 
 * conversions
 *
 *\return Light Sense in millivolts
 */
unsigned int ReadLightSenseAverage(void);


/*! Set the values of the low battery warning message and the value at which
 * the bluetooth radio should be turned off
 *
 * \param pData first character is LowBatteryWarningLevel in 10ths of a volt
 * and second byte is LowBatteryBtOffLevel in 10ths of a volt (37 = 3.7 volts)
 */
void SetBatteryLevels(unsigned char * pData);

unsigned int BatteryCriticalLevel(unsigned char Type);

/*! Reads battery sense value and takes the appropriate action.
 *
 *\param PowerGood 1 if power is good (5V), 0 otherwise
 * 
 *\note This function is meant to be called from a task.  When the low battery
 * warning level is reached a message is sent to the host.  When the low battery
 * bluetooth off message is reached then watch will vibrate, a message will be
 * sent to the phone, and the bluetooth radio will be turned off.
 * 
 */


void LowBatteryMonitor(unsigned char PowerGood);


/*! Set the default values for the low battery levels stored in flash if they
 * do not exist.  If they exists then read them from flash and store them
 * into a variable in ram
 */
void InitializeLowBatteryLevels(void);

/******************************************************************************/

/*! The board revision was going to be used to determine what patch to load
 *
 */
unsigned char GetBoardConfiguration(void);

#endif // ADC_H
