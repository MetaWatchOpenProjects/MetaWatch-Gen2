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
/*! \file Pedometer.c
*
*/
/******************************************************************************/


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "Messages.h"
#include "hal_board_type.h"
#include "hal_accelerometer.h"
#include "hal_rtc.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "BufferPool.h"     
#include "Utilities.h"      
#include "DebugUart.h"
#include "Pedometer.h"
#include "SerialProfile.h"

/******************************************************************************/
   
#define PEDOMETER_TASK_STACK_DEPTH  (configMINIMAL_STACK_DEPTH + 20)
#define PEDOMETER_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)


static void PedometerTask(void *pvParameters);

static xTaskHandle PedometerTaskHandle;
static xSemaphoreHandle PedometerCountingSemaphore;

static void PedometerHandler(void);

/******************************************************************************/

#define NUMBER_OF_INTERRUPT_BYTES ( 4 ) 
#define NUMBER_OF_XYZ_BYTES       ( 6 ) 

static unsigned char WriteRegisterData;
static unsigned char pInterruptAndXyzData[NUMBER_OF_INTERRUPT_BYTES+NUMBER_OF_XYZ_BYTES];

#if 0
static unsigned char pReadRegisterData[16];
#endif

/******************************************************************************/

void InitializePedometerTask(void)
{
  InitAccelerometerPeripheral();
  
  unsigned char Size = 5;
  unsigned char InitialCount = 0;
  PedometerCountingSemaphore = xSemaphoreCreateCounting(Size,InitialCount);
  
  xTaskCreate(PedometerTask, 
              "PEDOMETER", 
              PEDOMETER_TASK_STACK_DEPTH, 
              NULL, 
              PEDOMETER_TASK_PRIORITY, 
              &PedometerTaskHandle);
}

static void PedometerTask(void *pvParameters)
{
  if ( PedometerCountingSemaphore == 0 )
  {
    PrintString("PedometerCountingSemaphore not created!");
  }
  
  if ( PedometerTaskHandle == 0 )
  {
    PrintString("PedometerTaskHandle not created!");
  }
 
#if 0
  /* let the phone and watch connect before starting */
  while ( QueryPhoneConnected() == 0 )
  {
    TaskDelayLpmDisable();
    vTaskDelay(1000);
    TaskDelayLpmEnable();
  }
#else
  TaskDelayLpmDisable();
  vTaskDelay(10000);
  TaskDelayLpmEnable();
#endif
  
  /* 
   * periodic readings could be taken from the accelerometer using the real time 
   * clock 
   * however, it seems like the interrupt from the accelerometer should be used
   *
   * vTaskDelayUntil did not work for me...
   */
  
#if 0
  /* use the real time clock to periodically increment a counting semaphore */
  EnableRtcPrescaleInteruptUser(RTC_TIMER_PEDOMETER);
#endif
  
  PrintString("Accelerometer Initialization\r\n");
 
  /* 
   * make sure part is in standby mode because some registers can only
   * be changed when the part is not active.
   */
  WriteRegisterData = PC1_STANDBY_MODE;
  AccelerometerWrite(KIONIX_CTRL_REG1,&WriteRegisterData,ONE_BYTE);

  /* enable interrupt and make it active high */
  WriteRegisterData = IEN | IEA;
  AccelerometerWrite(KIONIX_INT_CTRL_REG1,&WriteRegisterData,ONE_BYTE);
  
  /* enable interrupt for all three axes */
  WriteRegisterData = XBW | YBW | ZBW;
  AccelerometerWrite(KIONIX_INT_CTRL_REG2,&WriteRegisterData,ONE_BYTE);
  
  /* 
   * KIONIX_CTRL_REG3 and DATA_CTRL_REG can remain at their default values 
   *
   * 50 Hz
  */
  
  /* change resolution to 12 bit mode and go into operating mode */
  /** todo make this a command from the host */
  WriteRegisterData = PC1_OPERATING_MODE | RESOLUTION_12BIT | WUF_ENABLE;
  AccelerometerWrite(KIONIX_CTRL_REG1,&WriteRegisterData,ONE_BYTE);
  
  
#if 0
  /* check that writes are correct and check read functions */
  AccelerometerRead(KIONIX_INT_CTRL_REG1,pReadRegisterData+0,ONE_BYTE);
  AccelerometerRead(KIONIX_CTRL_REG1,pReadRegisterData+1,ONE_BYTE);
  AccelerometerRead(KIONIX_CTRL_REG1,pReadRegisterData+2,ONE_BYTE);
  AccelerometerRead(KIONIX_INT_CTRL_REG1,pReadRegisterData+3,ONE_BYTE);
  AccelerometerRead(KIONIX_INT_CTRL_REG2,pReadRegisterData+4,ONE_BYTE);
  AccelerometerRead(KIONIX_INT_REL,pReadRegisterData+5,ONE_BYTE);
  AccelerometerRead(KIONIX_INT_SRC_REG1,pReadRegisterData+6,ONE_BYTE);
  AccelerometerRead(KIONIX_INT_SRC_REG2,pReadRegisterData+7,ONE_BYTE);
  AccelerometerRead(KIONIX_STATUS_REG,pReadRegisterData+8,ONE_BYTE);
  /* burst read */
  AccelerometerRead(KIONIX_CTRL_REG1,pReadRegisterData+9,ONE_BYTE*7);
#endif
  
  ACCELEROMETER_INT_ENABLE();
  
  PrintString("Pedometer Task Starting\r\n");
  
  /* if an interrupt happened before being ready then clear it out */
  xSemaphoreGive(PedometerCountingSemaphore);
  
  /* 
   * A task can only block on one item. 
   *
   * Therefore, any control functions for this task must be  
   * handled through the background task. 
   */
  for(;;)
  {
    
    if( xSemaphoreTake(PedometerCountingSemaphore, portMAX_DELAY) == pdTRUE )
    {
      
      PedometerHandler();
      
      CheckStackUsage(PedometerTaskHandle,"Pedometer Task");
      
    }
    
  }
   
}


static void PedometerHandler(void)
{
  tHostMsg* pOutgoingMsg;
  
  /*
   * make sure it was WUF and read the interrupt release register to 
   * clear the interrupt
   */
  AccelerometerRead(KIONIX_INT_SRC_REG1,
                    pInterruptAndXyzData,
                    NUMBER_OF_INTERRUPT_BYTES);
  
  /* OR KIONIX_XOUT_L */
  AccelerometerRead(KIONIX_XOUT_HPF_L,
                    pInterruptAndXyzData+NUMBER_OF_INTERRUPT_BYTES,
                    NUMBER_OF_XYZ_BYTES);
  
  /* check that we aren't overflowing output queue
   * if this happens then the BT memory buffer may be too small
  */
  while( QueueHandles[SPP_TASK_QINDEX]->uxMessagesWaiting ==
         QueueHandles[SPP_TASK_QINDEX]->uxLength )
  {
    PrintString("^");
    TaskDelayLpmDisable();
    vTaskDelay(1280);
    TaskDelayLpmEnable();
  }
  
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  
  UTL_BuildHstMsg(pOutgoingMsg,
                  AccelerometerRawData,
                  NO_MSG_OPTIONS,
                  pInterruptAndXyzData,
                  NUMBER_OF_INTERRUPT_BYTES+NUMBER_OF_XYZ_BYTES);

  RouteMsg(&pOutgoingMsg);
    
}



void IncrementPedometerSemaphoreFromIsr(void)
{
  /* higher priority task woken */
  signed int temp = pdFALSE;
  
#if 0
  DEBUG3_PULSE();
#endif
  
  if ( xSemaphoreGiveFromISR(PedometerCountingSemaphore,&temp) != pdTRUE )
  {
    /** todo */
    while(1);  
  }
  
  
}