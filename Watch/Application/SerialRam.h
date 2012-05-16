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

/*! This sets up the peripheral in the MSP430, the external serial ram, 
 * and clears the serial RAM memory to zero.
 */
void SerialRamInit(void);

/*! Handle the update display message */
void UpdateDisplayHandler(tMessage* pMsg);

/*! Handle the load template message */
void LoadTemplateHandler(tMessage* pMsg);

/*! Handle the write buffer message */
void WriteBufferHandler(tMessage* pMsg);


void RamTestHandler(tMessage* pMsg);

#endif /* SERIAL_RAM_H */
