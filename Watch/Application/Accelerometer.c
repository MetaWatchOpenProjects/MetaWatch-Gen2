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
#define MSG_OPT_ACCEL_DISABLE           (0)
#define MSG_OPT_ACCEL_ENABLE            (1)
#define MSG_OPT_ACCEL_STREAMING         (2)
#define MSG_OPT_ACCEL_WUF               (3)
#define MSG_OPT_ACCEL_G_RANGE           (4)
#define MSG_OPT_ACCEL_WUF_THRESHOLD     (5)
#define MSG_OPT_ACCEL_DATA              (0xFF)

#define MSG_OPT_ACCEL_WUF_DELAY         (7)
#define MSG_OPT_ACCEL_READ              (8)

#define ACCELEROMETER_POWER_UP_TIME_MS  (20 * portTICK_RATE_MS) //ms
#define XYZ_DATA_LENGTH                 (6)
#define PROOF_READ_CODE                 (0x55)
#define RANGE_8G                        (0x10)
#define WUF_DEFAULT_THRESHOLD           (0x08)
#define WUF_DEFAULT_DELAY_COUNT         (5)
#define WUF_DELAY_PER_COUNT             (40) //25Hz, 40ms/count
#define ACCEL_RANGE_MASK                (0x18)
#define ACCEL_RANGE_SHFT                (3)
#define INIT_MODE                       (0)

#define ACCEL_CONTROL_DEFAULT_BASE (RESOLUTION_8BIT | RANGE_8G)
#define ACCEL_CONTROL_DEFAULT_WUF (ACCEL_CONTROL_DEFAULT_BASE | WUF_ENABLE)
#define ACCEL_CONTROL_DEFAULT_DRDYE (ACCEL_CONTROL_DEFAULT_BASE | DRDYE_DATA_AVAILABLE)
#define ACCEL_CONTROL_DEFAULT_TPE (ACCEL_CONTROL_DEFAULT_BASE | TILT_ENABLE_TPE)

static unsigned char Data[6];
static unsigned char Control = INIT_MODE;

#define ENTER_STANDBY_MODE() {                              \
  Control &= ~PC1_OPERATING_MODE;                           \
  AccelerometerWrite(KIONIX_CTRL_REG1, &Control, ONE_BYTE); \
}

#define ENTER_OPERATING_MODE() {                            \
  Control |= PC1_OPERATING_MODE;                            \
  AccelerometerWrite(KIONIX_CTRL_REG1, &Control, ONE_BYTE); \
}

/******************************************************************************/
static void InitAccelerometer(void);
static void AccelerometerSendDataHandler(void);
static void EnableAccelerometer(void);
static void DisableAccelerometer(void);

//  /* 180 uA; KTXI9 115 uA */
//  *Data = PC1_OPERATING_MODE | RESOLUTION_8BIT | WUF_ENABLE;
//
//  /* 720 uA; KTXI9 330 uA */  
//  *Data = PC1_OPERATING_MODE | RESOLUTION_12BIT | WUF_ENABLE;
//
//  /* 180 uA; KTXI9 8.7 uA */
//  *Data = PC1_OPERATING_MODE | TILT_ENABLE_TPE;
//
//  /* KTXF9 300 uA; KTXI9 165 uA */
//  *Data = PC1_OPERATING_MODE | TAP_ENABLE_TDTE;  
  
static void InitAccelerometer(void)
{
  InitAccelerometerPeripheral();

  /* make sure accelerometer has had 20 ms to power up */
  vTaskDelay(ACCELEROMETER_POWER_UP_TIME_MS);

  Control = ACCEL_CONTROL_DEFAULT_TPE;
  AccelerometerWrite(KIONIX_CTRL_REG1, &Control, ONE_BYTE);
    
  /* change to output data rate to 25 Hz */
  *Data = WUF_ODR_25HZ;
  AccelerometerWrite(KIONIX_CTRL_REG3, Data, ONE_BYTE);
  
  /* enable interrupt and make it active high */
  *Data = IEN | IEA;
  AccelerometerWrite(KIONIX_INT_CTRL_REG1, Data, ONE_BYTE);
  
  /* enable motion detection interrupt for all three axis */
  *Data = ZBW;
  AccelerometerWrite(KIONIX_INT_CTRL_REG2, Data, ONE_BYTE);

  /* enable motion detection interrupt for all three axis */
  *Data = ZBW;
  AccelerometerWrite(KIONIX_INT_CTRL_REG2, Data, ONE_BYTE);

  /* WUF delay = COUNT * (1/WUF_ODR_25HZ)  */
  *Data = WUF_DEFAULT_DELAY_COUNT;
  AccelerometerWrite(KIONIX_WUF_TIMER, Data, ONE_BYTE);
    
  /* 0.5g = 0x08 */
  *Data = WUF_DEFAULT_THRESHOLD;
  AccelerometerWrite(KIONIX_WUF_THRESH, Data, ONE_BYTE);
     
  /* Make sure HW is functioning */
  AccelerometerRead(KIONIX_DCST_RESP, Data, ONE_BYTE);
  PrintF("%s Accel Initd", *Data == PROOF_READ_CODE ? OK : NOK);
}

/* Send interrupt notification to the phone or 
 * read data from the accelerometer and send it to the phone
 */
static void AccelerometerSendDataHandler(void)
{
  tMessage Msg;

  SetupMessageWithBuffer(&Msg, AccelIndMsg, MSG_OPT_NONE);
  if (Msg.pBuffer != NULL)
  {
    if (Control & DRDYE_DATA_AVAILABLE || Control & WUF_ENABLE)
    {
      AccelerometerRead(Control & DRDYE_DATA_AVAILABLE ? KIONIX_XOUT_L : KIONIX_XOUT_HPF_L,
        Data, XYZ_DATA_LENGTH);

      Msg.pBuffer[0] = Data[1];
      Msg.pBuffer[1] = Data[3];
      Msg.pBuffer[2] = Data[5];
      Msg.Length = 3;
      RouteMsg(&Msg);

      PrintH(Msg.pBuffer[0]); PrintC(SPACE);
      PrintH(Msg.pBuffer[1]); PrintC(SPACE);
      PrintH(Msg.pBuffer[2]); PrintR();
    }
    else if (Control & TILT_ENABLE_TPE)
    {
      AccelerometerRead(KIONIX_TILT_POS_CUR, Msg.pBuffer, ONE_BYTE);
      Msg.Length = 1;
      RouteMsg(&Msg);
      PrintH(*Msg.pBuffer); PrintR();
    }

    AccelerometerRead(KIONIX_INT_REL, Data, ONE_BYTE); //clear int
  }
}

static void EnableAccelerometer(void)
{
  ENTER_OPERATING_MODE();
  ACCELEROMETER_INT_ENABLE();
}

static void DisableAccelerometer(void)
{   
  /* put into low power mode */
  ACCELEROMETER_INT_DISABLE();
  ENTER_STANDBY_MODE();
}

void HandleAccelerometer(tMessage *pMsg)
{
  if (Control == INIT_MODE) InitAccelerometer();

  switch (pMsg->Options)
  {
  case MSG_OPT_ACCEL_DATA:
    AccelerometerSendDataHandler();
    break;

  case MSG_OPT_ACCEL_ENABLE:

    if (Connected(CONN_TYPE_BLE))
      CreateAndSendMessage(UpdConnParamMsg, ShortInterval);

    else if (Connected(CONN_TYPE_SPP))
      CreateAndSendMessage(SniffControlMsg, MSG_OPT_EXIT_SNIFF);

    EnableAccelerometer();
    break;

  case MSG_OPT_ACCEL_DISABLE:
    DisableAccelerometer();
    CreateAndSendMessage(UpdConnParamMsg, LongInterval);
    break;

  case MSG_OPT_ACCEL_STREAMING:
  
    ENTER_STANDBY_MODE();
    Control &= ~WUF_ENABLE;
    Control |= DRDYE_DATA_AVAILABLE;
    AccelerometerWrite(KIONIX_CTRL_REG1, &Control, ONE_BYTE);
    ENTER_OPERATING_MODE();
    break;

  case MSG_OPT_ACCEL_WUF:
  
    ENTER_STANDBY_MODE();
    Control &= ~DRDYE_DATA_AVAILABLE;
    Control |= WUF_ENABLE;
    AccelerometerWrite(KIONIX_CTRL_REG1, &Control, ONE_BYTE);
    ENTER_OPERATING_MODE();
    break;

  case MSG_OPT_ACCEL_WUF_THRESHOLD:

    ENTER_STANDBY_MODE();
    AccelerometerWrite(KIONIX_WUF_THRESH, pMsg->pBuffer, ONE_BYTE);
    ENTER_OPERATING_MODE();
    break;

  case MSG_OPT_ACCEL_G_RANGE:
  
    ENTER_STANDBY_MODE();
    Control &= ~ACCEL_RANGE_MASK;
    Control |= (*pMsg->pBuffer & 0x03) << ACCEL_RANGE_SHFT;
    AccelerometerWrite(KIONIX_CTRL_REG1, &Control, ONE_BYTE);
    ENTER_OPERATING_MODE();
    break;

  default:
    break;
  }
}
/* This interrupt port is used by the Bluetooth stack.
 * Do not change the name of this function because it is externed.
 */
/*
 * The interrupt can either send a message to the host or
 * it can send data (send a message that causes the task to read data from 
 * part and then send it to the host).
 */
void AccelerometerPinIsr(void)
{
  tMessage Msg;
  SetupMessage(&Msg, AccelMsg, MSG_OPT_ACCEL_DATA);  
  SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
}
