//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#include <string.h>
#include "FreeRTOS.h"
#include "Messages.h"
#include "DebugUart.h"
#include "DrawHandler.h"
#include "Icons.h"
#include "Fonts.h"
#include "BitmapData.h"
#include "LcdBuffer.h"
#include "ClockWidget.h"
#include "Widget.h"

static Draw_t const Clock1Q[] =
{ //1Q
  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | Time, 1, 3, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | Time, 1, 24, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 30, 28, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_V, 35, 3, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch7, 25, 26, DRAW_OPT_OVERLAP_BT | DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_SEC | MetaWatch16, 29, 32, DRAW_OPT_OVERLAP_BT | DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch7, 25, 36, DRAW_OPT_OVERLAP_BT | DRAW_OPT_OVERLAP_SEC | DRAW_OPT_OR}
};

static Draw_t const Clock2Q[] =
{ //2Q-TimeG
  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | TimeG, 7, 2, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | TimeG, 53, 2, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 1, 31, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch7, 18, 36, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_SEC | MetaWatch16, 43, 31, /*DRAW_OPT_OVERLAP_BATTERY | */DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch7, 71, 36, /*DRAW_OPT_OVERLAP_BT | DRAW_OPT_OVERLAP_SEC | */DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_H, 70, 35, DRAW_OPT_SET},//38
};

static Draw_t const Clock4QLogo[] =
{ // 4Q Logo TimeBlock
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_TEMPLATE | TMPL_WGT_LOGO, 0, 0, TMPL_TYPE_4Q, LCD_COL_NUM, LCD_ROW_NUM},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | TimeBlock, 1, 28, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | TimeBlock, 51, 28, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_AMPM | MetaWatch5, 80, 50, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 80, 79, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_H, 41, 15, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch16, 2, 12, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_SEC | MetaWatch16, 75, 12, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch16, 68, 12, DRAW_OPT_OVERLAP_SEC | DRAW_OPT_OR}
};

static Draw_t const Clock4QBigK[] =
{ //4Q Big TimeK
  {DRAW_ID_TYPE_BMP | FUNCT_DRAW_BITMAP, 0, 0, DRAW_OPT_FILL, 96, 17}, // x, w in bytes
  {DRAW_ID_TYPE_BMP | FUNCT_DRAW_BITMAP, 0, 79, DRAW_OPT_FILL, 96, 17},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | TimeK, 0, 20, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | TimeK, 51, 20, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_AMPM | MetaWatch16, 33, 80, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 76, 1, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_H, 3, 4, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch16, 61, 80, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_SEC | MetaWatch16, 39, 1, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch16, 3, 80, DRAW_OPT_NOT}
};

static Draw_t const Clock4QFish[] =
{ //4Q-fish
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_TEMPLATE | TMPL_WGT_FISH, 0, 0, TMPL_TYPE_4Q, LCD_COL_NUM, LCD_ROW_NUM},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | Time, 29, 33, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | Time, 58, 33, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_AMPM | MetaWatch5, 83, 33, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 82, 2, DRAW_OPT_OR},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_H, 50, 4, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch7, 55, 22, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_SEC | MetaWatch16, 58, 51, DRAW_OPT_OR},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch7, 58, 55, DRAW_OPT_OVERLAP_SEC}
};

static Draw_t const Clock4QHanzi[] =
{ //4Q-Hanzi
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_TEMPLATE | TMPL_WGT_HANZI, 0, 0, TMPL_TYPE_4Q, LCD_COL_NUM, LCD_ROW_NUM},
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_HANZI | Time, 0, 0, DRAW_OPT_DST_NOT}
};

static Draw_t const Clock4QCity[] =
{ //4Q Big TimeK
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_TEMPLATE | TMPL_WGT_CITY, 0, 0, TMPL_TYPE_4Q, LCD_COL_NUM, LCD_ROW_NUM}, // x, w in bytes

  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | Time, 9, 19, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | Time, 38, 19, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_SEC | MetaWatch16, 63, 25, DRAW_OPT_NOT},

  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch7, 9, 43, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch7, 34, 43, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_AMPM | MetaWatch7, 60, 43, DRAW_OPT_NOT},

  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 76, 1, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_H, 3, 4, DRAW_OPT_NOT}
};

#define NEW_HOUR_X          10
#define NEW_HOUR_Y          22
#define NEW_COLON_ERASE_X   (NEW_HOUR_X + 44)
#define NEW_COLON_ERASE_Y   NEW_HOUR_Y
#define NEW_MIN_X           (NEW_HOUR_X + 51)
#define NEW_MIN_Y           NEW_HOUR_Y
#define NEW_DAY_X           NEW_MIN_X
#define NEW_DAY_Y           (NEW_HOUR_Y + 25)
#define NEW_DATE_X          NEW_MIN_X
#define NEW_DATE_Y          (NEW_HOUR_Y + 37)
#define NEW_AMPM_X          NEW_MIN_X
#define NEW_AMPM_Y          (NEW_HOUR_Y + 49)

static Draw_t const Clock4QDigital[] =
{ //4Q Big TimeK
  {DRAW_ID_TYPE_BMP | FUNCT_DRAW_BITMAP, 0, 0, DRAW_OPT_FILL, 96, 96}, // x, w

  {DRAW_ID_TYPE_TEXT | FUNC_GET_HOUR | TimeK, NEW_HOUR_X, NEW_HOUR_Y, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_RECT, NEW_COLON_ERASE_X, NEW_COLON_ERASE_Y, DRAW_OPT_SET, 24, 56},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_MIN | Time, NEW_MIN_X, NEW_MIN_Y, DRAW_OPT_NOT},

  {DRAW_ID_TYPE_TEXT | FUNC_GET_DOW | MetaWatch7, NEW_DAY_X, NEW_DAY_Y, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_DATE | MetaWatch7, NEW_DATE_X, NEW_DATE_Y, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_TEXT | FUNC_GET_AMPM | MetaWatch7, NEW_AMPM_X, NEW_AMPM_Y, DRAW_OPT_NOT},

  {DRAW_ID_TYPE_BMP | FUNC_GET_BT_STATE | ICON_SET_BLUETOOTH_BIG, 76, 1, DRAW_OPT_NOT},
  {DRAW_ID_TYPE_BMP | FUNC_GET_BATT_ICON | ICON_SET_BATTERY_H, 3, 4, DRAW_OPT_NOT}
};

typedef struct
{
  Draw_t const * const DrawList;
  unsigned char ItemNum;
} CLockList_t;

static CLockList_t const ClockList[] =
{
  {Clock1Q, sizeof(Clock1Q) / DRAW_INFO_SIZE},
  {Clock2Q, sizeof(Clock2Q) / DRAW_INFO_SIZE},
  {Clock4QLogo, sizeof(Clock4QLogo) / DRAW_INFO_SIZE},
  {Clock4QBigK, sizeof(Clock4QBigK) / DRAW_INFO_SIZE},
  {Clock4QFish, sizeof(Clock4QFish) / DRAW_INFO_SIZE},
  {Clock4QHanzi, sizeof(Clock4QHanzi) / DRAW_INFO_SIZE},
  {Clock4QCity, sizeof(Clock4QCity) / DRAW_INFO_SIZE},
  {Clock4QDigital, sizeof(Clock4QDigital) / DRAW_INFO_SIZE}
};

void DrawClockWidget(unsigned char Id)
{
  if (!CreateDrawBuffer(CLOCK_ID(Id))) return;
  
  unsigned char FaceId = FACE_ID(Id);
  unsigned char FillByte = LCD_BLACK;
  Draw_t Info;

  memset((unsigned char *)&Info, 0, DRAW_INFO_SIZE);

  unsigned char i;
  for (i = 0; i < ClockList[FaceId].ItemNum; ++i)
  {
    memcpy(&Info, (unsigned char *)&(ClockList[FaceId].DrawList[i]), DRAW_INFO_SIZE);
    Info.Opt |= CLOCK_WIDGET_BIT;
    Info.WidgetId = CLOCK_ID(Id);

    Draw(&Info, &FillByte, IDLE_MODE);
  }

  DrawClockToSram(CLOCK_ID(Id));
}
