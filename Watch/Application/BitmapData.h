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
/*! \file BitmapData.h
 *
 */
/******************************************************************************/
#ifndef BITMAPDATA_H
#define BITMAPDATA_H

#define BYTES_PER_SCREEN          (1152)
#define BYTES_PER_HALF_SCREEN     (576)
#define SPLASH_ROWS               (32)
#define TEMPLATE_NUM_FLASH_PAGES  (2)
#define HAL_FLASH_PAGE_SIZE       (512)
#define TEMPLATE_FLASH_SIZE       (HAL_FLASH_PAGE_SIZE * TEMPLATE_NUM_FLASH_PAGES)

#define TMPL_NOTIF_MODE           (0)
#define TMPL_MUSIC_MODE           (1)
#define TMPL_WGT_LOGO             (2)
#define TMPL_WGT_FISH             (3)
#define TMPL_WGT_HANZI            (4)

#define TMPL_WGT_EMPTY            (0)
#define TMPL_WGT_LOADING          (1)

#define BOOTLOADER_COLS           (12)
#define BOOTLOADER_ROWS           (46)
#define BOOTLOADER_START_ROW      (7)
#define CALL_NOTIF_COLS           (12)
#define CALL_NOTIF_ROWS           (16)
#define CALL_NOTIF_START_ROW      (77)

extern const unsigned char pBootloader[BOOTLOADER_COLS * BOOTLOADER_ROWS];
extern const unsigned char pCallNotif[CALL_NOTIF_COLS * CALL_NOTIF_ROWS];

#if __IAR_SYSTEMS_ICC__
extern const unsigned char __data20 pTemplate[][BYTES_PER_SCREEN];
#else
extern const unsigned char pTemplate[][BYTES_PER_SCREEN];
#endif

#define TEMPLATE_NUM       (sizeof(pTemplate) / BYTES_PER_SCREEN)

extern const unsigned char pWidgetTemplate[][6 * 48];
#define WIDGET_TEMPLATE_NUM (sizeof(pWidgetTemplate) / (6 * 48))

#if __IAR_SYSTEMS_ICC__
extern const unsigned char __data20 pTemplate2Q[][BYTES_PER_HALF_SCREEN];
#else
extern const unsigned char pTemplate2Q[][BYTES_PER_HALF_SCREEN];
#endif

#define TEMPLATE_2Q_NUM    (sizeof(pTemplate2Q) / BYTES_PER_HALF_SCREEN)

#if __IAR_SYSTEMS_ICC__
extern __data20 const unsigned char pWatchFace[][1]; //TEMPLATE_FLASH_SIZE];
#define TEMPLATE20_NUM      (sizeof(pWatchFace) / TEMPLATE_FLASH_SIZE)
#endif

#endif /* BITMAPDATA_H */
