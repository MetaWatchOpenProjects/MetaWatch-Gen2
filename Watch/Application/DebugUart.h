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


/*! Initialize the uart peripheral and create mutex */
void InitDebugUart(void);

/*! Print a  character */
void PrintCharacter(char Character);

/*! Print a string */
void PrintString(tString * const pString);

/*! Print two strings */
void PrintString2(tString * const pString1,
                  tString * const pString2);

/*! Print three strings */
void PrintString3(tString * const pString1,
                  tString * const pString2,
                  tString * const pString3);

/*! Print a 16 bit value in decimal */
void PrintDecimal(unsigned int Value);

/*! Print a 16 bit value and a newline*/
void PrintDecimalAndNewline(unsigned int Value);

/*! Print a string and a 16 bit value */
void PrintStringAndDecimal(tString * const pString,unsigned int Value);

/*! Print a string, space and 16 bit value */
void PrintStringAndSpaceAndDecimal(tString * const pString,unsigned int Value);

/*! Print a string and an 16 bit value represented in hexadecimal */
void PrintStringAndHex(tString * const pString,unsigned int Value);

/*! Print a string and an 8 bit value represented in hexadecimal */
void PrintStringAndHexByte(tString * const pString,unsigned char Value);


/*! Print a string 16 bit value, and another string and 16 bit value */
void PrintStringAndTwoDecimals(tString * const pString1,
                               unsigned int Value1,
                               tString * const pString2,
                               unsigned int Value2);

/*! Print a string 16 bit value three times */
void PrintStringAndThreeDecimals(tString * const pString1,
                                 unsigned int Value1,
                                 tString * const pString2,
                                 unsigned int Value2,
                                 tString * const pString3,
                                 unsigned int Value3);

/*! Print a string and two 16 bit values with spaces inbetween */
void PrintStringSpaceAndTwoDecimals(tString * const pString1,
                                    unsigned int Value1,
                                    unsigned int Value2);

/*! Print a string and three 16 bit values with spaces inbetween */
void PrintStringSpaceAndThreeDecimals(tString * const pString1,
                                      unsigned int Value1,
                                      unsigned int Value2,
                                      unsigned int Value3);

/*! Print a signed number and a newline */
void PrintSignedDecimalAndNewline(signed int Value);

/*! Disable the SMCLK request of the UART after a transmission in finished.
 * This is called in the real time clock interrupt routine.
 *
 * MSP4305438A has an errata with the UART.
*/
void DisableUartSmClkIsr(void);


/*! Convert a 16 bit value into a string */
void ToDecimalString(unsigned int Value, tString * pString);

/*! Convert a 16 bit value into a hexadecimal string */
void IntToHexString(unsigned int Value, tString * pString);

/*! Convert a 8 bit value into a hexadecimal string */
void ByteToHexString(unsigned char Value, tString * pString);

/*! Print the RTCPS */
void PrintTimeStamp(void);

#endif
