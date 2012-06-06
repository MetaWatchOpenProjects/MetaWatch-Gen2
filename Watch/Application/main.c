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
/*! \file main.c
*
*/
/******************************************************************************/

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
#include "hal_analog_display.h"
#include "hal_battery.h"
#include "hal_miscellaneous.h"
#include "hal_calibration.h"

#include "DebugUart.h"
#include "Adc.h"
#include "Wrapper.h"
#include "Background.h"
#include "Buttons.h"
#include "LcdDisplay.h"
#include "Display.h"
#include "Utilities.h"
#include "Accelerometer.h"
#include "Buttons.h"
#include "Vibration.h"
#include "OneSecondTimers.h"
#include "Statistics.h"

#include "OSAL_Nv.h"
#include "NvIds.h"

void main(void)
{
  /* Turn off the watchdog timer */
  WDTCTL = WDTPW + WDTHOLD;

  /* clear reason for reset */
  SYSRSTIV = 0;

  /* disable DMA during read-modify-write cycles */
  DMACTL4 = DMARMWDIS;

  unsigned char MspVersion = GetMsp430HardwareRevision();

  SetupClockAndPowerManagementModule();

  OsalNvInit(0);

  InitDebugUart();

  InitializeCalibrationData();

  InitializeAdc();

  ConfigureDefaultIO(GetBoardConfiguration());

  InitializeDebugFlags();
  InitializeButtons();
  InitializeVibration();
  InitializeOneSecondTimers();

  InitializeBufferPool();

  InitializeWrapperTask();

  InitializeRealTimeClock();

  InitializeBackgroundTask();

  InitializeDisplayTask();

#if 0
  /* timeout is 16 seconds */
  hal_SetWatchdogTimeout(16);
#endif

#ifdef CHECK_FOR_PMM15
  /* make sure error pmm15 does not exist */
  while ( PMM15Check() );
#endif

  /* Errata PMM17 - automatic prolongation mechanism
   * SVSLOW is disabled
   */
  *(unsigned int*)(0x0110) = 0x9602;
  *(unsigned int*)(0x0112) |= 0x0800;

  PrintString("Starting Task Scheduler\r\n");
  vTaskStartScheduler();

  /* if vTaskStartScheduler exits an error occured. */
  PrintString("Program Error\r\n");
  ForceWatchdogReset();

}




/* The following function exists to put the MCU to sleep when in the idle task. */
static unsigned char SppReadyToSleep;
static unsigned char TaskDelayLockCount;
static unsigned char AllTaskQueuesEmptyFlag;

void vApplicationIdleHook(void)
{

  /* Put the processor to sleep if the serial port indicates it is OK and
   * all of the queues are empty.
   *
   * This will stop the OS scheduler.
   */

  SppReadyToSleep = SerialPortReadyToSleep();
  TaskDelayLockCount = GetTaskDelayLockCount();
  AllTaskQueuesEmptyFlag = AllTaskQueuesEmpty();

#if 0
  if ( SppReadyToSleep )
  {
    DEBUG3_HIGH();
  }
  else
  {
    DEBUG3_LOW();
  }
#endif

  if (   SppReadyToSleep
      && TaskDelayLockCount == 0
      && AllTaskQueuesEmptyFlag )

  {
    extern xTaskHandle IdleTaskHandle;
    CheckStackUsage(IdleTaskHandle,"Idle Task");

    /* Call MSP430 Utility function to enable low power mode 3.     */
    /* Put OS and Processor to sleep. Will need an interrupt        */
    /* to wake us up from here.   */
    MSP430_LPM_ENTER();

  }

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


