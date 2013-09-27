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

#include "DebugUart.h"
#include "DrawMsgHandler.h"
#include "Fonts.h"
#include "BitmapData.h"
#include "LcdDisplay.h"
#include "LcdBuffer.h"
#include "SerialRam.h"
#include "Icons.h"
#include "OneSecondTimers.h"
#include "Vibration.h"
#include "Property.h"
#include "Wrapper.h"

#define CALLER_NAME_HEIGHT    16
#define CALLER_NUMBER_ITEM    6

static Draw_t const CallItem[] =
{
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_ICON | ICON_QUIT, 79, 7, DRAW_OPT_BITWISE_OR},
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_ICON | ICON_PHONE_SMALL, 4, 77, DRAW_OPT_BITWISE_OR},
  {DRAW_ID_TYPE_BMP | FUNC_DRAW_ICON | ICON_TIMESTAMP, 55, 85, DRAW_OPT_BITWISE_OR},//58
  {DRAW_ID_TYPE_FONT | FUNC_GET_TIME | MetaWatch5, 62, 86, DRAW_OPT_BITWISE_NOT},
  {DRAW_ID_TYPE_FONT | MetaWatch16, 6, 5, DRAW_OPT_BITWISE_OR},
  {DRAW_ID_TYPE_FONT | MetaWatch16, 6, 24, DRAW_OPT_BITWISE_OR},
  {DRAW_ID_TYPE_FONT | MetaWatch7, 6, 61, DRAW_OPT_BITWISE_OR},
};

static unsigned char const Text[] = "call from:";
static unsigned char const NoName[] = "Unknown";

static unsigned char *pCallerNumber = NULL;
static unsigned char NumberLen = 0;
static unsigned char Ring = FALSE;

static void DrawCallNotification(unsigned char const *pVal, int Len);
static void SetCallerNumber(unsigned char const *pNumber, unsigned char Len);

void HandleCallNotification(unsigned char Opt, unsigned char *pVal, unsigned char Len)
{
  tMessage Msg;

  switch (Opt)
  {
  case CALLER_NAME:
    if (Ring == FALSE)
    {
      Ring = TRUE;
      SendMessage(&Msg, VibrateMsg, VIBRA_PATTERN_RING);
      StartTimer(VibraTimer);

      DrawCallNotification(pVal, Len);

      if (GetProperty(PROP_AUTO_BACKLIGHT)) SendMessage(&Msg, AutoBacklightMsg, MSG_OPT_NONE);
    }
    break;

  case CALL_END:
    StopTimer(VibraTimer);
    Ring = FALSE;
    SendMessage(&Msg, ChangeModeMsg, IDLE_MODE | MSG_OPT_UPD_INTERNAL);
    if (Connected(CONN_TYPE_HFP)) SendMessage(&Msg, HfpMsg, MSG_OPT_HFP_RING_STOP);
    break;

  case CALL_REJECTED:
    StopTimer(VibraTimer);
    Ring = FALSE;
    if (Connected(CONN_TYPE_HFP)) SendMessage(&Msg, HfpMsg, MSG_OPT_HFP_HANGUP);
    break;

  case CALLER_NUMBER:
    SetCallerNumber(pVal, Len);
    break;
  }
}

static void DrawCallNotification(unsigned char const *pVal, int Len)
{
  PrintW("-DrwCall:"); PrintQ(pVal, Len);

  Draw_t const *pItem = CallItem;
  Draw_t Info;
  unsigned char i = 0;

  ClearSram(NOTIF_MODE);
  // quit icon
  memcpy(&Info, pItem++, sizeof(Draw_t));
  Draw(&Info, NULL, NOTIF_MODE);

  // small phone icon
  memcpy(&Info, pItem++, sizeof(Draw_t));
  Draw(&Info, NULL, NOTIF_MODE);

  // timestamp icon
  memcpy(&Info, pItem++, sizeof(Draw_t));
  Draw(&Info, NULL, NOTIF_MODE);

  // timestamp
  memcpy(&Info, pItem++, sizeof(Draw_t));
  Draw(&Info, NULL, NOTIF_MODE);

  // call from
  memcpy(&Info, pItem++, sizeof(Draw_t));
  Info.Width = sizeof(Text) - 1;
  Draw(&Info, Text, NOTIF_MODE);

  // caller name
  memcpy(&Info, pItem, sizeof(Draw_t));

  if (*pVal >= '0' && *pVal <= '9' || *pVal == '+')
  {
    SetCallerNumber(pVal, Len);
    pVal = NoName;
    Len = sizeof(NoName) - 1;
  }

  // split and draw first, last names and call number if available
//  while (Len >= 0)
//  {
//    i = 0; while (pVal[i] != SPACE && i < Len) i++;
//
//    PrintF(" W:%u", i);
//    Info.Width = i;
//    Draw(&Info, pVal, NOTIF_MODE);
//
//    memcpy(&Info, pItem, sizeof(Draw_t));
//    Info.Y += CALLER_NAME_HEIGHT;
//    pVal += i + 1;
//    Len -= i + 1;
//  }

  i = 0; while (pVal[i] != SPACE && i < Len) i++;

  PrintF(" W:%u", i);
  Info.Width = i;
  Draw(&Info, pVal, NOTIF_MODE);

  if (i < Len - 1)
  {
    memcpy(&Info, pItem, sizeof(Draw_t));
    Info.Y += CALLER_NAME_HEIGHT;
    pVal += i + 1;
    Len -= i + 1;
    Info.Width = Len;
    Draw(&Info, pVal, NOTIF_MODE);
  }

  if (pCallerNumber)
  {
    memcpy(&Info, &CallItem[CALLER_NUMBER_ITEM], sizeof(Draw_t));
    Info.Width = NumberLen;
    Draw(&Info, pCallerNumber, NOTIF_MODE);

    vPortFree(pCallerNumber);
    pCallerNumber = NULL;
  }

  CreateAndSendMessage(ChangeModeMsg, NOTIF_MODE | MSG_OPT_UPD_INTERNAL);
}

static void SetCallerNumber(unsigned char const *pNumber, unsigned char Len)
{
  pCallerNumber = (unsigned char *)pvPortMalloc(Len);
  if (pCallerNumber) memcpy(pCallerNumber, pNumber, Len);
  NumberLen = Len;
}

unsigned char Ringing(void)
{
  return Ring;
}
