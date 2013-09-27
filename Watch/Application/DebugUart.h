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

extern char const OK;
extern char const NOK;
extern char const PLUS;
extern char const RCV;
extern char const SND;
extern char const SPACE;
extern char const ZERO;
extern char const COLON;
extern char const COMMA;
extern char const HASH;
extern char const DOLLAR;
extern char const TILDE;
extern char const DOT;
extern char const PERCENT;

void EnableDebugUart(unsigned char Enable);
void EnableTimeStamp(void);

/******************************************************************************/

/*! Print a  character */
void PrintC(char Char);

/*! Print CR */
void PrintR(void);

/*! Print a HEX */
void PrintH(unsigned char Value);

/*! Print String with CR */
void PrintS(char const *pString);

/*! Print String */
void PrintW(const char *pString);

/*! Print Formated string without CR */
void PrintE(char const *pFormat, ...);

/*! Print Formated string with CR */
void PrintF(char const *pFormat, ...);

/*! Print HEX sequence with CR */
void PrintQ(unsigned char const *pHex, unsigned char const Len);

/*! Check how much statck and message queue a task is currently using
 *
 * \param TaskHandle points to the task
 * \param Index is index of the message queue
 */
void CheckStackAndQueueUsage(unsigned char Index);

#endif
