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
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Messages.h"
#include "MessageQueues.h"

#include "Adc.h"
#include "hal_Battery.h"
#include "hal_lpm.h"
#include "hal_miscellaneous.h"
#include "hal_rtc.h"
#include "Wrapper.h"
#include "DebugUart.h"
#include "SerialRam.h"
#include "Icons.h"
#include "Fonts.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "Property.h"
#include "ClockWidget.h"
#include "LcdBuffer.h"

#define MAX_DRAW_ITEM_NUM             (12)
#define TEMPLATE_ID_MASK              (0x7F)
#define FLASH_TEMPLATE_BIT            (BIT7)
#define TEMP_TYPE_4Q                  (0)
#define TEMP_TYPE_2Q                  (1)

#define DRAW_OPT_NONE                 (0)
#define DRAW_OPT_SEPARATOR            (':')
#define SEPARATOR_MASK                (0x7F)
#define DRAW_OPT_EQU_WIDTH            (0)
#define DRAW_OPT_PROP_WIDTH           (0x80)
#define DRAW_OPT_OVERLAP_NONE         (0)
#define DRAW_OPT_OVERLAP_BT           (1)
#define DRAW_OPT_OVERLAP_BATTERY      (2)
#define DRAW_OPT_OVERLAP_SEC          (4)

typedef struct
{
  unsigned char X; // in pixels
  unsigned char Y;
  unsigned char Id; //DrawData_t Data;
  unsigned char Opt; //Option, e.g. divider
  unsigned char Op; // bitwise operation
} DrawInfo_t;

typedef struct
{
  void (*Draw)(DrawInfo_t *Info);
  DrawInfo_t Info;
} Draw_t;

static void DrawHour(DrawInfo_t *Info);
static void DrawAmPm(DrawInfo_t *Info);
static void DrawMin(DrawInfo_t *Info);
static void DrawSec(DrawInfo_t *Info);
static void DrawDate(DrawInfo_t *Info);
static void DrawDayofWeek(DrawInfo_t *Info);
static void DrawBluetoothState(DrawInfo_t *Info);
static void DrawBatteryStatus(DrawInfo_t *Info);
//static void DrawIcon(DrawInfo_t *Info);
static void DrawTemplate(DrawInfo_t *Info);
static void DrawBlock(DrawInfo_t *Info);
static unsigned char Overlapping(unsigned char Option);
static void DrawHanziClock(DrawInfo_t *Info);

/* widget is a list of Draw_t, the order is the WatchFaceId (0 - 14) */
static const Draw_t DrawList[][MAX_DRAW_ITEM_NUM] =
{
  { //1Q
    {DrawHour, {1, 3, Time, DRAW_OPT_SEPARATOR, DRAW_OPT_BITWISE_OR}},
    {DrawMin, {1, 24, Time, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
    {DrawBluetoothState, {30, 28, ICON_SET_BLUETOOTH_SMALL, DRAW_OPT_BITWISE_OR}},
    {DrawBatteryStatus, {35, 3, ICON_SET_BATTERY_V, DRAW_OPT_BITWISE_OR}},
    {DrawDate, {25, 26, MetaWatch7, DRAW_OPT_OVERLAP_BT, DRAW_OPT_BITWISE_OR}},
    {DrawSec, {29, 32, MetaWatch16, DRAW_OPT_OVERLAP_BT, DRAW_OPT_BITWISE_OR}},
    {DrawDayofWeek, {25, 36, MetaWatch7, DRAW_OPT_OVERLAP_BT | DRAW_OPT_OVERLAP_SEC, DRAW_OPT_BITWISE_OR}}
  },
  { //2Q-TimeG
    {DrawHour, {7, 2, TimeG, DRAW_OPT_SEPARATOR, DRAW_OPT_BITWISE_OR}},
    {DrawMin, {53, 2, TimeG, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
    {DrawBluetoothState, {76, 30, ICON_SET_BLUETOOTH_SMALL, DRAW_OPT_BITWISE_OR}},
    {DrawBatteryStatus, {38, 35, ICON_SET_BATTERY_H, DRAW_OPT_BITWISE_OR}},
    {DrawDate, {7, 35, MetaWatch7, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
    {DrawSec, {38, 29, MetaWatch16, DRAW_OPT_OVERLAP_BATTERY, DRAW_OPT_BITWISE_OR}},
    {DrawDayofWeek, {72, 35, MetaWatch7, DRAW_OPT_OVERLAP_BT, DRAW_OPT_BITWISE_OR}}
  },
//  { // 4Q Logo TimeBlock
//    {DrawTemplate, {0, 0, TMPL_WGT_LOGO, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
//    {DrawHour, {1, 28, TimeBlock, DRAW_OPT_SEPARATOR, DRAW_OPT_BITWISE_OR}},
//    {DrawMin, {51, 28, TimeBlock, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
//    {DrawAmPm, {80, 50, MetaWatch5, DRAW_OPT_NONE, DRAW_OPT_BITWISE_SET}},
//    {DrawBluetoothState, {80, 79, ICON_SET_BLUETOOTH_SMALL, DRAW_OPT_NONE, DRAW_OPT_BITWISE_SET}},
//    {DrawBatteryStatus, {41, 15, ICON_SET_BATTERY_H, DRAW_OPT_BITWISE_OR}},
//    {DrawDate, {2, 12, MetaWatch16, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
//    {DrawSec, {75, 12, MetaWatch16, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
//    {DrawDayofWeek, {68, 12, MetaWatch16, DRAW_OPT_OVERLAP_SEC, DRAW_OPT_BITWISE_OR}}
//  },
  { //4Q-Hanzi
    {DrawTemplate, {0, 0, TMPL_WGT_HANZI, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
    {DrawHanziClock, {0, 0, Time, DRAW_OPT_NONE, DRAW_OPT_BITWISE_DST_NOT}}
  },
  { //4Q Big TimeK
    {DrawBlock, {0, 0, 12, 17}}, // x, w in bytes
    {DrawBlock, {0, 79, 12, 17}},
    {DrawHour, {0, 20, TimeK, DRAW_OPT_SEPARATOR, DRAW_OPT_BITWISE_OR}},
    {DrawMin, {51, 20, TimeK, DRAW_OPT_NONE, DRAW_OPT_BITWISE_OR}},
    {DrawAmPm, {33, 80, MetaWatch16, DRAW_OPT_NONE, DRAW_OPT_BITWISE_NOT}},
    {DrawBluetoothState, {76, 1, ICON_SET_BLUETOOTH_SMALL, DRAW_OPT_NONE, DRAW_OPT_BITWISE_NOT}},
    {DrawBatteryStatus, {3, 4, ICON_SET_BATTERY_H, DRAW_OPT_NONE, DRAW_OPT_BITWISE_NOT}},
    {DrawDate, {61, 80, MetaWatch16, DRAW_OPT_NONE, DRAW_OPT_BITWISE_NOT}},
    {DrawSec, {39, 1, MetaWatch16, DRAW_OPT_NONE, DRAW_OPT_BITWISE_NOT}},
    {DrawDayofWeek, {3, 80, MetaWatch16, DRAW_OPT_OVERLAP_NONE, DRAW_OPT_BITWISE_NOT}}
  },
  { //4Q-fish
    {DrawTemplate, {0, 0, TMPL_WGT_FISH, 0}},
    {DrawHour, {29, 33, Time, DRAW_OPT_SEPARATOR}},
    {DrawMin, {58, 33, Time, 0}},
    {DrawAmPm, {83, 33, MetaWatch5, 0}},
    {DrawBluetoothState, {82, 2, ICON_SET_BLUETOOTH_SMALL, 0}},
    {DrawBatteryStatus, {50, 4, ICON_SET_BATTERY_H, 0}},
    {DrawDate, {55, 22, MetaWatch7, 0}},
    {DrawSec, {58, 51, MetaWatch16, 0}},
    {DrawDayofWeek, {58, 55, MetaWatch7, DRAW_OPT_OVERLAP_SEC}}
  },
};

//#define DRAW_LIST_ITEM_NUM(_x)    (sizeof(*DrawList[_x]) / sizeof(Draw_t))

typedef struct
{
  unsigned char LayoutType;
  unsigned char ItemNum;
  const Draw_t *pDrawList;
} Widget_t;

//
static const Widget_t ClockWidget[] =
{
  {LAYOUT_QUAD_SCREEN, 7, DrawList[0]},
  {LAYOUT_HORI_SCREEN, 7, DrawList[1]},
//  {LAYOUT_FULL_SCREEN, 9, DrawList[2]}, // logo
  {LAYOUT_FULL_SCREEN, 2, DrawList[2]}, // hanzi
  {LAYOUT_FULL_SCREEN, 10, DrawList[3]}, // big
  {LAYOUT_FULL_SCREEN, 9, DrawList[4]}, // fish
};

#define HOME_WIDGET_NUM (sizeof(ClockWidget) / sizeof(Widget_t))

static unsigned char *pDrawBuffer;

static void DrawText(const char *pText, unsigned char Len, unsigned char X, unsigned char Y, unsigned char Font, unsigned char EqualWidth, unsigned char Op);
static void DrawBitmap(const unsigned char *pBitmap, unsigned char X, unsigned char Y, unsigned char W, unsigned char H, unsigned char BmpWidthInBytes, unsigned char Op);

#define CN_CLK_DIAN     12
#define CN_CLK_ZHENG    13
#define CN_CLK_FEN      29

#define CN_CLK_HOURH    0
#define CN_CLK_HOUR_SHI 1
#define CN_CLK_HOURL    2

#define CN_CLK_MINH     12
#define CN_CLK_MIN_SHI  18
#define CN_CLK_MINL     19

#define CN_CLK_ZI_WIDTH     15
#define CN_CLK_ZI_WIDTH_IN_BYTES  ((CN_CLK_ZI_WIDTH >> 3) + 1)
#define CN_CLK_ZI_HEIGHT    18
#define CN_CLK_ZI_PER_LINE  6

static void InvertHanzi(unsigned char Index);

/******************************************************************************/

void DrawClockWidget(unsigned char Id)
{
  pDrawBuffer = (unsigned char *)GetDrawBuffer();
  *pDrawBuffer = Id; // save the ClockId for later usage in WrtClkWgt()

  Id = FACE_ID(Id);
  unsigned int BufSize = Layout[ClockWidget[Id].LayoutType].QuadNum * BYTES_PER_QUAD + SRAM_HEADER_LEN;
  
  unsigned int i;
  for (i = 1; i < BufSize; ++i) pDrawBuffer[i] = 0; // Clear the buffer starting from [1]

  for (i = 0; i < ClockWidget[Id].ItemNum; ++i)
  {
    ClockWidget[Id].pDrawList[i].Draw((DrawInfo_t *)&ClockWidget[Id].pDrawList[i].Info);
  }
  
  tMessage Msg;
  SetupMessage(&Msg, WriteBufferMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_HOME_WGT);
  Msg.pBuffer = pDrawBuffer;
  RouteMsg(&Msg);
}

static void DrawBitmap(const unsigned char *pBitmap, unsigned char X, unsigned char Y,
                       unsigned char W, unsigned char H, unsigned char BmpWidthInBytes,
                       unsigned char Op)
{
// W is bitmap width in pixel
  unsigned char *pByte = Y / HALF_SCREEN_ROWS * BYTES_PER_QUAD * 2 + Y % HALF_SCREEN_ROWS * BYTES_PER_QUAD_LINE +
                         X / HALF_SCREEN_COLS * BYTES_PER_QUAD + (X % HALF_SCREEN_COLS >> 3) +
                         pDrawBuffer + SRAM_HEADER_LEN;

  if (!BmpWidthInBytes) BmpWidthInBytes = W % 8 ? (W >> 3) + 1: W >> 3;

  unsigned char ColBit = 1 << X % 8; // dst
  unsigned char MaskBit = BIT0; // src
  unsigned int Delta;
  unsigned char Set; // src bit is set or clear
  unsigned char x, y;

  for (x = 0; x < W; ++x)
  {
    for(y = 0; y < H; ++y)
    {
      Set = *(pBitmap + y * BmpWidthInBytes) & MaskBit;
      Delta = (ClockWidget[FACE_ID(*pDrawBuffer)].LayoutType == LAYOUT_FULL_SCREEN) &&
              (Y < HALF_SCREEN_ROWS && (Y + y) >= HALF_SCREEN_ROWS) ?
              BYTES_PER_QUAD : 0;
      
      BitOp(pByte + y * BYTES_PER_QUAD_LINE + Delta, ColBit, Set, Op);
    }

    MaskBit <<= 1;
    if (MaskBit == 0)
    {
      MaskBit = BIT0;
      pBitmap ++;
    }
    
    ColBit <<= 1;
    if (ColBit == 0)
    {
      ColBit = BIT0;
      pByte ++;
      // check next pixel x
      if ((X + x + 1) == HALF_SCREEN_COLS) pByte += BYTES_PER_QUAD - BYTES_PER_QUAD_LINE;
    }
  }
}

static void DrawText(const char *pText, unsigned char Len, unsigned char X, unsigned char Y,
                     unsigned char Font, unsigned char EqualWidth, unsigned char Op)
{
  SetFont((etFontType)Font);
  const tFont *pFont = GetCurrentFont();
  unsigned char i;
  
  for (i = 0; i < Len && pText[i] != NUL; ++i)
  {
//    if (pFont->Type == FONT_TYPE_TIME) pText[i] -= ZERO;

    unsigned char *pBitmap = GetCharacterBitmapPointer(pText[i]);
    unsigned char CharWidth = GetCharacterWidth(pText[i]);
    
    DrawBitmap(pBitmap, X, Y, CharWidth, pFont->Height, pFont->WidthInBytes, Op);
    X += EqualWidth ? CharWidth + 1 : pFont->MaxWidth + 1; // add 1 pixel space
  }
}

static void DrawHour(DrawInfo_t *Info)
{
  char Hour[3];
  HourToString(Hour);
  Hour[2] = Info->Opt & SEPARATOR_MASK; // separator
  
  DrawText(Hour, Info->Opt ? 3 : 2, Info->X, Info->Y, Info->Id, Info->Opt & DRAW_OPT_PROP_WIDTH, Info->Op);
}

static void DrawHanziClock(DrawInfo_t *Info)
{
  unsigned char Time = RTCHOUR;
  if (!GetProperty(PROP_24H_TIME_FORMAT)) Time = To12H(Time);

  if (Time >= 0x20) InvertHanzi(CN_CLK_HOURH);
  if (Time >= 0x10) InvertHanzi(CN_CLK_HOUR_SHI);

  if (Time != 0x20 && Time != 0x10) InvertHanzi(CN_CLK_HOURL + BCD_L(Time));

  Time = RTCMIN;
  if (Time)
  {
    if (Time >= 0x20) InvertHanzi(CN_CLK_MINH + BCD_H(Time));
    if (Time >= 0x10) InvertHanzi(CN_CLK_MIN_SHI);
    else InvertHanzi(CN_CLK_MINL); // 0
    
    if (BCD_L(Time)) InvertHanzi(CN_CLK_MINL + BCD_L(Time));
    InvertHanzi(CN_CLK_FEN);
  }
  else  InvertHanzi(CN_CLK_ZHENG);
}

static void InvertHanzi(unsigned char Index)
{
  unsigned char X = Index % CN_CLK_ZI_PER_LINE * (CN_CLK_ZI_WIDTH + 1);
  unsigned char Y = Index / CN_CLK_ZI_PER_LINE * (CN_CLK_ZI_HEIGHT + 1) + 1;

  DrawBitmap(pDrawBuffer, X, Y, CN_CLK_ZI_WIDTH, CN_CLK_ZI_HEIGHT,
    CN_CLK_ZI_WIDTH_IN_BYTES, DRAW_OPT_BITWISE_DST_NOT);
}

static void DrawAmPm(DrawInfo_t *Info)
{
  if (GetProperty(PROP_24H_TIME_FORMAT)) return;
  DrawText(RTCHOUR > 11 ? "pm" : "am", 2, Info->X, Info->Y, Info->Id, DRAW_OPT_PROP_WIDTH, Info->Op);
}

static void DrawMin(DrawInfo_t *Info)
{
  char Min[2];
  Min[0] = BCD_H(RTCMIN) + ZERO;
  Min[1] = BCD_L(RTCMIN) + ZERO;
  DrawText(Min, 2, Info->X, Info->Y, Info->Id, Info->Opt & DRAW_OPT_PROP_WIDTH, Info->Op);
}

static void DrawSec(DrawInfo_t *Info)
{
  if (!GetProperty(PROP_TIME_SECOND) || Overlapping(Info->Opt)) return;

  char Sec[3];
  Sec[0] = DRAW_OPT_SEPARATOR;
  Sec[1] = BCD_H(RTCSEC) + ZERO;
  Sec[2] = BCD_L(RTCSEC) + ZERO;
  DrawText(Sec, 3, Info->X, Info->Y, Info->Id, DRAW_OPT_PROP_WIDTH, Info->Op);
}

static void DrawDate(DrawInfo_t *Info)
{
  if (Overlapping(Info->Opt)) return;
  
  char Date[5];
  char *pDate = Date;
  unsigned char DayFirst = GetProperty(PROP_DDMM_DATE_FORMAT);

  memset(pDate, 0, 5); // clear Date[]

  char Rtc[2];
  Rtc[DayFirst ? 0 : 1] = RTCDAY;
  Rtc[DayFirst ? 1 : 0] = RTCMON;

  *pDate = BCD_H(Rtc[0]);
  if (*pDate) *pDate++ += ZERO;
  *pDate++ = BCD_L(Rtc[0]) + ZERO;
  *pDate++ = '/';

  *pDate = BCD_H(Rtc[1]);
  if (*pDate) *pDate++ += ZERO;
  *pDate = BCD_L(Rtc[1]) + ZERO;

  DrawText(Date, 5, Info->X, Info->Y, Info->Id, DRAW_OPT_PROP_WIDTH, Info->Op);
}

static void DrawDayofWeek(DrawInfo_t *Info)
{
  if (Overlapping(Info->Opt)) return;
  
  const char *pDow = DaysOfTheWeek[CURRENT_LANG][RTCDOW];
  DrawText(pDow, strlen(pDow), Info->X, Info->Y, Info->Id, DRAW_OPT_PROP_WIDTH, Info->Op);
}

static unsigned char Overlapping(unsigned char Option)
{
  unsigned char BT = BluetoothState();
  
  return ((Option & DRAW_OPT_OVERLAP_BATTERY) &&
          (Charging() || Read(BATTERY) <= BatteryCriticalLevel(CRITICAL_WARNING)) ||
          (Option & DRAW_OPT_OVERLAP_BT) && BT != Connect ||
          (Option & DRAW_OPT_OVERLAP_SEC) && GetProperty(PROP_TIME_SECOND));
}

static void DrawBluetoothState(DrawInfo_t *Info)
{
  unsigned char Index = 0;
  
  if (!RadioOn()) Index = ICON_BLUETOOTH_OFF;
  else if (Connected(CONN_TYPE_MAIN)) Index = ICON_BLUETOOTH_CONN;
  else if (OnceConnected()) Index = ICON_BLUETOOTH_DISC;
  else Index = ICON_BLUETOOTH_ON;
  
  if (Index != ICON_BLUETOOTH_OFF && Index != ICON_BLUETOOTH_DISC) return;
  
  DrawBitmap(IconInfo[Info->Id].pIconSet + Index * IconInfo[Info->Id].Width * IconInfo[Info->Id].Height,
             Info->X, Info->Y, IconInfo[Info->Id].Width * 8, IconInfo[Info->Id].Height,
             IconInfo[Info->Id].Width, Info->Op);
  
//  Index ++; if (Index == 6) Index = 0;
}

static void DrawBatteryStatus(DrawInfo_t *Info)
{
  if (!Charging() && Read(BATTERY) > BatteryCriticalLevel(CRITICAL_WARNING)) return;

  DrawBitmap(GetBatteryIcon(Info->Id), Info->X, Info->Y,
    IconInfo[Info->Id].Width * 8, IconInfo[Info->Id].Height, IconInfo[Info->Id].Width, Info->Op);
}

static void DrawTemplate(DrawInfo_t *Info)
{
// draw template to 4 quads
  unsigned char *pByte = pDrawBuffer + SRAM_HEADER_LEN;
  unsigned char TempId = Info->Id & TEMPLATE_ID_MASK;
  unsigned char i;
  
  if (Info->Id & FLASH_TEMPLATE_BIT)
  {
//    const unsigned char __data20 *pTemp20 = pWatchFace[TempId];
    
    for (i = 0; i < LCD_ROW_NUM; ++i)
    {
      if (i == HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

//      memcpy(pByte, pTemp20, BYTES_PER_QUAD_LINE);
//      memcpy(pByte + BYTES_PER_QUAD, pTemp20 + BYTES_PER_QUAD_LINE, BYTES_PER_QUAD_LINE);
//      pByte += BYTES_PER_QUAD_LINE;
//      pTemp20 += BYTES_PER_LINE;
    }
  }
  else
  {
#if __IAR_SYSTEMS_ICC__
    const unsigned char __data20 *pTemp;
#else
    const unsigned char *pTemp;
#endif

    pTemp =  (Info->Opt == TEMP_TYPE_4Q) ? pTemplate[TempId] : pTemplate2Q[TempId];
    unsigned char RowNum = (Info->Opt == TEMP_TYPE_4Q) ? LCD_ROW_NUM : HALF_SCREEN_ROWS;
    
    unsigned char k;

    for (i = 0; i < RowNum; ++i)
    {
      if (i == HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

//      memcpy(pByte, pTemp, BYTES_PER_QUAD_LINE);
//      memcpy(pByte + BYTES_PER_QUAD, pTemp + BYTES_PER_QUAD_LINE, BYTES_PER_QUAD_LINE);
      for (k = 0; k < BYTES_PER_QUAD_LINE; ++k) pByte[k] = pTemp[k];
      for (k = 0; k < BYTES_PER_QUAD_LINE; ++k) pByte[k + BYTES_PER_QUAD] = pTemp[k + BYTES_PER_QUAD_LINE];

      pByte += BYTES_PER_QUAD_LINE;
      pTemp += BYTES_PER_LINE;
    }
  }
}

static void DrawBlock(DrawInfo_t *Info)
{
  unsigned char *pByte = pDrawBuffer + SRAM_HEADER_LEN + Info->X + Info->Y * BYTES_PER_QUAD_LINE;
  if (Info->Y > HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

  unsigned char i;

  for (i = 0; i < Info->Opt; ++i)
  {
    if (Info->Y + i == HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

    memset(pByte, 0xFF, BYTES_PER_QUAD_LINE);
    memset(pByte + BYTES_PER_QUAD, 0xFF, BYTES_PER_QUAD_LINE);
    pByte += BYTES_PER_QUAD_LINE;
  }
}
