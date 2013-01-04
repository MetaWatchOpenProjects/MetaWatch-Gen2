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


/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/* Version mm/dd/yy  Author        Description of Modification                */
/* ------ --------  -------  ------------------------------------------------ */
/* 3.5.0  07/03/12  Mu Yang  Support Gen2                                     */
/* 3.6.0  08/23/12  Mu Yang  Support DUO mode                                 */
/* 3.6.1  08/25/12  Mu Yang  Refactor MessageQueue.c                          */
/* 3.6.2  08/28/12  Mu Yang  Add code for new Quard UI                        */
/* 3.6.3  09/03/12  Mu Yang  Integrated WatchFaces                            */
/* 3.6.4  09/05/12  Mu Yang  Widget update works, fix Android 2-line-draw     */
/* 3.6.5  09/06/12  Mu Yang  Support switching idle screens                   */
/* 3.6.6  09/07/12  Mu Yang  Fix showing phone control date/time mess-up      */
/* 3.6.7  09/07/12  Mu Yang  Support widget removal                           */
/* 3.6.8  09/07/12  Mu Yang  Use msg->pBuffer for WriteBuf()                  */
/* 3.6.9  09/08/12  Mu Yang  Add support for Clock widget                     */
/* 3.7.0  09/09/12  Mu Yang  Add Clock widget                                 */
/* 3.7.1  09/10/12  Mu Yang  Fix "WgtLst out of sync" by semaphore            */
/* 3.7.2  09/10/12  Mu Yang  Fix "show notification too short"                */
/* 3.7.3  09/12/12  Mu Yang  Tap to light-on; device name to "SW12".          */
/* 3.7.4  09/12/12  Mu Yang  Add widget boarder.                              */
/* 3.7.5  09/13/12  Mu Yang  Draw white dots of vertical boarder as well.     */
/* 3.7.6  09/13/12  Mu Yang  Support MUX; fix "UART ERROR".                   */
/* 3.7.7  09/17/12  Mu Yang  Support grid on/off and pixel inversion.         */
/* 3.7.8  09/18/12  Mu Yang  Merge 3.7.6a watchdog and DebugUart change.      */
/* 3.7.9  09/19/12  Mu Yang  Fix bug: late update clock and conn status.      */
/* 3.8.0  09/19/12  Mu Yang  redefine buttons for music control               */
/* 3.8.1  09/21/12  Mu Yang  Set WDT to 0x02 (4mins)                          */
/* 3.8.2  09/24/12  Mu Yang  Fix no vibration: msginfo.h: type mismatch.      */
/* 3.8.3  09/24/12  Mu Yang  Fix menu item mismatch.                          */
/* 3.8.4  09/25/12  Mu Yang  Fix mem leak on open/closing SPP;new 1316 patch. */
/* 3.8.5  09/25/12  Mu Yang  Suppport button-hold event for music control     */
/* 3.8.6  09/25/12  Mu Yang  Merge bootloader from ah2 branch.                */
/* 3.8.7  10/02/12  Mu Yang  Fix buttons' assignments.                        */
/* 3.8.8  10/03/12  Mu Yang  Fix show-caller-info notification bug.           */
/* 3.8.9  10/04/12  Mu Yang  Share button buffer for none-idle modes.         */
/* 3.9.0  10/08/12  Mu Yang  Support 2Q and 4Q clock widgets.                 */
/* 3.9.1  10/10/12  A. Hedin Fix WDT lockup; Setting capacitor value 0        */
/* 3.9.2  10/10/12  Mu Yang  Change definition of button D, E                 */
/* 3.9.3  10/12/12  Mu Yang  Optimize update idle pages and home widgets      */
/* 3.9.4  10/16/12  Mu Yang  Merge Background task into Display task.         */
/* 3.9.5  10/18/12  Mu Yang  Key press keep LED on;keys same for RadioOff     */
/* 3.9.6  10/19/12  Mu Yang  Add empty/loading widget buffer templates.       */
/* 3.9.7  10/22/12  Mu Yang  Update menu and add Service menu.                */
/* 3.9.8  10/23/12  Mu Yang  Add play/stop icons; 2Q clock/menu order.        */
/* 3.9.9  10/23/12  Mu Yang  Screen update by mode priority;invert disp.      */
/* 4.0.0  10/24/12  Mu Yang  Fix change mode loop; Service menu.              */
/* 4.0.1  10/24/12  Mu Yang  Add ChangeModeMsg current idle screen no.        */
/* 4.0.2  10/24/12  Mu Yang  No DrawClock if no clock on current page.        */
/* 4.0.3  10/25/12  Mu Yang  New status/un/pair screens; enter SrvMenu.       */
/* 4.0.4  10/25/12  Mu Yang  SHow battery %; charge icons; SrvMenu order.     */
/* 405    10/26/12  Mu Yang  Idle>Music mode; new versioning.                 */
/* 406    10/26/12  Mu Yang  Fix update overtaking non-idle page;apply to call*/
/* 407    10/29/12  Mu Yang  New button handling (free 175b RAM); B-E reset.  */
/* 408    10/30/12  Mu Yang  Stop showing clock when conn and watch-ctrl-top. */
/* 409    10/30/12  Mu Yang  Battery % and icons corrected; Bootloader blink. */
/* 410    10/31/12  Mu Yang  show right icon if battery below warning level.  */
/* 411    10/31/12  Mu Yang  Add build no. to infoStringResp.                 */
/* 412    10/31/12  Mu Yang  Fix ServiceMenu button combination.              */
/* 413    11/01/12  Mu Yang  Change ALL_TASKS_HAVE_CHECKED_IN from 0x7 to 0x3.*/
/* 414    11/01/12  Mu Yang  Add version info to infoStringResp.              */
/* 415    11/01/12  Mu Yang  Integrate Andrew's bootloader fix.               */
/* 416    11/02/12  Mu Yang  Handle TestMode message.                         */
/* 417    11/03/12  Mu Yang  Press A-B-C to exit bootloader mode.             */
/* 418    11/04/12  Mu Yang  Fix battery below warning level herizontal icon. */
/* 419    11/04/12  Mu Yang  Fix caller notification mixed with status screen.*/
/* 420    11/05/12  Mu Yang  Add ChangeMuxMode() to BattChrgCtrlHdlr().       */
/* 421    11/05/12  Mu Yang  Fix: reset mode timeout on new notification.     */
/* 422    11/06/12  Mu Yang  Fix: pressing A during booting mess-up UI.       */
/* 423    11/07/12  Mu Yang  BC to G1/1.5; Fix:HmWgtId 15; call in other mode.*/
/* 424    11/08/12  Mu Yang  Reject call on call notif page; correct tVersion.*/
/* 425    11/10/12  Mu Yang  Caller name aligns center.                       */
/* 426    11/19/12  Mu Yang  Fix Nval settings for sec/month/12h.             */
/* 427    11/21/12  Mu Yang  Use quewaiting for handing long MAP message.     */
/* 428    11/22/12  Mu Yang  New TI 1316 patch to fix random disconnection.   */
/* 429    11/22/12  Mu Yang  Use ResetModeTimer() directly from UpdDspHdl().  */
/* 430    11/22/12  Mu Yang  Show splash till BT is successfully turned on.   */
/* 431    11/24/12  Mu Yang  Metaboot cmdbuf to 8; no reject call.            */
/* 432    11/24/12  Mu Yang  Call end due 5s timeout; APP_VER for metaboot.   */
/* 433    11/28/12  Mu Yang  Receiving multi-msgs handling for Android 4.2.   */
/* 434    11/28/12  Mu Yang  SPP receving handling support for Android 4.2.   */
/* 435    12/06/12  Mu Yang  Back compatible with MWM-CE.                     */
/* 436    12/07/12  Mu Yang  Add BLE/BR mark to Status screen.                */
/* 437    12/07/12  Mu Yang  Replace WDReset(PUC) by SwReset(BOR).            */
/* 438    12/11/12  Mu Yang  Add battery icon to initial screens.             */
/* 439    12/12/12  Mu Yang  Integrate Andrew's changes to WDT:               */
/*                            - remove ResetWatchdog() from idle loop         */
/*                              [WDT controlled entirely by task check-in]    */
/*                            - remove set all pins to output function        */
/*                            - don't touch watchdog in Flash routines        */
/*                            - fix macro for enabling accelerometer power    */
/* 440    12/13/12  Mu Yang  set midium interval for music mode.              */
/* 441    12/14/12  Mu Yang  Increase Display message queue to 30.            */
/* 442    12/15/12  Mu Yang  Master reset when entering bootloader mode.      */
/* 443    12/15/12  Mu Yang  Fix vibrating after flashing: InitBtn;Splshing.  */
/* 444    12/18/12  Mu Yang  Move InitBtn/InitRtc to end of main().           */
/* 445    12/20/12  Mu Yang  Fix button not responsive after flashing.        */
/* 446    12/22/12  Mu Yang  Fix wrongly entering Ship mode after flashing.   */
/* 447    12/25/12  Mu Yang  Fix 1st-boot-after-flashing-fail by changing     */
/*                           heap size 12328 to 12334 (diff learned from 439) */
/* 448    12/26/12  Mu Yang  Integrate new patch from TI.                     */
/* 449    12/31/12  Mu Yang  Optimise MAP message handing.                    */
/* 450    12/31/12  Mu Yang  Change WDT reset from PUC to BOR.                */
/******************************************************************************/


#define APP_VER  "1.2"

const char VERSION[] = APP_VER;
const char BUILD[] = "450";

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"
#include "portmacro.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "BufferPool.h"

#include "hal_lpm.h"
#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_miscellaneous.h"
#include "hal_battery.h"
#include "hal_calibration.h"
#include "hal_clock_control.h"
#include "hal_boot.h"

#include "DebugUart.h"
#include "Adc.h"
#include "Wrapper.h"
#include "Buttons.h"
#include "LcdDisplay.h"
#include "Display.h"
#include "Utilities.h"
#include "Accelerometer.h"
#include "Buttons.h"
#include "Vibration.h"
#include "OneSecondTimers.h"
#include "Statistics.h"
#include "IdleTask.h"
#include "TestMode.h"

#include "OSAL_Nv.h"
#include "NvIds.h"

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
  ENABLE_LCD_LED();
  
#if ENABLE_WATCHDOG
  RestartWatchdog();
#else
  WDTCTL = WDTPW + WDTHOLD;
#endif
  
  /* clear shipping mode, if set to allow configuration */
  PMMCTL0_H = PMMPW_H;
  PM5CTL0 &= ~LOCKLPM5;  
  
  /* disable DMA during read-modify-write cycles */
  DMACTL4 = DMARMWDIS;

  DetermineErrata();
  
#ifdef BOOTLOADER
  
  /*
   * enable RAM alternate interrupt vectors
   * these are defined in AltVect.s43 and copied to RAM by cstartup
   */
  SYSCTL |= SYSRIVECT;
  
  ClearBootloaderSignature();
  
#endif

  /* calibration data is used for clock setup */
  InitializeCalibrationData();
  
#ifndef BOOTLOADER
  SaveResetSource();
#endif
  
  SetupClockAndPowerManagementModule();
  
  OsalNvInit(0);

  /* change the mux settings according to presense of the charging clip */
  InitializeMuxMode();
  ChangeMuxMode();

  InitDebugUart();
  TestModeControl();

  /* adc is required to get the board configuration */
  InitializeAdc();
  ConfigureDefaultIO(GetBoardConfiguration());

  InitializeDebugFlags();

//  InitButton();

  InitializeVibration();
  InitializeOneSecondTimers();

  InitializeBufferPool();
  InitializeWrapperTask();
  InitializeRealTimeClock();

  InitializeDisplayTask();

  DISABLE_LCD_LED();

#if CHECK_FOR_PMM15
  /* make sure error pmm15 does not exist */
  while ( PMM15Check() );
#endif

  /* Errata PMM17 - automatic prolongation mechanism
   * SVSLOW is disabled
   */
  *(unsigned int*)(0x0110) = 0x9602;
  *(unsigned int*)(0x0112) |= 0x0800;

  WhoAmI();
  PrintResetSource();
  
  /* if a watchdog occurred then print and save information about the source */
  WatchdogTimeoutHandler(GetResetSource());

  PrintString("Starting Task Scheduler\r\n");
  SetUartNormalMode();

  vTaskStartScheduler();

  /* if vTaskStartScheduler exits an error occured. */
  PrintString("Program Error\r\n");
  ForceWatchdogReset();
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

/* This interrupt port is used by the Bluetooth stack.
 * Do not change the name of this function because it is externed.
 */
void AccelerometerPinIsr(void)
{
  AccelerometerIsr();
}

void UnusedIsr(void)
{
  __no_operation();
}
