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

/******************************************************************************/
#define VERSION_STRING "3.1.0"

/*! number of buffers in the message buffer pool */
#define NUM_MSG_BUFFERS 20

/******************************************************************************/

#if defined(HW_DEVBOARD_V2)

  #include "hal_devboard_v2_defs.h"

  #ifdef ANALOG
    #define SPP_DEVICE_NAME "MetaWatch Analog Development Board"
  #elif defined(DIGITAL)
    #define SPP_DEVICE_NAME "MetaWatch Digital Development Board"
  #else
    #error "ANALOG or DIGITAL not defined"
  #endif

#elif defined(WATCH)

  #if defined(DIGITAL)
  
    #include "hal_digital_v2_defs.h"
  
    #define SPP_DEVICE_NAME "MetaWatch Digital WDS112"
  
  #elif defined(ANALOG)
  
    #include "hal_analog_v2_defs.h"
  
    #define SPP_DEVICE_NAME "MetaWatch Analog WDS111"

  #else
  
    #error "ANALOG or DIGITAL not defined"
  
  #endif

#else

  #error "Hardware configuration not defined"

#endif

/******************************************************************************/

/* device type is read from the phone */

#define RESERVED_BOARD_TYPE      ( 0 )
#define ANALOG_BOARD_TYPE        ( 1 )
#define DIGITAL_BOARD_TYPE       ( 2 )
#define DIGITAL_DEV_BOARD_TYPE   ( 3 )
#define ANALOG_DEV_BOARD_TYPE    ( 4 )

/******************************************************************************/

#if defined(HW_DEVBOARD_V2)

  #ifdef ANALOG
    #define BOARD_TYPE ( ANALOG_DEV_BOARD_TYPE ) 
  #else
    #define BOARD_TYPE ( DIGITAL_DEV_BOARD_TYPE ) 
  #endif

#elif defined(WATCH)

  #if defined(DIGITAL)
  
    #define BOARD_TYPE ( DIGITAL_BOARD_TYPE ) 
  
  #elif defined(ANALOG)
  
    #define BOARD_TYPE ( ANALOG_BOARD_TYPE )
  
  #endif

#endif

/******************************************************************************/

#endif /* HAL_BOARD_TYPE_H */

