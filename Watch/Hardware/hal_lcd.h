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
/*! \file hal_lcd.h
 * parameters for the Sharp LS013B4DN02 LCD
 */
/******************************************************************************/

#ifndef HAL_LCD_H
#define HAL_LCD_H

/*! number of bytes in an lcd column 12 (12*8=96 bits) */
#define NUM_LCD_COL_BYTES  ( 12 )


/*! the first LCD line is one NOT zero */
#define FIRST_LCD_LINE_OFFSET  1

#define LCD_STATIC_CMD      0x00
#define LCD_WRITE_CMD       0x01     
#define LCD_CLEAR_CMD       0x04     
#define LCD_CLEAR_CMD_SIZE  0x03     
#define LCD_STATIC_CMD_SIZE 0x03

/*! This structure is organized so that the entire
 * display can be sent by the dma engine
 *
 * \param Row is the lcd row
 * \param Data is the column data
 * \param Dummy is a dummy byte that must be written at the end of the row
 */
typedef struct
{
  unsigned char Row;
  unsigned char Data[NUM_LCD_COL_BYTES];
  unsigned char Dummy;
  
} tLcdLine;


#endif /* HAL_LCD_H */
