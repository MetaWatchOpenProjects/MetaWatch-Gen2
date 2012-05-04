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
/*! \file hal_calibration.h
 *
 * This contains the functions that use the calibration data that is programmed
 * into information flash during production.
 */
/******************************************************************************/

#ifndef HAL_CALIBRATION_H
#define HAL_CALIBRATION_H

/*! Read the calibration data from information memory segment A and determine
 * if is valid.
 *
 * This should be the first thing called in main after disabling watchdog.
 *
 */
void InitializeCalibrationData(void);

/*!
 * \return 1 if calibration is valid
 */
unsigned char QueryCalibrationValid(void);


/*!
 * \return Battery calibration value in the range of 0-254 adc counts
 */
unsigned char GetBatteryCalibrationValue(void);


/*! Set the values for the capacitors connected to XT1 */
void SetOscillatorCapacitorValues(void);


/*!
 * \return RTC calibration value in the range of -63 to +63
 */
signed char GetRtcCalibrationValue(void);

#endif /* HAL_CALBRATION_H */
