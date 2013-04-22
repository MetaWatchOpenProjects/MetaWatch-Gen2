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

#include "hal_board_type.h"
#include "hal_battery.h"
#include "hal_miscellaneous.h"
#include "hal_lcd.h"
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
#include "hal_lpm.h"
#include "hal_rtc.h"

#if COUNTDOWN_TIMER
#include "Countdown.h"
#endif

#define SPLASH_START_ROW   (41)

/* the internal buffer */
#define STARTING_ROW                  ( 0 )
#define PHONE_DRAW_SCREEN_ROW_NUM     ( 66 )

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

extern const char BUILD[];
extern const char VERSION[];

#if BOOTLOADER
#include "bl_boot.h"
//#pragma location="BOOT_VERSION"
//__no_init __root const char BL_VERSION[VERSION_LEN];
extern const char BootVersion[VERSION_LEN];
#endif

static tLcdLine pMyBuffer[LCD_ROW_NUM];
#define LCD_BUFFER_SIZE (sizeof(tLcdLine) * LCD_ROW_NUM)

static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;

static void CopyColumnsIntoMyBuffer(unsigned char const *pImage, unsigned char StartRow,
                                    unsigned char RowNum, unsigned char StartCol, unsigned char ColNum);

static void DrawMenu1(void);
static void DrawMenu2(void);
static void DrawMenu3(void);
static void DrawCommonMenuIcons(void);

static void DrawSecs(void);
static void DrawMins(void);
static void DrawHours(void);
static void WriteFontCharacter(unsigned char Character);
static void WriteFontString(char const *pString);
static void DrawLocalAddress(unsigned char Col, unsigned Row);
static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font);

/******************************************************************************/

void DrawMenu(eIdleModePage Page)
{
  /* clear entire region */
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);

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
  const unsigned char *pSwash;

  if (!RadioOn()) pSwash = pBootPageBluetoothOffSwash;
  else if (ValidAuthInfo()) pSwash = pBootPagePairedSwash;
  else pSwash = pBootPageNoPairSwash;
  
  FillMyBuffer(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM, 0x00);
  CopyRowsIntoMyBuffer(pSwash, WATCH_DRAW_SCREEN_ROW_NUM + 1, 32);

  /* add the firmware version */
  gRow = 68;
  gColumn = 4;
  gBitColumnMask = BIT0;
  SetFont(MetaWatch5);
  WriteFontString("V ");
  WriteFontString((char *)VERSION);

  DrawLocalAddress(1, 80);
  
  SendMyBufferToLcd(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM);
}

void DrawWatchStatusScreen(void)
{
  FillMyBuffer(0, LCD_ROW_NUM, 0);
  CopyColumnsIntoMyBuffer(RadioOn() ? pRadioOnIcon : pRadioOffIcon,
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
  SetFont(MetaWatch5);

  if (PairedDeviceType() == DEVICE_TYPE_BLE)
  {
    if (Connected(CONN_TYPE_HFP) && Connected(CONN_TYPE_MAP)) WriteFontString("DUO");
    else WriteFontString("BLE");
  }
  else if (PairedDeviceType() == DEVICE_TYPE_SPP) WriteFontString("BR");

  DrawBatteryOnIdleScreen(6, 9, MetaWatch7);

  // Add Wavy line
  gRow += 12;
  CopyRowsIntoMyBuffer(pWavyLine, gRow, NUMBER_OF_ROWS_IN_WAVY_LINE);
  CopyColumnsIntoMyBuffer(pIconWatch, 54, 21, 0, 2); //54, 21, 2, 2);

  /* add the firmware version */
  gColumn = 2;
  gRow = 56;
  gBitColumnMask = BIT2;
  WriteFontString("SW: ");
  WriteFontString((char *)VERSION);
  WriteFontString(" (");
  WriteFontCharacter(BUILD[0]);
  WriteFontCharacter(BUILD[1]);
  WriteFontCharacter(BUILD[2]);
  WriteFontCharacter(')');
  
  gColumn = 2;
  gRow = 65;
  gBitColumnMask = BIT2;
  WriteFontString("HW: REV ");
  WriteFontCharacter(GetMsp430HardwareRevision());

  DrawLocalAddress(1, 80);
  
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

static void DrawLocalAddress(unsigned char Col, unsigned Row)
{
  char pAddr[12 + 3]; // BT_ADDR is 6 bytes long
  GetBDAddrStr(pAddr);

  gRow = Row;
  gColumn = Col;
  gBitColumnMask = BIT2;
  SetFont(MetaWatch7);
  
  unsigned char i;
  for (i = 2; i < 14; ++i)
  {
    WriteFontCharacter(pAddr[i]);
    if ((i & 0x03) == 1 && i != 13) WriteFontCharacter('-');
  }
}

void DrawCallScreen(char *pCallerId, char *pCallerName)
{
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
  CopyRowsIntoMyBuffer(pCallNotif, CALL_NOTIF_START_ROW, CALL_NOTIF_ROWS);

  gRow = 70;
  gColumn = 1;
  SetFont(MetaWatch7);
  gBitColumnMask = BIT4;
  WriteFontString(pCallerId);

  gRow = 48;
  SetFont(MetaWatch16);

  // align center
  unsigned char i = 0;
  unsigned char NameWidth = 0;
  for (; i < strlen(pCallerName); ++i) NameWidth += GetCharacterWidth(pCallerName[i]);
  NameWidth += GetFontSpacing() * strlen(pCallerName);
  
  gColumn = ((LCD_COL_NUM - NameWidth) >> 1) >> 3;
  gBitColumnMask = 1 << (((LCD_COL_NUM - NameWidth) >> 1) & 0x07);

  WriteFontString(pCallerName);
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);    
}

static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font)
{
  // Battery
  CopyColumnsIntoMyBuffer(GetBatteryIcon(ICON_SET_BATTERY_V), Row,
    IconInfo[ICON_SET_BATTERY_V].Height, Col, IconInfo[ICON_SET_BATTERY_V].Width);

  SetFont(Font);
  gRow = Row + (Font == MetaWatch7 ? 23 : 21); //29
  gColumn = Col - 1; //8
  gBitColumnMask = (Font == MetaWatch7) ? BIT4 : BIT7;

  unsigned char BattVal = BatteryPercentage();
  
  if (BattVal < 100)
  {
    BattVal = ToBCD(BattVal);
    WriteFontCharacter(BattVal > 9 ? BCD_H(BattVal) +'0' : ' ');
    WriteFontCharacter(BCD_L(BattVal) +'0');
  }
  else WriteFontString("100");

  WriteFontCharacter('%');
}

const unsigned char *GetBatteryIcon(unsigned char Id)
{
  unsigned int Level = Read(BATTERY);
  unsigned char Index = 0;

  if (Level >= BATTERY_FULL_LEVEL) Index = BATTERY_LEVEL_NUMBER;
  else if (Level <= BatteryCriticalLevel(CRITICAL_WARNING) && !Charging()) Index = 1; // warning icon index
  else
  {
    unsigned int Empty = BatteryCriticalLevel(CRITICAL_BT_OFF);
    unsigned int Step = (BATTERY_FULL_LEVEL - Empty) / BATTERY_LEVEL_NUMBER;
    
    while (Level > (Empty + Step * Index)) Index ++;
  }
  
  const unsigned char *pIcon = IconInfo[Id].pIconSet + (Index * IconInfo[Id].Width * IconInfo[Id].Height);
  if (!Charging()) pIcon += BATTERY_ICON_NUM * IconInfo[Id].Width * IconInfo[Id].Height;

  return pIcon;
}

void DrawDateTime(void)
{
  // clean date&time area
  FillMyBuffer(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM, 0x00);

  DrawHours();
  DrawMins();
  
  if (GetProperty(PROP_TIME_SECOND))
  {
    WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
    DrawSecs();
  }
  else if (Charging()) DrawBatteryOnIdleScreen(3, 9, MetaWatch5);
  else
  {
    if (!GetProperty(PROP_24H_TIME_FORMAT))
    {
      gRow = DEFAULT_AM_PM_ROW;
      gColumn = DEFAULT_AM_PM_COL;
      gBitColumnMask = DEFAULT_AM_PM_COL_BIT;
      SetFont(DEFAULT_AM_PM_FONT);
      WriteFontString((RTCHOUR >= 12) ? "PM" : "AM");
    }

    gRow = GetProperty(PROP_24H_TIME_FORMAT) ? DEFAULT_DOW_24HR_ROW : DEFAULT_DOW_12HR_ROW;
    gColumn = DEFAULT_DOW_COL;
    gBitColumnMask = DEFAULT_DOW_COL_BIT;
    SetFont(DEFAULT_DOW_FONT);
    WriteFontString((tString *)DaysOfTheWeek[LANG_EN][RTCDOW]);

    //add year when time is in 24 hour mode
    if (GetProperty(PROP_24H_TIME_FORMAT))
    {
      gRow = DEFAULT_DATE_YEAR_ROW;
      gColumn = DEFAULT_DATE_YEAR_COL;
      gBitColumnMask = DEFAULT_DATE_YEAR_COL_BIT;
      SetFont(DEFAULT_DATE_YEAR_FONT);

      unsigned char Year = RTCYEARH;
      WriteFontCharacter(BCD_H(Year) +'0');
      WriteFontCharacter(BCD_L(Year) +'0');
      Year = RTCYEARL;
      WriteFontCharacter(BCD_H(Year) +'0');
      WriteFontCharacter(BCD_L(Year) +'0');
    }

    //Display month and day
    //Watch controls time - use default date position
    unsigned char DayFirst = GetProperty(PROP_DDMM_DATE_FORMAT);
    unsigned char Rtc[2];
    Rtc[DayFirst ? 0 : 1] = RTCDAY;
    Rtc[DayFirst ? 1 : 0] = RTCMON;

    gRow = DEFAULT_DATE_FIRST_ROW;
    gColumn = DEFAULT_DATE_FIRST_COL;
    gBitColumnMask = DEFAULT_DATE_FIRST_COL_BIT;
    SetFont(DEFAULT_DATE_MONTH_FONT);
    
    WriteFontCharacter(BCD_H(Rtc[0]) + '0');
    WriteFontCharacter(BCD_L(Rtc[0]) + '0');
    
    //Display separator
    SetFont(DEFAULT_DATE_SEPARATOR_FONT);
    WriteFontCharacter(DayFirst ? '.' : '/');
    
    //Display day second
    gRow = DEFAULT_DATE_SECOND_ROW;
    gColumn = DEFAULT_DATE_SECOND_COL;
    gBitColumnMask = DEFAULT_DATE_SECOND_COL_BIT;
    SetFont(DEFAULT_DATE_DAY_FONT);
    
    WriteFontCharacter(BCD_H(Rtc[1]) + '0');
    WriteFontCharacter(BCD_L(Rtc[1]) + '0');
  }
  
  SendMyBufferToLcd(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM);
}

static void DrawHours(void)
{
  unsigned char HL = RTCHOUR;
  unsigned char HH = BCD_H(HL);
  HL = BCD_L(HL);

  /* if required convert to twelve hour format */
  if (!GetProperty(PROP_24H_TIME_FORMAT))
  {
    if (HH == 1 && HL > 2) {HH = 0; HL -= 2;}
    else if (HH == 2) {HH = 0; HL += 8; if (HL > 9) {HL -= 10; HH ++;}}
  }
  
  //Watch controls time - use default position
  gRow = DEFAULT_HOURS_ROW;
  gColumn = DEFAULT_HOURS_COL;
  gBitColumnMask = DEFAULT_HOURS_COL_BIT;
  SetFont(DEFAULT_HOURS_FONT);
  
  /* if first digit is zero then leave location blank */
  if (HH == 0 && !GetProperty(PROP_24H_TIME_FORMAT))
    WriteFontCharacter(TIME_CHARACTER_SPACE_INDEX);
  else WriteFontCharacter(HH);

  WriteFontCharacter(HL);
  
  //Draw colon char to separate HH and MM (uses same font has HH, but 
  //can have different position
  //Watch controls time - use current position (location after
  //writing HH, which updates gRow and gColumn and gColumnBitMask)
  SetFont(DEFAULT_TIME_SEPARATOR_FONT);
  WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
}

static void DrawMins(void)
{  
  //Watch controls time - use default position
  gRow = DEFAULT_MINS_ROW;
  gColumn = DEFAULT_MINS_COL;
  gBitColumnMask = DEFAULT_MINS_COL_BIT;
  SetFont(DEFAULT_MINS_FONT);
  
  unsigned char Minute = RTCMIN;
  WriteFontCharacter(BCD_H(Minute));
  WriteFontCharacter(BCD_L(Minute));
}

static void DrawSecs(void)
{
  //Watch controls time - use default position
  gRow = DEFAULT_SECS_ROW;
  gColumn = DEFAULT_SECS_COL;
  gBitColumnMask = DEFAULT_SECS_COL_BIT;
  SetFont(DEFAULT_SECS_FONT);
  
  //Draw colon separator - only needed when seconds are displayed
  WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
  unsigned char Second = RTCSEC;
  WriteFontCharacter(BCD_H(Second));
  WriteFontCharacter(BCD_L(Second));
}

void DrawTextToLcd(DrawLcd_t *pData)
{
  gRow = pData->Row;
  gColumn = pData->Col;
  gBitColumnMask = pData->ColMask;
  SetFont(pData->Font);

  unsigned char i;
  for (i = 0; i < pData->Length; ++i) WriteFontCharacter(pData->pText[i]);
}

/* fonts can be up to 16 bits wide */
static void WriteFontCharacter(unsigned char Character)
{
  unsigned int MaskBit = BIT0; //src
  unsigned char CharacterRows = GetCurrentFontHeight();
  unsigned char CharacterWidth = GetCharacterWidth(Character);
  unsigned int bitmap[MAX_FONT_ROWS];

  GetCharacterBitmap(Character, (unsigned int*)&bitmap);

  /* do things bit by bit */
  unsigned char i;
  unsigned char row;

  for (i = 0; i < CharacterWidth && gColumn < BYTES_PER_LINE; i++)
  {
  	for (row = 0; row < CharacterRows; row++)
    {
      if ((MaskBit & bitmap[row])) pMyBuffer[gRow+row].Data[gColumn] |= gBitColumnMask;
//      else pMyBuffer[gRow+row].Data[gColumn] &= ~gBitColumnMask;
    }

    /* the shift direction seems backwards... */
    MaskBit <<= 1;
    
    gBitColumnMask <<= 1;
    if (gBitColumnMask == 0)
    {
      gBitColumnMask = BIT0;
      gColumn++;
    }
  }

  /* add spacing between characters */
  if (gColumn < BYTES_PER_LINE)
  {
    unsigned char FontSpacing = GetFontSpacing();
    for(i = 0; i < FontSpacing; i++)
    {
      gBitColumnMask = gBitColumnMask << 1;
      if (gBitColumnMask == 0)
      {
        gBitColumnMask = BIT0;
        gColumn++;
      }
    }
  }
}

static void WriteFontString(char const *pString)
{
  while (*pString && gColumn < BYTES_PER_LINE)
  {
    WriteFontCharacter(*pString++);
  }
}

/*! Display the startup image or Splash Screen */
void DrawSplashScreen(void)
{
  ClearLcd();
  // clear LCD buffer
  memset(pMyBuffer, 0, LCD_BUFFER_SIZE);

#if COUNTDOWN_TIMER
  DrawWwzSplashScreen();
#else
  /* draw metawatch logo */
  CopyColumnsIntoMyBuffer(pIconSplashLogo, 21, 13, 3, 6);
  CopyRowsIntoMyBuffer(pIconSplashMetaWatch, SPLASH_START_ROW, 12);
  CopyColumnsIntoMyBuffer(pIconSplashHandsFree, 58, 5, 2, 8);
  SendMyBufferToLcd(21, 42); // 58 + 5 - 21
#endif
}

void DrawBootloaderScreen(void)
{
#if BOOTLOADER
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
  CopyRowsIntoMyBuffer(pBootloader, BOOTLOADER_START_ROW, BOOTLOADER_ROWS);
  
  // Draw version
  SetFont(MetaWatch5);
  gRow = 61;
  gColumn = 4;
  gBitColumnMask = BIT3;
  WriteFontString("V ");
  WriteFontString((char const *)VERSION);

  unsigned char i = 0;
  char const *pBootVer = BootVersion;

  PrintF("BL_VER addr:%04X", pBootVer);
  for (; i < 8; ++i) {PrintH(pBootVer[i]); PrintC(SPACE);}
  i = 0;

  while (*pBootVer && i++ < 8)
  {
    if ((*pBootVer < '0' || *pBootVer > '9') && *pBootVer != '.') break;
    pBootVer ++;
  }
  
  if (i > 4) // at least x.x.x
  {
    gRow += 7;
    gColumn = 4;
    gBitColumnMask = BIT3;
    WriteFontString("B ");
    WriteFontString(BootVersion);
  }

  DrawLocalAddress(1, 80);
  
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
#endif
}

void FillMyBuffer(unsigned char StartRow, unsigned char RowNum, unsigned char Value)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    pMyBuffer[StartRow].Row = StartRow + FIRST_LCD_LINE_OFFSET;
    for (k = 0; k < BYTES_PER_LINE; ++k) pMyBuffer[StartRow].Data[k] = Value;
    StartRow ++;
  }
}

void SendMyBufferToLcd(unsigned char StartRow, unsigned char RowNum)
{
  /*
   * flip the bits before sending to LCD task because it will
   * dma this portion of the screen
  */
  if (!GetProperty(PROP_INVERT_DISPLAY))
  {
    signed char i = StartRow + RowNum - 1;

    for (; i >= StartRow; --i)
    {
      unsigned char k = 0;
      for (; k < BYTES_PER_LINE; ++k) pMyBuffer[i].Data[k] = ~(pMyBuffer[i].Data[k]);
    }
  }

  UpdateMyDisplay((unsigned char *)&pMyBuffer[StartRow], RowNum);
}

void CopyRowsIntoMyBuffer(unsigned char const *pImage, unsigned char StartRow, unsigned char RowNum)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    pMyBuffer[StartRow].Row = StartRow + FIRST_LCD_LINE_OFFSET;
    for (k = 0; k < BYTES_PER_LINE; ++k) pMyBuffer[StartRow].Data[k] = *pImage++;
    StartRow ++;
  }
}

#if __IAR_SYSTEMS_ICC__
void CopyRowsIntoMyBuffer_20(unsigned char const __data20 *pImage, unsigned char StartRow, unsigned char RowNum)
{
  unsigned char i, k;
  for (i = 0; i < RowNum; ++i)
  {
    pMyBuffer[StartRow].Row = StartRow + FIRST_LCD_LINE_OFFSET;
    for (k = 0; k < BYTES_PER_LINE; ++k) pMyBuffer[StartRow].Data[k] = *pImage++;
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
    pMyBuffer[DestRow].Row = DestRow + FIRST_LCD_LINE_OFFSET;
    
    while (DestColumn < BYTES_PER_LINE && ColumnCounter < ColNum)
    {
      pMyBuffer[DestRow].Data[DestColumn] = pImage[SourceIndex];

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
  return (void *)pMyBuffer;
}
