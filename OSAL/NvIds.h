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
/*! \file NvIds.h
 *
 * Identifiers for tokens stored in non-volatile memory
 */
/******************************************************************************/

#ifndef NVIDS_H
#define NVIDS_H

/* the operation to store/initialize the link key required 16 ms */

#define NVID_RESERVED ( 0x0000 )

/* 
 * These are the IDs for each item that is stored in flash using
 * OSAL_Nv functions (IDs are unsigned ints - 16 bits) 
 */

/*! link key structure (4 link keys) */
#define NVID_LINK_KEY ( 0x0001 )

/* structures */
#define NVID_IDLE_BUFFER_CONFIGURATION    ( 0x0002 )
#define NVID_IDLE_BUFFER_INVERT           ( 0x0003 )
#define NVID_IDLE_MODE_TIMEOUT            ( 0x0004 )
#define NVID_APP_MODE_TIMEOUT             ( 0x0005 )
#define NVID_NOTIF_MODE_TIMEOUT           ( 0x0006 )
#define NVID_MUSIC_MODE_TIMEOUT           ( 0x0007 )
#define NVID_IDLE_DISPLAY_TIMEOUT         ( 0x0008 )
#define NVID_APPLICATION_DISPLAY_TIMEOUT  ( 0x0009 )
#define NVID_NOTIFICATION_DISPLAY_TIMEOUT ( 0x000a )
#define NVID_RESERVED_DISPLAY_TIMEOUT     ( 0x000b )

/* single byte debug control */
#define NVID_SNIFF_DEBUG           ( 0x1001 )
#define NVID_BATTERY_DEBUG         ( 0x1002 )
#define NVID_CONNECTION_DEBUG      ( 0x1003 )
#define NVID_RSTNMI_CONFIGURATION  ( 0x1004 )
#define NVID_MASTER_RESET          ( 0x1005 )
#define NVID_SAVE_PAIRING_INFO     ( 0x1006 )
#define NVID_ENABLE_SNIFF_ENTRY    ( 0x1007 )
#define NVID_EXIT_SNIFF_ON_RECEIVE ( 0x1008 )
#define NVID_MUX_CONTROL_5V        ( 0x1009 )
#define NVID_MUX_CONTROL_NORMAL    ( 0x100A )
#define NVID_UART_DEBUG_ZONES      ( 0x100B )

/* */
#define NVID_LOW_BATTERY_WARNING_LEVEL    ( 0x2001 )
#define NVID_LOW_BATTERY_BTOFF_LEVEL      ( 0x2002 )
#define NVID_BATTERY_SENSE_INTERVAL       ( 0x2003 )
#define NVID_LIGHT_SENSE_INTERVAL         ( 0x2004 )
#define NVID_SECURE_SIMPLE_PAIRING_ENABLE ( 0x2005 )
#define NVID_LINK_ALARM_ENABLE            ( 0x2006 )
#define NVID_LINK_ALARM_DURATION          ( 0x2007 )
#define NVID_PAIRING_MODE_DURATION        ( 0x2008 )
#define NVID_TIME_FORMAT                  ( 0x2009 )
#define NVID_DATE_FORMAT                  ( 0x200a )
#define NVID_DISPLAY_SECONDS              ( 0x200b )
#define NVID_LANGUAGE                     ( 0x200c )
#define NVID_ENABLE_SHOW_GRID             ( 0x200d )

#define NVID_TOP_OLED_CONTRAST_DAY        ( 0x3000 )
#define NVID_BOTTOM_OLED_CONTRAST_DAY     ( 0x3001 )
#define NVID_TOP_OLED_CONTRAST_NIGHT      ( 0x3002 )
#define NVID_BOTTOM_OLED_CONTRAST_NIGHT   ( 0x3003 )

#define NVID_WATCHDOG_RESET_COUNT ( 0x4000 )
#define NVID_WATCHDOG_INFORMATION ( 0x4001 )

#endif /* NVIDS_H */
