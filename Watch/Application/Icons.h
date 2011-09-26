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
/*! \file Icons.h
 *
 * Icons are small bitmaps that are stored in flash.  This is for the LCD only.
 */
/******************************************************************************/

#ifndef ICONS_H
#define ICONS_H

#define LEFT_BUTTON_COLUMN  ( 0 )
#define RIGHT_BUTTON_COLUMN ( 6 )
#define BUTTON_ICON_SIZE_IN_COLUMNS ( 6 )
#define BUTTON_ICON_SIZE_IN_ROWS    ( 32 )

#define BUTTON_ICON_A_F_ROW ( 0 )
#define BUTTON_ICON_B_E_ROW ( 32 )
#define BUTTON_ICON_C_D_ROW ( 64 )

extern const unsigned char pPairableInitIcon[32*6];
extern const unsigned char pPairableIcon[32*6];
extern const unsigned char pUnpairableIcon[32*6];
extern const unsigned char pBluetoothInitIcon[32*6];
extern const unsigned char pBluetoothOnIcon[32*6];
extern const unsigned char pBluetoothOffIcon[32*6];
extern const unsigned char pLinkAlarmOnIcon[32*6]; 
extern const unsigned char pLinkAlarmOffIcon[32*6];
extern const unsigned char pLedIcon[32*6];
extern const unsigned char pNextIcon[32*6];
extern const unsigned char pExitIcon[32*6];
extern const unsigned char pSspInitIcon[32*6];
extern const unsigned char pSspEnabledIcon[32*6]; 
extern const unsigned char pSspDisabledIcon[32*6];
extern const unsigned char pRstPinIcon[32*6];
extern const unsigned char pNmiPinIcon[32*6]; 
extern const unsigned char pResetButtonIcon[32*6];
extern const unsigned char pNormalDisplayMenuIcon[32*6];
extern const unsigned char pSecondsOnMenuIcon[32*6];
extern const unsigned char pSecondsOffMenuIcon[32*6];
extern const unsigned char pShippingModeIcon[32*6];

extern const unsigned char pBootPageBluetoothOffSwash[32*12];
extern const unsigned char pBootPageConnectionSwash[32*12];
extern const unsigned char pBootPagePairingSwash[32*12];
extern const unsigned char pBootPageUnknownSwash[32*12];

#define NUMBER_OF_ROWS_IN_WAVY_LINE ( 5 )
extern const unsigned char pWavyLine[5*12];

#define LEFT_STATUS_ICON_COLUMN   ( 0 )
#define CENTER_STATUS_ICON_COLUMN ( 4 ) 
#define RIGHT_STATUS_ICON_COLUMN  ( 8 )

#define STATUS_ICON_SIZE_IN_COLUMNS ( 4 )
#define STATUS_ICON_SIZE_IN_ROWS    ( 36 )

extern const unsigned char pBluetoothOnStatusScreenIcon[36*4];
extern const unsigned char pBluetoothOffStatusScreenIcon[36*4];
extern const unsigned char pPhoneConnectedStatusScreenIcon[36*4];
extern const unsigned char pPhoneDisconnectedStatusScreenIcon[36*4];
extern const unsigned char pBatteryChargingStatusScreenIcon[36*4];
extern const unsigned char pBatteryLowStatusScreenIcon[36*4];
extern const unsigned char pBatteryFullStatusScreenIcon[36*4];
extern const unsigned char pBatteryMediumStatusScreenIcon[36*4];


#define IDLE_PAGE_ICON_STARTING_ROW ( 10 )
#define IDLE_PAGE_ICON_STARTING_COL ( 8 )
#define IDLE_PAGE_ICON_SIZE_IN_ROWS ( 20 )
#define IDLE_PAGE_ICON_SIZE_IN_COLS ( 2 )

extern const unsigned char pBluetoothOffIdlePageIcon[20*2];
extern const unsigned char pPhoneDisconnectedIdlePageIcon[20*2];

#define IDLE_PAGE_ICON2_STARTING_ROW ( 0 )
#define IDLE_PAGE_ICON2_STARTING_COL ( 10 )
#define IDLE_PAGE_ICON2_SIZE_IN_ROWS ( 30 )
#define IDLE_PAGE_ICON2_SIZE_IN_COLS ( 2 )

extern const unsigned char pBatteryChargingIdlePageIconType2[30*2];
extern const unsigned char pLowBatteryIdlePageIconType2[30*2];

#endif /*ICONS_H*/