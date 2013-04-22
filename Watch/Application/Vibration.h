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
/*! \file Vibration.h
 *
 * This was its own task but now it is part of the control task.  The vibrator 
 * is controlled by a PWM output that is cycle on and off based on a timer. 
 */
/******************************************************************************/

#ifndef VIBRATION_H
#define VIBRATION_H

/*! Set Vibrate Mode Payload Structure
 *
 * \param Enable when > 0 disabled when == 0.
 * \param OnDuration is the duration in milliseconds
 * \param OffDuration is the off duration in milliseconds.
 * \param NumberOfCycles is the number of on/off cycles to perform
 *
 * \note when durations were changed to integers the on duration was correct
 * but the number of cycles became first byte of checksum (packing problem)
 */
typedef struct
{
  unsigned char Enable;
  unsigned char OnDurationLsb;
  unsigned char OnDurationMsb;
  unsigned char OffDurationLsb;
  unsigned char OffDurationMsb;
  unsigned char NumberOfCycles;

} tSetVibrateModePayload;


/*! Setup the timer that controls vibration and setup
 * the pins that control the motor
 */
void InitVibration(void);

/*! The function is called periodically by the control task to control the
 * pulsing of the vibration motor on and off.
 */
void VibrationMotorStateMachineIsr(void);

/*! Parse the message from the phone
 *
 * \param pMsg - Message from the host containing vibration information
 */
void SetVibrateModeHandler(tMessage* pMsg);

#endif /* VIBRATION_H */ 
