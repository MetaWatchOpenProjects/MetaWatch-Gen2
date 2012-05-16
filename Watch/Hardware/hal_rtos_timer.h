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
/*! \file hal_rtos_timer.h
 *
 * The timer for the RTOS is shared with 4 other crystal timers that use the other
 * compare registers.
 */
/******************************************************************************/

#ifndef HAL_RTOS_TIMER_H
#define HAL_RTOS_TIMER_H

/*! Macro for setting the RTOS tick interrupt flag */
#define RTOS_TICK_SET_IFG() { TA0CCR0 |= CCIFG; }

/*! Enable the RTOS tick (and the RTOS) */
void EnableRtosTick(void);

/*! Disable the RTOS tick (and the RTOS) */
void DisableRtosTick(void);

/*! \return 0 if Tick is Disabled , 1 if RTOS tick is enabled */
unsigned char QuerySchedulerState(void);

#endif // HAL_RTOS_TIMER_H

