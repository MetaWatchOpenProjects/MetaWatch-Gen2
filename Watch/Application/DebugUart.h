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
/*! \file DebugUart.h
 *
 * The debug uart is used to send messages to a terminal. The settings
 * are 115200, 8 bits, no parity, 1 stop bit
 *
 * This only is useful on the development board and modified watches.
 */
/******************************************************************************/

#ifndef DEBUG_UART_H
#define DEBUG_UART_H

extern const char OK[];
extern const char NOK[];
extern const char SPACE;

void EnableDebugUart(unsigned char Enable);
void EnableTimeStamp(void);

/******************************************************************************/

/*! Print a  character */
void PrintC(char Char);
void PrintR(void);
void PrintH(unsigned char Value);
void PrintS(const char *pString);
void PrintF(const char *pFormat, ...);

#endif
