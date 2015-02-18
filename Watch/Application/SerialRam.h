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

#ifndef SERIAL_RAM_H
#define SERIAL_RAM_H

#define SRAM_HEADER_LEN           3
#define SPI_READ                (0x03)
#define SPI_WRITE               (0x02)
#define DMA_FILL                  1
#define DMA_COPY                  0

/* defines for write buffer command */
#define MSG_OPT_WRTBUF_1_LINE      (0x10)
#define MSG_OPT_WRTBUF_MULTILINE   (0x40)

typedef struct
{
  unsigned char Reserved[3];
  tLcdLine Line;
} LcdReadBuffer_t;
#define LCD_READ_BUFFER_SIZE (sizeof(LcdReadBuffer_t))

unsigned char CurrentIdleScreen(void);

/*! Handle the write buffer message */
void WriteBufferHandler(tMessage *pMsg);

/*! Handle the update display message */
void UpdateDisplayHandler(tMessage *pMsg);

/*! Handle the load template message */
void LoadTemplateHandler(tMessage *pMsg);

void Write(unsigned long const pData, unsigned int Length, unsigned char Op);
void Read(unsigned char *pWriteData, unsigned char *pReadData, unsigned int Length);

void DrawBitmapToSram(Draw_t *Info, unsigned char WidthInBytes, unsigned char const *pBitmap, unsigned char Mode);
void DrawTemplateToSram(Draw_t *Info, unsigned char Mode);
void DrawStatusBar(void);
void ClearSram(unsigned char Mode);
void LoadBuffer(unsigned char QuadIndex, unsigned char const *pTemp);

/*! This sets up the peripheral in the MSP430, the external serial ram,
 * and clears the serial RAM memory to zero. */
void InitSerialRam(void);


#endif /* SERIAL_RAM_H */
