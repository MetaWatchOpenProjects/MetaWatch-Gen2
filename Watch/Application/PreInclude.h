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

/* patch selection (include at least one in IAR or CCS defined symbols) */
//#define INCLUDE_1316_PATCH ( 1 ) 
//#define INCLUDE_1315_PATCH ( 0 )

#if !INCLUDE_1316_PATCH && !INCLUDE_1315_PATCH
  #error "At least one patch must be included"
#endif

/*! set this to one to enable watchdog */
#define ENABLE_WATCHDOG ( 1 )

/*! 0 = normal operation 
 *  1 = don't kick the watchdog test
 *  2 = call force watchdog reset test
 *  others = disabled
 */
#define WATCHDOG_TEST_MODE ( 0 )
     
/*! when 0 the watchdog timer and interrupt is used and an invalid password
 * is used to reset the part, when 1 the watchdog expiring will cause the micro
 * to reset and if ACLK goes away VLOCLK will be used.
 */
#define USE_FAILSAFE_WATCHDOG ( 1 )

/*! light LED */
#define USE_LED_FOR_WATCHDOG_DEBUG ( 0 ) 

/*! use DMA to write data to LCD */
#define LCD_DMA ( 1 )

/*! enable entry into low power mode 3 */
#define SUPPORT_LPM ( 1 ) 

#define SUPPORT_SHIPPING_MODE (0)

/*! print task information */
#define TASK_DEBUG ( 0 )

/*! perform software check of errata PMM15 */
#define CHECK_FOR_PMM15 ( 1 ) 

/*! print stack usage information */
#define CHECK_STACK_USAGE ( 1 )

/*! keep track of maximum queue depth */
#define CHECK_QUEUE_USAGE ( 0 )

/*! use debug pin 5 on development board to keep track of when SMCLK is on */
#define CLOCK_CONTROL_DEBUG ( 0 )

/*! set all of the radio control pins to inputs so HCI tester can be used */
#define ISOLATE_RADIO ( 0 )

/*! print the value of power good */
#define DEBUG_POWER_GOOD ( 0 ) 

/*! when 0 the message type is printed in hex, when 1 it is not */
#define PRINT_MESSAGE_OPTIONS ( 1 )
   
/*! use mutex to attempt to make string printing look prettier */
#define PRETTY_PRINT ( 1 )

/*! use debug signals to time the software FLL circuit */
#define DEBUG_SOFTWARE_FLL ( 1 )

#endif /* PRE_INCLUDE_H */
