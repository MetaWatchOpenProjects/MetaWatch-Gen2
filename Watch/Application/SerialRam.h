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
 * All interfacing to the serial ram module is done using the message queues.
 *
 *
 * The serial ram is used to store images for the LCD.  When the phone draws 
 * images they are sent directly to Serial RAM.  When an image is ready to be 
 * displayed it is read from the serial RAM and sent to the LCD Task.
 *
 */
/******************************************************************************/

#ifndef SERIAL_RAM_H
#define SERIAL_RAM_H

/*! Initialize the serial ram task and the Queue
 *
 * The main sram initialization is done when the task starts and calls
 * SerialRamInit.  This sets up the peripheral and clears the serial RAM to
 * zero.
 */
void InitializeSerialRamTask(void);


#endif /* SERIAL_RAM_H */