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

/* software reset (done using PMMSWBOR) */
#define VALID_RESET_TYPE_FOR_BOOTLOADER_ENTRY (0x0006)
   
/* "MetaBoot" backwards */
#define BOOTLOADER_SIGNATURE (0x746F6F426174654D)
#define MASTER_RESET         (0xAA)
#define NORMAL_RESET         (0x55)

#define RESET_UNKNOWN         0
#define RESET_STACK_OVFL      1
#define RESET_TASK_FAIL       2
#define RESET_WRAPPER_INIT    3
#define RESET_BUTTON_PRESS    4
#define RESET_BOOTLOADER      5
#define RESET_SHIPMODE        6
#define RESET_INVALID_HCILL   7
#define RESET_UART_ERROR      8
#define RESET_TERM_MODE       9
#define RESET_HCI_HW          10
#define RESET_MEM_ALLOC       11
#define RESET_MEM_FREE        12
#define RESET_WDT             13

/******************************************************************************/
#define RESET_TYPE_ADDR       (0x1C02)
#define RESET_CODE_ADDR       (0x1C03)
#define RESET_VALUE_ADDR      (0x1C04)
#define RADIO_SLEEP_ADDR      (0x1C05)
#define DISPLAY_QUEUE_ADDR    (0x1C06)
#define WRAPPER_QUEUE_ADDR    (0x1C07)
#define WDT_NUM_ADDR          (0x1C08)
#define MUX_MODE_ADDR         (0x1C09)
#define PROPERTY_ADDR         (0x1C0A)
#define BT_STATE_ADDR         (0x1C0B)
#define CONN_TYPE_ADDR        (0x1C0C)
#define DSCONN_TYPE_ADDR      (0x1C0D)
#define BATTERY_ADDR          (0x1C0E)
#define SERVICE_CHANGED_ADDR  (0x1C0F)
#define GATT_HANDLE_ADDR      (0x1C10)
#define ANCS_HANDLE_ADDR      (0x1C10)
#define MWM_HANDLE_ADDR       (0x1C11)
#define BUILD_NUMBER_ADDR     (0x1C12) // 3

#define RTC_MIN_ADDR          (0x1C15)
#define RTC_HOUR_ADDR         (0x1C16)
#define RTC_DAY_ADDR          (0x1C17)
#define RTC_MON_ADDR          (0x1C18)
#define RTC_DOW_ADDR          (0x1C19)
#define RTC_YEAR_ADDR         (0x1C1A) //2

#define REMOTE_DEVICE_ADDR    (0x1C1C)
#define LINK_KEY_ADDR         (0x1C22)

#define LOG_BUFFER_ADDR       (0x1C34)
#define RESET_LOG_ADDR        (0x1C36)  // 18x4=72 -> 1C7C

#if WWZ
#define CDT_DAY_ADDR          (0x1C7C)
#define CDT_MIN_ADDR          (0x1C7E)
#endif

extern unsigned char niResetType;

void SetBootloaderSignature(void);
void ClearBootloaderSignature(void);
unsigned long long GetBootloaderSignature(void);

void CheckResetType(void);
void SoftwareReset(unsigned char Code, unsigned char Value);

#endif /* HAL_BOOT */
