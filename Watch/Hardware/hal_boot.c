
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
#include "hal_rtc.h"

#if __IAR_SYSTEMS_ICC__
__no_init __root unsigned long long Signature @SIGNATURE_ADDR;
#else
extern unsigned long long Signature;
#endif

void SetBootloaderSignature(void)
{
  Signature = BOOTLOADER_SIGNATURE;
}

void ClearBootloaderSignature(void)
{
  Signature = 0;
}

unsigned long long GetBootloaderSignature(void)
{
  return Signature;
}

/******************************************************************************/

#if __IAR_SYSTEMS_ICC__
__no_init __root unsigned int ResetSource @RESET_REASON_ADDR;
#else
extern unsigned int ResetSource;
#endif

void SaveResetSource(void)
{
  /* save and then clear reason for reset */
  ResetSource = SYSRSTIV;
  SYSRSTIV = 0;
}

unsigned int GetResetSource(void)
{
  return ResetSource;
}

/******************************************************************************/
#define BUILD_SIZE    (3)
extern const char BUILD[];

#if __IAR_SYSTEMS_ICC__
__no_init __root unsigned int niReset @ RESET_TYPE_ADDR;
__no_init __root char niBuild[BUILD_SIZE + 1] @ BUILD_NUMBER_ADDR;
#else
extern unsigned int niReset;
extern char niBuild[BUILD_SIZE + 1];
#endif

void CheckResetCode(void)
{
  unsigned char i = 0;
  
  for (; i < BUILD_SIZE; ++i) if (niBuild[i] != BUILD[i]) break;
  
  if (i < BUILD_SIZE)
  { // it's a flash reset
    for (i = 0; i < BUILD_SIZE; ++i) niBuild[i] = BUILD[i];
    niBuild[i] = '\0';
    niReset = FLASH_RESET_CODE;
  }
}

void SetMasterReset(void)
{
  niReset = MASTER_RESET_CODE;
}

void ClearResetCode(void)
{
  niReset = NORMAL_RESET_CODE;
}

void SoftwareReset(void)
{
  BackupRtc();
  PMMCTL0 = PMMPW | PMMSWBOR;
}
