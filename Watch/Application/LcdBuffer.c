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
#include "semphr.h"
#include "hal_board_type.h"
#include "hal_battery.h"
#include "hal_miscellaneous.h"
#include "hal_boot.h"
#include "hal_lpm.h"
#include "hal_rtc.h"
#include "DebugUart.h"
#include "LcdDriver.h"
#include "Wrapper.h"
#include "Adc.h"
#include "Icons.h"
#include "Fonts.h"
#include "LcdDisplay.h"
#include "MuxMode.h"
#include "BitmapData.h"
#include "Property.h"
#include "hal_vibe.h"
#include "Messages.h"
#include "OneSecondTimers.h"
#include "MessageInfo.h"
#include "Wrapper.h"
#include "DrawHandler.h"
#include "LcdBuffer.h"

#if WWZ
#include "Wwz.h"
#endif

#define SPLASH_START_ROW              41

/* the internal buffer */
#define PHONE_DRAW_SCREEN_ROW_NUM     66

#define FTM_START_ROW                 54
#define FTM_ROW_NUM                   24

/*
 * days of week are 0-6 and months are 1-12
 */
const char DaysOfTheWeek[][7][4] =
{
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  {"su", "ma", "ti", "ke", "to", "pe", "la"},
  {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"}
};

const char MonthsOfYear[][13][7] =
{
  {"???","Jan","Feb","Mar","Apr","May","June",
  "July","Aug","Sep","Oct","Nov","Dec"},
  {"???","tami", "helmi", "maalis", "huhti", "touko", "kesä",
   "heinä", "elo", "syys", "loka", "marras", "joulu"},
  {"???","Jan","Feb","Mar","Apr","Mai","Jun",
   "Jul","Aug","Sep","Okt","Nov","Dez"}
};

unsigned char const niLang = CURRENT_LANG;

extern const char BUILD[];
extern const char VERSION[];

#if BOOTLOADER
#include "bl_boot.h"
//#pragma location="BOOT_VERSION"
//__no_init __root const char BL_VERSION[VERSION_LEN];
extern const char BootVersion[VERSION_LEN];
#endif

#define COPY      0
#define CLEAR     1
#define SET       2

//static tLcdLine LcdBuf[LCD_ROW_NUM];
static tLcdLine *LcdBuf; // = LcdBuf;

#define LCD_BUFFER_SIZE (sizeof(tLcdLine) * LCD_ROW_NUM)

static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;

//static void FillLcdBuffer(unsigned char StartRow, unsigned char RowNum, unsigned char Value);
static void CopyColumnsIntoMyBuffer(unsigned char const *pImage, unsigned char StartRow,
                                    unsigned char RowNum, unsigned char StartCol, unsigned char ColNum);

static void DrawMenu1(void);
static void DrawMenu2(void);
static void DrawMenu3(void);
static void DrawCommonMenuIcons(void);

static void GetHour(char *Hour);
static void GetMinute(char *Min);
static void GetSecond(char *Sec);
static void DrawChar(char const Char, etFontType Font, unsigned char Op);
static void DrawString(char const *pString, etFontType Font, unsigned char Op);
static void DrawLocalAddress(void);
static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font);
//static void WriteToBuffer(unsigned char const *pData, unsigned char StartRow, unsigned char RowNum,
//                          unsigned char StartCol, unsigned char ColNum, unsigned char Op);

/******************************************************************************/

void DrawMenu(unsigned char Page)
{
  /* clear entire region */
  FillLcdBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);

  switch (Page)
  {
  case Menu1Page: DrawMenu1(); break;
  case Menu2Page: DrawMenu2(); break;
  case Menu3Page: DrawMenu3(); break;
  default: PrintF("# No Such Menu Page:%d", Page); break;
  }

  /* these icons are common to all menus */
  DrawCommonMenuIcons();
  WriteBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

static void DrawMenu1(void)
{
  unsigned char i = 0;

  if (BluetoothState() == Initial) i = MENU_ITEM_RADIO_INIT;
  else i = RadioOn() ? MENU_ITEM_RADIO_ON : MENU_ITEM_RADIO_OFF;

  CopyColumnsIntoMyBuffer(pIconMenuItem[i],
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_INVERT],
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[GetProperty(PROP_TIME_SECOND) ? MENU_ITEM_SECOND_ON : MENU_ITEM_SECOND_OFF],
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_COUNTDOWN],
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);
}

static void DrawMenu2(void)
{
  CopyColumnsIntoMyBuffer(pIconMenuItem[RESET_PIN ? MENU_ITEM_NMI : MENU_ITEM_RST],
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  unsigned char MuxMode = GetMuxMode();
  unsigned char i = 0;

  if (MuxMode == MUX_MODE_SERIAL) i = MENU_ITEM_SERIAL;
  else if (MuxMode == MUX_MODE_GND) i = MENU_ITEM_GROUND;
  else i = MENU_ITEM_SBW;
  
  CopyColumnsIntoMyBuffer(pIconMenuItem[i],
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_NEXT],
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_RESET],
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);
}

static void DrawMenu3(void)
{
  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_BOOTLOADER],
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[ChargeEnabled() ? MENU_ITEM_CHARGE_EN : MENU_ITEM_CHARGE_DISABLE],
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_NEXT],
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_TEST],
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);
}

static void DrawCommonMenuIcons(void)
{
  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_BACKLIGHT],
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);

  CopyColumnsIntoMyBuffer(pIconMenuItem[MENU_ITEM_EXIT],
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_COLS);
}

void DrawConnectionScreen(void)
{
  FillLcdBuffer(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM, LCD_WHITE);

  if (!RadioOn())
  {
    gRow = WATCH_DRAW_SCREEN_ROW_NUM + 9;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Bluetooth", MetaWatch16, DRAW_OPT_OR);

    gRow += 16;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("OFF", MetaWatch16, DRAW_OPT_OR);

    CopyColumnsIntoMyBuffer(IconInfo[ICON_RADIO_OFF].Data,
      WATCH_DRAW_SCREEN_ROW_NUM + 9, IconInfo[ICON_RADIO_OFF].Height,
      0, IconInfo[ICON_RADIO_OFF].Width);
  }
  else if (BlePaired() || BtPaired() || Connected(CONN_TYPE_ANCS))
  {
    gRow = WATCH_DRAW_SCREEN_ROW_NUM + 5;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("Start", MetaWatch7, DRAW_OPT_OR);

    gRow += 8;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("MWM", MetaWatch7, DRAW_OPT_OR);

    gRow += 8;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("Phone", MetaWatch7, DRAW_OPT_OR);

    gRow += 8;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("App", MetaWatch7, DRAW_OPT_OR);

    CopyColumnsIntoMyBuffer(IconInfo[ICON_START_MWM].Data,
      WATCH_DRAW_SCREEN_ROW_NUM + 9, IconInfo[ICON_START_MWM].Height,
      6, IconInfo[ICON_START_MWM].Width);
  }
  else
  {
    FillLcdBuffer(WATCH_DRAW_SCREEN_ROW_NUM, 11, LCD_BLACK);
    gRow = WATCH_DRAW_SCREEN_ROW_NUM + 2;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString(GetLocalName(), MetaWatch7, DRAW_OPT_NOT);

    gRow += 12;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Ready To", MetaWatch7, DRAW_OPT_OR);

    gRow += 8;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Pair With", MetaWatch7, DRAW_OPT_OR);

    gRow += 8;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Phone", MetaWatch7, DRAW_OPT_OR);

    CopyColumnsIntoMyBuffer(IconInfo[ICON_PHONE].Data,
      WATCH_DRAW_SCREEN_ROW_NUM + 14, IconInfo[ICON_PHONE].Height,
      1, IconInfo[ICON_PHONE].Width);
  }

  /* add the firmware version */
  gRow = 73;
  gColumn = 4;
  gBitColumnMask = BIT0;
  DrawString("V ", MetaWatch5, DRAW_OPT_OR);
  DrawString((char *)VERSION, MetaWatch5, DRAW_OPT_OR);

  DrawLocalAddress();
  
  WriteBufferToLcd(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM);
}

void DrawWatchStatusScreen(unsigned char Full)
{
  FillLcdBuffer(STARTING_ROW, FTM_START_ROW - NUMBER_OF_ROWS_IN_WAVY_LINE, LCD_WHITE);

  CopyColumnsIntoMyBuffer(RadioOn() ? pRadioOnIcon : pIconRadioOff,
                          3,
                          STATUS_ICON_SIZE_IN_ROWS,
                          LEFT_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);
  // Connection status
  CopyColumnsIntoMyBuffer(Connected(CONN_TYPE_MAIN) ? pConnectedIcon : pDisconnectedIcon,
                          3,
                          STATUS_ICON_SIZE_IN_ROWS,
                          CENTER_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);

  gRow = 31;
  gColumn = 5;
  gBitColumnMask = BIT4;

  if (BtPaired())
  {
    DrawString(Connected(CONN_TYPE_SPP) && QuerySniffState() == Active ? "ACT" : "BR",
      MetaWatch5, DRAW_OPT_OR);
  }
#if SUPPORT_BLE
  else if (BlePaired())
  {
    DrawString(Connected(CONN_TYPE_BLE) && CurrentInterval(INTERVAL_STATE) == SHORT ? "ACT" :
      (Connected(CONN_TYPE_HFP) && Connected(CONN_TYPE_MAP) ? "DUO" : "BLE"),
      MetaWatch5, DRAW_OPT_OR);
  }
#endif

  DrawBatteryOnIdleScreen(6, 9, MetaWatch7);
  WriteBufferToLcd(STARTING_ROW, 31 + 5);

  if (Full)
  {
    FillLcdBuffer(31 + 5, 60, LCD_WHITE);

    // Add Wavy line
//    gRow += 12;
    CopyRowsIntoMyBuffer(pWavyLine, 31 + 10, NUMBER_OF_ROWS_IN_WAVY_LINE);

    CopyColumnsIntoMyBuffer(pIconWatch, FTM_START_ROW, 21, 0, 2);

    /* add the firmware version */
    gColumn = 2;
    gRow = FTM_START_ROW + 2;
    gBitColumnMask = BIT2;
    DrawString("SW: ", MetaWatch7, DRAW_OPT_OR);
    DrawString((char *)VERSION, MetaWatch7, DRAW_OPT_OR);
    DrawString(" (", MetaWatch7, DRAW_OPT_OR);
    DrawString(BUILD, MetaWatch7, DRAW_OPT_OR);
    DrawChar(')', MetaWatch7, DRAW_OPT_OR);
    
    gColumn = 2;
    gRow = 65;
    gBitColumnMask = BIT2;
    DrawString("HW: REV ", MetaWatch7, DRAW_OPT_OR);
    DrawChar(GetMsp430HardwareRevision(), MetaWatch7, DRAW_OPT_OR);

    DrawLocalAddress();
    WriteBufferToLcd(31 + 5, 60);
  }
}

#define FTM_IDLE      0
#define FTM_VIBRA_A   1
#define FTM_VIBRA_B   2
#define FTM_EXIT      3
static unsigned char State = FTM_EXIT;

void HandleFieldTestMode(unsigned char const Option)
{
  static unsigned char Vibrate = FALSE;
  static unsigned long Counter = 0;

  switch (Option)
  {
  case FIELD_TEST_ENTER:

    State = FTM_IDLE;
    Counter = 0;
    break;

  case FIELD_TEST_TIMEOUT:

    if (State == FTM_VIBRA_B)
    {
      Vibrate = !Vibrate;
      SetVibeMotorState(Vibrate);
    }

    Counter ++;
    break;

  case FIELD_TEST_BUTTON_B:

    if (State == FTM_EXIT) return;

    State = FTM_VIBRA_B;
    StartTimer(FieldTestTimer);
    SetVibeMotorState(TRUE);
    Counter = 0;
    break;

  case FIELD_TEST_BUTTON_C:

    if (State == FTM_EXIT) return;

    State = FTM_IDLE;
    StopTimer(FieldTestTimer);
    Vibrate = FALSE;
    SetVibeMotorState(Vibrate);
    return;

  case FIELD_TEST_BUTTON_A:

    if (State != FTM_EXIT)
    {
      State = FTM_VIBRA_A;
      StartTimer(FieldTestTimer);
      SetVibeMotorState(TRUE);
      Counter = 0;
      break;
    }

  case FIELD_TEST_EXIT:

    State = FTM_EXIT;
    StopTimer(FieldTestTimer);
    SendMessage(IdleUpdateMsg, MSG_OPT_NONE);
    return;

  default: break;
  }

  // Draw counter
  char Elapse[10];
  char *pChar;
  unsigned long Hms;

  gRow = FTM_START_ROW;
  gColumn = 0;
  gBitColumnMask = BIT7;
  pChar = Elapse;

  Hms = Counter / 3600;
  Hms %= 1000;

  if (Hms > 99)
  {
    gBitColumnMask = BIT1; //maxwith + 1
    *pChar++ = Hms / 100 + ZERO;
    Hms %= 100;
  }
  *pChar++ = Hms / 10 + ZERO;
  *pChar++ = Hms % 10 + ZERO;
  *pChar++ = COLON;

  Hms = Counter % 3600 / 60;
  *pChar++ = Hms / 10 + ZERO;
  *pChar++ = Hms % 10 + ZERO;
  *pChar++ = COLON;

  Hms = Counter % 60;
  *pChar++ = Hms / 10 + ZERO;
  *pChar++ = Hms % 10 + ZERO;
  *pChar = 0;

  FillLcdBuffer(FTM_START_ROW, FTM_ROW_NUM, LCD_WHITE);
  DrawString(Elapse, Time, DRAW_OPT_OR);
  WriteBufferToLcd(FTM_START_ROW, FTM_ROW_NUM);
}

static void DrawLocalAddress(void)
{
  char pAddr[12 + 3]; // BT_ADDR is 6 bytes long
  GetBDAddrStr(pAddr);

  gRow = 82;
  gColumn = 1;
  gBitColumnMask = BIT2;

  DrawString(pAddr, MetaWatch7, DRAW_OPT_OR);
//  unsigned char i;
//  for (i = 2; i < 14; ++i)
//  {
//    DrawChar(pAddr[i], MetaWatch7, DRAW_OPT_OR);
//    if ((i & 0x03) == 1 && i != 13) DrawChar('-', MetaWatch7, DRAW_OPT_OR);
//  }
}

static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font)
{
  unsigned char BattVal = BatteryPercentage();
  unsigned char i = (BattVal == BATTERY_UNKNOWN) ? ICON_BATTERY_UNKNOWN : ICON_SET_BATTERY_V;
  CopyColumnsIntoMyBuffer(GetIcon(i), Row, IconInfo[i].Height, Col, IconInfo[i].Width);

  gRow = Row + (Font == MetaWatch7 ? 23 : 21); //29
  gColumn = Col - 1; //8
  gBitColumnMask = (Font == MetaWatch7) ? BIT4 : BIT5;

  if (BattVal != BATTERY_UNKNOWN)
  {
    if (BattVal < 100)
    {
      BattVal = ToBCD(BattVal);
      DrawChar(BattVal > 9 ? BCD_H(BattVal) + ZERO : SPACE, Font, DRAW_OPT_OR);
      DrawChar(BCD_L(BattVal) + ZERO, Font, DRAW_OPT_OR);
    }
    else DrawString("100", Font, DRAW_OPT_OR);

    DrawChar('%', Font, DRAW_OPT_OR);
  }
  else DrawString("Wait", MetaWatch5, DRAW_OPT_OR);
}

unsigned char const *GetIcon(unsigned char Id)
{
  unsigned char Index = 0;
  unsigned char Percent;
  unsigned char const *pIcon = IconInfo[Id].Data;

  switch (Id)
  {
  case ICON_SET_BLUETOOTH_BIG:
  case ICON_SET_BLUETOOTH_SMALL:

    if (!RadioOn()) Index = ICON_BLUETOOTH_OFF;
    else if (Connected(CONN_TYPE_MAIN)) Index = ICON_BLUETOOTH_CONN;
    else if (OnceConnected()) Index = ICON_BLUETOOTH_DISC;
    else Index = ICON_BLUETOOTH_ON;
    break;

  case ICON_SET_BATTERY_H:
  case ICON_SET_BATTERY_V:

    Percent = BatteryPercentage();

    if (Percent == 100) Index = BATTERY_LEVEL_NUMBER;
    else if (Percent <= WARNING_LEVEL && !Charging()) Index = 1; // warning icon index
    else
    {
      while (Percent > BATTERY_LEVEL_INTERVAL)
      {
        Index ++;
        Percent -= BATTERY_LEVEL_INTERVAL;
      }
    }

    if (!Charging()) pIcon += BATTERY_ICON_NUM * IconInfo[Id].Width * IconInfo[Id].Height;
    break;

  default: break;
  }

  pIcon += Index * IconInfo[Id].Width * IconInfo[Id].Height;
  return pIcon;
}

void DrawDateTime(void)
{
  // clean date&time area
  FillLcdBuffer(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM, LCD_WHITE);

  gRow = DEFAULT_HOURS_ROW;
  gColumn = DEFAULT_HOURS_COL;
  gBitColumnMask = DEFAULT_HOURS_COL_BIT;
  char Hms[4];
  GetHour(Hms);
  DrawString(Hms, DEFAULT_HOURS_FONT, DRAW_OPT_OR);

  gRow = DEFAULT_MINS_ROW;
  gColumn = DEFAULT_MINS_COL;
  gBitColumnMask = DEFAULT_MINS_COL_BIT;
  GetMinute(Hms);
  DrawString(Hms, DEFAULT_HOURS_FONT, DRAW_OPT_OR);

  if (GetProperty(PROP_TIME_SECOND))
  {
    gRow = DEFAULT_SECS_ROW;
    gColumn = DEFAULT_SECS_COL;
    gBitColumnMask = DEFAULT_SECS_COL_BIT;
    GetSecond(Hms);
    DrawString(Hms, DEFAULT_SECS_FONT, DRAW_OPT_OR);
  }
  else if (Charging() || BatteryPercentage() <= WARNING_LEVEL)
  {
    DrawBatteryOnIdleScreen(3, 9, MetaWatch5);
  }
  else
  {
    if (!GetProperty(PROP_24H_TIME_FORMAT))
    {
      gRow = DEFAULT_AM_PM_ROW;
      gColumn = DEFAULT_AM_PM_COL;
      gBitColumnMask = DEFAULT_AM_PM_COL_BIT;
      DrawString((RTCHOUR > 0x11) ? "PM" : "AM", DEFAULT_AM_PM_FONT, DRAW_OPT_OR);
    }

    gRow = GetProperty(PROP_24H_TIME_FORMAT) ? DEFAULT_DOW_24HR_ROW : DEFAULT_DOW_12HR_ROW;
    gColumn = DEFAULT_DOW_COL;
    gBitColumnMask = DEFAULT_DOW_COL_BIT;

    DrawString((tString *)DaysOfTheWeek[niLang][RTCDOW], DEFAULT_DOW_FONT, DRAW_OPT_OR);

    //add year when time is in 24 hour mode
    if (GetProperty(PROP_24H_TIME_FORMAT))
    {
      gRow = DEFAULT_DATE_YEAR_ROW;
      gColumn = DEFAULT_DATE_YEAR_COL;
      gBitColumnMask = DEFAULT_DATE_YEAR_COL_BIT;

      unsigned char Year = RTCYEARH;
      DrawChar(BCD_H(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_OR);
      DrawChar(BCD_L(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_OR);
      Year = RTCYEARL;
      DrawChar(BCD_H(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_OR);
      DrawChar(BCD_L(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_OR);
    }

    if (BlePaired() || BtPaired())
    {
      //Display month and day
      //Watch controls time - use default date position
      unsigned char DayFirst = GetProperty(PROP_DDMM_DATE_FORMAT);
      char Rtc[2];
      Rtc[DayFirst ? 0 : 1] = RTCDAY;
      Rtc[DayFirst ? 1 : 0] = RTCMON;

      gRow = DEFAULT_DATE_FIRST_ROW;
      gColumn = DEFAULT_DATE_FIRST_COL;
      gBitColumnMask = DEFAULT_DATE_FIRST_COL_BIT;

      DrawChar(BCD_H(Rtc[0]) + ZERO, DEFAULT_DATE_MONTH_FONT, DRAW_OPT_OR);
      DrawChar(BCD_L(Rtc[0]) + ZERO, DEFAULT_DATE_MONTH_FONT, DRAW_OPT_OR);
      
      //Display separator
      DrawChar(DayFirst ? '.' : '/', DEFAULT_DATE_SEPARATOR_FONT, DRAW_OPT_OR);
      
      //Display day second
      gRow = DEFAULT_DATE_SECOND_ROW;
      gColumn = DEFAULT_DATE_SECOND_COL;
      gBitColumnMask = DEFAULT_DATE_SECOND_COL_BIT;

      DrawChar(BCD_H(Rtc[1]) + ZERO, DEFAULT_DATE_DAY_FONT, DRAW_OPT_OR);
      DrawChar(BCD_L(Rtc[1]) + ZERO, DEFAULT_DATE_DAY_FONT, DRAW_OPT_OR);
    }
  }
  
  WriteBufferToLcd(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM);
}

static void GetHour(char *Hour)
{
  Hour[0] = RTCHOUR;
  
  if (!GetProperty(PROP_24H_TIME_FORMAT)) Hour[0] = To12H(Hour[0]);

  Hour[1] = BCD_L(Hour[0]) + ZERO;

  Hour[0] = BCD_H(Hour[0]);
  if(!Hour[0]) Hour[0] = SPACE;
  else Hour[0] += ZERO;
  Hour[2] = COLON;
  Hour[3] = NULL;
}

static void GetMinute(char *Min)
{  
  *Min++ = BCD_H(RTCMIN) + ZERO;
  *Min++ = BCD_L(RTCMIN) + ZERO;
  *Min = NULL;
}

static void GetSecond(char *Sec)
{
  *Sec++ = COLON;
  *Sec++ = BCD_H(RTCSEC) + ZERO;
  *Sec++ = BCD_L(RTCSEC) + ZERO;
  *Sec = NULL;
}

#define STATUS_BAR_HEIGHT       12
#define STATUS_BAR_START_ROW    0

void DrawStatusBarToLcd(void)
{
  FillLcdBuffer(STATUS_BAR_START_ROW, STATUS_BAR_HEIGHT, LCD_BLACK);

  Draw_t Info;
//  Info.Y = STATUS_BAR_START_ROW + 3;
//  Info.X = 2;

  gRow = STATUS_BAR_START_ROW + 3;
  gColumn = 0;
  gBitColumnMask = BIT2;
  char Hms[4];
  GetHour(Hms);
  DrawString(Hms, MetaWatch7, DRAW_OPT_NOT);
  GetMinute(Hms);
  DrawString(Hms, MetaWatch7, DRAW_OPT_NOT);

  if (!GetProperty(PROP_24H_TIME_FORMAT))
    DrawChar(RTCHOUR > 0x11 ? 'p' : 'a', MetaWatch7, DRAW_OPT_NOT);

  if (Charging() || BatteryPercentage() <= WARNING_LEVEL)
  {
    Info.X = 36;
    Info.Y = STATUS_BAR_START_ROW + 2;
    Info.Height = IconInfo[ICON_SET_BATTERY_H].Height;
    Info.Width = IconInfo[ICON_SET_BATTERY_H].Width << 3;
    Info.Opt = DRAW_OPT_NOT;
    DrawBitmapToLcd(&Info, IconInfo[ICON_SET_BATTERY_H].Width, GetIcon(ICON_SET_BATTERY_H));
  }
  else if (GetProperty(PROP_TIME_SECOND))
  {
    GetSecond(Hms);
    gColumn = 5;
    gBitColumnMask = BIT2;
    DrawString(Hms, MetaWatch7, DRAW_OPT_NOT);
  }

  if (Connected(CONN_TYPE_MAIN) || !OnceConnected())
  {
    unsigned char DayFirst = GetProperty(PROP_DDMM_DATE_FORMAT);
    char Rtc[2];
    Rtc[DayFirst ? 0 : 1] = RTCDAY;
    Rtc[DayFirst ? 1 : 0] = RTCMON;

    gColumn = 8;
    gBitColumnMask = BIT7;

    DrawChar(BCD_H(Rtc[0]) + ZERO, MetaWatch7, DRAW_OPT_NOT);
    DrawChar(BCD_L(Rtc[0]) + ZERO, MetaWatch7, DRAW_OPT_NOT);
    DrawChar(DayFirst ? '.' : '/', MetaWatch7, DRAW_OPT_NOT);
    DrawChar(BCD_H(Rtc[1]) + ZERO, MetaWatch7, DRAW_OPT_NOT);
    DrawChar(BCD_L(Rtc[1]) + ZERO, MetaWatch7, DRAW_OPT_NOT);
  }
  else
  {// bluetooth state 73, 3
    Info.X = 80;
    Info.Y = STATUS_BAR_START_ROW + 2;
    Info.Height = IconInfo[ICON_SET_BLUETOOTH_SMALL].Height;
    Info.Width = IconInfo[ICON_SET_BLUETOOTH_SMALL].Width << 3;
    Info.Opt = DRAW_OPT_NOT;
    DrawBitmapToLcd(&Info, IconInfo[ICON_SET_BLUETOOTH_SMALL].Width, GetIcon(ICON_SET_BLUETOOTH_SMALL));
  }

  WriteBufferToLcd(STATUS_BAR_START_ROW, STATUS_BAR_HEIGHT);
}

void DrawNotifPageNo(unsigned char PageNo)
{
  CopyColumnsIntoMyBuffer(IconInfo[ICON_NOTIF_NEXT].Data,
    NOTIF_PAGE_NO_START_ROW, IconInfo[ICON_NOTIF_NEXT].Height,
    11, IconInfo[ICON_NOTIF_NEXT].Width);

  gRow = NOTIF_PAGE_NO_START_ROW + 2;
  if (PageNo == 0)
  {
    gColumn = 8;
    gBitColumnMask = BIT6;
    DrawString("All!", MetaWatch5, DRAW_OPT_OR);
  }
  else
  {
    gColumn = 7;
    gBitColumnMask = BIT3;
    DrawChar(PageNo + ZERO, MetaWatch5, DRAW_OPT_OR);
    DrawString(" MORE", MetaWatch5, DRAW_OPT_OR);
  }

  WriteBufferToLcd(NOTIF_PAGE_NO_START_ROW, NOTIF_PAGE_NO_END_ROW - NOTIF_PAGE_NO_START_ROW + 1);
}

static void DrawString(char const *pString, etFontType Font, unsigned char Op)
{
  while (*pString && gColumn < BYTES_PER_LINE)
  {
    DrawChar(*pString++, Font, Op);
  }
}

void DrawTextToLcd(Draw_t *Info, char const *pText)
{
  gRow = Info->Y;
  gColumn = Info->X >> 3;
  gBitColumnMask = BIT0 << (Info->X & 0x07);

  unsigned char i = 0;
  for (; i < Info->TextLen; ++i) DrawChar(pText[i], (etFontType)Info->Id, Info->Opt);

}

/* fonts can be up to 16 bits wide */
static void DrawChar(char const Char, etFontType Font, unsigned char Op)
{
  tFont const *pFont = GetFont(Font);
  unsigned char const *pBitmap = GetFontBitmap(Char, Font);
  unsigned char CharWidth = GetCharWidth(Char, Font);

  unsigned char MaskBit = BIT0; //src
  unsigned char Set; // src bit is set or clear
  unsigned char x, y;

  for (x = 0; x < CharWidth && gColumn < BYTES_PER_LINE; ++x)
  {
  	for (y = 0; y < pFont->Height; ++y)
    {
      Set = *(pBitmap + y * pFont->WidthInBytes) & MaskBit;
      BitOp(&LcdBuf[gRow + y].Data[gColumn], gBitColumnMask, Set, Op);
    }

    /* the shift direction seems backwards... */
    MaskBit <<= 1;
    if (MaskBit == 0)
    {
      MaskBit = BIT0;
      pBitmap ++;
    }
    
    gBitColumnMask <<= 1;
    if (gBitColumnMask == 0)
    {
      gBitColumnMask = BIT0;
      gColumn ++;
    }
  }
}

void DrawBitmapToLcd(Draw_t *Info, unsigned char WidthInBytes, unsigned char const *pData)
{
  unsigned char Col = Info->X >> 3;
  unsigned char ColBit = BIT0 << (Info->X & 0x07); // dst, % 8
  unsigned char MaskBit = BIT0; //src
  unsigned char Set; // src bit is set or clear
  unsigned char x, y;

  for (x = 0; x < Info->Width && Col < BYTES_PER_LINE; ++x)
  {
  	for (y = 0; y < Info->Height; ++y)
    {
      if (Info->Opt != DRAW_OPT_FILL) Set = *(pData + y * WidthInBytes) & MaskBit;
      else Set = *pData & MaskBit;
      BitOp(&LcdBuf[Info->Y + y].Data[Col], ColBit, Set, Info->Opt);
    }

    /* the shift direction seems backwards... */
    MaskBit <<= 1;
    if (MaskBit == 0)
    {
      MaskBit = BIT0;
      if (Info->Opt != DRAW_OPT_FILL) pData ++;
    }
    
    ColBit <<= 1;
    if (ColBit == 0)
    {
      ColBit = BIT0;
      Col ++;
    }
  }
}

// Bit: 00010000; Set/Clear: 1/0; Op: OR, SET, NOT
void BitOp(unsigned char *pByte, unsigned char Bit, unsigned int Set, unsigned char Op)
{
  switch (Op)
  {
  case DRAW_OPT_OR:
    if (Set) *pByte |= Bit;
    break;

  case DRAW_OPT_SET:
  case DRAW_OPT_FILL:
    if (Set) *pByte |= Bit;
    else *pByte &= ~Bit;
    break;
    
  case DRAW_OPT_NOT: //~src set dst
    if (Set) *pByte &= ~Bit;
    else *pByte |= Bit;
    break;
    
  case DRAW_OPT_DST_NOT: //~dst
    if (*pByte & Bit) *pByte &= ~Bit;
    else *pByte |= Bit;
    break;

  default: break;
  }
}

void DrawBootloaderScreen(void)
{
#if BOOTLOADER
//  ClearLcd();
  FillLcdBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);
  CopyRowsIntoMyBuffer(pBootloader, BOOTLOADER_START_ROW, BOOTLOADER_ROWS);
  
  // Draw version
  gRow = 61;
  gColumn = 4;
  gBitColumnMask = BIT3;
  DrawString("V ", MetaWatch5, DRAW_OPT_OR);
  DrawString((char const *)VERSION, MetaWatch5, DRAW_OPT_OR);

  unsigned char i = 0;
  char const *pBootVer = BootVersion;

  PrintF("BL_VER addr:%04X %s", pBootVer, BootVersion);

  while (*pBootVer && i++ < 8)
  {
    if ((*pBootVer < ZERO || *pBootVer > '9') && *pBootVer != '.') break;
    pBootVer ++;
  }
  
  if (i > 4) // at least x.x.x
  {
    gRow += 7;
    gColumn = 4;
    gBitColumnMask = BIT3;
    DrawString("B ", MetaWatch5, DRAW_OPT_OR);
    DrawString(BootVersion, MetaWatch5, DRAW_OPT_OR);
  }

  DrawLocalAddress();
  
  WriteBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
#endif
}

void DrawSplashScreen(void)
{
//  ClearLcd();
  FillLcdBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);

#if WWZ
  DrawWwzSplashScreen();
#else
  /* draw metawatch logo */
  CopyColumnsIntoMyBuffer(pIconSplashLogo, 21, 13, 3, 6);
  CopyRowsIntoMyBuffer(pIconSplashMetaWatch, SPLASH_START_ROW, 12);
  CopyColumnsIntoMyBuffer(pIconSplashHandsFree, 58, 5, 2, 8);
//  Index = 0;
//  WriteToBuffer(pIconSplashMetaWatch, SPLASH_START_ROW, 12, 0, 12, CLEAR);
//  WriteToLcd(pBuf, 12);

#endif
//  WriteBufferToLcd(SPLASH_START_ROW, 12);
//  WriteBufferToLcd(21, 58 + 5 - 21);
  WriteBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

void FillLcdBuffer(unsigned char StartRow, unsigned char RowNum, unsigned char Value)
{
  if (GetLcdBuffer() == NULL) return;

  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    LcdBuf[StartRow].Row = StartRow;
    for (k = 0; k < BYTES_PER_LINE; ++k) LcdBuf[StartRow].Data[k] = Value;
    StartRow ++;
  }
}

void WriteBufferToLcd(unsigned char StartRow, unsigned char RowNum)
{
  WriteToLcd(&LcdBuf[StartRow], RowNum);

  PrintF("%04X)", LcdBuf);
  vPortFree(LcdBuf);
  LcdBuf = NULL;
}

void CopyRowsIntoMyBuffer(unsigned char const *pImage, unsigned char StartRow, unsigned char RowNum)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    LcdBuf[StartRow].Row = StartRow;
    for (k = 0; k < BYTES_PER_LINE; ++k) LcdBuf[StartRow].Data[k] = *pImage++;
    StartRow ++;
  }
}

#if __IAR_SYSTEMS_ICC__
void CopyRowsIntoMyBuffer_20(unsigned char const __data20 *pImage, unsigned char StartRow, unsigned char RowNum)
{
  if (GetLcdBuffer() == NULL) return;

  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    LcdBuf[StartRow].Row = StartRow;
    for (k = 0; k < BYTES_PER_LINE; ++k) LcdBuf[StartRow].Data[k] = *pImage++;
    StartRow ++;
  }
}
#endif

static void CopyColumnsIntoMyBuffer(unsigned char const *pImage,
                                    unsigned char StartRow,
                                    unsigned char RowNum,
                                    unsigned char StartCol,
                                    unsigned char ColNum)
{
  unsigned char DestRow = StartRow;
  unsigned char RowCounter = 0;
  unsigned char DestColumn = StartCol;
  unsigned char ColumnCounter = 0;
  unsigned int SourceIndex = 0;

  /* copy rows into display buffer */
  while (DestRow < LCD_ROW_NUM && RowCounter < RowNum)
  {
    DestColumn = StartCol;
    ColumnCounter = 0;
    LcdBuf[DestRow].Row = DestRow;
    
    while (DestColumn < BYTES_PER_LINE && ColumnCounter < ColNum)
    {
      LcdBuf[DestRow].Data[DestColumn] = pImage[SourceIndex];

      DestColumn ++;
      ColumnCounter ++;
      SourceIndex ++;
    }

    DestRow ++;
    RowCounter ++;
  }
}

void *GetLcdBuffer(void)
{
  if (LcdBuf == NULL)
  {
    LcdBuf = (tLcdLine *)pvPortMalloc(LCD_BUFFER_SIZE);
    if (!LcdBuf) PrintS("@LcdBuf");
  }
  return (void *)LcdBuf;
}
