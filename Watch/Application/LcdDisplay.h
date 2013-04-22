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

#define TEMPLATE_1 2  //starts at 2 because 0,1 are clear and fill
#define TEMPLATE_2 3
#define TEMPLATE_3 4
#define TEMPLATE_4 5

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
  CountdownPage,
  Menu1Page,
  Menu2Page,
  Menu3Page
} eIdleModePage;

/*! Create task and queue for display task. Call from main or another task. */
void CreateDisplayTask(void);
void Init(void);
void ResetModeTimer(void);

/*! Called from RTC one second interrupt
 * \return 1 if lpm should be exited, 0 otherwise
 */
unsigned char LcdRtcUpdateHandlerIsr(void);
void EnableRtcUpdate(unsigned char Enable);

unsigned char BackLightOn(void);
unsigned char CurrentIdlePage(void);
unsigned char LinkAlarmEnabled(void);
void EnterBootloader(void);
void WhoAmI(void);

#endif /* LCD_DISPLAY_H */
