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
/*! \file hal_analog_display.h
 *
 * The analog movement is controlled by a timer's pwm output.  After the time
 * is set for the first time the hands can be incremented with a command.
 */
/******************************************************************************/

#ifndef HAL_ANALOG_DISPLAY_H
#define HAL_ANALOG_DISPLAY_H

/*! The display timer count value for the analog watch.  This provides a
 * 0.100 Hz rate */
#define ANALOG_DISP_TIMER_DIVIDER  (5120)

/*! Fast advance speed increase */
#define ANALOG_DISP_DIVIDE_RATIO   (512)

/*! Sets up timer 0, B0 to generate the pulses needed to drive the analog 
 * display mechanical movement.
 *
 * It consists of two square waves with a period of 20 seconds.  At each transition
 * there is a 4 mS phase difference that causes the display to move.
 */
void SetupTimerForAnalogDisplay(void);

/*!
 * \param NumberOfSeconds to advance watch hands
 */
void AdvanceAnalogDisplay(unsigned int NumberOfSeconds);

#endif /* HAL_ANALOG_DISPLAY_H */
