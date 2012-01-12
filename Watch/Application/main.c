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

#include "Adc.h"
#include "SerialProfile.h"
#include "Background.h"         
#include "Buttons.h"   
#include "LcdDisplay.h"
#include "Display.h"
#include "Utilities.h"
#include "Accelerometer.h"
#include "Buttons.h"
#include "Vibration.h"
#include "OneSecondTimers.h"
#include "DebugUart.h"
#include "Statistics.h"

#include "OSAL_Nv.h"
#include "NvIds.h"

static void ConfigureHardware(void);
static void Housekeeping(void);
static void ForceWatchdogReset(void);

void main(void)
{
  /* Turn off the watchdog timer */
  WDTCTL = WDTPW + WDTHOLD;

  /* clear reason for reset */
  SYSRSTIV = 0;
  
  /* disable DMA during read-modify-write cycles */
  DMACTL4 = DMARMWDIS;
  
  InitializeCalibrationData();
  
  ConfigureHardware();

  OsalNvInit(0);
  
  InitializeDebugFlags();
  InitializeButtons();
  InitializeVibration();
  InitializeOneSecondTimers();
  
  InitializeBufferPool();    

  InitializeSppTask();
  
  InitializeRealTimeClock();       

  InitializeBackgroundTask();   

  InitializeDisplayTask();   

  InitializeAdc();

#if 0
  /* timeout is 16 seconds */
  hal_SetWatchdogTimeout(16); 
#endif

#ifdef CHECK_FOR_PMM15
  /* make sure error pmm15 does not exist */
  while ( PMM15Check() );
#endif
  
  PrintString("Starting Task Scheduler.\r\n");
  vTaskStartScheduler();

  /* if vTaskStartScheduler exits an error occured. */
  PrintString("Program Error\r\n");

}


/* The following is responsible for initializing the target hardware.*/
static void ConfigureHardware(void)
{
  SetAllPinsToOutputs();

  SetupClockAndPowerManagementModule();
  
  SetupAclkToRadio();

  InitDebugUart();
    
  BLUETOOTH_SIDEBAND_CONFIG();
  CONFIG_OLED_PINS();
  
#ifdef HW_DEVBOARD_V2
  CONFIG_DEBUG_PINS();
  CONFIG_LED_PINS();
#endif

#ifdef DIGITAL
  DISABLE_LCD_LED();
#endif

  CONFIG_SRAM_PINS();
  APPLE_CONFIG();

  /* the accelerometer may not be used so configure its pins here */  
  CONFIG_ACCELEROMETER_PINS();
  
}

/* The following function exists to put the MCU to sleep when in the idle task. */
static unsigned char SppReadyToSleep;
static unsigned char TaskDelayLockCount;
static unsigned char AllTaskQueuesEmptyFlag;

void vApplicationIdleHook(void)
{
  Housekeeping();
  
  /* Put the processor to sleep if the serial port indicates it is OK and 
   * all of the queues are empty.
   *
   * This will stop the OS scheduler.
   */

  SppReadyToSleep = SerialPortReadyToSleep();
  TaskDelayLockCount = GetTaskDelayLockCount();
  AllTaskQueuesEmptyFlag = AllTaskQueuesEmpty();
  
  if ( SppReadyToSleep )
  {
    DEBUG3_HIGH();  
  }
  else
  {
    DEBUG3_LOW();
  }
  
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

/* when debugging one may want to disable this 
 * the reset may not always occur (if code gets messed up enough it won't
 * get here)
 */
static void Housekeeping(void)
{
  if (   gBtStats.MallocFailed
      || gBtStats.RxDataOverrun
      || gBtStats.RxInvalidStartCallback )
  {
    PrintString("************Bluetooth Failure************\r\n");
    __delay_cycles(100000);
    ForceWatchdogReset();
  }
  
  if ( gAppStats.BufferPoolFailure )
  {
    PrintString("************Application Failure************\r\n");
    __delay_cycles(100000);
    ForceWatchdogReset();
  }
  
}

#if 0
/* set watchdog for 16 second timeout */
static void SetWatchdogReset(void)
{
  /* write password, select aclk, divide by 512*1024 = 16 ms */
  WDTCTL = WDTPW + WDTSSEL__ACLK + WDTIS_3;
}
#endif

/* write the inverse of the password and force reset */
static void ForceWatchdogReset(void)
{
  WDTCTL = ~WDTPW; 
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

void DebugCallback3(void)
{
  //DEBUG3_PULSE();  
}

void DebugCallback4(void)
{
  //DEBUG4_PULSE();  
}
          
          
void Debug5Callback(void)
{
  //DEBUG5_PULSE();  
}

void MsgHandlerDebugCallback(void)
{
  //DEBUG5_PULSE();
}

/* This interrupt port is used by the stack. Do not change the name of this
 * function.
 *
 * Place a call to your interrupt routine here.
 */
void AccelerometerPinIsr(void)
{
  AccelerometerIsr();
}

     



