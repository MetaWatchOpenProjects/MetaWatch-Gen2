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
/*! \file LcdDisplay.h
 *
 * This task draws to a local buffer which is then sent to the LCD by the LCD
 * task.  This is how the time is drawn.
 *
 * Menu and button control logic determines what Idle sub-mode the watch is in.
 *
 * When the watch draws into buffers those messages go to the SerialRam task
 * because this task is more of a control task.  This caused some duplication
 * of messages to preserve draw order.
 */
/******************************************************************************/

#ifndef LCD_BUFFER_H
#define LCD_BUFFER_H

#define LCD_ROW_NUM  (96)
#define LCD_COL_NUM  (96)

#define WATCH_DRAW_SCREEN_ROW_NUM     (30)

//Set Default Positions
#define DO_NOT_DISPLAY_ROW 0
#define DO_NOT_DISPLAY_COL 255

#define DEFAULT_HOURS_ROW 6
#define DEFAULT_HOURS_COL 0
#define DEFAULT_HOURS_COL_BIT BIT4

#define DEFAULT_TIME_SEPARATOR_ROW 6
#define DEFAULT_TIME_SEPARATOR_COL 3
#define DEFAULT_TIME_SEPARATOR_COL_BIT BIT4

#define DEFAULT_MINS_ROW 6
#define DEFAULT_MINS_COL 4
#define DEFAULT_MINS_COL_BIT BIT1

#define DEFAULT_SECS_ROW 6
#define DEFAULT_SECS_COL 7
#define DEFAULT_SECS_COL_BIT BIT1

#define DEFAULT_AM_PM_ROW 3
#define DEFAULT_AM_PM_COL 8
#define DEFAULT_AM_PM_COL_BIT BIT1

#define DEFAULT_DOW_24HR_ROW 3
#define DEFAULT_DOW_12HR_ROW 13
#define DEFAULT_DOW_COL 8
#define DEFAULT_DOW_COL_BIT BIT1

#define DEFAULT_DATE_YEAR_ROW 13
#define DEFAULT_DATE_YEAR_COL 8
#define DEFAULT_DATE_YEAR_COL_BIT BIT1

#define DEFAULT_DATE_FIRST_ROW 22   //month or day can be displayed in any order
#define DEFAULT_DATE_FIRST_COL 8
#define DEFAULT_DATE_FIRST_COL_BIT BIT1

#define DEFAULT_DATE_SECOND_ROW 22  //month or day can be displayed in any order
#define DEFAULT_DATE_SECOND_COL 10
#define DEFAULT_DATE_SECOND_COL_BIT BIT0

#define DEFAULT_DATE_SEPARATOR_ROW 22
#define DEFAULT_DATE_SEPARATOR_COL 9
#define DEFAULT_DATE_SEPARATOR_COL_BIT BIT4

//Set Default Fonts
#define DEFAULT_HOURS_FONT Time
#define DEFAULT_TIME_SEPARATOR_FONT Time
#define DEFAULT_MINS_FONT Time
#define DEFAULT_SECS_FONT Time

#define DEFAULT_AM_PM_FONT MetaWatch7

#define DEFAULT_DOW_FONT MetaWatch7
#define DEFAULT_DATE_YEAR_FONT MetaWatch7
#define DEFAULT_DATE_MONTH_FONT MetaWatch7
#define DEFAULT_DATE_DAY_FONT MetaWatch7
#define DEFAULT_DATE_SEPARATOR_FONT MetaWatch7

/*! Languages */ 
#define LANG_EN (0)
#define LANG_FI (1)
#define LANG_DE (2)

extern const char DaysOfTheWeek[][7][4];
extern const char MonthsOfYear[][13][7];

typedef struct
{
  unsigned char Col;
  unsigned char ColMask;
  unsigned char Row;
  etFontType Font;
  char *pText;
  unsigned char Length;
} DrawLcd_t;

#define DRAW_OPT_BITWISE_OR           (0)
#define DRAW_OPT_BITWISE_NOT          (1)
#define DRAW_OPT_BITWISE_SET          (2)
#define DRAW_OPT_BITWISE_DST_NOT      (3)
#define DRAW_OPT_BITWISE_MASK         (0x0F)

void HourToString(char *Hour);
void BitOp(unsigned char *pByte, unsigned char Bit, unsigned int Set, unsigned char Op);
void DrawTextToLcd(DrawLcd_t *pData);

void DrawSplashScreen(void);
void DrawDateTime(void);
void DrawConnectionScreen(void);
void DrawMenu(eIdleModePage Page);
void DrawWatchStatusScreen(void);
void DrawBootloaderScreen(void);
void DrawCallScreen(char *pCallerId, char *pCallerName);
void CopyRowsIntoMyBuffer(unsigned char const *pImage, unsigned char StartRow, unsigned char RowNum);
void SendMyBufferToLcd(unsigned char StartRow, unsigned char RowNum);
void FillMyBuffer(unsigned char StartRow, unsigned char RowNum, unsigned char Value);

const unsigned char *GetBatteryIcon(unsigned char Id);
void *GetDrawBuffer(void);

#if __IAR_SYSTEMS_ICC__
void CopyRowsIntoMyBuffer_20(unsigned char const __data20 *pImage, unsigned char StartRow, unsigned char RowNum);
#endif

#endif /* LCD_BUFFER_H */
