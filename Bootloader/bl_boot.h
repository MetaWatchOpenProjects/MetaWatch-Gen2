//==============================================================================
//  Copyright 2012 Meta Watch Ltd. - http://www.MetaWatch.org/
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

/*******************************************************************************
*
*! \file            bl_boot.h
*
*  Project:         MetaWatch Serial Bootloader
*
*  Description:     Serial bootloader header file
*
*******************************************************************************/

#include "intrinsics.h"
#include "msp430f5438a.h"


#define FALSE   (0)
#define TRUE    (1)

#define NO_MSG      (0)
#define MSG_RCVD    (1)
#define REBOOT  (2)

#define VALID_IMAGE     (1)
#define INVALID_IMAGE   (0)

#define BOOTLOADER_MODE (0)
#define WATCH_MODE      (1)

/* ACLK multiplier value */
#define ACLK_MULTIPLIER    ((unsigned int)512)

// macro to assign bootloader functions to bootloader segment (BL_CODE)
#define _BL_CODE_SEG    _Pragma("location=\"BL_CODE\"")

// macro to assign bootloader data to bootloader data (BL_DATA16)
// Be aware that *ALL* bootloader variables are uninitialized and MUST be
// initialized programmatically at run time!
#define _BL_DATA_SEG    _Pragma("location=\"BL_DATA16\"") __no_init

// macro to assign bootloader interrupt VECTOR_OFFSETs to RAM (BLINTVEC)
#define _BL_INTVEC      _Pragma("location=\"BL_INTVEC\"") __no_init

#define FLASH_SEGMENT_SIZE      (512)       // size of FLASH memory segment
#define FLASH_SEGMENT_ADDR_MASK (0x7FE00)
#define FLASH_BLOCK1_ADDR_MIN   (0x07000)
#define FLASH_BLOCK1_ADDR_MAX   (0x0F1FF)
#define FLASH_BLOCK2_ADDR_MIN   (0x0FA00)
#define FLASH_BLOCK2_ADDR_MAX   (0x0FDFF)
#define FLASH_BLOCK3_ADDR_MIN   (0x10000)
#define FLASH_BLOCK3_ADDR_MAX   (0x45BFF)



typedef struct _dwordb_s
{
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;
} dwordb_s_t;

typedef struct _dword_s
{
    unsigned int lw;
    unsigned int hw;
} dword_s_t;

typedef union _dword_u
{
    unsigned long   dw;
    dword_s_t       w;
    dwordb_s_t      b;
} dword_u_t;

typedef struct _word_s
{
    unsigned char lb;
    unsigned char hb;
} word_s_t;

typedef union _word_u
{
    unsigned int w;
    word_s_t     b;
} word_u_t;

typedef struct _mem_block
{
  unsigned long BaseAddress;    // starting address of data in Buffer
  unsigned int  Signature;      // CRC-16 of data in Buffer
  unsigned char Buf[FLASH_SEGMENT_SIZE];  // RAM buffer to store one FLASH segment for programming
} mem_block_t;

#define BUILDDATE_LEN   (12)
#define VERSION_LEN     (8)

typedef struct __image_data
{
    const unsigned char __data20 * const pImageEnd;
    const unsigned char BuildDate[BUILDDATE_LEN];
    const unsigned char ToolSet;
    const unsigned char ApplVersion[VERSION_LEN];
} image_data_t;


//*****************************************************************************
//
// Interrupt Vector Table offset values
//
//*****************************************************************************
#define RTC_VECTOR_OFFSET          (41) /* 0xFFD2 RTC */
#define PORT2_VECTOR_OFFSET        (42) /* 0xFFD4 Port 2 */
#define USCI_B3_VECTOR_OFFSET      (43) /* 0xFFD6 USCI B3 Receive/Transmit */
#define USCI_A3_VECTOR_OFFSET      (44) /* 0xFFD8 USCI A3 Receive/Transmit */
#define USCI_B1_VECTOR_OFFSET      (45) /* 0xFFDA USCI B1 Receive/Transmit */
#define USCI_A1_VECTOR_OFFSET      (46) /* 0xFFDC USCI A1 Receive/Transmit */
#define PORT1_VECTOR_OFFSET        (47) /* 0xFFDE Port 1 */
#define TIMER1_A1_VECTOR_OFFSET    (48) /* 0xFFE0 Timer1_A3 CC1-2, TA1 */
#define TIMER1_A0_VECTOR_OFFSET    (49) /* 0xFFE2 Timer1_A3 CC0 */
#define DMA_VECTOR_OFFSET          (50) /* 0xFFE4 DMA */
#define USCI_B2_VECTOR_OFFSET      (51) /* 0xFFE6 USCI B2 Receive/Transmit */
#define USCI_A2_VECTOR_OFFSET      (52) /* 0xFFE8 USCI A2 Receive/Transmit */
#define TIMER0_A1_VECTOR_OFFSET    (53) /* 0xFFEA Timer0_A5 CC1-4, TA */
#define TIMER0_A0_VECTOR_OFFSET    (54) /* 0xFFEC Timer0_A5 CC0 */
#define ADC12_VECTOR_OFFSET        (55) /* 0xFFEE ADC */
#define USCI_B0_VECTOR_OFFSET      (56) /* 0xFFF0 USCI B0 Receive/Transmit */
#define USCI_A0_VECTOR_OFFSET      (57) /* 0xFFF2 USCI A0 Receive/Transmit */
#define WDT_VECTOR_OFFSET          (58) /* 0xFFF4 Watchdog Timer */
#define TIMER0_B1_VECTOR_OFFSET    (59) /* 0xFFF6 Timer0_B7 CC1-6, TB */
#define TIMER0_B0_VECTOR_OFFSET    (60) /* 0xFFF8 Timer0_B7 CC0 */
#define UNMI_VECTOR_OFFSET         (61) /* 0xFFFA User Non-maskable */
#define SYSNMI_VECTOR_OFFSET       (62) /* 0xFFFC System Non-maskable */
#define RESET_VECTOR_OFFSET        (63) /* 0xFFFE Reset [Highest Priority] */

#define N_VECTORS                  (64) /* number of interrupt vectors in MSP430F5438A */


unsigned char ValidateImage(void);
void bl_EraseSegment(unsigned char __data20 *);
unsigned char bl_VerifySegmentErased(unsigned char __data20 *);
void bl_WriteSegment(unsigned char __data20 *, unsigned char *);
unsigned char bl_VerifySegmentData(unsigned char __data20 *, unsigned char *);

