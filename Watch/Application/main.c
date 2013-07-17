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

#include "Version.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hal_boot.h"
#include "hal_board_type.h"
#include "Wrapper.h"
#include "LcdDisplay.h"
#include "IdleTask.h"

#define APP_VER     "1.4.0"
#define BUILD_VER   "543.000"

char const VERSION[] = APP_VER;
char const BUILD[] = BUILD_VER;

/******************************************************************************/

#ifdef BOOTLOADER

/* for image_data_t definiton */
#include "bl_boot.h"        

/*******************************************************************************

__checksum contains a the checksum (CRC16) of the code image up to and
including EndOfImage.  The code image inits the value to 0xFFFF and the actual
value is calculated and stored here by the boot loader tool.

*******************************************************************************/
#if 0
#pragma location="CHECKSUM"
__root __no_init const unsigned int __checksum;
#endif

/******************************************************************************

Shadow checksum contains a copy of the checksum as calculated by the firmware
at runtime after new code is loaded.  The code image inits the value to 0xFFFF
If the bootloader verifies the image (by calculating the image checksum and 
comparing it to the stored value at 'CHECKSUM'), it writes the checksum value
calculated to the shadow location.  This avoids having to recalculate the 
checksum after it has been validated once.

*******************************************************************************/

#pragma location="SHADOW_CHECKSUM"
__root __no_init unsigned int ShadowChecksum;

/*******************************************************************************

EndOfImage is the last byte of memory used by this image.  By retrieving the
address of it, we know the size of the image.  The address of EndOfImage is
stored at pImageEnd.  We use the end address in calculating the image checksum
and also so as to not require programming all of memory if the image is smaller,
thus saving time.  See the linker file for the _IMAGE_END and IMAGE_END
labels to see how this is accomplished.

*******************************************************************************/

#pragma location="_IMAGE_END"
__root const unsigned char EndOfImage = 0x26;

#define IAR_TOOLS   (1)
#define CCS_TOOLS   (2)
#ifdef __IAR_SYSTEMS_ICC__ 
 #define TOOLSET     IAR_TOOLS
#else
 #define TOOLSET     CCS_TOOLS
#endif

#pragma location="IMAGE_DATA"
__root const image_data_t imageData = {&EndOfImage, __DATE__, TOOLSET, APP_VER};
#endif //BOOTLOADER

/******************************************************************************/

void main(void)
{
  ResetWatchdog();

#if CHECK_CSTACK
  PopulateCStack();
#endif

  CreateWrapperTask();
  CreateDisplayTask();
  
  vTaskStartScheduler();

  WatchdogReset();
}

/*
 * Callbacks are for debug signals and nothing else!
 */

void SppHostMessageWriteCallback(void)
{
  //DEBUG4_PULSE();
}

void SppReceivePacketCallback(void)
{
  //DEBUG5_PULSE();
}

void SniffModeEntryAttemptCallback(void)
{
  //DEBUG3_PULSE();
}

void DebugBtUartError(void)
{
  //DEBUG5_HIGH();
}

void MsgHandlerDebugCallback(void)
{
  //DEBUG5_PULSE();
}

void UnusedIsr(void)
{
  __no_operation();
}
