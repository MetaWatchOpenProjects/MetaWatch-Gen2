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

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"

#include "hal_board_type.h"
#include "hal_accelerometer.h"
#include "Messages.h"
#include "hal_rtc.h"

#include "MessageQueues.h"     
#include "DebugUart.h"
#include "Utilities.h" 
#include "Accelerometer.h"
#include "Wrapper.h"

/******************************************************************************/
#define ACCELEROMETER_POWER_UP_TIME_MS (20)
#define XYZ_DATA_LENGTH                 (6)

#define ACCEL_STATE_UNKNOWN             (0)
#define ACCEL_STATE_INIT                (1)
#define ACCEL_STATE_DISABLED            (2)
#define ACCEL_STATE_ENABLED             (3)

static unsigned char WriteRegisterData;
static unsigned char pReadRegisterData[16];

/* send interrupt only or send data (Send Interrupt Data [SID]) */
static unsigned char OperatingModeRegister;
static unsigned char InterruptControl;
static unsigned char SidControl;
static unsigned char SidAddr;
static unsigned char SidLength;
static unsigned char AccelState;

/******************************************************************************/

static void InitAccelerometer(void);
static void AccelerometerSendDataHandler(void);
static void AccelerometerSetupHandler(tMessage* pMsg);
static void AccelerometerAccessHandler(tMessage* pMsg);
static void EnableAccelerometer(void);
static void DisableAccelerometer(void);
static void ReadInterruptReleaseRegister(void);

/******************************************************************************/

static void InitAccelerometer(void)
{
  InitAccelerometerPeripheral();

  /* make sure accelerometer has had 20 ms to power up */
  TaskDelayLpmDisable();
  vTaskDelay(ACCELEROMETER_POWER_UP_TIME_MS);
  TaskDelayLpmEnable();

  /*
   * make sure part is in standby mode because some registers can only
   * be changed when the part is not active.
   */
  WriteRegisterData = PC1_STANDBY_MODE;
  AccelerometerWrite(KIONIX_CTRL_REG1, &WriteRegisterData, ONE_BYTE);

  /* enable face-up and face-down detection */
  WriteRegisterData = TILT_FDM | TILT_FUM;
  AccelerometerWrite(KIONIX_CTRL_REG2, &WriteRegisterData, ONE_BYTE);
    
  /* 
   * the interrupt from the accelerometer can be used to get periodic data
   * the real time clock can also be used
   */
  
  /* change to output data rate to 25 Hz */
  WriteRegisterData = WUF_ODR_25HZ | TAP_ODR_400HZ;
  AccelerometerWrite(KIONIX_CTRL_REG3, &WriteRegisterData, ONE_BYTE);
  
  /* enable interrupt and make it active high */
  WriteRegisterData = IEN | IEA;
  AccelerometerWrite(KIONIX_INT_CTRL_REG1, &WriteRegisterData, ONE_BYTE);
  
  /* enable motion detection interrupt for all three axis */
  WriteRegisterData = ZBW;
  AccelerometerWrite(KIONIX_INT_CTRL_REG2, &WriteRegisterData, ONE_BYTE);

  /* enable tap interrupt for Z-axis */
  WriteRegisterData = TFDM;
  AccelerometerWrite(KIONIX_INT_CTRL_REG3, &WriteRegisterData, ONE_BYTE);
  
  /* set TDT_TIMER to 0.2 secs*/
  WriteRegisterData = 0x50;
  AccelerometerWrite(KIONIX_TDT_TIMER, &WriteRegisterData, ONE_BYTE);
  
  /* set tap low and high thresholds (default: 26 and 182) */
  WriteRegisterData = 40; //78;
  AccelerometerWrite(KIONIX_TDT_L_THRESH, &WriteRegisterData, ONE_BYTE);
  WriteRegisterData = 128;
  AccelerometerWrite(KIONIX_TDT_H_THRESH, &WriteRegisterData, ONE_BYTE);
    
  /* set WUF_TIMER counter */
  WriteRegisterData = 10;
  AccelerometerWrite(KIONIX_WUF_TIMER, &WriteRegisterData, ONE_BYTE);
    
  /* this causes data to always be sent */
  // WriteRegisterData = 0x00;
  WriteRegisterData = 0x08;
  AccelerometerWrite(KIONIX_WUF_THRESH, &WriteRegisterData, ONE_BYTE);
     
  /* single byte read test */
  AccelerometerRead(KIONIX_DCST_RESP,pReadRegisterData,1);
  //PrintStringAndHex("KIONIX_DCST_RESP (0x55) = 0x",pReadRegisterData[0]);
  
  /* multiple byte read test */
  AccelerometerRead(KIONIX_WHO_AM_I,pReadRegisterData,2);
//  PrintStringAndHex("KIONIX_WHO_AM_I (0x01) = 0x",pReadRegisterData[0]);
//  PrintStringAndHex("KIONIX_TILT_POS_CUR (0x20) = 0x",pReadRegisterData[1]);  
    
  /* 
   * KIONIX_CTRL_REG3 and DATA_CTRL_REG can remain at their default values 
   *
   * 50 Hz
  */
#if 0  
  /* KTXF9 300 uA; KTXI9 165 uA */
  WriteRegisterData = PC1_OPERATING_MODE | TAP_ENABLE_TDTE;
  
  /* 180 uA; KTXI9 115 uA */
  WriteRegisterData = PC1_OPERATING_MODE | RESOLUTION_8BIT | WUF_ENABLE;

  /* 180 uA; KTXI9 8.7 uA */
  WriteRegisterData = PC1_OPERATING_MODE | TILT_ENABLE_TPE;

  /* 720 uA; KTXI9 330 uA */  
  WriteRegisterData = PC1_OPERATING_MODE | RESOLUTION_12BIT | WUF_ENABLE;
#endif
  
  /* setup the default for the AccelerometerEnable command */
  OperatingModeRegister = PC1_OPERATING_MODE | RESOLUTION_12BIT | 
    TAP_ENABLE_TDTE | TILT_ENABLE_TPE; // | WUF_ENABLE;
  InterruptControl = INTERRUPT_CONTROL_DISABLE_INTERRUPT;
  SidControl = SID_CONTROL_SEND_DATA;
  SidAddr = KIONIX_XOUT_L;
  SidLength = XYZ_DATA_LENGTH;  

  AccelState = ACCEL_STATE_INIT;
  PrintString2("- Accel Initd", CR);
}

/* 
 * The interrupt can either send a message to the host or
 * it can send data (send a message that causes the task to read data from 
 * part and then send it to the host).
 */
void AccelerometerIsr(void)
{
#if 0
  /* disabling the interrupt is the easiest way to make sure that
   * the stack does not get blasted with
   * data when it is in sleep mode
   */
  ACCELEROMETER_INT_DISABLE();
#endif
  
  /* can't allocate buffer here so we must go to task to send interrupt
   * occurred message
   */
  tMessage Msg;
  SetupMessage(&Msg, AccelerometerSendDataMsg, MSG_OPT_NONE);  
  SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
}

static void ReadInterruptReleaseRegister(void)
{
#if 0
  /* interrupts are rising edge sensitive so clear and enable interrupt
   * before clearing it in the accelerometer 
   */
  ACCELEROMETER_INT_ENABLE();
#endif
  
  unsigned char temp;
  AccelerometerRead(KIONIX_INT_REL,&temp,1);
}

/* Send interrupt notification to the phone or 
 * read data from the accelerometer and send it to the phone
 */
static void AccelerometerSendDataHandler(void)
{
  /* burst read */
  AccelerometerRead(KIONIX_TDT_TIMER, pReadRegisterData, 6);
  
  if (   pReadRegisterData[0] != 0x78 
      || pReadRegisterData[1] != 0xCB /* b6 */ 
      || pReadRegisterData[2] != 0x1A 
      || pReadRegisterData[3] != 0xA2 
      || pReadRegisterData[4] != 0x24 
      || pReadRegisterData[5] != 0x28 )
  {
    // need to be checked
    //PrintString("Invalid i2c burst read\r\n");
  }
          
  /* single read */
  AccelerometerRead(KIONIX_DCST_RESP,pReadRegisterData,1);
  
  if (pReadRegisterData[0] != 0x55)
  {
    PrintStringAndHex("Invalid i2c Read: ", pReadRegisterData[0]);
  }

  AccelerometerRead(KIONIX_INT_SRC_REG2, pReadRegisterData, ONE_BYTE);

  tMessage Msg;
  
  if ((*pReadRegisterData & INT_TAP_SINGLE) == INT_TAP_SINGLE)
  {
    SendMessage(&Msg, LedChange, LED_ON_OPTION);
  }
//  else if ((*pReadRegisterData & INT_TAP_DOUBLE) == INT_TAP_DOUBLE)

  if (Connected(CONN_TYPE_MAIN))
  {
    if (SidControl == SID_CONTROL_SEND_INTERRUPT)
    {
      SetupMessageAndAllocateBuffer(&Msg,
                            AccelerometerHostMsg,
                            ACCELEROMETER_MSG_IS_INTERRUPT_OPTION);
    }
    else
    {
      SetupMessageAndAllocateBuffer(&Msg,
                                AccelerometerHostMsg,
                                ACCELEROMETER_MSG_IS_DATA_OPTION);

      Msg.Length = SidLength;
      AccelerometerRead(SidAddr, Msg.pBuffer, SidLength);

      // read orientation and tap status starting
      // AccelerometerReadSingle(KIONIX_INT_SRC_REG1, Msg.pBuffer + SidLength);
      //*(Msg.pBuffer + SidLength) = *pReadRegisterData;
      //Msg.Length ++;
    }
    RouteMsg(&Msg);
  }

  ReadInterruptReleaseRegister();
}

static void EnableAccelerometer(void)
{
  /* put into the mode specified by the OperatingModeRegister */
  AccelerometerWrite(KIONIX_CTRL_REG1, &OperatingModeRegister, ONE_BYTE);
  
  if (InterruptControl == INTERRUPT_CONTROL_ENABLE_INTERRUPT)
  {
    ReadInterruptReleaseRegister();
  }
  ACCELEROMETER_INT_ENABLE();
  AccelState = ACCEL_STATE_ENABLED;
}

static void DisableAccelerometer(void)
{   
  /* put into low power mode */
  WriteRegisterData = PC1_STANDBY_MODE;
  AccelerometerWrite(KIONIX_CTRL_REG1,&WriteRegisterData,ONE_BYTE);

  ACCELEROMETER_INT_DISABLE();
  AccelState = ACCEL_STATE_DISABLED;
}

void HandleAccelerometer(tMessage *pMsg)
{
  if (AccelState == ACCEL_STATE_UNKNOWN) InitAccelerometer();

  switch (pMsg->Type)
  {
  case EnableAccelerometerMsg:
    if (AccelState == ACCEL_STATE_DISABLED) EnableAccelerometer();
    break;

  case DisableAccelerometerMsg:
    if (AccelState == ACCEL_STATE_ENABLED) DisableAccelerometer();
    break;

  case AccelerometerSendDataMsg:
    AccelerometerSendDataHandler();
    break;

  case AccelerometerAccessMsg:
    AccelerometerAccessHandler(pMsg);
    break;

  case AccelerometerSetupMsg:
    AccelerometerSetupHandler(pMsg);
    break;

  default:
    break;
  }
}

/*
 * Control how the msp430 responds to an interrupt,
 * control function of EnableAccelerometerMsg,
 * and allow enabling and disabling interrupt in msp430
 */
static void AccelerometerSetupHandler(tMessage* pMsg)
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
    if (pMsg->pBuffer[0] == 0) {ACCELEROMETER_INT_DISABLE();}
    else {ACCELEROMETER_INT_ENABLE();}
    break;
    
  default:
    break;
  }
}

/* Perform a read or write access of the accelerometer */
static void AccelerometerAccessHandler(tMessage* pMsg)
{
  tAccelerometerAccessPayload* pPayload = 
    (tAccelerometerAccessPayload*) pMsg->pBuffer;

  if ( pMsg->Options == ACCELEROMETER_ACCESS_WRITE_OPTION )
  {
    AccelerometerWrite(pPayload->Address,&pPayload->Data,pPayload->Size);
  }
  else
  {
    tMessage Msg;
    SetupMessageAndAllocateBuffer(&Msg,
                                  AccelerometerResponseMsg,
                                  pPayload->Size);
    
    AccelerometerRead(pPayload->Address,
                      &Msg.pBuffer[ACCELEROMETER_DATA_START_INDEX],
                      pPayload->Size);  
  }
}


/* This interrupt port is used by the Bluetooth stack.
 * Do not change the name of this function because it is externed.
 */
void AccelerometerPinIsr(void)
{
  AccelerometerIsr();
}
