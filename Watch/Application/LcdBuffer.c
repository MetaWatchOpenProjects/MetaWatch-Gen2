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
#include "LcdBuffer.h"
#include "hal_vibe.h"
#include "Messages.h"
#include "OneSecondTimers.h"
#include "queue.h"
#include "MessageQueues.h"
#include "MessageInfo.h"
#include "Wrapper.h"

#if COUNTDOWN_TIMER
#include "Countdown.h"
#endif

#define SPLASH_START_ROW              41

/* the internal buffer */
#define STARTING_ROW                  0
#define PHONE_DRAW_SCREEN_ROW_NUM     66

#define FTM_START_ROW       54
#define FTM_ROW_NUM         24

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

//#if __IAR_SYSTEMS_ICC__
//__no_init __root unsigned char niLang @ RTC_LANG_ADDR;
//#else
//extern unsigned char niLang;
//#endif
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

static tLcdLine pLcdBuf[LCD_ROW_NUM];
static tLcdLine *pBuf = pLcdBuf;

#define LCD_BUFFER_SIZE (sizeof(tLcdLine) * LCD_ROW_NUM)

static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;
static unsigned char Index;

#define FTM_IDLE      0
#define FTM_VIBRA_A   1
#define FTM_VIBRA_B   2
#define FTM_EXIT      3
static unsigned char State = FTM_EXIT;

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
static void WriteToBuffer(unsigned char const *pData, unsigned char StartRow, unsigned char RowNum,
                          unsigned char StartCol, unsigned char ColNum, unsigned char Op);

/******************************************************************************/

void DrawMenu(eIdleModePage Page)
{
  /* clear entire region */
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);

  switch (Page)
  {
  case Menu1Page: DrawMenu1(); break;
  case Menu2Page: DrawMenu2(); break;
  case Menu3Page: DrawMenu3(); break;
  default: PrintF("# No Such Menu Page:%d", Page); break;
  }

  /* these icons are common to all menus */
  DrawCommonMenuIcons();
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

static void DrawMenu1(void)
{
  const unsigned char *pIcon;

  if (BluetoothState() == Initializing)
  {
    pIcon = pBluetoothInitIcon;
  }
  else
  {
    pIcon = RadioOn() ? pBluetoothOnIcon : pBluetoothOffIcon;
  }

  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pInvertDisplayIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(GetProperty(PROP_TIME_SECOND) ? pSecondsOnMenuIcon : pSecondsOffMenuIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(LinkAlarmEnabled() ? pLinkAlarmOnIcon : pLinkAlarmOffIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void DrawMenu2(void)
{
  CopyColumnsIntoMyBuffer(RESET_PIN ? pNmiPinIcon : pRstPinIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  const unsigned char *pIcon;
  unsigned char MuxMode = GetMuxMode();

  if (MuxMode == MUX_MODE_SERIAL) pIcon = pSerialIcon;
  else if (MuxMode == MUX_MODE_GND) pIcon = pGroundIcon;
  else pIcon = pSbwIcon;
  
  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pNextIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pResetButtonIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void DrawMenu3(void)
{
  CopyColumnsIntoMyBuffer(pBootloaderIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(ChargeEnabled() ? pIconChargingEnabled : pIconChargingDisabled,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pNextIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pTestIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void DrawCommonMenuIcons(void)
{
  CopyColumnsIntoMyBuffer(pLedIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pExitIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

void DrawConnectionScreen(void)
{
  FillMyBuffer(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM, LCD_WHITE);

  if (!RadioOn())
  {
    gRow = WATCH_DRAW_SCREEN_ROW_NUM + 9;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Bluetooth", MetaWatch16, DRAW_OPT_BITWISE_OR);

    gRow += 16;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("OFF", MetaWatch16, DRAW_OPT_BITWISE_OR);

    CopyColumnsIntoMyBuffer(IconInfo[ICON_RADIO_OFF].Data,
      WATCH_DRAW_SCREEN_ROW_NUM + 9, IconInfo[ICON_RADIO_OFF].Height,
      0, IconInfo[ICON_RADIO_OFF].Width);
  }
  else if (PairedDeviceType()|| Connected(CONN_TYPE_ANCS))
  {
    gRow = WATCH_DRAW_SCREEN_ROW_NUM + 5;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("Start", MetaWatch7, DRAW_OPT_BITWISE_OR);

    gRow += 8;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("MWM", MetaWatch7, DRAW_OPT_BITWISE_OR);

    gRow += 8;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("Phone", MetaWatch7, DRAW_OPT_BITWISE_OR);

    gRow += 8;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString("App", MetaWatch7, DRAW_OPT_BITWISE_OR);

    CopyColumnsIntoMyBuffer(IconInfo[ICON_START_MWM].Data,
      WATCH_DRAW_SCREEN_ROW_NUM + 9, IconInfo[ICON_START_MWM].Height,
      6, IconInfo[ICON_START_MWM].Width);

//    old paired init screen
//    CopyRowsIntoMyBuffer(IconInfo[ICON_INIT_PAGE_PAIRED].Data,
//      WATCH_DRAW_SCREEN_ROW_NUM + 1, IconInfo[ICON_INIT_PAGE_PAIRED].Height);
  }
  else
  {
    FillMyBuffer(WATCH_DRAW_SCREEN_ROW_NUM, 11, LCD_BLACK);
    gRow = WATCH_DRAW_SCREEN_ROW_NUM + 2;
    gColumn = 1;
    gBitColumnMask = BIT2;
    DrawString(GetLocalName(), MetaWatch7, DRAW_OPT_BITWISE_NOT);

    gRow += 12;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Ready To", MetaWatch7, DRAW_OPT_BITWISE_OR);

    gRow += 8;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Pair With", MetaWatch7, DRAW_OPT_BITWISE_OR);

    gRow += 8;
    gColumn = 4;
    gBitColumnMask = BIT0;
    DrawString("Phone", MetaWatch7, DRAW_OPT_BITWISE_OR);

    CopyColumnsIntoMyBuffer(IconInfo[ICON_PHONE].Data,
      WATCH_DRAW_SCREEN_ROW_NUM + 14, IconInfo[ICON_PHONE].Height,
      1, IconInfo[ICON_PHONE].Width);
  }

  /* add the firmware version */
  gRow = 73;
  gColumn = 4;
  gBitColumnMask = BIT0;
  DrawString("V ", MetaWatch5, DRAW_OPT_BITWISE_OR);
  DrawString((char *)VERSION, MetaWatch5, DRAW_OPT_BITWISE_OR);

  DrawLocalAddress();
  
  SendMyBufferToLcd(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM);
}

void DrawWatchStatusScreen(unsigned char Full)
{
  FillMyBuffer(STARTING_ROW, FTM_START_ROW - NUMBER_OF_ROWS_IN_WAVY_LINE, LCD_WHITE);

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

  if (PairedDeviceType() == DEVICE_TYPE_SPP)
  {
    DrawString(Connected(CONN_TYPE_SPP) && QuerySniffState() == Active ? "ACT" : "BR",
      MetaWatch5, DRAW_OPT_BITWISE_OR);
  }
#if SUPPORT_BLE
  else if (PairedDeviceType() == DEVICE_TYPE_BLE)
  {
    DrawString(Connected(CONN_TYPE_BLE) && CurrentInterval(INTERVAL_STATE) == SHORT ? "ACT" :
      (Connected(CONN_TYPE_HFP) && Connected(CONN_TYPE_MAP) ? "DUO" : "BLE"),
      MetaWatch5, DRAW_OPT_BITWISE_OR);
  }
#endif

  DrawBatteryOnIdleScreen(6, 9, MetaWatch7);
  SendMyBufferToLcd(STARTING_ROW, 31 + 5);

  if (Full)
  {
    FillMyBuffer(31 + 5, 60, LCD_WHITE);

    // Add Wavy line
//    gRow += 12;
    CopyRowsIntoMyBuffer(pWavyLine, 31 + 10, NUMBER_OF_ROWS_IN_WAVY_LINE);

    CopyColumnsIntoMyBuffer(pIconWatch, FTM_START_ROW, 21, 0, 2);

    /* add the firmware version */
    gColumn = 2;
    gRow = FTM_START_ROW + 2;
    gBitColumnMask = BIT2;
    DrawString("SW: ", MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawString((char *)VERSION, MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawString(" (", MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(BUILD[0], MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(BUILD[1], MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(BUILD[2], MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(')', MetaWatch7, DRAW_OPT_BITWISE_OR);
    
    gColumn = 2;
    gRow = 65;
    gBitColumnMask = BIT2;
    DrawString("HW: REV ", MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(GetMsp430HardwareRevision(), MetaWatch7, DRAW_OPT_BITWISE_OR);

    DrawLocalAddress();
    SendMyBufferToLcd(31 + 5, 60);
  }
}

void HandleFieldTestMode(unsigned char const Option)
{
  static unsigned char Vibrate = FALSE;
  static unsigned long Counter = 0;

  switch (Option)
  {
  case FIELD_TEST_ENTER:

    State = FTM_IDLE;
    
    FillMyBuffer(FTM_START_ROW, FTM_ROW_NUM, LCD_WHITE);
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

    if (State == FTM_EXIT)
    {
      CreateAndSendMessage(MusicControlMsg, MSG_OPT_MUSIC_PLAY);
      return;
    }
    State = FTM_VIBRA_B;
    SetTimer(VibraTimer, TOUT_FIELD_TEST, REPEAT_FOREVER, DISPLAY_QINDEX, FieldTestMsg, FIELD_TEST_TIMEOUT);
    SetVibeMotorState(TRUE);
    Counter = 0;
    break;

  case FIELD_TEST_BUTTON_C:

    if (State == FTM_EXIT) return;

    State = FTM_IDLE;
    StopTimer(VibraTimer);
    Vibrate = FALSE;
    SetVibeMotorState(Vibrate);
    return;

  case FIELD_TEST_BUTTON_A:

    if (State != FTM_EXIT)
    {
      State = FTM_VIBRA_A;
      SetTimer(VibraTimer, TOUT_FIELD_TEST, REPEAT_FOREVER, DISPLAY_QINDEX, FieldTestMsg, FIELD_TEST_TIMEOUT);
      SetVibeMotorState(TRUE);
      Counter = 0;
      break;
    }

  case FIELD_TEST_EXIT:

    State = FTM_EXIT;
    StopTimer(VibraTimer);
    CreateAndSendMessage(IdleUpdateMsg, MSG_OPT_NONE);
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
    *pChar++ = Hms / 100 + '0';
    Hms %= 100;
  }
  *pChar++ = Hms / 10 + '0';
  *pChar++ = Hms % 10 + '0';
  *pChar++ = ':';

  Hms = Counter % 3600 / 60;
  *pChar++ = Hms / 10 + '0';
  *pChar++ = Hms % 10 + '0';
  *pChar++ = ':';

  Hms = Counter % 60;
  *pChar++ = Hms / 10 + '0';
  *pChar++ = Hms % 10 + '0';
  *pChar = 0;

  FillMyBuffer(FTM_START_ROW, FTM_ROW_NUM, LCD_WHITE);
  DrawString(Elapse, Time, DRAW_OPT_BITWISE_OR);
  SendMyBufferToLcd(FTM_START_ROW, FTM_ROW_NUM);
}

static void DrawLocalAddress(void)
{
  char pAddr[12 + 3]; // BT_ADDR is 6 bytes long
  GetBDAddrStr(pAddr);

  gRow = 82;
  gColumn = 1;
  gBitColumnMask = BIT2;

  unsigned char i;
  for (i = 2; i < 14; ++i)
  {
    DrawChar(pAddr[i], MetaWatch7, DRAW_OPT_BITWISE_OR);
    if ((i & 0x03) == 1 && i != 13) DrawChar('-', MetaWatch7, DRAW_OPT_BITWISE_OR);
  }
}

static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font)
{
  unsigned char BattVal = BatteryPercentage();
  unsigned char i = (BattVal == BATTERY_UNKNOWN) ? ICON_BATTERY_UNKNOWN : ICON_SET_BATTERY_V;
  CopyColumnsIntoMyBuffer(GetIcon(i), Row, IconInfo[i].Height, Col, IconInfo[i].Width);

  gRow = Row + (Font == MetaWatch7 ? 23 : 21); //29
  gColumn = Col - 1; //8
  gBitColumnMask = (Font == MetaWatch7) ? BIT4 : BIT7;

  if (BattVal != BATTERY_UNKNOWN)
  {
    if (BattVal < 100)
    {
      BattVal = ToBCD(BattVal);
      DrawChar(BattVal > 9 ? BCD_H(BattVal) + ZERO : SPACE, Font, DRAW_OPT_BITWISE_OR);
      DrawChar(BCD_L(BattVal) + ZERO, Font, DRAW_OPT_BITWISE_OR);
    }
    else DrawString("100", Font, DRAW_OPT_BITWISE_OR);

    DrawChar('%', Font, DRAW_OPT_BITWISE_OR);
  }
  else DrawString("Wait", MetaWatch5, DRAW_OPT_BITWISE_OR);
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
  FillMyBuffer(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM, LCD_WHITE);

  gRow = DEFAULT_HOURS_ROW;
  gColumn = DEFAULT_HOURS_COL;
  gBitColumnMask = DEFAULT_HOURS_COL_BIT;
  char Hms[4];
  GetHour(Hms);
  DrawString(Hms, DEFAULT_HOURS_FONT, DRAW_OPT_BITWISE_OR);

  gRow = DEFAULT_MINS_ROW;
  gColumn = DEFAULT_MINS_COL;
  gBitColumnMask = DEFAULT_MINS_COL_BIT;
  GetMinute(Hms);
  DrawString(Hms, DEFAULT_HOURS_FONT, DRAW_OPT_BITWISE_OR);

  if (GetProperty(PROP_TIME_SECOND))
  {
    gRow = DEFAULT_SECS_ROW;
    gColumn = DEFAULT_SECS_COL;
    gBitColumnMask = DEFAULT_SECS_COL_BIT;
    GetSecond(Hms);
    DrawString(Hms, DEFAULT_SECS_FONT, DRAW_OPT_BITWISE_OR);
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
      DrawString((RTCHOUR > 0x11) ? "PM" : "AM", DEFAULT_AM_PM_FONT, DRAW_OPT_BITWISE_OR);
    }

    gRow = GetProperty(PROP_24H_TIME_FORMAT) ? DEFAULT_DOW_24HR_ROW : DEFAULT_DOW_12HR_ROW;
    gColumn = DEFAULT_DOW_COL;
    gBitColumnMask = DEFAULT_DOW_COL_BIT;

    DrawString((tString *)DaysOfTheWeek[niLang][RTCDOW], DEFAULT_DOW_FONT, DRAW_OPT_BITWISE_OR);

    //add year when time is in 24 hour mode
    if (GetProperty(PROP_24H_TIME_FORMAT))
    {
      gRow = DEFAULT_DATE_YEAR_ROW;
      gColumn = DEFAULT_DATE_YEAR_COL;
      gBitColumnMask = DEFAULT_DATE_YEAR_COL_BIT;

      unsigned char Year = RTCYEARH;
      DrawChar(BCD_H(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_BITWISE_OR);
      DrawChar(BCD_L(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_BITWISE_OR);
      Year = RTCYEARL;
      DrawChar(BCD_H(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_BITWISE_OR);
      DrawChar(BCD_L(Year) + ZERO, DEFAULT_DATE_YEAR_FONT, DRAW_OPT_BITWISE_OR);
    }

    if (PairedDeviceType())
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

      DrawChar(BCD_H(Rtc[0]) + ZERO, DEFAULT_DATE_MONTH_FONT, DRAW_OPT_BITWISE_OR);
      DrawChar(BCD_L(Rtc[0]) + ZERO, DEFAULT_DATE_MONTH_FONT, DRAW_OPT_BITWISE_OR);
      
      //Display separator
      DrawChar(DayFirst ? '.' : '/', DEFAULT_DATE_SEPARATOR_FONT, DRAW_OPT_BITWISE_OR);
      
      //Display day second
      gRow = DEFAULT_DATE_SECOND_ROW;
      gColumn = DEFAULT_DATE_SECOND_COL;
      gBitColumnMask = DEFAULT_DATE_SECOND_COL_BIT;

      DrawChar(BCD_H(Rtc[1]) + ZERO, DEFAULT_DATE_DAY_FONT, DRAW_OPT_BITWISE_OR);
      DrawChar(BCD_L(Rtc[1]) + ZERO, DEFAULT_DATE_DAY_FONT, DRAW_OPT_BITWISE_OR);
    }
  }
  
  SendMyBufferToLcd(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM);
}

static void GetHour(char *Hour)
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
  FillMyBuffer(STATUS_BAR_START_ROW, STATUS_BAR_HEIGHT, LCD_WHITE);

  gRow = STATUS_BAR_START_ROW + 3;

  gColumn = 0;
  gBitColumnMask = BIT2;
  char Hms[4];
  GetHour(Hms);
  DrawString(Hms, MetaWatch7, DRAW_OPT_BITWISE_OR);
  GetMinute(Hms);
  DrawString(Hms, MetaWatch7, DRAW_OPT_BITWISE_OR);

  if (!GetProperty(PROP_24H_TIME_FORMAT))
    DrawChar(RTCHOUR > 0x11 ? 'p' : 'a', MetaWatch7, DRAW_OPT_BITWISE_OR);

  if (Charging() || BatteryPercentage() <= WARNING_LEVEL)
  {
    CopyColumnsIntoMyBuffer(GetIcon(ICON_SET_BATTERY_H), gRow - 1,
      IconInfo[ICON_SET_BATTERY_H].Height, 4, IconInfo[ICON_SET_BATTERY_H].Width);
  }
  else if (GetProperty(PROP_TIME_SECOND))
  {
    GetSecond(Hms);
    gColumn = 5;
    gBitColumnMask = BIT0;
    DrawString(Hms, MetaWatch7, DRAW_OPT_BITWISE_OR);
  }

  if (Connected(CONN_TYPE_MAIN) || !OnceConnected())
  {
    unsigned char DayFirst = GetProperty(PROP_DDMM_DATE_FORMAT);
    char Rtc[2];
    Rtc[DayFirst ? 0 : 1] = RTCDAY;
    Rtc[DayFirst ? 1 : 0] = RTCMON;

    gColumn = 8;
    gBitColumnMask = BIT7;

    DrawChar(BCD_H(Rtc[0]) + ZERO, MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(BCD_L(Rtc[0]) + ZERO, MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(DayFirst ? '.' : '/', MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(BCD_H(Rtc[1]) + ZERO, MetaWatch7, DRAW_OPT_BITWISE_OR);
    DrawChar(BCD_L(Rtc[1]) + ZERO, MetaWatch7, DRAW_OPT_BITWISE_OR);
  }
  else
  {// bluetooth state 73, 3
    CopyColumnsIntoMyBuffer(GetIcon(ICON_SET_BLUETOOTH_SMALL), gRow - 1,
      IconInfo[ICON_SET_BLUETOOTH_SMALL].Height, 10, IconInfo[ICON_SET_BLUETOOTH_SMALL].Width);
  }

  SendMyBufferToLcd(STATUS_BAR_START_ROW, STATUS_BAR_HEIGHT);
}

void DrawTextToLcd(DrawLcd_t *pData)
{
  gRow = pData->Row;
  gColumn = pData->Col;
  gBitColumnMask = pData->ColMask;

  unsigned char i;
  for (i = 0; i < pData->Length; ++i) DrawChar(pData->pText[i], pData->Font, DRAW_OPT_BITWISE_OR);
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
      BitOp(&pLcdBuf[gRow + y].Data[gColumn], gBitColumnMask, Set, Op);
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

// Bit: 00010000; Set/Clear: 1/0; Op: OR, SET, NOT
void BitOp(unsigned char *pByte, unsigned char Bit, unsigned int Set, unsigned char Op)
{
  switch (Op)
  {
  case DRAW_OPT_BITWISE_OR:
    if (Set) *pByte |= Bit;
    break;

  case DRAW_OPT_BITWISE_SET: //Set
    if (Set) *pByte |= Bit;
    else *pByte &= ~Bit;
    break;
    
  case DRAW_OPT_BITWISE_NOT: //~src set dst
    if (Set) *pByte &= ~Bit;
    else *pByte |= Bit;
    break;
    
  case DRAW_OPT_BITWISE_DST_NOT: //~dst
    if (*pByte & Bit) *pByte &= ~Bit;
    else *pByte |= Bit;
    break;

  default: break;
  }
}

static void DrawString(char const *pString, etFontType Font, unsigned char Op)
{
  while (*pString && gColumn < BYTES_PER_LINE)
  {
    DrawChar(*pString++, Font, Op);
  }
}

void DrawBootloaderScreen(void)
{
#if BOOTLOADER
//  ClearLcd();
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);
  CopyRowsIntoMyBuffer(pBootloader, BOOTLOADER_START_ROW, BOOTLOADER_ROWS);
  
  // Draw version
  gRow = 61;
  gColumn = 4;
  gBitColumnMask = BIT3;
  DrawString("V ", MetaWatch5, DRAW_OPT_BITWISE_OR);
  DrawString((char const *)VERSION, MetaWatch5, DRAW_OPT_BITWISE_OR);

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
    DrawString("B ", MetaWatch5, DRAW_OPT_BITWISE_OR);
    DrawString(BootVersion, MetaWatch5, DRAW_OPT_BITWISE_OR);
  }

  DrawLocalAddress();
  
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
#endif
}

void DrawSplashScreen(void)
{
//  ClearLcd();
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, LCD_WHITE);

#if COUNTDOWN_TIMER
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
//  SendMyBufferToLcd(SPLASH_START_ROW, 12);
//  SendMyBufferToLcd(21, 58 + 5 - 21);
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

void FillMyBuffer(unsigned char StartRow, unsigned char RowNum, unsigned char Value)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    pLcdBuf[StartRow].Row = StartRow;
    for (k = 0; k < BYTES_PER_LINE; ++k) pLcdBuf[StartRow].Data[k] = Value;
    StartRow ++;
  }
}

void SendMyBufferToLcd(unsigned char StartRow, unsigned char RowNum)
{
  WriteToLcd(&pLcdBuf[StartRow], RowNum);
}

static void WriteToBuffer(unsigned char const *pData, unsigned char StartRow, unsigned char RowNum,
                          unsigned char StartCol, unsigned char ColNum, unsigned char Op)
{
  PrintF("Idx:%u Start:%u Num:%u", Index, StartRow, RowNum);

  unsigned char i, k;

  for (i = 0; i < RowNum; ++i)
  {
    pBuf[Index].Row = StartRow++;
    PrintF("i:%u R:%u", Index, pBuf[Index].Row);

    if (Op == CLEAR) for (k = 0; k < BYTES_PER_LINE; ++k) pBuf[Index].Data[k] = 0;

    for (k = 0; k < ColNum; ++k)
    {
      pBuf[Index].Data[StartCol++] = *pData;
      if (Op != SET) pData ++;
    }
    Index ++;
  }
}

void CopyRowsIntoMyBuffer(unsigned char const *pImage, unsigned char StartRow, unsigned char RowNum)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    pLcdBuf[StartRow].Row = StartRow;
    for (k = 0; k < BYTES_PER_LINE; ++k) pLcdBuf[StartRow].Data[k] = *pImage++;
    StartRow ++;
  }
}

#if __IAR_SYSTEMS_ICC__
void CopyRowsIntoMyBuffer_20(unsigned char const __data20 *pImage, unsigned char StartRow, unsigned char RowNum)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    pLcdBuf[StartRow].Row = StartRow;
    for (k = 0; k < BYTES_PER_LINE; ++k) pLcdBuf[StartRow].Data[k] = *pImage++;
    StartRow ++;
  }
}
#endif

static void CopyColumnsIntoMyBuffer(unsigned char const* pImage,
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
    pLcdBuf[DestRow].Row = DestRow;
    
    while (DestColumn < BYTES_PER_LINE && ColumnCounter < ColNum)
    {
      pLcdBuf[DestRow].Data[DestColumn] = pImage[SourceIndex];

      DestColumn ++;
      ColumnCounter ++;
      SourceIndex ++;
    }

    DestRow ++;
    RowCounter ++;
  }
}

void *GetDrawBuffer(void)
{
  return (void *)pLcdBuf;
}

//tLcdLine *AllocLcdBuffer(unsigned int LineNum)
//{
//  pLcdBuf = (tLcdLine *)pvPortMalloc(LineNum * sizeof(tLcdLine));
//  if (pLcdBuf == NULL) PrintS("### LcdBuf");
//  return pLcdBuf;
//}
