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
/*! \file Background.c
*
* The background task handles everything that does not belong with a specialized
* task.
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "Messages.h"
#include "BufferPool.h"     
#include "MessageQueues.h"

#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_analog_display.h"
#include "hal_battery.h"
#include "hal_miscellaneous.h"
#include "hal_clock_control.h"
#include "hal_lpm.h"

#include "Buttons.h"
#include "Background.h"
#include "Utilities.h"     
#include "SerialProfile.h"
#include "Adc.h"
#include "OneSecondTimers.h"
#include "Vibration.h"
#include "DebugUart.h"
#include "Statistics.h"
#include "OSAL_Nv.h"
#include "NvIds.h"

static void BackgroundTask(void *pvParameters);

static void BackgroundMessageHandler(tHostMsg* pMsg);

static void AdvanceWatchHandsHandler(tHostMsg* pMsg);
static void EnableButtonMsgHandler(tHostMsg* pMsg);
static void DisableButtonMsgHandler(tHostMsg* pMsg);
static void ReadButtonConfigHandler(tHostMsg* pMsg);
static void ReadBatteryVoltageHandler(tHostMsg* pMsg);
static void ReadLightSensorHandler(tHostMsg* pMsg);
static void NvalOperationHandler(tHostMsg* pMsg);
static void SoftwareResetHandler(tHostMsg* pMsg);

#define BACKGROUND_MSG_QUEUE_LEN   8    
#define BACKGROUND_STACK_DEPTH	    (configMINIMAL_STACK_DEPTH + 50)
#define BACKGROUND_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)

xTaskHandle xBkgTaskHandle;

static tHostMsg* pBackgroundMsg;

static tTimerId BatteryMonitorTimerId;
static void InitializeBatteryMonitorInterval(void);

static unsigned int nvBatteryMonitorIntervalInSeconds;

static unsigned char LedOn;
static tTimerId LedTimerId;
static void LedChangeHandler(tHostMsg* pMsg);

/******************************************************************************/

/* externed with hal_lpm */
unsigned char nvRstNmiConfiguration;
static void InitializeRstNmiConfiguration(void);

/******************************************************************************/

/*! Does the initialization and allocates the resources for the background task
 *
 */
void InitializeBackgroundTask( void )
{
  // This is a Rx message queue, messages come from Serial IO or button presses
  QueueHandles[BACKGROUND_QINDEX] = 
    xQueueCreate( BACKGROUND_MSG_QUEUE_LEN, MESSAGE_QUEUE_ITEM_SIZE );
  
  // prams are: task function, task name, stack len , task params, priority, task handle
  xTaskCreate(BackgroundTask, 
              "BACKGROUND", 
              BACKGROUND_STACK_DEPTH, 
              NULL, 
              BACKGROUND_TASK_PRIORITY, 
              &xBkgTaskHandle);

}


/*! Function to implement the BackgroundTask loop
 *
 * \param pvParameters not used
 *
 */
static void BackgroundTask(void *pvParameters)
{
  if ( QueueHandles[BACKGROUND_QINDEX] == 0 )
  {
    PrintString("Background Queue not created!\r\n");
  }
  
  PrintString(SPP_DEVICE_NAME);
  PrintString2("\r\nSoftware Version ",VERSION_STRING);
  PrintString("\r\n\r\n");
  
  InitializeRstNmiConfiguration();
  
  /*
   * check on the battery
   */
  ConfigureBatteryPins();
  BatteryChargingControl();
  BatterySenseCycle();
    
  /*
   * now set up a timer that will cause the battery to be checked at
   * a regular frequency.
   */
  BatteryMonitorTimerId = AllocateOneSecondTimer();

  InitializeBatteryMonitorInterval();

  SetupOneSecondTimer(BatteryMonitorTimerId,
                      nvBatteryMonitorIntervalInSeconds,
                      REPEAT_FOREVER,
                      BatteryChargeControl,
                      NO_MSG_OPTIONS);
  
  StartOneSecondTimer(BatteryMonitorTimerId);
  
  /*
   * Setup a timer to use with the LED for the LCD.
   */
  LedTimerId = AllocateOneSecondTimer();
  
  SetupOneSecondTimer(LedTimerId,
                      ONE_SECOND*3,
                      NO_REPEAT,
                      LedChange,
                      LED_OFF_OPTION);

#if 0
  BPL_AllocMessageBuffer(&pBackgroundMsg);
  UTL_BuildHstMsg(pBackgroundMsg,GetDeviceType,NO_MSG_OPTIONS,
                  pBackgroundMsg->pPayload,0);
  RouteMsg(&pBackgroundMsg);
  
  BPL_AllocMessageBuffer(&pBackgroundMsg);  
  UTL_BuildHstMsg(pBackgroundMsg,GetRealTimeClock,NO_MSG_OPTIONS,
                  pBackgroundMsg->pPayload,0);
  RouteMsg(&pBackgroundMsg);
#endif
  
  for(;;)
  {
    if( pdTRUE == xQueueReceive(QueueHandles[BACKGROUND_QINDEX], 
                                &pBackgroundMsg, portMAX_DELAY ) )
    {
      BackgroundMessageHandler(pBackgroundMsg);
      
      BPL_FreeMessageBuffer(&pBackgroundMsg);
      
      CheckStackUsage(xBkgTaskHandle,"Background Task");

    }

  }

}

/*! Handle the messages routed to the background task */
static void BackgroundMessageHandler(tHostMsg* pMsg)
{  
  tHostMsg* pOutgoingMsg;    

  eMessageType Type = (eMessageType)pMsg->Type;
      
  switch(Type)
  {
  case GetDeviceType:
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    
    pOutgoingMsg->pPayload[0] = BOARD_TYPE;
  
    UTL_BuildHstMsg(pOutgoingMsg,GetDeviceTypeResponse,NO_MSG_OPTIONS,
                    pOutgoingMsg->pPayload,sizeof(unsigned char));
    
    RouteMsg(&pOutgoingMsg);
    break;

  case AdvanceWatchHandsMsg:
    AdvanceWatchHandsHandler(pMsg);
    break;
 
  case SetVibrateMode:
    SetVibrateModeHandler(pMsg);
    break;
    
  case SetRealTimeClock:
    halRtcSet((tRtcHostMsgPayload*)pMsg->pPayload);
    
#ifdef DIGITAL
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    pOutgoingMsg->Type = IdleUpdate;
    pOutgoingMsg->Options = NO_MSG_OPTIONS;
    RouteMsg(&pOutgoingMsg);
#endif
    break;
  
  case GetRealTimeClock:
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    halRtcGet((tRtcHostMsgPayload*)pOutgoingMsg->pPayload);
    
    UTL_BuildHstMsg(pOutgoingMsg,GetRealTimeClockResponse,NO_MSG_OPTIONS,
                    pOutgoingMsg->pPayload,sizeof(tRtcHostMsgPayload));
    
    RouteMsg(&pOutgoingMsg);
    break;

  case EnableButtonMsg:
    EnableButtonMsgHandler(pMsg);
    break;
  
  case DisableButtonMsg:
    DisableButtonMsgHandler(pMsg);
    break;

  case ReadButtonConfigMsg:
    ReadButtonConfigHandler(pMsg);
    break;
 
  case BatteryChargeControl:
    
#ifdef DIGITAL
    /* update the screen if there has been a change in charging status */
    if ( BatteryChargingControl() )
    {
      BPL_AllocMessageBuffer(&pOutgoingMsg);
      pOutgoingMsg->Type = IdleUpdate;
      RouteMsg(&pOutgoingMsg);   
    }
#endif 
    
    BatterySenseCycle();
    LowBatteryMonitor();
#ifdef TASK_DEBUG
    UTL_FreeRtosTaskStackCheck();
#endif
    LightSenseCycle();
    break;

  case LedChange:
    LedChangeHandler(pMsg);
    break;

  case BatteryConfigMsg:
    SetBatteryLevels(pMsg->pPayload);
    break;
    
  case ReadBatteryVoltageMsg:
    ReadBatteryVoltageHandler(pMsg);
    break;

  case ReadLightSensorMsg:
    ReadLightSensorHandler(pMsg);
    break;
    
  case SoftwareResetMsg:
    SoftwareResetHandler(pMsg);
    break;

  case NvalOperationMsg:
    NvalOperationHandler(pMsg);
    break;
    
  case GeneralPurposeWatchMsg:
    /* insert handler here */
    break;
    
  default:
    PrintStringAndHex("<<Unhandled Message>> in Background Task: Type 0x", Type);
    break;
  }

}

/*! Handle the AdvanceWatchHands message
 *
 * The AdvanceWatchHands message specifies the hours, min, and sec to advance
 * the analog watch hands.
 *
 */
static void AdvanceWatchHandsHandler(tHostMsg* pMsg)
{
#ifdef ANALOG
  // overlay a structure pointer on the data section
  tAdvanceWatchHandsPayload* pPayload;
  pPayload = (tAdvanceWatchHandsPayload*) pMsg->pPayload;

  if ( pPayload->Hours <= 12 )
  {
    // the message values are bytes and we are computing a 16 bit value
    unsigned int numSeconds;
    numSeconds =  (unsigned int) pPayload->Seconds;
    numSeconds += (unsigned int)(pPayload->Minutes) * 60;
    numSeconds += (unsigned int)(pPayload->Hours) * 3600;

    // set the analog watch timer to fast mode to increment the number of
    // extra seconds we specify.  The resolution is 10 seconds.  The update
    // will automatically stop when completed
    AdvanceAnalogDisplay(numSeconds);
  }
#endif
}



/*! Led Change Handler
 *
 * \param tHostMsg* pMsg The message options contain the type of operation that
 * should be performed on the LED outout.
 */
static void LedChangeHandler(tHostMsg* pMsg)
{
  switch (pMsg->Options)
  {
  case LED_ON_OPTION:
    LedOn = 1;
    ENABLE_LCD_LED();
    StartOneSecondTimer(LedTimerId);
    break;
    
  case LED_TOGGLE_OPTION:
    if ( LedOn )
    {
      LedOn = 0;
      DISABLE_LCD_LED();
      StopOneSecondTimer(LedTimerId);  
    }
    else
    {
      LedOn = 1;
      ENABLE_LCD_LED();
      StartOneSecondTimer(LedTimerId);
    }
    break;
    
  case LED_START_OFF_TIMER:
    LedOn = 1;
    ENABLE_LCD_LED();
    StartOneSecondTimer(LedTimerId);
    break;
    
  case LED_OFF_OPTION:
  default:
    LedOn = 0;
    DISABLE_LCD_LED();
    StopOneSecondTimer(LedTimerId);  
    break;
    
  }
  
}

/*! Attach callback to button press type. Each button press type is associated
 * with a display mode.
 *
 * No error checking
 *
 * \param tHostMsg* pMsg - A message with a tButtonActionPayload payload
 */
static void EnableButtonMsgHandler(tHostMsg* pMsg)
{
  tButtonActionPayload* pButtonActionPayload = (tButtonActionPayload*)pMsg->pPayload;  

  EnableButtonAction(pButtonActionPayload->DisplayMode,
                     pButtonActionPayload->ButtonIndex,
                     pButtonActionPayload->ButtonPressType,
                     pButtonActionPayload->CallbackMsgType,
                     pButtonActionPayload->CallbackMsgOptions);
  
}

/*! Remove callback for the specified button press type. 
 * Each button press type is associated with a display mode.
 *
 * \param tHostMsg* pMsg - A message with a tButtonActionPayload payload
 */
static void DisableButtonMsgHandler(tHostMsg* pMsg)
{
  tButtonActionPayload* pButtonActionPayload = (tButtonActionPayload*)pMsg->pPayload;  
  
  DisableButtonAction(pButtonActionPayload->DisplayMode,
                      pButtonActionPayload->ButtonIndex,
                      pButtonActionPayload->ButtonPressType);
  
}

/*! Read configuration of a specified button.  This is used to read the 
 * configuration of a button that needs to be restored at a later time 
 * by the application.
 *
 * \param tHostMsg* pMsg - A message with a tButtonActionPayload payload
 */
static void ReadButtonConfigHandler(tHostMsg* pMsg)
{
  tButtonActionPayload* pButtonActionPayload = (tButtonActionPayload*)pMsg->pPayload;  
  
  tHostMsg* pOutgoingMsg;
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  
  ReadButtonConfiguration(pButtonActionPayload->DisplayMode,
                          pButtonActionPayload->ButtonIndex,
                          pButtonActionPayload->ButtonPressType,
                          pOutgoingMsg->pPayload);
  
  UTL_BuildHstMsg(pOutgoingMsg,ReadButtonConfigResponse,NO_MSG_OPTIONS,
                  pOutgoingMsg->pPayload,5);
  
  RouteMsg(&pOutgoingMsg);
  
}

/*! Read the voltage of the battery. This provides power good, battery charging, 
 * battery voltage, and battery voltage average.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
static void ReadBatteryVoltageHandler(tHostMsg* pMsg)
{
  tHostMsg* pOutgoingMsg;
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  
  /* if the battery is not present then these values are meaningless */
  pOutgoingMsg->pPayload[0] = QueryPowerGood();
  pOutgoingMsg->pPayload[1] = QueryBatteryCharging();
  
  unsigned int bv = ReadBatterySense();
  pOutgoingMsg->pPayload[2] = bv & 0xFF;
  pOutgoingMsg->pPayload[3] = (bv >> 8 ) & 0xFF;
  
  bv = ReadBatterySenseAverage();
  pOutgoingMsg->pPayload[4] = bv & 0xFF;
  pOutgoingMsg->pPayload[5] = (bv >> 8 ) & 0xFF;

  UTL_BuildHstMsg(pOutgoingMsg,ReadBatteryVoltageResponse,NO_MSG_OPTIONS,
                  pOutgoingMsg->pPayload,6);
  
  RouteMsg(&pOutgoingMsg);
  
}

/*! Initiate a light sensor cycle.  Then send the instantaneous and average
 * light sense values to the host.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
static void ReadLightSensorHandler(tHostMsg* pMsg)
{
  /* start cycle and wait for it to finish */
  LightSenseCycle();
  
  /* send message to the host */
  tHostMsg* pOutgoingMsg;
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  
  /* instantaneous value */
  unsigned int lv = ReadLightSense();
  pOutgoingMsg->pPayload[0] = lv & 0xFF;
  pOutgoingMsg->pPayload[1] = (lv >> 8 ) & 0xFF;

  /* average value */
  lv = ReadLightSenseAverage();
  pOutgoingMsg->pPayload[2] = lv & 0xFF;
  pOutgoingMsg->pPayload[3] = (lv >> 8 ) & 0xFF;

  UTL_BuildHstMsg(pOutgoingMsg,ReadLightSensorResponse,NO_MSG_OPTIONS,
                  pOutgoingMsg->pPayload,4);
  
  RouteMsg(&pOutgoingMsg);
  
}

/*! Setup the battery monitor interval - only happens at startup */
static void InitializeBatteryMonitorInterval(void)
{
  nvBatteryMonitorIntervalInSeconds = 8;

  OsalNvItemInit(NVID_BATTERY_SENSE_INTERVAL, 
                 sizeof(nvBatteryMonitorIntervalInSeconds), 
                 &nvBatteryMonitorIntervalInSeconds);
  
}

/* choose whether or not to do a master reset (reset non-volatile values) */
static void SoftwareResetHandler(tHostMsg* pMsg)
{
  if ( pMsg->Options == MASTER_RESET_OPTION )
  {
    WriteMasterResetKey();
  }
  
  SoftwareReset();
  
}

static void NvalOperationHandler(tHostMsg* pMsg)
{
  /* overlay */
  tNvalOperationPayload* pNvPayload = (tNvalOperationPayload*)pMsg->pPayload;  

  /* create the outgoing message */
  tHostMsg* pOutgoingMsg;
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  pOutgoingMsg->Options = NV_FAILURE;
  pOutgoingMsg->Type = NvalOperationResponseMsg;
  /* add identifier to outgoing message */
  tWordByteUnion Identifier;
  Identifier.word = pNvPayload->NvalIdentifier;
  pOutgoingMsg->pPayload[0] = Identifier.byte0;
  pOutgoingMsg->pPayload[1] = Identifier.byte1;
  pOutgoingMsg->Length = 2;
  
  /* option byte in return message is status */
  switch (pMsg->Options)
  {
    
  case NVAL_INIT_OPERATION:
    /* may allow access to a specific range of nval ids that
     * the phone can initialize and use
     */
    break;
    
  case NVAL_READ_OPERATION:
    
    pOutgoingMsg->Options = osal_nv_read(pNvPayload->NvalIdentifier,
                                         NV_ZERO_OFFSET,
                                         pNvPayload->Size,
                                         &pOutgoingMsg->pPayload[2]);
    
    pOutgoingMsg->Length += pNvPayload->Size;
    
    break;
  
  case NVAL_WRITE_OPERATION:
    /* check that the size matches (otherwise NV_FAILURE is sent) */
    if ( osal_nv_item_len(pNvPayload->NvalIdentifier) == pNvPayload->Size )
    {
      pOutgoingMsg->Options = osal_nv_write(pNvPayload->NvalIdentifier,
                                            NV_ZERO_OFFSET,
                                            pNvPayload->Size,
                                            (void*)(&pNvPayload->DataStartByte));
    }
     
    break;
  
  default:
    break;
  }
  
  UTL_PrepareHstMsg(pOutgoingMsg);
  RouteMsg(&pOutgoingMsg);
  
}


void InitializeRstNmiConfiguration(void)
{
  /****************************************************************************/
  
  nvRstNmiConfiguration = RST_PIN_DISABLED;
  OsalNvItemInit(NVID_RSTNMI_CONFIGURATION, 
                 sizeof(nvRstNmiConfiguration), 
                 &nvRstNmiConfiguration);
  
  ConfigureResetPinFunction(nvRstNmiConfiguration);

} 


void SaveRstNmiConfiguration(void)
{
  osal_nv_write(NVID_RSTNMI_CONFIGURATION,
                NV_ZERO_OFFSET,
                sizeof(nvRstNmiConfiguration),
                &nvRstNmiConfiguration);  
}

