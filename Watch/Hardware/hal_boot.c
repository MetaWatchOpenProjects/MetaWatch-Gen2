
//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#include "msp430.h"
#include "hal_boot.h"
#include "Log.h"
#include "hal_rtc.h"
#include "DebugUart.h"

#define BUILD_SIZE            3

/* Things shared between bootloader and main application */
/* This address is in an unused location reserved for the alternate interrupt
 * table */
#define SIGNATURE_ADDR        (0x5B80)

#if __IAR_SYSTEMS_ICC__
__no_init __root unsigned char niResetType @ RESET_TYPE_ADDR;
__no_init __root unsigned char niResetCode @ RESET_CODE_ADDR;
__no_init __root unsigned char niResetValue @ RESET_VALUE_ADDR;
__no_init __root char niBuild[BUILD_SIZE] @ BUILD_NUMBER_ADDR;
__no_init __root unsigned long long niSignature @ SIGNATURE_ADDR;
#else
extern unsigned char niResetType;
extern unsigned char niResetCode;
extern unsigned char niResetValue;
extern char niBuild[BUILD_SIZE];
extern unsigned long long niSignature;
#endif

void SetBootloaderSignature(void)
{
  niSignature = BOOTLOADER_SIGNATURE;
}

void ClearBootloaderSignature(void)
{
  niSignature = 0;
}

unsigned long long GetBootloaderSignature(void)
{
  return niSignature;
}

extern char const BUILD[];

void CheckResetType(void)
{
  unsigned char i = 0;
  
  for (; i < BUILD_SIZE; ++i) if (niBuild[i] != BUILD[i]) break;
  
  if (i < BUILD_SIZE)
  { // it's a flash reset
    for (i = 0; i < BUILD_SIZE; ++i) niBuild[i] = BUILD[i];
    niResetType = MASTER_RESET; //FLASH_RESET;
  }
}

void SoftwareReset(unsigned char Code, unsigned char Value)
{
  niResetCode = Code;
  niResetValue = Value;
  if (Code == RESET_BUTTON_PRESS || Code == RESET_BOOTLOADER) niResetType = Value;

  PrintF("Reset Code:%u Type:%02X Val:%u", niResetCode, niResetType, niResetValue);

  BackupRtc();
//  WDTCTL = ~WDTPW;
  PMMCTL0 = PMMPW | PMMSWBOR;
}
