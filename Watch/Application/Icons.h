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

#ifndef ICONS_H
#define ICONS_H

#define LEFT_BUTTON_COLUMN  ( 0 )
#define RIGHT_BUTTON_COLUMN ( 6 )
#define BUTTON_ICON_SIZE_IN_COLUMNS ( 6 )
#define BUTTON_ICON_SIZE_IN_ROWS    ( 32 )

#define ICON_MUSIC_WIDTH    (2)
#define ICON_MUSIC_HEIGHT   (11)

#define BUTTON_ICON_A_F_ROW ( 0 )
#define BUTTON_ICON_B_E_ROW ( 32 )
#define BUTTON_ICON_C_D_ROW ( 64 )

extern const unsigned char pBluetoothOffIcon[];
extern const unsigned char pBluetoothInitIcon[];
extern const unsigned char pBluetoothOnIcon[];
extern const unsigned char pSecondsOffMenuIcon[];
extern const unsigned char pSecondsOnMenuIcon[];
extern const unsigned char pLinkAlarmOffIcon[];
extern const unsigned char pLinkAlarmOnIcon[];
extern const unsigned char pInvertDisplayIcon[];

extern const unsigned char pLedIcon[];
extern const unsigned char pExitIcon[];
extern const unsigned char pNextIcon[];

extern const unsigned char pRstPinIcon[];
extern const unsigned char pNmiPinIcon[]; 
extern const unsigned char pResetButtonIcon[];
extern const unsigned char pGroundIcon[];
extern const unsigned char pSbwIcon[];
extern const unsigned char pSerialIcon[];
extern const unsigned char pTestIcon[];

extern const unsigned char pIconChargingEnabled[];
extern const unsigned char pIconChargingDisabled[];
extern const unsigned char pBootloaderIcon[];

extern const unsigned char pIconMusicState[][ICON_MUSIC_WIDTH * ICON_MUSIC_HEIGHT];

extern const unsigned char pInitPageBluetoothOff[];
extern const unsigned char pInitPagePaired[];
extern const unsigned char pInitPageNoPair[];
extern unsigned char const pInitPageStartMwm[];

extern const unsigned char pIconSplashLogo[6 * 13];
extern const unsigned char pIconSplashMetaWatch[12 * 12];
extern const unsigned char pIconSplashHandsFree[8 * 5];

#define NUMBER_OF_ROWS_IN_WAVY_LINE ( 5 )
extern const unsigned char pWavyLine[5*12];

extern const unsigned char pIconWatch[2 * 21];

#define LEFT_STATUS_ICON_COLUMN   ( 0 )
#define CENTER_STATUS_ICON_COLUMN ( 4 ) 
#define RIGHT_STATUS_ICON_COLUMN  ( 8 )

#define STATUS_ICON_SIZE_IN_COLUMNS ( 4 )
#define STATUS_ICON_SIZE_IN_ROWS    ( 28 )

extern const unsigned char pRadioOnIcon[];
extern const unsigned char pIconRadioOff[];
extern const unsigned char pConnectedIcon[];
extern const unsigned char pDisconnectedIcon[];
extern const unsigned char pBatteryChargingStatusScreenIcon[];
extern const unsigned char pBatteryLowStatusScreenIcon[];
extern const unsigned char pBatteryFullStatusScreenIcon[];
extern const unsigned char pBatteryMediumStatusScreenIcon[];

// A set of icons with same size
#define ICON_SET_BLUETOOTH_SMALL          0
#define ICON_SET_BLUETOOTH_BIG            1
#define ICON_SET_BATTERY_H                2
#define ICON_SET_BATTERY_V                3
#define ICON_TIMESTAMP                    4
#define ICON_QUIT                         5

#define ICON_PHONE_SMALL                  6
#define ICON_SET_SPLASH_MW                7
#define ICON_SET_SPLASH_HANSFREE          8
#define ICON_SET_SPLASH_LOGO              9
#define ICON_SET_MENU                     10
#define ICON_START_MWM                    11
#define ICON_RADIO_OFF                    12
#define ICON_PHONE                        13
#define ICON_INIT_PAGE_PAIRED             14
#define ICON_BATTERY_UNKNOWN              15

#define BATTERY_ICON_NUM         (8)

#define ICON_BLUETOOTH_OFF       (0)
#define ICON_BLUETOOTH_ON        (1)
#define ICON_BLUETOOTH_CONN      (2)
#define ICON_BLUETOOTH_DISC      (3)

#define BLUETOOTH_ICON_NUM       (4)

#define MENU_ITEM_RADIO_STATUS   (0)
#define MENU_ITEM_SHOW_SECOND    (3)
#define MENU_ITEM_LINK_ALARM     (5)
#define MENU_ITEM_DISCOVERABLE   (7)
#define MENU_ITEM_BACKLIGHT      (8)
#define MENU_ITEM_EXIT           (9)

typedef struct
{
  unsigned char Width; // in bytes
  unsigned char Height;
  unsigned char const *Data;
} Icon_t;

extern const Icon_t IconInfo[];

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
