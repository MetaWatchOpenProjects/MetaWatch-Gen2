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
/*! \file LcdDriver.h
 *
 * DMA is used to transfer data to the LCD.
 */
/******************************************************************************/

#ifndef LCD_TASK_H
#define LCD_TASK_H

/*! number of bytes in a lcd line (12*8=96 bits) */
#define BYTES_PER_LINE         12

/*! This structure is organized so that the entire
 * display can be sent by the dma engine
 *
 * \param Row is the number of lcd row to draw
 * \param Data is the line data
 * \param Trailer is a dummy byte that must be written at the end of the row
 */
typedef struct
{
  unsigned char Row;
  unsigned char Data[BYTES_PER_LINE];
  unsigned char Trailer;
} tLcdLine;

/*! Initialize spi peripheral and LCD pins */
void LcdPeripheralInit(void);

void WriteToLcd(tLcdLine *pData, unsigned char LineNum);

void ClearLcd(void);

/*! Callback from the DMA interrupt service routing that lets LCD task know 
 * that the dma has finished
 */
void LcdDmaIsr(void);

#endif /* LCD_TASK_H */
