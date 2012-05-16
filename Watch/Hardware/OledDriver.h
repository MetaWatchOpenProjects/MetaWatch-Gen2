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
/*! \file OledDriver.h
 *
 * The configuraton of the OLED is not trivial and is handled in this layer.
 */
/******************************************************************************/

#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#define OLED_1_DEV_ADRS 0x3C
#define OLED_2_DEV_ADRS 0x3D

// This is the first visible colummn on the display when starting with a zero index
#define OLED_COLUMN_OFFSET 26

#define NUM_OLED_COL_BYTES           2   // two bytes per column
#define NUM_OLED_PAGE_BYTES        132   // number of bytes per page
#define NUM_OLED_DISPLAY_COLUMNS    80   // number of visible columns on the display
#define OLED_FIRST_PAGE_INDEX        6   // there are 8 pages, we use 6 and 7

#define OLED_CMD_SEGMENT_ORDER_NORMAL  ( 0xA0 )
#define OLED_CMD_SEGMENT_ORDER_REVERSE ( 0xA1 )

#define OLED_CMD_DISPLAY_OFF_SLEEP ( 0xae )
#define OLED_CMD_DISPLAY_ON_NORMAL ( 0xa4 )
#define OLED_CMD_DISPLAY_ON_ALL    ( 0xa5 )

#define OLED_CMD_DISPLAY_ON_NORMAL_MODE ( 0xaf )

#define OLED_CMD_CONTRAST ( 0x81 )

/*! Enumerate top and bottom oled positions */
typedef enum 
{
  TopOled,
  BottomOled
  
} etOledPosition;

/*! perform the required power up sequence for the OLEDs */
void OledPowerUpSequence(void);

/* Disable the IO and 10V supplies to the OLEDs */
void OledPowerDown(void);

/*! Setup the OLED for use
 * \param OledPosition - top or bottom
 * \param Contrast
 */
void InitializeOledDisplayController(etOledPosition OledPosition,
                                     unsigned char Contrast);

/*! Set the I2C address used by the MSP430 peripheral's master operations 
 *
 * \param OledPosition is TopOled or BottomOled
 */
void SetOledDeviceAddress(etOledPosition OledPosition);

/*! 
 * \param cmd is the One byte commmand that should be written to OLED
 */
void WriteOneByteOledCommand(unsigned char cmd);

/*! Write a multiple byte data packet (screen information) using the 
 * data continuation command 
 *
 * \param pData is a pointer to an array
 * \param Length is the number of bytes to write
 */
void WriteOledData(unsigned char* pData,unsigned char Length);

/*! Set the row in the oled that will be filled with display information
 *
 * \param RowNumber is the top or bottom row
 * \param OledPosition is TopOled or BottomOled
 *
 */
void SetRowInOled(unsigned char RowNumber,etOledPosition OledPosition);

#endif /* OLED_DRIVER_H */
