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
/*! \file SerialRam.h
 *
 * The serial ram is used to store images for the LCD.  When the phone draws 
 * images they are sent directly to Serial RAM.  When an image is ready to be 
 * displayed it is read from the serial RAM and sent to the LCD driver.
 *
 */
/******************************************************************************/

#ifndef SERIAL_RAM_H
#define SERIAL_RAM_H

#define BYTES_PER_QUAD          (288) //48x6
#define BYTES_PER_QUAD_LINE     (6)
#define HALF_SCREEN_ROWS        (LCD_ROW_NUM >> 1)
#define QUAD_ROW_NUM            (HALF_SCREEN_ROWS)
#define HALF_SCREEN_COLS        (LCD_COL_NUM >> 1)
#define QUAD_NUM                (4)
#define SRAM_HEADER_LEN         (3)
#define LAYOUT_NUM              (4)
#define LAYOUT_MASK             (0x0C)
#define LAYOUT_SHFT             (2)

/* Layout type */
#define LAYOUT_QUAD_SCREEN      (0)
#define LAYOUT_HORI_SCREEN      (1)
#define LAYOUT_VERT_SCREEN      (2)
#define LAYOUT_FULL_SCREEN      (3)

#define LAYOUT_MODE_SCREEN      (4)

// two extra page for non-4Q idle screen
#define IDLE_MODE_MENU          (4)
#define IDLE_MODE_SERVICE       (5)
#define IDLE_MODE_PAGE_MASK     (0x7)

/* defines for write buffer command */
#define MSG_OPT_WRTBUF_1_LINE      (0x10)
#define MSG_OPT_WRTBUF_MULTILINE   (0x40)

#define FACE_ID(_x) ((_x & 0xF0) >> 4)

typedef struct
{
  unsigned char QuadNum;
  unsigned char Step;
} Layout_t;

extern const Layout_t Layout[];

unsigned char CurrentIdleScreen(void);

void UpdateClockWidgets(void);

/*! This sets up the peripheral in the MSP430, the external serial ram,
 * and clears the serial RAM memory to zero.
 */
void SerialRamInit(void);

/*! Handle the update display message */
void UpdateDisplayHandler(tMessage *pMsg);

/*! Handle the load template message */
void LoadTemplateHandler(tMessage *pMsg);

/*! Handle the write buffer message */
void WriteBufferHandler(tMessage *pMsg);

void SetWidgetList(tMessage *pMsg);
void WriteClockWidget(unsigned char FaceId, unsigned char *pBuffer);
#endif /* SERIAL_RAM_H */
