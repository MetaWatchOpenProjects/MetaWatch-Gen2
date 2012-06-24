//==============================================================================
//  Copyright 2012 Meta Watch Ltd. - http://www.MetaWatch.org/
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
/*! \file PreInclude.h
 *
 * This controls the defines that are common between different watch builds
 */
/******************************************************************************/

#ifndef PRE_INCLUDE_H
#define PRE_INCLUDE_H

/* patch selection (include at least one) */
#define INCLUDE_1316_PATCH
//#define INCLUDE_1315_PATCH

#if !defined(INCLUDE_1316_PATCH) && !defined(INCLUDE_1315_PATCH)
#error "At least one patch must be included"
#endif

/* add patch support for BLE */
//#define USE_LOW_ENERGY

/* use DMA to write data to LCD */
#define DMA

/* enable entry into low power mode 3 */
#define LPM_ENABLED

/* print task information */
#undef TASK_DEBUG

/* print debug messages for accelerometer, setup accelerometer for 25 hz */
#undef ACCELEROMETER_DEBUG

/* light LED and wait forever instead of reset */
#undef DEBUG_WATCHDOG_RESET

/* perform software check of errata PMM15 */
#undef CHECK_FOR_PMM15

/* print stack usage information */
#undef CHECK_STACK_USAGE

/* keep track of maximum queue depth */
#undef CHECK_QUEUE_USAGE

/* use debug pin 5 on development board to keep track of when SMCLK is on */
#undef CLOCK_CONTROL_DEBUG

/* set all of the radio control pins to inputs so HCI tester can be used */
#undef ISOLATE_RADIO

#endif /* PRE_INCLUDE_H */
