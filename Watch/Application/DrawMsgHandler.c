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
#include "DrawMsgHandler.h"
#include "ClockWidget.h"
#include "SerialRam.h"
#include "Icons.h"
#include "Fonts.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "Property.h"
#include "LcdBuffer.h"

extern unsigned char const niLang;

static unsigned char GetHour(char *Hour);
static unsigned char GetTime(char *pText);
static unsigned char GetDate(char *pText);
static unsigned char GetSecond(char *pText);
static unsigned char GetDayofWeek(char *pText);
static unsigned char GetMin(char *pText);
static unsigned char GetAmPm(char *pText);

static void DrawText(Draw_t *Info, char const *pText);
static unsigned char Overlapping(unsigned char Option);

static unsigned char const *GetBluetoothState(Draw_t *Info);
static unsigned char const *GetBatteryStatus(Draw_t *Info);
static unsigned char const *GetTemplateData(Draw_t *Info);
static unsigned char const *GetIconInfo(Draw_t *Info);

static unsigned char (* const GetText[])(char *) =
{
  GetTime, GetDate, GetSecond, GetDayofWeek, GetHour, GetMin, GetAmPm
};

static unsigned char const * (* const GetDrawData[])(Draw_t *) =
{
  GetBluetoothState, GetBatteryStatus, GetTemplateData, GetIconInfo
};

#define FUNC_DRAW_DATA_NUM    (sizeof(GetDrawData) / sizeof(*GetDrawData))

void DrawMsgHandler(tMessage *pMsg)
{
  static Draw_t *pInfo;
  static unsigned char *pData;
  static unsigned int Size = 0;

  if (pMsg->Options & DRAW_MSG_BEGIN)
  {
    pInfo = (Draw_t *)pMsg->pBuffer;
    pData = pMsg->pBuffer + sizeof(Draw_t);

    if (!(pMsg->Options & DRAW_MSG_END))
    {
      pInfo = (Draw_t *)pvPortMalloc(sizeof(Draw_t));
      PrintF("%cA:%04X %u", pInfo ? PLUS : NOK, pInfo, sizeof(Draw_t));
      if (pInfo == NULL) return;

      memcpy(pInfo, pMsg->pBuffer, sizeof(Draw_t));

      Size = (pInfo->Id & DRAW_ID_TYPE_BMP) ?
             (WIDTH_IN_BYTES(pInfo->Width) * pInfo->Height) : pInfo->Width;

      pData = (unsigned char *)pvPortMalloc(Size); 
      PrintF("%cA:%04X %u", pData ? PLUS : NOK, pData, Size);
      if (pData == NULL)
      {
        vPortFree(pInfo);
        return;
      }

      Size = pMsg->Length - sizeof(Draw_t);
      memcpy(pData, pMsg->pBuffer + sizeof(Draw_t), Size);
    }
  }
  else // pure data in payload
  {
    if (pData == NULL)
    {
      PrintS("#DrwMsg:nul");
      return;
    }
    memcpy(pData + Size, pMsg->pBuffer, pMsg->Length);
    Size += pMsg->Length;
  }

  if (pMsg->Options & DRAW_MSG_END)
  {
//    PrintQ(pData, Size);

    Draw(pInfo, pData, (pMsg->Options & DRAW_MODE) >> 6);
  
    if (Size)
    {
      PrintF("-F:%04X %04X", pInfo, pData);
      vPortFree(pData);
      vPortFree(pInfo);
      Size = 0;
    }
  }
}

void Draw(Draw_t *Info, unsigned char const *pData, unsigned char Mode)
{
  unsigned char DrawType = (Info->Id & DRAW_ID_TYPE) >> 7;
  unsigned char FuncId = (Info->Id & DRAW_ID_SUB_TYPE) >> 4;

//  PrintF("-Id:0x%02x F:0x%02x", Info->Id, FuncId);

  if (DrawType == DRAW_ID_TYPE_FONT)
  {
    // pass 2-bit-MSB mode to DrawText
    Info->Id = Info->Id & DRAW_ID_SUB_ID | (Mode << 6);

    if (FuncId)
    {
      char Text[7]; // max len is 7 for GetTime() hh:mmAm

      if (!Overlapping(Info->Opt & DRAW_OPT_OVERLAP_MASK))
        Info->Width = GetText[FuncId - 1](Text);
      if (Info->Width) DrawText(Info, Text);
    }
    else DrawText(Info, (char const *)pData);
  }
  else
  {
    if (FuncId > 0 && FuncId <= FUNC_DRAW_DATA_NUM) // must be draw to idle mode
    {
      pData = GetDrawData[FuncId - 1](Info);
      if (pData == NULL) return;
    }

    FuncId <<= 4;

    if (Mode == IDLE_MODE)
    {
      switch (FuncId)
      {
      case FUNC_DRAW_TEMPLATE: DrawTemplateToIdle(Info); break;
      case FUNC_DRAW_BLOCK: DrawBlockToIdle(Info); break;
      case FUNC_DRAW_HANZI: DrawHanziClock(Info); break;
      default: DrawBitmapToIdle(Info, WIDTH_IN_BYTES(Info->Width), pData); break;
      }
    }
    else
    {
      if (FuncId == FUNC_DRAW_TEMPLATE) DrawTemplateToSram(Info, pData, Mode);
      else DrawBitmapToSram(Info, WIDTH_IN_BYTES(Info->Width), pData, Mode);
    }
  }
}

// Id(font, mode), width
static void DrawText(Draw_t *Info, char const *pText)
{
  etFontType Font = (etFontType)(Info->Id & DRAW_ID_SUB_ID);
  tFont const *pFont = GetFont(Font);
  Info->Height = pFont->Height;
  unsigned char Len = Info->Width;

  while (Len --)
  {
    unsigned char const *pBitmap = GetFontBitmap(*pText, Font);
    Info->Width = GetCharWidth(*pText, Font);

    if (Info->X + Info->Width >= LCD_COL_NUM) break;
    
//    PrintF("-DrwTxt: %c x:%d y:%d", *pText, Info->X, Info->Y);

    if ((Info->Id & DRAW_MODE) == IDLE_MODE) DrawBitmapToIdle(Info, pFont->WidthInBytes, pBitmap);
    else DrawBitmapToSram(Info, pFont->WidthInBytes, pBitmap, (Info->Id & DRAW_MODE) >> 6);

    Info->X += (pFont->Type == FONT_TYPE_TIME ? pFont->MaxWidth : Info->Width);
    pText ++;
  }
}

static unsigned char GetHour(char *Hour)
{
  Hour[0] = RTCHOUR;
  
  if (!GetProperty(PROP_24H_TIME_FORMAT))
  {
    Hour[0] = To12H(Hour[0]);
    if (Hour[0] == 0) Hour[0] = 0x12; // 0am -> 12am
  }

  Hour[1] = BCD_L(Hour[0]) + ZERO;

  Hour[0] = BCD_H(Hour[0]);
  if(!Hour[0]) Hour[0] = SPACE;
  else Hour[0] += ZERO;
  Hour[2] = COLON;
  return 3;
}

static unsigned char GetMin(char *pText)
{
  pText[0] = BCD_H(RTCMIN) + ZERO;
  pText[1] = BCD_L(RTCMIN) + ZERO;
  return 2;
}

static unsigned char GetTime(char *pText)
{
  GetHour(pText);
  pText[3] = BCD_H(RTCMIN) + ZERO;
  pText[4] = BCD_L(RTCMIN) + ZERO;
  if (GetAmPm(pText + 5)) return 7;
  return 5;
}

static unsigned char GetAmPm(char *pText)
{
  if (GetProperty(PROP_24H_TIME_FORMAT)) return 0;
  pText[0] = RTCHOUR > 0x11 ? 'p' : 'a';
  pText[1] = 'm';
  return 2;
}

static unsigned char GetSecond(char *pText)
{
  if (!GetProperty(PROP_TIME_SECOND)) return 0;

  pText[0] = COLON;
  pText[1] = BCD_H(RTCSEC) + ZERO;
  pText[2] = BCD_L(RTCSEC) + ZERO;
  return 3;
}

static unsigned char GetDate(char *pText)
{  
  char *pDate = pText;
  char DayFirst = GetProperty(PROP_DDMM_DATE_FORMAT);

  char Rtc[2];
  Rtc[DayFirst ? 0 : 1] = RTCDAY;
  Rtc[DayFirst ? 1 : 0] = RTCMON;

  *pDate = BCD_H(Rtc[0]);
  if (*pDate) *pDate++ += ZERO;
  *pDate++ = BCD_L(Rtc[0]) + ZERO;
  *pDate++ = '/';

  *pDate = BCD_H(Rtc[1]);
  if (*pDate) *pDate++ += ZERO;
  *pDate++ = BCD_L(Rtc[1]) + ZERO;
//  PrintF("%s %u", pText, pDate - pText);
  return pDate - pText;
}

static unsigned char GetDayofWeek(char *pText)
{
  strcpy(pText, DaysOfTheWeek[niLang][RTCDOW]);
//  PrintF("DoW:%u", strlen(pText));
  return strlen(pText);
}

static unsigned char Overlapping(unsigned char Option)
{
  unsigned char BT = BluetoothState();
  
  return ((Option & DRAW_OPT_OVERLAP_BT) && BT != Connect ||
          (Option & DRAW_OPT_OVERLAP_SEC) && GetProperty(PROP_TIME_SECOND));
}

static unsigned char const *GetBluetoothState(Draw_t *Info)
{
  if (Connected(CONN_TYPE_MAIN)) return NULL;
  return GetIconInfo(Info);
}

static unsigned char const *GetBatteryStatus(Draw_t *Info)
{
  if (Charging() || BatteryPercentage() <= WARNING_LEVEL) return GetIconInfo(Info);
  return NULL;
}

static unsigned char const *GetIconInfo(Draw_t *Info)
{
  unsigned char Id = Info->Id & DRAW_ID_SUB_ID;
  
  Info->Width = IconInfo[Id].Width * 8;
  Info->Height = IconInfo[Id].Height;
  return GetIcon(Id);
}

static unsigned char const *GetTemplateData(Draw_t *Info)
{
  return TmplInfo[Info->Id & DRAW_ID_SUB_ID].pData;
}
