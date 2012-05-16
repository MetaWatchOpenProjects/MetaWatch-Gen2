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
/*! \file hal_oled.h
 *
 * Abstraction for initializing the OLED and writing data.
 */
/******************************************************************************/

#ifndef HAL_OLED_H
#define HAL_OLED_H

/******************************************************************************/

#define DISPLAY_ADDRESS_TOP    ( 0x3c )
#define DISPLAY_ADDRESS_BOTTOM ( 0x3d )

/******************************************************************************/


/*! Initialize the peripheral in the MSP430 for controlling the oled */
void InitOledI2cPeripheral(void);


/*! Write command and data to the oled
 *
 * \param Command is the first byte sent to OLED
 * \param pData is a pointer to an array 
 * \param Length is the amount of bytes from the array that should be written
 * \note must be called from a task 
 */
void OledWrite(unsigned char Command,unsigned char* pData,unsigned char Length);

#endif /* HAL_OLED_H */
