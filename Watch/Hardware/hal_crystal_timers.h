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
/*! \file hal_crystal_timers.h
*
* Timers based off of the 32.768 watch crystal. These share the timer used
* by the rtos timer and are defined in hal_rtos_timer.c
*
* These timers are not managed 'automatically'
*
* ID1 is used by the stack
* ID2 is used by the OLED
*/
/******************************************************************************/

#ifndef HAL_CRYSTAL_TIMERS
#define HAL_CRYSTAL_TIMERS

/*! Crystal timer 1 is used by the serial port profile */
#define CRYSTAL_TIMER_ID1 ( 1 )

/*! Crystal timer 1 is used by the OLED display task */
#define CRYSTAL_TIMER_ID2 ( 2 )

/*! Crystal timer 3 is unused */
#define CRYSTAL_TIMER_ID3 ( 3 )

/*! Crystal timer 4 is unused */
#define CRYSTAL_TIMER_ID4 ( 4 )

/*! Start a timer that will expire in the specified number of ticks 
 *
 * \param TimerId
 * \param pCallback is a pointer to the function to call when the timer expires
 * \param Ticks are 30.5176 us (1/32.768kHz)
 *
 * \note Callback will be called in interrupt context
 */
void StartCrystalTimer(unsigned char TimerId,
                       unsigned char (*pCallback) (void),
                       unsigned int Ticks);

/*! Stop the specified timer
 *
 * \param TimerId
 */
void StopCrystalTimer(unsigned char TimerId);

#endif /* HAL_CRYSTAL_TIMERS */
