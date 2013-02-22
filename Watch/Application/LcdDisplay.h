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

#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#define LCD_ROW_NUM  96
#define LCD_COL_NUM  96

#define TEMPLATE_1 2  //starts at 2 because 0,1 are clear and fill
#define TEMPLATE_2 3
#define TEMPLATE_3 4
#define TEMPLATE_4 5

#define NORMAL_DISPLAY ( 0 )
#define INVERT_DISPLAY ( 1 )

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
#define DEFAULT_HOURS_FONT MetaWatchTime
#define DEFAULT_TIME_SEPARATOR_FONT MetaWatchTime
#define DEFAULT_MINS_FONT MetaWatchTime
#define DEFAULT_SECS_FONT MetaWatchTime

#define DEFAULT_AM_PM_FONT MetaWatch7

#define DEFAULT_DOW_FONT MetaWatch7
#define DEFAULT_DATE_YEAR_FONT MetaWatch7
#define DEFAULT_DATE_MONTH_FONT MetaWatch7
#define DEFAULT_DATE_DAY_FONT MetaWatch7
#define DEFAULT_DATE_SEPARATOR_FONT MetaWatch7

#define WATCH_DRAW_TOP                (0)
#define IDLE_BUFFER_CONFIG_MASK       (BIT0)
#define WATCH_DRAW_SCREEN_ROW_NUM     (30)

#define PAGE_TYPE_IDLE     (0)
#define PAGE_TYPE_INFO     (1)
#define PAGE_TYPE_MENU     (2)

extern unsigned char CurrentMode;
extern unsigned char PageType;

typedef enum
{
  ConnectedPage,
  DisconnectedPage,
  InitPage,
  StatusPage,
  CallPage,
  Menu1Page,
  Menu2Page,
  Menu3Page
} eIdleModePage;

/*! Create task and queue for display task. Call from main or another task. */
void CreateDisplayTask(void);

void Init(void);

void ResetModeTimer(void);

/*! Called from RTC one second interrupt
 *
 * \return 1 if lpm should be exited, 0 otherwise
 */
unsigned char LcdRtcUpdateHandlerIsr(void);

unsigned char BackLightOn(void);

unsigned char CurrentIdlePage(void);

void EnterBootloader(void);

void WhoAmI(void);

#endif /* LCD_DISPLAY_H */
