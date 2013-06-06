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

#ifndef HAL_BOOT_H
#define HAL_BOOT_H

/******************************************************************************/

/* Things shared between bootloader and main application */

/* This address is in an unused location reserved for the alternate interrupt
 * table
 */
#define SIGNATURE_ADDR ( 0x5B80 )

/* this is at the start of ram and space is reserved in the linker file */
#define RESET_REASON_ADDR     (0x1C00)
#define WATCHDOG_INFO_ADDR    (0x1C02) // 6
#define WATCHDOG_COUNTER_ADDR (0x1C08)
#define MUX_MODE_ADDR         (0x1C0A)
#define RESET_TYPE_ADDR       (0x1C0C)
#define BUILD_NUMBER_ADDR     (0x1C0E) // 4
#define BLE_DISCONN_ADDR      (0x1C12)
#define AUTH_INFO_ADDR        (0x1C14) // 23
#define PROPERTY_ADDR         (0x1C2C)

#define RTC_YEAR_ADDR         (0x1C2E)
#define RTC_MON_ADDR          (0x1C30)
#define RTC_DAY_ADDR          (0x1C31)
#define RTC_DOW_ADDR          (0x1C32)
#define RTC_HOUR_ADDR         (0x1C33)
#define RTC_MIN_ADDR          (0x1C34)
#define RTC_SEC_ADDR          (0x1C35)
#define RTC_LANG_ADDR         (0x1C36)

#define CDT_DAY_ADDR          (0x1C38)
#define CDT_MIN_ADDR          (0x1C3A)

/******************************************************************************/

/* software reset (done using PMMSWBOR) */
#define VALID_RESET_TYPE_FOR_BOOTLOADER_ENTRY ( 0x0006 )
   
/******************************************************************************/
     
/* "MetaBoot" backwards */
#define BOOTLOADER_SIGNATURE      (0x746F6F426174654D)
#define MASTER_RESET_CODE         (0xDEAF)
#define FLASH_RESET_CODE          (0xABCD)
#define NORMAL_RESET_CODE         (0x0000)

void SetBootloaderSignature(void);
void ClearBootloaderSignature(void);
unsigned long long GetBootloaderSignature(void);

void SaveResetSource(void);
unsigned int GetResetSource(void);

void CheckResetCode(void);
void SetMasterReset(void);
void ClearResetCode(void);
void SoftwareReset(void);

#endif /* HAL_BOOT */
