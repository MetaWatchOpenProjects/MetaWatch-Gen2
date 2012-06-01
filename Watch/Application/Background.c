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
#include "MessageQueues.h"

#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_analog_display.h"
#include "hal_battery.h"
#include "hal_miscellaneous.h"
#include "hal_clock_control.h"
#include "hal_lpm.h"
#include "hal_crystal_timers.h"

#include "Buttons.h"
#include "Background.h"
#include "DebugUart.h"
#include "Utilities.h"
#include "SerialProfile.h"
#include "Adc.h"
#include "OneSecondTimers.h"
#include "Vibration.h"
#include "Statistics.h"
#include "OSAL_Nv.h"
#include "NvIds.h"
#include "Display.h"
#include "LcdDisplay.h"
#include "OledDriver.h"
#include "OledDisplay.h"
#include "Accelerometer.h"

static void BackgroundTask(void *pvParameters);

static void BackgroundMessageHandler(tMessage* pMsg);

static void AdvanceWatchHandsHandler(tMessage* pMsg);
static void EnableButtonMsgHandler(tMessage* pMsg);
static void DisableButtonMsgHandler(tMessage* pMsg);
static void ReadButtonConfigHandler(tMessage* pMsg);
static void ReadBatteryVoltageHandler(void);
static void ReadLightSensorHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void SoftwareResetHandler(tMessage* pMsg);
static void SetCallbackTimerHandler(tMessage* pMsg);

#define BACKGROUND_MSG_QUEUE_LEN   8
#define BACKGROUND_STACK_SIZE	   (configMINIMAL_STACK_SIZE + 100)
#define BACKGROUND_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)

xTaskHandle xBkgTaskHandle;

static tMessage BackgroundMsg;

static tTimerId BatteryMonitorTimerId;
static void InitializeBatteryMonitorInterval(void);

static unsigned int nvBatteryMonitorIntervalInSeconds;

static unsigned char LedOn;
static tTimerId LedTimerId;
static void LedChangeHandler(tMessage* pMsg);
static tTimerId CallbackTimerId;

/******************************************************************************/

/* externed with hal_lpm */
unsigned char nvRstNmiConfiguration;
static void InitializeRstNmiConfiguration(void);

/******************************************************************************/

static void NvUpdater(unsigned int NvId);

/******************************************************************************/

#ifdef RAM_TEST

static tTimerId RamTestTimerId;

#endif

/******************************************************************************/

#ifdef RATE_TEST

static unsigned char RateTestCallback(void);

#define RATE_TEST_INTERVAL_MS ( 1000 )

#endif

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
              (const signed char *)"BACKGROUND",
              BACKGROUND_STACK_SIZE,
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
                      BACKGROUND_QINDEX,
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
                      BACKGROUND_QINDEX,
                      LedChange,
                      LED_OFF_OPTION);

  // Allocate a timer for wake-up iOS background BLE app
  CallbackTimerId = AllocateOneSecondTimer();

  /****************************************************************************/

#ifdef RAM_TEST

  RamTestTimerId = AllocateOneSecondTimer();

  SetupOneSecondTimer(RamTestTimerId,
                      ONE_SECOND*20,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      RamTestMsg,
                      NO_MSG_OPTIONS);

  StartOneSecondTimer(RamTestTimerId);

#endif

  /****************************************************************************/

  InitializeAccelerometer();

#ifdef ACCELEROMETER_DEBUG

  SetupMessageAndAllocateBuffer(&BackgroundMsg,
                                AccelerometerSetupMsg,
                                ACCELEROMETER_SETUP_INTERRUPT_CONTROL_OPTION);

  BackgroundMsg.pBuffer[0] = INTERRUPT_CONTROL_ENABLE_INTERRUPT;
  BackgroundMsg.Length = 1;
  RouteMsg(&BackgroundMsg);

  /* don't call AccelerometerEnable() directly use a message*/
  SetupMessage(&BackgroundMsg,AccelerometerEnableMsg,NO_MSG_OPTIONS);
  RouteMsg(&BackgroundMsg);

#endif

  /****************************************************************************/

#ifdef RATE_TEST

  StartCrystalTimer(CRYSTAL_TIMER_ID3,RateTestCallback,RATE_TEST_INTERVAL_MS);

#endif

  /****************************************************************************/

  for(;;)
  {
    if( pdTRUE == xQueueReceive(QueueHandles[BACKGROUND_QINDEX],
                                &BackgroundMsg, portMAX_DELAY ) )
    {
      PrintMessageType(&BackgroundMsg);

      BackgroundMessageHandler(&BackgroundMsg);

      SendToFreeQueue(&BackgroundMsg);

      CheckStackUsage(xBkgTaskHandle,"Background Task");

      CheckQueueUsage(QueueHandles[BACKGROUND_QINDEX]);

    }

  }

}

/*! Handle the messages routed to the background task */
static void BackgroundMessageHandler(tMessage* pMsg)
{
  tMessage OutgoingMsg;

  switch(pMsg->Type)
  {
    case SetCallbackTimerMsg:
      SetCallbackTimerHandler(pMsg);
      break;

  case GetDeviceType:

    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  GetDeviceTypeResponse,
                                  NO_MSG_OPTIONS);

    OutgoingMsg.pBuffer[0] = BOARD_TYPE;
    OutgoingMsg.Length = 1;
    RouteMsg(&OutgoingMsg);

    break;

  case AdvanceWatchHandsMsg:
    AdvanceWatchHandsHandler(pMsg);
    break;

  case SetVibrateMode:
    SetVibrateModeHandler(pMsg);
    break;

  case SetRealTimeClock:
    halRtcSet((tRtcHostMsgPayload*)pMsg->pBuffer);

#ifdef DIGITAL
    SetupMessage(&OutgoingMsg,IdleUpdate,NO_MSG_OPTIONS);
    RouteMsg(&OutgoingMsg);
#endif
    break;

  case GetRealTimeClock:

    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  GetRealTimeClockResponse,
                                  NO_MSG_OPTIONS);

    halRtcGet((tRtcHostMsgPayload*)OutgoingMsg.pBuffer);
    OutgoingMsg.Length = sizeof(tRtcHostMsgPayload);
    RouteMsg(&OutgoingMsg);
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
      SetupMessage(&OutgoingMsg,IdleUpdate,NO_MSG_OPTIONS);
      RouteMsg(&OutgoingMsg);
    }
#endif

    BatterySenseCycle();
    LowBatteryMonitor();

#ifdef TASK_DEBUG
    UTL_FreeRtosTaskStackCheck();
#endif

#if 0
    LightSenseCycle();
#endif

    break;

  case LedChange:
    LedChangeHandler(pMsg);
    break;

  case BatteryConfigMsg:
    SetBatteryLevels(pMsg->pBuffer);
    break;

  case ReadBatteryVoltageMsg:
    ReadBatteryVoltageHandler();
    break;

  case ReadLightSensorMsg:
    ReadLightSensorHandler();
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

  case ButtonStateMsg:
    ButtonStateHandler();
    break;

  /*
   * Accelerometer Messages
   */
  case AccelerometerEnableMsg:
    AccelerometerEnable();
    break;

  case AccelerometerDisableMsg:
    AccelerometerDisable();
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

  /*
   *
   */
  case RateTestMsg:
    SetupMessageAndAllocateBuffer(&OutgoingMsg,DiagnosticLoopback,NO_MSG_OPTIONS);
    /* don't care what data is */
    OutgoingMsg.Length = 10;
    RouteMsg(&OutgoingMsg);
    break;

  /*
   *
   */
  default:
    PrintStringAndHex("<<Unhandled Message>> in Background Task: Type 0x", pMsg->Type);
    break;
  }

}

/*! Handle the AdvanceWatchHands message
 *
 * The AdvanceWatchHands message specifies the hours, min, and sec to advance
 * the analog watch hands.
 *
 */
static void AdvanceWatchHandsHandler(tMessage* pMsg)
{
#ifdef ANALOG
  // overlay a structure pointer on the data section
  tAdvanceWatchHandsPayload* pPayload;
  pPayload = (tAdvanceWatchHandsPayload*) pMsg->pBuffer;

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
static void LedChangeHandler(tMessage* pMsg)
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
static void EnableButtonMsgHandler(tMessage* pMsg)
{
  tButtonActionPayload* pButtonActionPayload =
    (tButtonActionPayload*)pMsg->pBuffer;

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
static void DisableButtonMsgHandler(tMessage* pMsg)
{
  tButtonActionPayload* pButtonActionPayload =
    (tButtonActionPayload*)pMsg->pBuffer;

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
static void ReadButtonConfigHandler(tMessage* pMsg)
{
  /* map incoming message payload to button information */
  tButtonActionPayload* pButtonActionPayload =
    (tButtonActionPayload*)pMsg->pBuffer;

  tMessage OutgoingMsg;
  SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                ReadButtonConfigResponse,
                                NO_MSG_OPTIONS);

  ReadButtonConfiguration(pButtonActionPayload->DisplayMode,
                          pButtonActionPayload->ButtonIndex,
                          pButtonActionPayload->ButtonPressType,
                          OutgoingMsg.pBuffer);

  OutgoingMsg.Length = 5;

  RouteMsg(&OutgoingMsg);

}

/*! Read the voltage of the battery. This provides power good, battery charging,
 * battery voltage, and battery voltage average.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
static void ReadBatteryVoltageHandler(void)
{
  tMessage OutgoingMsg;
  SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                ReadBatteryVoltageResponse,
                                NO_MSG_OPTIONS);

  /* if the battery is not present then these values are meaningless */
  OutgoingMsg.pBuffer[0] = QueryPowerGood();
  OutgoingMsg.pBuffer[1] = QueryBatteryCharging();

  unsigned int bv = ReadBatterySense();
  OutgoingMsg.pBuffer[2] = bv & 0xFF;
  OutgoingMsg.pBuffer[3] = (bv >> 8 ) & 0xFF;

  bv = ReadBatterySenseAverage();
  OutgoingMsg.pBuffer[4] = bv & 0xFF;
  OutgoingMsg.pBuffer[5] = (bv >> 8 ) & 0xFF;

  OutgoingMsg.Length = 6;

  RouteMsg(&OutgoingMsg);

}

/*! Initiate a light sensor cycle.  Then send the instantaneous and average
 * light sense values to the host.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
static void ReadLightSensorHandler(void)
{
  /* start cycle and wait for it to finish */
  LightSenseCycle();

  /* send message to the host */
  tMessage OutgoingMsg;
  SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                ReadLightSensorResponse,
                                NO_MSG_OPTIONS);

  /* instantaneous value */
  unsigned int lv = ReadLightSense();
  OutgoingMsg.pBuffer[0] = lv & 0xFF;
  OutgoingMsg.pBuffer[1] = (lv >> 8 ) & 0xFF;

  /* average value */
  lv = ReadLightSenseAverage();
  OutgoingMsg.pBuffer[2] = lv & 0xFF;
  OutgoingMsg.pBuffer[3] = (lv >> 8 ) & 0xFF;

  OutgoingMsg.Length = 4;

  RouteMsg(&OutgoingMsg);

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
static void SoftwareResetHandler(tMessage* pMsg)
{
  if ( pMsg->Options == MASTER_RESET_OPTION )
  {
    WriteMasterResetKey();
  }

  SoftwareReset();

}

static void NvalOperationHandler(tMessage* pMsg)
{
  /* overlay */
  tNvalOperationPayload* pNvPayload = (tNvalOperationPayload*)pMsg->pBuffer;

  /* create the outgoing message */
  tMessage OutgoingMsg;
  SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                NvalOperationResponseMsg,
                                NV_FAILURE);

  /* add identifier to outgoing message */
  tWordByteUnion Identifier;
  Identifier.word = pNvPayload->NvalIdentifier;
  OutgoingMsg.pBuffer[0] = Identifier.Bytes.byte0;
  OutgoingMsg.pBuffer[1] = Identifier.Bytes.byte1;
  OutgoingMsg.Length = 2;

  /* option byte in return message is status */
  switch (pMsg->Options)
  {

  case NVAL_INIT_OPERATION:
    /* may allow access to a specific range of nval ids that
     * the phone can initialize and use
     */
    break;

  case NVAL_READ_OPERATION:

    /* read the value and update the length */
    OutgoingMsg.Options = OsalNvRead(pNvPayload->NvalIdentifier,
                                     NV_ZERO_OFFSET,
                                     pNvPayload->Size,
                                     &OutgoingMsg.pBuffer[2]);

    OutgoingMsg.Length += pNvPayload->Size;

    break;

  case NVAL_WRITE_OPERATION:

    /* check that the size matches (otherwise NV_FAILURE is sent) */
    if ( OsalNvItemLength(pNvPayload->NvalIdentifier) == pNvPayload->Size )
    {
      OutgoingMsg.Options = OsalNvWrite(pNvPayload->NvalIdentifier,
                                        NV_ZERO_OFFSET,
                                        pNvPayload->Size,
                                        (void*)(&pNvPayload->DataStartByte));
    }

    /* update the copy in ram */
    NvUpdater(pNvPayload->NvalIdentifier);
    break;

  default:
    break;
  }

  RouteMsg(&OutgoingMsg);

}


/******************************************************************************/

void InitializeRstNmiConfiguration(void)
{
  nvRstNmiConfiguration = RST_PIN_DISABLED;
  OsalNvItemInit(NVID_RSTNMI_CONFIGURATION,
                 sizeof(nvRstNmiConfiguration),
                 &nvRstNmiConfiguration);

  ConfigureResetPinFunction(nvRstNmiConfiguration);

}


void SaveRstNmiConfiguration(void)
{
  OsalNvWrite(NVID_RSTNMI_CONFIGURATION,
              NV_ZERO_OFFSET,
              sizeof(nvRstNmiConfiguration),
              &nvRstNmiConfiguration);
}




/******************************************************************************/

/* The value in RAM must be updated if the phone writes the value in
 * flash (until the code is changed to read the value from flash)
 */
static void NvUpdater(unsigned int NvId)
{
  switch (NvId)
  {
#ifdef DIGITAL
    case NVID_IDLE_BUFFER_CONFIGURATION:
      InitializeIdleBufferConfig();
      break;
    case NVID_IDLE_BUFFER_INVERT:
      InitializeIdleBufferInvert();
      break;
#endif

    case NVID_IDLE_MODE_TIMEOUT:
    case NVID_APPLICATION_MODE_TIMEOUT:
    case NVID_NOTIFICATION_MODE_TIMEOUT:
    case NVID_RESERVED_MODE_TIMEOUT:
      InitializeModeTimeouts();
      break;

#ifdef ANALOG
    case NVID_IDLE_DISPLAY_TIMEOUT:
    case NVID_APPLICATION_DISPLAY_TIMEOUT:
    case NVID_NOTIFICATION_DISPLAY_TIMEOUT:
    case NVID_RESERVED_DISPLAY_TIMEOUT:
      InitializeDisplayTimeouts();
      break;
#endif

    case NVID_SNIFF_DEBUG:
    case NVID_BATTERY_DEBUG:
    case NVID_CONNECTION_DEBUG:
      InitializeDebugFlags();
      break;

    case NVID_RSTNMI_CONFIGURATION:
      InitializeRstNmiConfiguration();
      break;

    case NVID_MASTER_RESET:
      /* this gets handled on reset */
      break;

    case NVID_LOW_BATTERY_WARNING_LEVEL:
    case NVID_LOW_BATTERY_BTOFF_LEVEL:
      InitializeLowBatteryLevels();
      break;

    case NVID_BATTERY_SENSE_INTERVAL:
      InitializeBatteryMonitorInterval();
      break;

    case NVID_LIGHT_SENSE_INTERVAL:
      break;

    case NVID_SECURE_SIMPLE_PAIRING_ENABLE:
      /* not for phone control - reset watch */
      break;

    case NVID_LINK_ALARM_ENABLE:
      InitializeLinkAlarmEnable();
      break;

    case NVID_LINK_ALARM_DURATION:
      break;

    case NVID_PAIRING_MODE_DURATION:
      /* not for phone control - reset watch */
      break;

    case NVID_TIME_FORMAT:
      InitializeTimeFormat();
      break;

    case NVID_DATE_FORMAT:
      InitializeDateFormat();
      break;

#ifdef DIGITAL
    case NVID_DISPLAY_SECONDS:
      InitializeDisplaySeconds();
      break;
#endif

#ifdef ANALOG
    case NVID_TOP_OLED_CONTRAST_DAY:
    case NVID_BOTTOM_OLED_CONTRAST_DAY:
    case NVID_TOP_OLED_CONTRAST_NIGHT:
    case NVID_BOTTOM_OLED_CONTRAST_NIGHT:
      InitializeContrastValues();
      break;
#endif

  }
}

#ifdef RATE_TEST
static unsigned char RateTestCallback(void)
{
  unsigned char ExitLpm = 0;

  StartCrystalTimer(CRYSTAL_TIMER_ID3,
                    RateTestCallback,
                    RATE_TEST_INTERVAL_MS);

  /* send messages once we are connected and sniff mode */
  if (   QueryConnectionState() == Connected
      && QuerySniffState() == Sniff )
  {

    DEBUG5_PULSE();

    tMessage Msg;
    SetupMessage(&Msg,RateTestMsg,NO_MSG_OPTIONS);
    SendMessageToQueueFromIsr(BACKGROUND_QINDEX,&Msg);
    ExitLpm = 1;

  }

  return ExitLpm;

}
#endif

static void SetCallbackTimerHandler(tMessage* pMsg)
{
  tSetCallbackTimerPayload *pPayload = (tSetCallbackTimerPayload *)(pMsg->pBuffer);

  StopOneSecondTimer(CallbackTimerId);

  if (pPayload->Repeat)
  {
    SetupOneSecondTimer(CallbackTimerId,
                       pPayload->Timeout,
                       pPayload->Repeat,
                       SPP_TASK_QINDEX,
                       CallbackTimeoutMsg,
                       pMsg->Options);

    StartOneSecondTimer(CallbackTimerId);
  }
}

