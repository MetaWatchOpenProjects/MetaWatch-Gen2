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
/*! \file hal_vibe.h
*
* The vibrator has a global enable/disable.  The vibrator control signal is
* pwmed so that when it is on it is really getting 'square' wave.
*/
/******************************************************************************/

#ifndef HAL_VIBE_H
#define HAL_VIBE_H

/*! Set the state of the vibration 
 *
 * \param motorOn 0 = turns off vibration and all other values turn on vibration
 *
 */
void SetVibeMotorState(unsigned char motorOn);

/*! Enable the timer that controls the vibrator PWM */
void EnableVibratorPwm(void);

/*! Disable the timer that controls the vibrator PWM */
void DisableVibratorPwm(void);

/*! Setup the timer that controls the PWM  for the vibrator*/
void SetupVibrationMotorTimerAndPwm(void);

/*! Enable vibration events */
void VibeEnable(void);

/*! Disable vibration */
void VibeDisable(void);

#endif /* HAL_VIBE_H */
