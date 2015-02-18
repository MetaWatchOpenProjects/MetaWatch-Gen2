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

#ifndef WIDGET_H
#define WIDGET_H

#define BYTES_PER_QUAD          288 //48x6
#define BYTES_PER_QUAD_LINE     6
#define HALF_SCREEN_ROWS        (LCD_ROW_NUM >> 1)
#define QUAD_ROW_NUM            (HALF_SCREEN_ROWS)
#define HALF_SCREEN_COLS        (LCD_COL_NUM >> 1)
#define QUAD_NUM                4
#define SRAM_HEADER_LEN         3
#define LAYOUT_NUM              4
#define LAYOUT_MASK             (0x0C)
#define LAYOUT_SHFT             2

/* Layout type */
#define LAYOUT_QUAD_SCREEN      0
#define LAYOUT_HORI_SCREEN      1
#define LAYOUT_VERT_SCREEN      2
#define LAYOUT_FULL_SCREEN      3
#define LAYOUT_MODE_SCREEN      4

#define CLOCK_WIDGET_BIT        (BIT7)
#define INVALID_ID              (0xFF)

#define FACE_ID(_x)             ((_x & 0xF0) >> 4)
#define CLOCK_ID(_x)            (_x & 0x0F)

void SetWidgetList(tMessage *pMsg);
void ClearWidgetList(void);
unsigned char UpdateClockWidget(void);

void WriteWidgetBuffer(tMessage *pMsg);
void DrawWidgetToLcd(unsigned char Page);
void DrawStatusBarToWidget(void);
void InitWidget(void);

unsigned int BufferSize(unsigned char Id);
unsigned char CreateDrawBuffer(unsigned char Id);
void DrawBitmapToIdle(Draw_t *Info, unsigned char WidthInBytes, unsigned char const *pBitmap);
void DrawWidgetToSram(unsigned char Id);
void DrawClockToSram(unsigned char Id);
void DrawTemplateToIdle(Draw_t *Info);

#endif /* WIDGET_H */
