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

#include "FreeRTOS.h"
#include "Messages.h"
#include "DrawHandler.h"
#include "LcdBuffer.h"
#include "LcdDisplay.h"
#include "OneSecondTimers.h"
#include "Fonts.h"
#include "Icons.h"
#include "DebugUart.h"
#include "Vibration.h"
#include "Countdown.h"

#define CDT_SETTING               0
#define CDT_COUNTING              1
#define CDT_TIMES_UP              2

#define CDT_TIMER_LENGTH          5
#define CDT_SET_HOUR              0
#define CDT_SET_MIN               1
#define CDT_SET_SEC               2

#define CDT_EXIT_ROW              14
#define CDT_STATUS_ROW_NUM        26
#define CDT_ICON_START_ROW        22
#define CDT_ICON_ROW_NUM          18

#define CDT_SETTING_START_ROW     46
#define CDT_SETTING_ROW_NUM       45
#define CDT_COUNTER_START_ROW     53
#define CDT_COUNTER_ROW_NUM       29

#define CDT_START_STOP_ROW        82
#define CDT_START_STOP_ROW_NUM    9

static char const BUTTON_INCREASE_TEXT[] = {'+'};
static char const BUTTON_DECREASE_TEXT[] = {'-'};
static char const BUTTON_EXIT_TEXT[] = {'E', 'x', 'i', 't', CHAR_SQUARE};
static char const BUTTON_START_TEXT[] = {'S', 't', 'a', 'r', 't', CHAR_SQUARE};
static char const BUTTON_STOP_TEXT[] = {'S', 't', 'o', 'p', CHAR_SQUARE};
static char const BUTTON_HHMM_TEXT[] = {'H', 'H', ':', 'M', 'M', CHAR_SQUARE};

static unsigned char State = CDT_SETTING;
static signed char Hour = 0;
static signed char Minute = 0;
static signed char Second = 0;
static signed char InitHour = 0;
static signed char InitMinute = 0;
static signed char InitSecond = 0;
static unsigned char SetTime = CDT_SET_MIN;
static Draw_t Info;

static void DrawSettingView(void);
static void DrawTimerStatus(unsigned char Status);
static void DrawCountdownTimer(void);
static void DrawTimerIcon(void);
static void ClearSettingView(void);
static void DrawBlackBar(unsigned char Row, unsigned char Height, unsigned char Fill);

void CountdownHandler(unsigned char Option)
{
  if (Option == CDT_ENTER)
  {
    FillLcdBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);
    WriteBufferToLcd(STARTING_ROW, LCD_ROW_NUM);

    DrawStatusBarToLcd();
    DrawTimerStatus(State);

    if (State == CDT_SETTING || State == CDT_TIMES_UP)
    {
      Hour = InitHour;
      Minute = InitMinute;
      Second = InitSecond;
      State = CDT_SETTING;
      DrawSettingView();
    }
    else ClearSettingView();

    DrawCountdownTimer();
  }
  else if (Option == CDT_START)
  {
    if (State == CDT_SETTING)
    {
      if (!Hour && !Minute && !Second) return;

      InitHour = Hour;
      InitMinute = Minute;
      InitSecond = Second;
      StartTimer(CountdownTimer);
      ClearSettingView();
      State = CDT_COUNTING;
    }
    else //CDT_COUNTING
    {
      StopTimer(CountdownTimer);
      State = CDT_SETTING;
      DrawSettingView();

      Hour = InitHour;
      Minute = InitMinute;
      Second = InitSecond;
    }

    DrawCountdownTimer();
    DrawTimerStatus(State);
  }
  else if (Option == CDT_COUNT) CountingDown();
  else if (State == CDT_SETTING) SetCountdownTimer(Option);
}

static void DrawSettingView(void)
{
  FillLcdBuffer(CDT_SETTING_START_ROW, CDT_SETTING_ROW_NUM, LCD_WHITE);

  DrawBlackBar(CDT_SETTING_START_ROW, CDT_SETTING_ROW_NUM, LCD_BLACK);

  Info.Id = MetaWatch5;
  Info.TextLen = 1;
  Info.X = 2;
  Info.Opt = DRAW_OPT_NOT;

  char const *pText = BUTTON_INCREASE_TEXT;
  Info.Y = CDT_SETTING_START_ROW + 2;
  DrawTextToLcd(&Info, pText);

  pText = BUTTON_DECREASE_TEXT;
  Info.Y = CDT_START_STOP_ROW;
  Info.X = 3;
  DrawTextToLcd(&Info, pText);

  pText = BUTTON_HHMM_TEXT;
  Info.TextLen = 6;
  Info.Y = CDT_SETTING_START_ROW;
  Info.X = 67;
  Info.Opt = DRAW_OPT_OR;
  DrawTextToLcd(&Info, pText);

  pText = BUTTON_START_TEXT;
  Info.TextLen = 6;
  Info.Y = CDT_START_STOP_ROW + 2;
  Info.X = 67;
  Info.Opt = DRAW_OPT_OR;
  DrawTextToLcd(&Info, pText);
  WriteBufferToLcd(CDT_SETTING_START_ROW, CDT_SETTING_ROW_NUM);
}

static void DrawTimerStatus(unsigned char Status)
{
  FillLcdBuffer(CDT_EXIT_ROW, CDT_STATUS_ROW_NUM, LCD_WHITE);

  Info.Id = MetaWatch5;
  char const *pText = BUTTON_EXIT_TEXT;
  Info.TextLen = 5;
  Info.Y = CDT_EXIT_ROW;

  Info.X = 73;
  Info.Opt = DRAW_OPT_OR;
  DrawTextToLcd(&Info, pText);

  DrawTimerIcon();

  Info.Id = MetaWatch7;
  Info.X = 26;
  Info.Opt = DRAW_OPT_OR;

  switch (Status)
  {
  case CDT_SETTING:
    pText = "Set";
    Info.TextLen = 3;
    Info.Y = CDT_ICON_START_ROW + 1;
    DrawTextToLcd(&Info, pText);

    pText = "Your Timer";
    Info.TextLen = 10;
    Info.Y = CDT_ICON_START_ROW + 11;
    DrawTextToLcd(&Info, pText);
    break;

  case CDT_COUNTING:
    pText = "Timer";
    Info.TextLen = 5;
    Info.Y = CDT_ICON_START_ROW + 1;
    DrawTextToLcd(&Info, pText);

    pText = "Activated";
    Info.TextLen = 9;
    Info.Y = CDT_ICON_START_ROW + 11;
    DrawTextToLcd(&Info, pText);
    break;

  case CDT_TIMES_UP:
    pText = "Times Up!";
    Info.TextLen = 10;
    Info.Y = CDT_ICON_START_ROW + 1 + 5;
    DrawTextToLcd(&Info, pText);
    break;

  default: break;
  }

  WriteBufferToLcd(CDT_EXIT_ROW, CDT_STATUS_ROW_NUM);
}

static void DrawTimerIcon(void)
{
  Draw_t Info;
  Info.X = 8;
  Info.Y = CDT_ICON_START_ROW;
  Info.Width = IconInfo[ICON_TIMER].Width << 3;
  Info.Height = IconInfo[ICON_TIMER].Height;
  Info.Opt = DRAW_OPT_OR;
  DrawBitmapToLcd(&Info, IconInfo[ICON_TIMER].Width, IconInfo[ICON_TIMER].Data);
}

static void DrawBlackBar(unsigned char Row, unsigned char Height, unsigned char Fill)
{
  Info.X = 0;
  Info.Y = Row;
  Info.Width = 8;
  Info.Height = Height;
  Info.Opt = DRAW_OPT_FILL;

  DrawBitmapToLcd(&Info, 1, &Fill);
}

static void ClearSettingView(void)
{
  if (State == CDT_SETTING)
  {
    // clear HH:MM
    FillLcdBuffer(CDT_SETTING_START_ROW, 8, LCD_WHITE);
    WriteBufferToLcd(CDT_SETTING_START_ROW, 8);
  }

  FillLcdBuffer(CDT_START_STOP_ROW, CDT_START_STOP_ROW_NUM, LCD_WHITE);
  Info.Id = MetaWatch5;
  char const *pText = BUTTON_STOP_TEXT;
  Info.TextLen = 5;
  Info.Y = CDT_START_STOP_ROW + 2;
  Info.X = 72;
  Info.Opt = DRAW_OPT_OR;
  DrawTextToLcd(&Info, pText);
  WriteBufferToLcd(CDT_START_STOP_ROW, CDT_START_STOP_ROW_NUM);
}

static void DrawCountdownTimer(void)
{
  char *pBuffer = (char *)pvPortMalloc(CDT_TIMER_LENGTH);

  pBuffer[0] = Hour / 10 + ZERO;
  pBuffer[1] = Hour % 10 + ZERO;
  pBuffer[2] = COLON;
  pBuffer[3] = Minute / 10 + ZERO;
  pBuffer[4] = Minute % 10 + ZERO;

  FillLcdBuffer(CDT_COUNTER_START_ROW, CDT_COUNTER_ROW_NUM, LCD_WHITE);

  char const *pText = pBuffer;
  Info.Id = Time;
  Info.TextLen = CDT_TIMER_LENGTH;
  Info.Y = CDT_COUNTER_START_ROW + 5;
  Info.X = 14;
  Info.Opt = DRAW_OPT_OR;
  DrawTextToLcd(&Info, pText);

  pBuffer[0] = Second / 10 + ZERO;
  pBuffer[1] = Second % 10 + ZERO;
  Info.Id = MetaWatch16;
  Info.TextLen = 2;
  Info.Y = CDT_COUNTER_START_ROW + 6 + 5;
  Info.X = 70;
  DrawTextToLcd(&Info, pText);

  if (State == CDT_SETTING)
  {
    DrawBlackBar(CDT_COUNTER_START_ROW, CDT_COUNTER_ROW_NUM, LCD_BLACK);

    Info.Id = MetaWatch5;
    Info.TextLen = 1;
    Info.X = SetTime == CDT_SET_HOUR ? 23 : (SetTime == CDT_SET_MIN ? 52 : 69);
//    Info.XMask = SetTime == CDT_SET_HOUR ? BIT7 :
//      (SetTime == CDT_SET_MIN ? BIT4 : BIT5);

    pBuffer[0] = CHAR_UP;
    Info.Y = CDT_COUNTER_START_ROW;
    if (SetTime == CDT_SET_SEC) Info.Y += 8;
    DrawTextToLcd(&Info, pText);

    pBuffer[0] = CHAR_DOWN;
    Info.Y = CDT_COUNTER_START_ROW + 19 + 5;
    DrawTextToLcd(&Info, pText);
  }

  vPortFree(pBuffer);
  WriteBufferToLcd(CDT_COUNTER_START_ROW, CDT_COUNTER_ROW_NUM);
}

void SetCountdownTimer(unsigned char Set)
{
  if (Set == CDT_HHMM)
  {
    SetTime ++;
    if (SetTime > CDT_SET_SEC) SetTime = CDT_SET_HOUR;
  }
  else
  {
    switch (SetTime)
    {
    case CDT_SET_HOUR:
      Hour = Set == CDT_INC ? Hour + 1 : Hour - 1;
      if (Hour == 24) Hour = 0;
      else if (Hour < 0) Hour = 23;
      break;

    case CDT_SET_MIN:
      Minute = Set == CDT_INC ? Minute + 1 : Minute - 1;
      if (Minute == 60) Minute = 0;
      else if (Minute < 0) Minute = 59;
      break;

    case CDT_SET_SEC:
      Second = Set == CDT_INC ? Second + 10 : Second - 10;
      if (Second == 60) Second = 0;
      else if (Second < 0) Second = 50;
      break;

    default: break;
    }
  }

  DrawCountdownTimer();
}

void CountingDown(void)
{
  Second --;
  if (Second < 0) {Second = 59; Minute --;}
  if (Minute < 0) {Minute = 59; Hour --;}
  if (Hour < 0) Hour = 23;

  if (!Second && !Minute && !Hour)
  {
    StopTimer(CountdownTimer);

    SendMessage(VibrateMsg, VIBRA_PATTERN_ALARM);
    SendMessage(SetBacklightMsg, LED_ON);

    State = CDT_TIMES_UP;
    if (CurrentIdlePage() == CountdownPage)
    {
      DrawTimerStatus(State);

      State = CDT_SETTING;
      DrawSettingView();
    }
    else SendMessage(CountdownMsg, CDT_ENTER);
  }

  if (CurrentMode == IDLE_MODE && CurrentIdlePage() == CountdownPage) DrawCountdownTimer();
}
