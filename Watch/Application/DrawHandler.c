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
#include "Messages.h"
#include "Adc.h"
#include "hal_Battery.h"
#include "hal_lpm.h"
#include "hal_miscellaneous.h"
#include "hal_rtc.h"
#include "Wrapper.h"
#include "DebugUart.h"
#include "DrawHandler.h"
#include "ClockWidget.h"
#include "Icons.h"
#include "Fonts.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "Property.h"
#include "LcdDriver.h"
#include "LcdBuffer.h"
#include "SerialRam.h"
#include "Widget.h"

#define DRAW_PAGE     0x06

extern unsigned char const niLang;
unsigned char const FILL_BLACK = 0xFF;
unsigned char const FILL_WHITE = 0x00;

static unsigned char GetHour(char *Hour);
static unsigned char GetTime(char *pText);
static unsigned char GetDate(char *pText);
static unsigned char GetSecond(char *pText);
static unsigned char GetDayofWeek(char *pText);
static unsigned char GetMin(char *pText);
static unsigned char GetAmPm(char *pText);

static unsigned char const *GetIconInfo(Draw_t *Info);
static unsigned char const *GetTemplateData(Draw_t *Info);
static unsigned char const *GetRectData(Draw_t *Info);
static unsigned char const *GetHanziData(Draw_t *Info);
static unsigned char const *GetBluetoothState(Draw_t *Info);
static unsigned char const *GetBatteryStatus(Draw_t *Info);

static void DrawText(Draw_t *Info, char const *pText);
static unsigned char Overlapping(unsigned char Option);

static unsigned char (* const GetText[])(char *) =
{
  GetTime, GetDate, GetSecond, GetDayofWeek, GetHour, GetMin, GetAmPm
};

static unsigned char const * (* const GetDrawData[])(Draw_t *) =
{
  GetIconInfo, GetTemplateData, GetRectData, GetHanziData, GetBluetoothState, GetBatteryStatus
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
    pData = pMsg->pBuffer + DRAW_INFO_SIZE;

    if (!(pMsg->Options & DRAW_MSG_END))
    {
      pInfo = (Draw_t *)pvPortMalloc(DRAW_INFO_SIZE);
      PrintF("%cA:%04X %u", pInfo ? PLUS : NOK, pInfo, DRAW_INFO_SIZE);
      if (pInfo == NULL) return;

      memcpy(pInfo, pMsg->pBuffer, DRAW_INFO_SIZE);
      PrintF("Id:%02X X:%u Y:%u Opt:%02X", pInfo->Id, pInfo->X, pInfo->Y, pInfo->Opt);
      PrintF("W:%u H:%u WgtId:%02X", pInfo->Width, pInfo->Height, pInfo->WidgetId);
      PrintF("TxtLen:%u Align:%u", pInfo->TextLen, pInfo->Align);

      if (pInfo->Id & DRAW_ID_TYPE_BMP)
        Size = pInfo->Opt & DRAW_OPT_FILL ? 1 : WIDTH_IN_BYTES(pInfo->Width) * pInfo->Height;
      else Size = pInfo->TextLen;

      pData = (unsigned char *)pvPortMalloc(Size);
      PrintF("%cA:%04X %u", pData ? PLUS : NOK, pData, Size);
      if (pData == NULL)
      {
        vPortFree(pInfo);
        return;
      }

      Size = pMsg->Length - DRAW_INFO_SIZE;
      memcpy(pData, pMsg->pBuffer + DRAW_INFO_SIZE, Size);
    }
  }
  else // pure data in payload
  {
    if (pData == NULL)
    {
      PrintS("#DrwMsg:empty payload");
      return;
    }
    memcpy(pData + Size, pMsg->pBuffer, pMsg->Length);
    Size += pMsg->Length;
  }

  if (pMsg->Options & DRAW_MSG_END)
  {
    unsigned char Mode = (pMsg->Options & DRAW_MSG_MODE) >> 6;
    if (Mode == IDLE_MODE) CreateDrawBuffer(pInfo->WidgetId);

    Draw(pInfo, pData, Mode);

    if (Mode == IDLE_MODE && (pMsg->Options & DRAW_WIDGET_END))
      DrawWidgetToSram(pInfo->WidgetId);

    if (Size)
    {
      PrintF("-F:%04X %04X", pInfo, pData);
      vPortFree(pData);
      vPortFree(pInfo);
      Size = 0;
    }
  }
}

void Draw(Draw_t *Info, unsigned char const *pData, unsigned char ModePage)
{
  unsigned char DrawType = (Info->Id & DRAW_ID_TYPE) >> 7;
  unsigned char FuncId = (Info->Id & DRAW_ID_SUB_TYPE) >> 4;

  if (DrawType == DRAW_ID_TYPE_TEXT)
  {
    // pass 2-bit-MSB mode to DrawText
    Info->Id = Info->Id & DRAW_ID_SUB_ID | (ModePage << 4);

    if (FuncId)
    {
      char Text[7]; // max len is 7 for GetTime() hh:mmAm

      if (!Overlapping(Info->Opt & DRAW_OPT_OVERLAP_MASK))
        Info->TextLen = GetText[FuncId - 1](Text);

      if (Info->TextLen) DrawText(Info, Text);
    }
    else DrawText(Info, (char const *)pData);
  }
  else
  {
    if (FuncId > 0 && FuncId <= FUNC_DRAW_DATA_NUM) pData = GetDrawData[FuncId - 1](Info);

    if ((ModePage & DRAW_MODE) == IDLE_MODE)
    {
      switch (FuncId << 4)
      {
      case FUNC_DRAW_TEMPLATE: DrawTemplateToIdle(Info); break;
      case FUNC_DRAW_HANZI: DrawHanziClock(Info); break;

      case FUNC_DRAW_RECT:

        pData = (Info->Opt & DRAW_OPT_MASK) == DRAW_OPT_SET ? &FILL_BLACK : &FILL_WHITE;
        Info->Opt &= 0xF0;
        Info->Opt |= DRAW_OPT_FILL;
        PrintS("DRW_RECT->"); PrintQ((unsigned char *)Info, 9); PrintF("%02X", *pData);
      default:
        if (pData)
        { // to idle page other than ConnectedPage
          if (ModePage & DRAW_PAGE) DrawBitmapToLcd(Info, WIDTH_IN_BYTES(Info->Width), pData);
          else DrawBitmapToIdle(Info, WIDTH_IN_BYTES(Info->Width), pData);
        }
        break;
      }
    }
    else
    {
      if ((FuncId << 4) == FUNC_DRAW_TEMPLATE)
        DrawTemplateToSram(Info, ModePage & DRAW_MODE);
      else if (pData)
        DrawBitmapToSram(Info, WIDTH_IN_BYTES(Info->Width), pData, ModePage & DRAW_MODE);
    }
  }
}

// Id(page|mode|font)
static void DrawText(Draw_t *Info, char const *pText)
{
  etFontType Font = (etFontType)(Info->Id & DRAW_ID_SUB_ID);
  tFont const *pFont = GetFont(Font);
  Info->Height = pFont->Height;
  unsigned char ModePage = Info->Id >> 4;

  while (Info->TextLen --)
  {
    unsigned char const *pBitmap = GetFontBitmap(*pText, Font);
    Info->Width = GetCharWidth(*pText, Font);

    if (Info->X + Info->Width > LCD_COL_NUM) break;
    
//    PrintF("-DrwTxt: %c x:%d y:%d", *pText, Info->X, Info->Y);
    if ((ModePage & DRAW_MODE) == IDLE_MODE)
    {
      if (ModePage & DRAW_PAGE) DrawBitmapToLcd(Info, pFont->WidthInBytes, pBitmap);
      else DrawBitmapToIdle(Info, pFont->WidthInBytes, pBitmap);
    }
    else DrawBitmapToSram(Info, pFont->WidthInBytes, pBitmap, ModePage & DRAW_MODE);

    Info->X += (pFont->Type == FONT_TYPE_TIME ? pFont->MaxWidth : Info->Width);
    pText ++;
  }
}

static unsigned char GetHour(char *Hour)
{
  Hour[0] = RTCHOUR;
  
  if (!GetProperty(PROP_24H_TIME_FORMAT)) Hour[0] = To12H(Hour[0]);

  Hour[1] = BCD_L(Hour[0]) + ZERO;

  Hour[0] = BCD_H(Hour[0]);

  if(Hour[0] == 0)
  {
    Hour[0] = Hour[1];
    Hour[1] = COLON;
    return 2;
  }
  else
  {
    Hour[0] += ZERO;
    Hour[2] = COLON;
    return 3;
  }
}

static unsigned char GetMin(char *pText)
{
  pText[0] = BCD_H(RTCMIN) + ZERO;
  pText[1] = BCD_L(RTCMIN) + ZERO;
  return 2;
}

static unsigned char GetTime(char *pText)
{
  char TxtLen = GetHour(pText);

  pText[TxtLen] = BCD_H(RTCMIN) + ZERO;
  pText[TxtLen + 1] = BCD_L(RTCMIN) + ZERO;
  TxtLen += 2;
  if (GetAmPm(pText + TxtLen)) TxtLen += 2;
  return TxtLen;
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
  
  Info->Width = IconInfo[Id].Width << 3;
  Info->Height = IconInfo[Id].Height;
  return GetIcon(Id);
}

static unsigned char const *GetTemplateData(Draw_t *Info)
{
//  unsigned char Id = Info->Id & DRAW_ID_SUB_ID;
//  return Id < TEMPLATE_NUM ? (unsigned char const *)pTemplate[Id] : NULL;
  return NULL;
}

static unsigned char const *GetRectData(Draw_t *Info)
{
  return NULL;
}

static unsigned char const *GetHanziData(Draw_t *Info)
{
  return NULL;
}
