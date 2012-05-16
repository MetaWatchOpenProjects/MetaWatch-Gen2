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
/*! \file OledDisplay.h
*
*/
/******************************************************************************/

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#define NUMBER_OF_ROWS      ( 2 )
#define ROW_SIZE            ( 80 )
#define NUMBER_OF_COLUMNS   ( ROW_SIZE )
#define DISPLAY_BUFFER_SIZE ( NUMBER_OF_ROWS * NUMBER_OF_COLUMNS )
#define COLUMN_HEIGHT       ( 8 )

/*! OLED image buffer structure
 * 
 * \param Mode is used to associate buffer with Idle, Application, or scroll mode
 * \param Valid is used for housekeeping
 * \param OledPosition
 * \param pPixelData an array of bytes that hold the OLED image
 */
typedef struct
{
  unsigned char Mode;
  unsigned char Valid;
  etOledPosition OledPosition;
  unsigned char pPixelData[DISPLAY_BUFFER_SIZE];

} tImageBuffer;

#define DEFAULT_IDLE_DISPLAY_TIMEOUT         ( ONE_SECOND*7 )
#define DEFAULT_APPLICATION_DISPLAY_TIMEOUT  ( ONE_SECOND*5 )
#define DEFAULT_NOTIFICATION_DISPLAY_TIMEOUT ( ONE_SECOND*5 )

/*! Setup queue, task, and non-volatile items */
void InitializeDisplayTask(void);

/*! Initialize flash/ram values for oled display timeouts */
void InitializeDisplayTimeouts(void);

void InitializeContrastValues(void);

#endif /* OLED_DISPLAY_H */
