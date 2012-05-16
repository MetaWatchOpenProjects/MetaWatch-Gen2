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
/*! \file hal_clock_control.h
*
* Handle the use of the SMCLK because of the MSP430 errata.
*
* (PLL is not turned off when UART uses SMCLK in sleep mode).
* 
* The automatic control of the clock is what normally makes working with an
* MSP430 nice.
*/
/******************************************************************************/


#ifndef HAL_CLOCK_CONTROL_H
#define HAL_CLOCK_CONTROL_H

/* The MSP430 has a bug with the SMCLK and the UART.  SMCLK is not turned off
 * properly in LPM3 */
#define BT_UART_USER        BIT0
#define BT_DEBUG_UART_USER  BIT1
#define ADC_USER            BIT2
#define OLED_I2C_USER       BIT3
#define LCD_USER            BIT4
#define SERIAL_RAM_USER     BIT5
#define ACCELEROMETER_USER  BIT6
#define RESERVED_USER3      BIT7

/*! 
 * Enable a user of the SMCLK
 */
void EnableSmClkUser(unsigned char User);

/*! 
 * Disable a user of the SMCLK.  If there aren't any more users
 * then the clock will be disabled.
 */
void DisableSmClkUser(unsigned char User);

#endif /* HAL_CLOCK_CONTROL_H */
