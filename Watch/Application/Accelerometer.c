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
/*! \file Accelerometer.c
*
*/
/******************************************************************************/


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"

#include "Messages.h"
#include "hal_board_type.h"
#include "hal_accelerometer.h"
#include "hal_rtc.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "Utilities.h"      
#include "DebugUart.h"
#include "Accelerometer.h"
#include "SerialProfile.h"

/******************************************************************************/

static unsigned char WriteRegisterData;

#define ACCELEROMETER_DEBUG ( 0 )

#if ACCELEROMETER_DEBUG == 1
static unsigned char pReadRegisterData[16];
#endif

/******************************************************************************/

/* send interrupt only or send data (Send Interrupt Data [SID]) */
static unsigned char OperatingModeRegister;
static unsigned char InterruptControl;
static unsigned char SidControl;
static unsigned char SidAddr;
static unsigned char SidLength;

/******************************************************************************/

void InitializeAccelerometer(void)
{
  InitAccelerometerPeripheral();
  
  CONFIG_ACCELEROMETER_PINS_FOR_USE();
  
#if 0
  TaskDelayLpmDisable();
  vTaskDelay(10000);
  TaskDelayLpmEnable();
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

#if ACCELEROMETER_DEBUG == 1
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
    
  /* 
   * KIONIX_CTRL_REG3 and DATA_CTRL_REG can remain at their default values 
   *
   * 50 Hz
  */
#if 0  
  /* 300 uA */
  WriteRegisterData = PC1_OPERATING_MODE | TAP_ENABLE_TDTE;
  
  /* 180 uA */
  WriteRegisterData = PC1_OPERATING_MODE | RESOLUTION_8BIT | WUF_ENABLE;

  /* 180 uA */
  WriteRegisterData = PC1_OPERATING_MODE | TILT_ENABLE_TPE;

  /* 720 uA */  
  WriteRegisterData = PC1_OPERATING_MODE | RESOLUTION_12BIT | WUF_ENABLE;
#endif
  
  /* setup the default for the AccelerometerEnable command */
  OperatingModeRegister = PC1_OPERATING_MODE | RESOLUTION_12BIT | WUF_ENABLE;
  
  AccelerometerDisable();
   
  /* 
   * the interrupt from the accelerometer can be used to get periodic data
   *
   * the real time clock can also be used
   */
  
#if 0
  /* change to output data rate to 25 Hz */
  WriteRegisterData = WUF_ODR_25HZ;
  AccelerometerWrite(KIONIX_CTRL_REG3,&WriteRegisterData,ONE_BYTE);
#endif
  
#if 0
  /* this causes data to always be sent */  
  WriteRegisterData = 0x00;
  AccelerometerWrite(KIONIX_WUF_THRESH,&WriteRegisterData,ONE_BYTE);
#endif
  
#if 0
  /* use the real time clock to periodically send a message */
  EnableRtcPrescaleInteruptUser(RTC_TIMER_PEDOMETER);
#endif
  
  PrintString("Accelerometer Init Complete\r\n");
   
}


/* 
 * The interrupt can either send a message to the host or
 * it can send data (send a message that causes the task to read data from 
 * part and then send it to the host).
 */
void AccelerometerIsr(void)
{
  /* disabling the interrupt is the easiest way to make sure that
   * the stack does not get blasted with
   * data when it is in sleep mode
   */
  ACCELEROMETER_INT_DISABLE();
  
  /* can't allocate buffer here so we must go to task to send interrupt
   * occurred message
   */
  tMessage Msg;
  SetupMessage(&Msg,AccelerometerSendDataMsg,NO_MSG_OPTIONS);  
  RouteMsgFromIsr(&Msg); 

}


/* Send interrupt notification to the phone or 
 * read data from the accelerometer and send it to the phone
 */
void AccelerometerSendDataHandler(void)
{
  tMessage OutgoingMsg;
  
  if ( SidControl == SID_CONTROL_SEND_INTERRUPT )
  {
    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  AccelerometerHostMsg,
                                  ACCELEROMETER_HOST_MSG_IS_INTERRUPT_OPTION);
  }
  else
  {
    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  AccelerometerHostMsg,
                                  ACCELEROMETER_HOST_MSG_IS_DATA_OPTION);
    
    OutgoingMsg.Length = SidLength;
    
    AccelerometerRead(SidAddr,
                      OutgoingMsg.pBuffer,
                      SidLength);
  }
  
  RouteMsg(&OutgoingMsg);
    
}

void AccelerometerEnable(void)
{
  /* put into the mode specified by the OperatingModeRegister */
  AccelerometerWrite(KIONIX_CTRL_REG1,&OperatingModeRegister,ONE_BYTE);
  
  if ( InterruptControl == INTERRUPT_CONTROL_ENABLE_INTERRUPT )
  {
    ACCELEROMETER_INT_ENABLE();
  } 
}

void AccelerometerDisable(void)
{   
  /* put into low power mode */
  WriteRegisterData = PC1_STANDBY_MODE;
  AccelerometerWrite(KIONIX_CTRL_REG1,&WriteRegisterData,ONE_BYTE);

  ACCELEROMETER_INT_DISABLE();
}

/* 
 * Control how the msp430 responds to an interrupt,
 * control function of EnableAccelerometerMsg,
 * and allow enabling and disabling interrupt in msp430
 */
void AccelerometerSetupHandler(tMessage* pMsg)
{
  switch (pMsg->Options)
  {
  case ACCELEROMETER_SETUP_OPMODE_OPTION:
    OperatingModeRegister = pMsg->pBuffer[0];
    break;
  case ACCELEROMETER_SETUP_INTERRUPT_CONTROL_OPTION:
    InterruptControl = pMsg->pBuffer[0];
    break;
  case ACCELEROMETER_SETUP_SID_CONTROL_OPTION:
    SidControl = pMsg->pBuffer[0];
    break;
  case ACCELEROMETER_SETUP_SID_ADDR_OPTION:
    SidAddr = pMsg->pBuffer[0];
    break;
  case ACCELEROMETER_SETUP_SID_LENGTH_OPTION:
    SidLength = pMsg->pBuffer[0];
    break;
  case ACCELEROMETER_SETUP_INTERRUPT_ENABLE_DISABLE_OPTION:
    if ( pMsg->pBuffer[0] == 0 )
    {
      ACCELEROMETER_INT_DISABLE(); 
    }
    else
    {
      ACCELEROMETER_INT_ENABLE();  
    }
    break;
  default:
    PrintString("Unhandled Accelerometer Setup Option\r\n");
    break;
  }
  
}

/* Perform a read or write access of the accelerometer */
void AccelerometerAccessHandler(tMessage* pMsg)
{
  tAccelerometerAccessPayload* pPayload = 
    (tAccelerometerAccessPayload*) pMsg->pBuffer;

  if ( pMsg->Options == ACCELEROMETER_ACCESS_WRITE_OPTION )
  {
    AccelerometerWrite(pPayload->Address,&pPayload->Data,pPayload->Size);
  }
  else
  {
    tMessage OutgoingMsg;
    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  AccelerometerResponseMsg,
                                  pPayload->Size);
    
    AccelerometerRead(pPayload->Address,
                      &OutgoingMsg.pBuffer[ACCELEROMETER_DATA_START_INDEX],
                      pPayload->Size);  
  }
}