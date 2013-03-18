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
/*! \file hal_board_type.h
 *
 * Chooses the hardware abstraction layer header file and sets device
 * name contants.
 *
 */
/******************************************************************************/

#ifndef HAL_BOARD_TYPE_H
#define HAL_BOARD_TYPE_H

#include "msp430.h"
#include "hal_io_macros.h"

#if defined(HW_DEVBOARD_V2)

  #include "hal_devboard_v2_defs.h"

  #ifdef ANALOG
    #define BR_DEVICE_NAME "MetaWatch DB111"
    
  #elif defined(DIGITAL)
    #define BR_DEVICE_NAME "MetaWatch DB12"
    
  #else
    #error "ANALOG or DIGITAL not defined"
  #endif

#elif defined(WATCH)

  #if defined(DIGITAL)

    #include "hal_digital_v2_defs.h"

    #define BR_DEVICE_NAME "MetaWatch SW12"

  #elif defined(ANALOG)

    #include "hal_analog_v2_defs.h"

    #define BR_DEVICE_NAME "MetaWatch WDS111"

  #else

    #error "ANALOG or DIGITAL not defined"

  #endif

#else

  #error "Hardware configuration not defined"

#endif

/******************************************************************************/

/* device type is read from the phone */

#define RESERVED_BOARD_TYPE       ( 0 )
#define ANALOG_WATCH_TYPE         ( 1 )
#define DIGITAL_WATCH_TYPE_G1     ( 2 )
#define DIGITAL_DEVBOARD_TYPE_G1  ( 3 )
#define ANALOG_DEV_BOARD_TYPE     ( 4 )
#define DIGITAL_WATCH_TYPE_G2     ( 5 )
#define DIGITAL_DEVBOARD_TYPE_G2  ( 6 )

/******************************************************************************/

#if defined(HW_DEVBOARD_V2)

  #ifdef ANALOG
    #define BOARD_TYPE ( ANALOG_DEV_BOARD_TYPE )
    
  #else
    #define BOARD_TYPE ( DIGITAL_DEVBOARD_TYPE_G2 )
    
  #endif

#elif defined(WATCH)

  #ifdef DIGITAL
    #define BOARD_TYPE ( DIGITAL_WATCH_TYPE_G2 )

  #else
    #define BOARD_TYPE ( ANALOG_WATCH_TYPE )

  #endif

#endif

/******************************************************************************/

#endif /* HAL_BOARD_TYPE_H */

