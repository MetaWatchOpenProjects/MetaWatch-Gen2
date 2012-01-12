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
/*! \file hal_crc.h
*
*/
/******************************************************************************/

#ifndef HAL_CRC_H
#define HAL_CRC_H


/*! 
 * The CRC is CCITT-16.  It is intialized with 0xFFFF. The MSP430 uses bit 
 * reversed generation. 
 * Test vector: 
 * CRC({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39}) = 0x89F6
 */
#define CRC_INIT_VECTOR 0xFFFF

/*!
 * Initialize the CRC generator in the MSP430
 *
 * Use this before computing a new CRC.
 * 
 * This takes the CRC semaphore
 */
void halCrcInit(void);

void halCrcGiveMutex(void);

/*!
 * Add a byte to the current CRC operation.
 *
 * This function is used when building a message.
 */
void halCrcAddByte(unsigned char nextByte);

/*! 
 * Calculate the CRC of a pre-built message.
 *
 * \param pData A pointer to an array of characters
 * \param dataLength The length of the array
 *
 */
unsigned int halCrcCalculate(unsigned char* pData, unsigned int dataLength);

/*! 
 * Get the result from the CRC generator as a 16-bit value.
 */
unsigned int halCrcGetResult(void);

/*! 
 * Get the least signficant byte of the CRC.
 */
unsigned char halCrcGetResultLow(void);

/*! 
 * Get the most signficant byte of the CRC.
 */
unsigned char halCrcGetResultHigh(void);

#endif
