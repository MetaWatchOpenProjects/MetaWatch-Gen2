//==============================================================================
//  Copyright 2011 - 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#define BUILD_LENGTH       6

#define PAGE_TYPE_IDLE     0
#define PAGE_TYPE_INFO     1
#define PAGE_TYPE_MENU     2

typedef enum
{
  ConnectedPage,
  DisconnectedPage,
  InitPage,
  StatusPage,
  CountdownPage,
  Menu1Page,
  Menu2Page,
  Menu3Page
} eIdleModePage;

extern unsigned char CurrentMode;
extern unsigned char PageType;
extern unsigned char OnCall;

/*! Create task and queue for display task. Call from main or another task. */
void CreateDisplayTask(void);
void ChangeMode(unsigned char Option);
void ResetModeTimer(void);

/*! Called from RTC one second interrupt
 * \return 1 if lpm should be exited, 0 otherwise
 */
unsigned char LcdRtcUpdateHandlerIsr(void);
void EnableRtcUpdate(unsigned char Enable);

void GetBuildNumber(unsigned char *pBuild);
unsigned char BackLightOn(void);
unsigned char CurrentIdlePage(void);
void EnterBootloader(void);

#endif /* LCD_DISPLAY_H */
