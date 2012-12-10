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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "MSP430FlashUtil.h"
#include "Messages.h"
#include "MessageQueues.h"

#include "hal_board_type.h"
#include "hal_calibration.h"
#include "hal_rtc.h"
#include "hal_battery.h"
#include "hal_lpm.h"
#include "hal_miscellaneous.h"
#include "hal_lcd.h"
#include "hal_clock_control.h"
#include "hal_crystal_timers.h"
#include "hal_boot.h"

#include "DebugUart.h"
#include "Utilities.h"
#include "LcdDriver.h"
#include "Wrapper.h"
#include "OneSecondTimers.h"
#include "Adc.h"
#include "Accelerometer.h"
#include "Buttons.h"
#include "Statistics.h"
#include "OSAL_Nv.h"
#include "Vibration.h"
#include "NvIds.h"
#include "Display.h"
#include "SerialRam.h"
#include "Icons.h"
#include "Fonts.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "IdleTask.h"
#include "TestMode.h"

#define IDLE_FULL_UPDATE   (0)
#define DATE_TIME_ONLY     (1)

#define SPLASH_START_ROW   (41)

#define PAGE_TYPE_NUM      (3)

/* the internal buffer */
#define STARTING_ROW                  ( 0 )
#define PHONE_DRAW_SCREEN_ROW_NUM     ( 66 )

#define WIDGET_DRAW_ITEM_NUM          (12)
#define TEMPLATE_ID_MASK              (0x7F)
#define FLASH_TEMPLATE_BIT            (BIT7)

#define BATTERY_FULL_LEVEL            (4000)
#define BATTERY_CRITICAL_LEVEL        (3300)
#define BATTERY_LEVEL_NUMBER          (7)
#define BATTERY_LEVEL_INTERVAL        ((BATTERY_FULL_LEVEL - BATTERY_CRITICAL_LEVEL) / BATTERY_LEVEL_NUMBER)

#define DRAW_OPT_SEPARATOR            (':')
#define DRAW_OPT_OVERLAP_BT           (1)
#define DRAW_OPT_OVERLAP_BATTERY      (2)
#define DRAW_OPT_OVERLAP_SEC          (4)

#define MUSIC_STATE_START_ROW         (43)

#define INTERVAL_SEC_BATTERY_MONITOR  (10)

extern const char BUILD[];
extern const char VERSION[];

/* background.c */
static unsigned int nvBatteryMonitorIntervalInSeconds;
static unsigned char LedOn;
static tTimerId BatteryMonitorTimerId;
static tTimerId LedTimerId;
unsigned char nvRstNmiConfiguration;

xTaskHandle DisplayHandle;

static tMessage DisplayMsg;
static tTimerId DisplayTimerId;
static unsigned char RtcUpdateEnable = 0;
static unsigned char lastMin = 61;

static tLcdLine pMyBuffer[LCD_ROW_NUM];

static unsigned char NvIdle;
static unsigned char nvIdleBufferInvert;
static unsigned char nvDisplaySeconds = 0;

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static eIdleModePage CurrentPage[PAGE_TYPE_NUM];

static unsigned char Splashing;
static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;

typedef struct
{
  unsigned char X; // in pixels
  unsigned char Y;
  unsigned char Id; //DrawData_t Data;
  unsigned char Opt; //Option, e.g. divider
} DrawInfo_t;

typedef struct
{
  void (*Draw)(DrawInfo_t *Info);
  DrawInfo_t Info;
} Draw_t;

static void DrawHour(DrawInfo_t *Info);
static void DrawAmPm(DrawInfo_t *Info);
static void DrawMin(DrawInfo_t *Info);
static void DrawSec(DrawInfo_t *Info);
static void DrawDate(DrawInfo_t *Info);
static void DrawDayofWeek(DrawInfo_t *Info);
static void DrawBluetoothState(DrawInfo_t *Info);
static void DrawBatteryStatus(DrawInfo_t *Info);
//static void DrawIcon(DrawInfo_t *Info);
static void DrawTemplate(DrawInfo_t *Info);
static unsigned char Overlapping(unsigned char Option);

// widget is a list of Draw_t, DrawList can be multiple for each type of layout
const static Draw_t DrawList[][WIDGET_DRAW_ITEM_NUM] =
{
  { //1Q
    {DrawHour, {1, 2, MetaWatchTime, DRAW_OPT_SEPARATOR}},
    {DrawMin, {1, 23, MetaWatchTime, 0}},
    {DrawBluetoothState, {30, 27, ICON_SET_BLUETOOTH_SMALL}},
    {DrawBatteryStatus, {35, 2, ICON_SET_BATTERY_V}},
    {DrawDate, {25, 25, MetaWatch7, DRAW_OPT_OVERLAP_BT}},
    {DrawSec, {29, 31, MetaWatch16, DRAW_OPT_OVERLAP_BT}},
    {DrawDayofWeek, {25, 35, MetaWatch7, DRAW_OPT_OVERLAP_BT | DRAW_OPT_OVERLAP_SEC}}
  },
  { //4Q-fish
    {DrawTemplate, {0, 0, TMPL_WGT_FISH, 0}},
    {DrawHour, {29, 33, MetaWatchTime, DRAW_OPT_SEPARATOR}},
    {DrawMin, {58, 33, MetaWatchTime, 0}},
    {DrawAmPm, {83, 33, MetaWatch5, 0}},
    {DrawBluetoothState, {82, 2, ICON_SET_BLUETOOTH_SMALL, 0}},
//    {DrawBatteryStatus, {2, 74, ICON_SET_BATTERY_V}},
    {DrawBatteryStatus, {50, 4, ICON_SET_BATTERY_H}},
    {DrawDate, {55, 22, MetaWatch7, 0}},
    {DrawSec, {58, 51, MetaWatch16, 0}},
    {DrawDayofWeek, {58, 55, MetaWatch7, DRAW_OPT_OVERLAP_SEC}}
  },
  { //2Q-TimeG
    {DrawHour, {7, 2, TimeG, DRAW_OPT_SEPARATOR}},
    {DrawMin, {53, 2, TimeG, 0}},
    {DrawBluetoothState, {76, 30, ICON_SET_BLUETOOTH_SMALL}},
    {DrawBatteryStatus, {38, 33, ICON_SET_BATTERY_H}},
//    {DrawDate, {5, 29, MetaWatch16, 0}},
    {DrawDate, {7, 35, MetaWatch7, 0}},
    {DrawSec, {38, 29, MetaWatch16, DRAW_OPT_OVERLAP_BATTERY}},
    {DrawDayofWeek, {72, 35, MetaWatch7, DRAW_OPT_OVERLAP_BT}}
  }
};

#define DRAW_LIST_NUM (sizeof(DrawList) / sizeof(Draw_t))

typedef struct
{
  unsigned char LayoutType;
  unsigned char ItemNum;
  const Draw_t *pDrawList;
} Widget_t;

const static Widget_t HomeWidget[] =
{
  {0, 7, DrawList[0]},
  {3, 9, DrawList[1]},
  {1, 7, DrawList[2]},
//  {0, WIDGET_DRAW_ITEM_NUM, DrawList[2]},
//  {0, WIDGET_DRAW_ITEM_NUM, DrawList[3]}
};

#define HOME_WIDGET_NUM (sizeof(HomeWidget) / sizeof(Widget_t))

static unsigned char *pDrawBuffer;
static const tSetVibrateModePayload RingTone = {1, 0x00, 0x01, 0x00, 0x01, 2};

//Watch Faces Flash memory declaration
extern __data20 const unsigned char pWatchFace[][1]; //TEMPLATE_FLASH_SIZE];

/******************************************************************************/
static void DisplayTask(void *pvParameters);
static void DisplayQueueMessageHandler(tMessage* pMsg);

/* Message handlers */
static void IdleUpdateHandler();
static void ChangeModeHandler(unsigned char Option);
static void ModeTimeoutHandler();
static void WatchStatusScreenHandler(void);
static void ListPairedDevicesHandler(void);
static void ConfigureDisplayHandler(tMessage* pMsg);
static void ConfigureIdleBufferSizeHandler(tMessage* pMsg);
static void ModifyTimeHandler(tMessage* pMsg);
static void MenuModeHandler(unsigned char Option);
static void MenuButtonHandler(unsigned char MsgOptions);
static void ToggleSecondsHandler(void);
static void ServiceMenuHandler(void);
static void ToggleSerialGndSbw(unsigned char PowerGood);
static void BluetoothStateChangeHandler(tMessage *pMsg);

/******************************************************************************/
static void DrawConnectionScreen(void);
static void InitMyBuffer(void);
static void DisplayStartupScreen(void);
static void DetermineIdlePage(void);

static void DrawMenu1(void);
static void DrawMenu2(void);
static void DrawMenu3(void);
static void DrawCommonMenuIcons(void);

static void FillMyBuffer(unsigned char StartingRow,
                         unsigned char NumberOfRows,
                         unsigned char FillValue);

static void SendMyBufferToLcd(unsigned char StartingRow,
                              unsigned char NumberOfRows);

static void CopyRowsIntoMyBuffer(unsigned char const* pImage,
                                 unsigned char StartingRow,
                                 unsigned char NumberOfRows);

static void CopyColumnsIntoMyBuffer(unsigned char const* pImage,
                                    unsigned char StartingRow,
                                    unsigned char NumberOfRows,
                                    unsigned char StartingColumn,
                                    unsigned char NumberOfColumns);

static void DrawDateTime(void);
static void DrawSecs(void);
static void DrawMins(void);
static void DrawHours(void);
static void WriteFontCharacter(unsigned char Character);
static void WriteFontString(tString* pString);

static void DrawLocalAddress(unsigned char Col, unsigned Row);

static void WriteToTemplateHandler(tMessage *pMsg);
static void EraseTemplateHandler(tMessage *pMsg);
static void HandleMusicPlayStateChange(unsigned char State);

static void DrawHomeWidget(Widget_t *pData);
static void UpdateHomeWidget(unsigned char Option);
static void DrawText(unsigned char *pText, unsigned char Len, unsigned char X, unsigned char Y, unsigned char Font, unsigned char EqualWidth);
static void DrawBitmap(const unsigned char *pBitmap, unsigned char X, unsigned char Y, unsigned char W, unsigned char H, unsigned char BmpWidthInBytes);
static const unsigned char *GetBatteryIcon(unsigned char Id);

static void InitBackgroundTask(void);
static void InitializeBatteryMonitorInterval(void);
static void ShowNotification(tString *pString, unsigned char Type);

/*! Initialize flash/ram value for the idle buffer configuration */
static void InitializeIdleBufferConfig(void);

/*! Initialize flash/ram value for controlling whether or not the idle buffer
 *  is inverted */
static void InitializeIdleBufferInvert(void);

/*! Initialize flash/ram value for whether or not to display seconds */
static void InitializeDisplaySeconds(void);

static void LedChangeHandler(tMessage* pMsg);
static void InitializeRstNmiConfiguration(void);
static void ReadBatteryVoltageHandler(void);
static void ReadLightSensorHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void SoftwareResetHandler(tMessage* pMsg);
static void NvUpdater(tNvalOperationPayload *pNval);
static void EnterBootloader(void);
static void BatteryChargeControlHandler(void);
static void HandleVersionInfo(void);

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

/*! Initialize the LCD display task
 *
 * Initializes the display driver, clears the display buffer and starts the
 * display task
 *
 * \return none, result is to start the display task
 */

#define DISPLAY_TASK_QUEUE_LENGTH 16
#define DISPLAY_TASK_STACK_SIZE  	(configMINIMAL_STACK_SIZE + 100) //total 48-88
#define DISPLAY_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

void InitializeDisplayTask(void)
{
  InitMyBuffer();

  QueueHandles[DISPLAY_QINDEX] =
    xQueueCreate(DISPLAY_TASK_QUEUE_LENGTH, MESSAGE_QUEUE_ITEM_SIZE);

  // task function, task name, stack len, task params, priority, task handle
  xTaskCreate(DisplayTask,
              (const signed char *)"DISPLAY",
              DISPLAY_TASK_STACK_SIZE,
              NULL,
              DISPLAY_TASK_PRIORITY,
              &DisplayHandle);
  
  PrintString3("Build:", BUILD, CR);
  ClearShippingModeFlag();
}

static void InitBackgroundTask(void)
{
  InitializeRstNmiConfiguration();

  /*
   * check on the battery
   */
  ConfigureBatteryPins();
  BatteryChargingControl(ExtPower());
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
                      DISPLAY_QINDEX,
                      BatteryChargeControl,
                      MSG_OPT_NONE);

  StartOneSecondTimer(BatteryMonitorTimerId);

  /*
   * Setup a timer to use with the LED for the LCD.
   */
  LedTimerId = AllocateOneSecondTimer();

  SetupOneSecondTimer(LedTimerId,
                      ONE_SECOND * 5,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      LedChange,
                      LED_OFF_OPTION);

#if RAM_TEST

  RamTestTimerId = AllocateOneSecondTimer();

  SetupOneSecondTimer(RamTestTimerId,
                      ONE_SECOND*20,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      RamTestMsg,
                      MSG_OPT_NONE);

  StartOneSecondTimer(RamTestTimerId);

#endif

  InitializeAccelerometer();
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg,
                                AccelerometerSetupMsg,
                                ACCELEROMETER_SETUP_INTERRUPT_CONTROL_OPTION);

  Msg.pBuffer[0] = INTERRUPT_CONTROL_ENABLE_INTERRUPT;
  Msg.Length = 1;
  RouteMsg(&Msg);

  /* don't call AccelerometerEnable() directly. Use a message*/
//  SetupMessage(&Msg,AccelerometerEnableMsg,MSG_OPT_NONE);
//  RouteMsg(&Msg);

#if RATE_TEST
  StartCrystalTimer(CRYSTAL_TIMER_ID3,RateTestCallback,RATE_TEST_INTERVAL_MS);
#endif

  /****************************************************************************/

#if WATCHDOG_TEST_MODE == 2
  /* force watchdog after the scheduler has started */
  ForceWatchdogReset();
#endif
}

/*! LCD Task Main Loop
 *
 * \param pvParameters
 *
 */
static void DisplayTask(void *pvParameters)
{
  if ( QueueHandles[DISPLAY_QINDEX] == 0 )
  {
    PrintString("Display Queue not created!\r\n");
  }

  InitBackgroundTask();
  
  LcdPeripheralInit();
//  unsigned char ModePage = 0;
//  OsalNvItemInit(NVID_DISPLAY_MODE_PAGE, sizeof(ModePage), &ModePage);
//  if (ModePage) //<##>
  DisplayStartupScreen();
  SerialRamInit();

  InitializeIdleBufferConfig();
  InitializeIdleBufferInvert();
  InitializeDisplaySeconds();
  InitLinkAlarmEnable();
  InitModeTimeout();
  Init12H();
  InitMonthFirst();
  DisplayTimerId = AllocateOneSecondTimer();

#if !ISOLATE_RADIO
  /* turn the radio on; initialize the serial port profile or BLE/GATT */
  CreateAndSendMessage(TurnRadioOnMsg, MSG_OPT_NONE);
#endif

  for(;;)
  {
    if( pdTRUE == xQueueReceive(QueueHandles[DISPLAY_QINDEX],
                                &DisplayMsg, portMAX_DELAY) )
    {
      PrintMessageType(&DisplayMsg);
      DisplayQueueMessageHandler(&DisplayMsg);
      SendToFreeQueue(&DisplayMsg);
//      CheckStackUsage(DisplayHandle, "~DspStk ");
      CheckQueueUsage(QueueHandles[DISPLAY_QINDEX]);
    }
  }
}

/*! Display the startup image or Splash Screen */
static void DisplayStartupScreen(void)
{
  Splashing = pdTRUE;
  /* draw metawatch logo */
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
  
  CopyColumnsIntoMyBuffer(pIconSplashLogo, 21, 13, 3, 6);
  CopyRowsIntoMyBuffer(pIconSplashMetaWatch, SPLASH_START_ROW, 12);
  CopyColumnsIntoMyBuffer(pIconSplashHandsFree, 58, 5, 2, 8);
  
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

/*! Handle the messages routed to the display queue */
static void DisplayQueueMessageHandler(tMessage* pMsg)
{
  tMessage Msg;
  
  switch(pMsg->Type)
  {
  case WriteBufferMsg:
    WriteBufferHandler(pMsg);
    break;

  case SetWidgetListMsg:
    SetWidgetList(pMsg);
    break;
  
  case UpdateDisplay:
    UpdateDisplayHandler(pMsg);
    break;
    
  case UpdateHomeWidgetMsg:
    UpdateHomeWidget(pMsg->Options);
    break;
    
  case BluetoothStateChangeMsg:
    BluetoothStateChangeHandler(pMsg);
    break;

  case IdleUpdateMsg:
    IdleUpdateHandler();
    break;

  case ButtonStateMsg:
    ButtonStateHandler();
    break;

  case CallerIdMsg:
//    PrintStringAndDecimal("LcdDisp:CallerId Len:", pMsg->Length);
    pMsg->pBuffer[pMsg->Length] = NULL;
    ShowNotification((char *)pMsg->pBuffer, pMsg->Options);
    break;
    
  case CallerNameMsg:
    if (pMsg->Length)
    {
      pMsg->pBuffer[pMsg->Length] = NULL;
      ShowNotification((char *)pMsg->pBuffer, SHOW_NOTIF_CALLER_NAME);
    }
    else ShowNotification("", pMsg->Options);
    break;

  case MusicPlayStateMsg:
    HandleMusicPlayStateChange(pMsg->Options);
    break;
    
  case ChangeModeMsg:
    ChangeModeHandler(pMsg->Options);
    break;
  
  case ConfigureIdleBufferSize:
    ConfigureIdleBufferSizeHandler(pMsg);
    break;

  case ConfigureDisplay:
    ConfigureDisplayHandler(pMsg);
    break;

  case ModifyTimeMsg:
    ModifyTimeHandler(pMsg);
    break;

  case MenuModeMsg:
    MenuModeHandler(pMsg->Options);
    break;

  case MenuButtonMsg:
    MenuButtonHandler(pMsg->Options);
    break;

  case EnableButtonMsg:
    EnableButtonMsgHandler(pMsg);
    break;

  case GetDeviceType:
    SetupMessageAndAllocateBuffer(&Msg, GetDeviceTypeResponse, MSG_OPT_NONE);
    Msg.pBuffer[0] = BOARD_TYPE;

#ifdef WATCH
    if (GetMsp430HardwareRevision() < 'F') Msg.pBuffer[0] = DIGITAL_WATCH_TYPE;
#endif

    Msg.Length = 1;
    RouteMsg(&Msg);
    PrintStringAndDecimal("- DevTypeResp:", Msg.pBuffer[0]);
    break;

  case GetInfoString:
    HandleVersionInfo();
    break;
    
  case SetVibrateMode:
    SetVibrateModeHandler(pMsg);
    break;

  case SetRealTimeClock:
    halRtcSet((tRtcHostMsgPayload*)pMsg->pBuffer);

#ifdef DIGITAL
    SendMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
#endif
    break;

  case GetRealTimeClock:
    SetupMessageAndAllocateBuffer(&Msg, GetRealTimeClockResponse, MSG_OPT_NONE);
    halRtcGet((tRtcHostMsgPayload*)Msg.pBuffer);
    Msg.Length = sizeof(tRtcHostMsgPayload);
    RouteMsg(&Msg);
    break;

  case ServiceMenuMsg:
    ServiceMenuHandler();
    break;
    
  case DisableButtonMsg:
    DisableButtonMsgHandler(pMsg);
    break;

  case ReadButtonConfigMsg:
    ReadButtonConfigHandler(pMsg);
    break;

  case BatteryChargeControl:
    BatteryChargeControlHandler();
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

  case ResetMsg:
    SoftwareResetHandler(pMsg);
    break;

  case NvalOperationMsg:
    NvalOperationHandler(pMsg);
    break;
      
  case LoadTemplate:
    LoadTemplateHandler(pMsg);
    break;

  case LinkAlarmMsg:
    if (LinkAlarmEnable()) GenerateLinkAlarm();
    break;

  case ModeTimeoutMsg:
    ModeTimeoutHandler();
    break;

  case WatchStatusMsg:
    WatchStatusScreenHandler();
    break;

  case ListPairedDevicesMsg:
    ListPairedDevicesHandler();
    break;

  case WatchDrawnScreenTimeout:
    IdleUpdateHandler();
    break;

  case ToggleSecondsMsg:
    ToggleSecondsHandler();
    break;
    
  case TestModeMsg:
    TestModeCommandHandler();
    break;
    
  case LowBatteryWarningMsg:
    break;
    
  case LowBatteryBtOffMsg:
    UpdateHomeWidget(MSG_OPT_NONE);
    break;

  case EraseTemplateMsg:
    EraseTemplateHandler(pMsg);
    break;
    
  case WriteToTemplateMsg:
    WriteToTemplateHandler(pMsg);
    break;

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

  case ReadLightSensorMsg:
    ReadLightSensorHandler();
    break;

  case RateTestMsg:
    SetupMessageAndAllocateBuffer(&Msg, DiagnosticLoopback, MSG_OPT_NONE);
    /* don't care what data is */
    Msg.Length = 10;
    RouteMsg(&Msg);
    break;
    
  case RamTestMsg:
    RamTestHandler(pMsg);
    break;
    
  default:
    PrintStringAndHexByte("# Disp Msg: 0x", pMsg->Type);
    break;
  }
}

/*! Switch from other-mode/menu page back to idle type page
 */
static void IdleUpdateHandler(void)
{
  PageType = PAGE_TYPE_IDLE;

  if (CurrentPage[PageType] == InitPage || NvIdle == WATCH_DRAW_TOP) DrawDateTime();
  
  if (OnceConnected())
    CreateAndSendMessage(UpdateDisplay, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL);
  else DrawConnectionScreen();
}

static void ChangeModeHandler(unsigned char Option)
{  
  unsigned char Mode = Option & MODE_MASK;

  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, ModeChangeIndMsg,  (CurrentIdleScreen() << 4) | Mode);
  Msg.pBuffer[0] = eScUpdateComplete;
  Msg.Length = 1;
  RouteMsg(&Msg);

  PrintStringAndHexByte("- ChgModInd:", Msg.Options);
  
  if (Option & MSG_OPT_CHGMOD_IND) return; // ask for current idle page only
    
  if (Mode != IDLE_MODE)
  {
    PageType = PAGE_TYPE_IDLE;
    SetupOneSecondTimer(DisplayTimerId,
                        QueryModeTimeout(Mode),
                        NO_REPEAT,
                        DISPLAY_QINDEX,
                        ModeTimeoutMsg,
                        Mode);
    StartOneSecondTimer(DisplayTimerId);
    
    if (Option & MSG_OPT_UPD_INTERNAL)
    {
      SendMessage(&Msg, UpdateDisplay, Option);
    }
  }
  else
  {
    StopOneSecondTimer(DisplayTimerId);
    IdleUpdateHandler();
  }
  
  CurrentMode = Mode;
}

static void ModeTimeoutHandler()
{
  /* send a message to the host indicating that a timeout occurred */
  PrintString2("- ModChgTout", CR);
  
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, ModeChangeIndMsg, CurrentMode);
  Msg.pBuffer[0] = eScModeTimeout;
  Msg.Length = 1;
  RouteMsg(&Msg);
  
  ChangeModeHandler(IDLE_MODE);
}

void ResetModeTimer(void)
{
  StartOneSecondTimer(DisplayTimerId);
}

static void BluetoothStateChangeHandler(tMessage *pMsg)
{
  if (Splashing)
  {
    if (pMsg->Options == On)
    {
      Splashing = pdFALSE;
      RtcUpdateEnable = 1;
      DetermineIdlePage();
      IdleUpdateHandler();
      
      pMsg->Options = MUSIC_MODE | (TMPL_MUSIC_MODE << 4);
      LoadTemplateHandler(pMsg);

      pMsg->Options = NOTIF_MODE | (TMPL_NOTIF_MODE << 4);
      LoadTemplateHandler(pMsg);
      
      InitButton();
    }
  }
  else
  {
    //decide which idle page to be
    DetermineIdlePage();
    
    if (CurrentMode == IDLE_MODE)
    {
      if (PageType == PAGE_TYPE_IDLE)
      {
        if (!OnceConnected()) DrawConnectionScreen();
      }
      else if (PageType == PAGE_TYPE_MENU)
      {
        MenuModeHandler(0);
      }
      else if (CurrentPage[PAGE_TYPE_INFO] == StatusPage)
      {
        WatchStatusScreenHandler();
      }
    }

    UpdateHomeWidget(MSG_OPT_NONE);
  }
}

static void DetermineIdlePage(void)
{
  if (Connected(CONN_TYPE_MAIN)) CurrentPage[PAGE_TYPE_IDLE] = ConnectedPage;
  else CurrentPage[PAGE_TYPE_IDLE] = OnceConnected() ? DisconnectedPage: InitPage;
  
  PrintStringAndDecimal("- Idle Pg:", CurrentPage[PAGE_TYPE_IDLE]);
}

static void MenuModeHandler(unsigned char Option)
{
  /* draw entire region */
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
  PageType = PAGE_TYPE_MENU;

  if (Option) CurrentPage[PageType] = (eIdleModePage)Option;
  
  switch (CurrentPage[PageType])
  {
  case Menu1Page:
    DrawMenu1();
    break;
    
  case Menu2Page:
    DrawMenu2();
    break;
    
  case Menu3Page:
    DrawMenu3();
    break;
    
  default:
    PrintStringAndDecimal("# No Such Menu Page:", Option);
    break;
  }

  /* these icons are common to all menus */
  DrawCommonMenuIcons();

  /* only invert the part that was just drawn */
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
}

static void MenuButtonHandler(unsigned char MsgOptions)
{
  tMessage Msg;
  unsigned char RstControl;
  
  switch (MsgOptions)
  {
  case MENU_BUTTON_OPTION_EXIT:

    /* save all of the non-volatile items */
//    SaveSecureSimplePairingState();
    SaveLinkAlarmEnable();
    RstControl = ResetPin();
    OsalNvWrite(NVID_RSTNMI_CONFIGURATION, NV_ZERO_OFFSET, sizeof(RstControl), &RstControl);
    OsalNvWrite(NVID_IDLE_BUFFER_INVERT, NV_ZERO_OFFSET, sizeof(nvIdleBufferInvert), &nvIdleBufferInvert);
    OsalNvWrite(NVID_DISPLAY_SECONDS, NV_ZERO_OFFSET, sizeof(nvDisplaySeconds), &nvDisplaySeconds);

    IdleUpdateHandler();
    break;

  case MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH:

    if (BluetoothState() != Initializing)
      SendMessage(&Msg, RadioOn() ? TurnRadioOffMsg : TurnRadioOnMsg, MSG_OPT_NONE);
    break;

  case MENU_BUTTON_OPTION_DISPLAY_SECONDS:
    nvDisplaySeconds = !nvDisplaySeconds;
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM:
    ToggleLinkAlarmEnable();
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_INVERT_DISPLAY:
    nvIdleBufferInvert = !nvIdleBufferInvert;
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_MENU1:
    break;
  
  case MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN:
    ConfigResetPin(RST_PIN_TOGGLED);
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_TOGGLE_SERIAL_SBW_GND:

    ToggleSerialGndSbw(EXTERNAL_POWER_GOOD);
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_ENABLE_CHARGING:
    EnableCharge(!ChargeEnabled());
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE:
    EnterBootloader();
    break;

/*
  case MENU_BUTTON_OPTION_TOGGLE_SBW_OFF_CLIP:
    ToggleSerialGndSbw(NO_EXTERNAL_POWER);
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_TOGGLE_SECURE_SIMPLE_PAIRING:
    if ( BluetoothState() != Initializing )
    {
      SetupMessage(&Msg, PairingControlMsg, PAIRING_CONTROL_OPTION_TOGGLE_SSP);
      RouteMsg(&Msg);
    }
    break;

  case MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY:
  
    SetupMessage(&Msg, PairingControlMsg, QueryDiscoverable() ?
      PAIRING_CONTROL_OPTION_DISABLE_PAIRING :
      PAIRING_CONTROL_OPTION_ENABLE_PAIRING);
      
    RouteMsg(&Msg);
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_TOGGLE_ACCEL:

    SetupMessage(&Msg, QueryAccelerometerState() ?
      AccelerometerDisableMsg : AccelerometerEnableMsg,
      MSG_OPT_NONE);

    RouteMsg(&Msg);
    MenuModeHandler(0);
    break;
*/
  default:
    break;
  }
}

static void ToggleSecondsHandler(void)
{
  nvDisplaySeconds = !nvDisplaySeconds;
  PrintStringAndDecimal("- TglSec:", nvDisplaySeconds);
  if (!nvDisplaySeconds) UpdateHomeWidget(MSG_OPT_NONE);
}

static void ToggleSerialGndSbw(unsigned char PowerGood)
{
  unsigned char MuxMode = GetMuxMode(PowerGood);

  if (MuxMode == MUX_MODE_SERIAL) MuxMode = MUX_MODE_SPY_BI_WIRE;
  else if (MuxMode == MUX_MODE_SPY_BI_WIRE) MuxMode = MUX_MODE_GND;
  else MuxMode = MUX_MODE_SERIAL;
  
  SetMuxMode(MuxMode, PowerGood);
  ChangeMuxMode();

  PrintString3("-TglMux: ", MuxMode == MUX_MODE_SERIAL ? "Serial" :
    (MuxMode == MUX_MODE_SPY_BI_WIRE ? "SBW" : "GND"), ExtPower() ? " Pwr" : "");
}

static void ServiceMenuHandler(void)
{
  MenuModeHandler(Menu2Page);
}

static void DrawConnectionScreen()
{
  unsigned char const* pSwash;

  if (!RadioOn()) pSwash = pBootPageBluetoothOffSwash;
  else if (ValidPairingInfo()) pSwash = pBootPagePairedSwash;
  else pSwash = pBootPageNoPairSwash;
  
  FillMyBuffer(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM, 0x00);
  CopyRowsIntoMyBuffer(pSwash, WATCH_DRAW_SCREEN_ROW_NUM + 1, 32);

  /* add the firmware version */
  gColumn = 4;
  gRow = 68;
  gBitColumnMask = BIT5;
  SetFont(MetaWatch5);
  WriteFontString("V ");
  WriteFontString((char *)VERSION);

  DrawLocalAddress(1, 80);
  
  SendMyBufferToLcd(WATCH_DRAW_SCREEN_ROW_NUM, PHONE_DRAW_SCREEN_ROW_NUM);
  
}

static void DrawLocalAddress(unsigned char Col, unsigned Row)
{
  char pAddr[BT_ADDR_LEN + BT_ADDR_LEN + 3];
  GetBDAddrStr(pAddr);

  gRow = Row;
  gColumn = Col;
  gBitColumnMask = BIT2;
  SetFont(MetaWatch7);
  
  unsigned char i;
  for (i = 2; i < 14; ++i)
  {
    WriteFontCharacter(pAddr[i]);
    if (i % 4 == 1 && i != 13) WriteFontCharacter('-');
  }
}

static void DrawMenu1(void)
{
  unsigned char const * pIcon;

  if (BluetoothState() == Initializing)
  {
    pIcon = pBluetoothInitIcon;
  }
  else
  {
    pIcon = RadioOn() ? pBluetoothOnIcon : pBluetoothOffIcon;
  }

  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pInvertDisplayIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(nvDisplaySeconds ? pSecondsOnMenuIcon : pSecondsOffMenuIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(LinkAlarmEnable() ? pLinkAlarmOnIcon : pLinkAlarmOffIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void DrawMenu2(void)
{
  CopyColumnsIntoMyBuffer(ResetPin() == RST_PIN_ENABLED ? pRstPinIcon : pNmiPinIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  unsigned char const * pIcon;
  unsigned char MuxMode = GetMuxMode(EXTERNAL_POWER_GOOD);

  if (MuxMode == MUX_MODE_SERIAL) pIcon = pSerialIcon;
  else if (MuxMode == MUX_MODE_GND) pIcon = pGroundIcon;
  else pIcon = pSbwIcon;
  
  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pNextIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pResetButtonIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void DrawMenu3(void)
{
  CopyColumnsIntoMyBuffer(pBootloaderIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(ChargeEnabled() ? pIconChargingEnabled : pIconChargingDisabled,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pNextIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

//  unsigned char const * pIcon;
//  unsigned char MuxMode = GetMuxMode(NO_EXTERNAL_POWER);
//
//  if (MuxMode == MUX_MODE_SERIAL) pIcon = pSerialOffClipIcon;
//  else if (MuxMode == MUX_MODE_GND) pIcon = pGndOffClipIcon;
//  else pIcon = pSbwOffClipIcon;
//
//  CopyColumnsIntoMyBuffer(pIcon,
//                          BUTTON_ICON_B_E_ROW,
//                          BUTTON_ICON_SIZE_IN_ROWS,
//                          LEFT_BUTTON_COLUMN,
//                          BUTTON_ICON_SIZE_IN_COLUMNS);
  
}

static void DrawCommonMenuIcons(void)
{
  CopyColumnsIntoMyBuffer(pLedIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(pExitIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void WatchStatusScreenHandler(void)
{
  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
  
  CopyColumnsIntoMyBuffer(RadioOn() ? pRadioOnIcon : pRadioOffIcon,
                          3,
                          STATUS_ICON_SIZE_IN_ROWS,
                          LEFT_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(Connected(CONN_TYPE_MAIN) ? pConnectedIcon : pDisconnectedIcon,
                          3,
                          STATUS_ICON_SIZE_IN_ROWS,
                          CENTER_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(GetBatteryIcon(ICON_SET_BATTERY_V),
                          6,
                          IconInfo[ICON_SET_BATTERY_V].Height,
                          9,
                          IconInfo[ICON_SET_BATTERY_V].Width);

  gRow = 27+4;
  gColumn = 5;
  gBitColumnMask = BIT4;
  SetFont(MetaWatch5);

  if (PairedDeviceType() == DEVICE_TYPE_BLE) WriteFontString("BLE");
  else if (PairedDeviceType() == DEVICE_TYPE_SPP) WriteFontString("BR");
  /* display battery voltage */
  
  gRow = 27+2;
  gColumn = 8;
  gBitColumnMask = BIT5;
  SetFont(MetaWatch7);

  unsigned char BattVal = BatteryPercentage();
  
  if (BattVal == 100) WriteFontString("100");
  else
  {
    WriteFontCharacter(BattVal > 9 ? BattVal / 10 +'0' : ' ');
    WriteFontCharacter(BattVal % 10 +'0');
  }
  WriteFontCharacter('%');

  // Add Wavy line
  gRow += 12;
  CopyRowsIntoMyBuffer(pWavyLine,gRow,NUMBER_OF_ROWS_IN_WAVY_LINE);

  CopyColumnsIntoMyBuffer(pIconWatch, 54, 21, 0, 2); //54, 21, 2, 2);

  /* add the firmware version */
  gColumn = 2; //4;
  gRow = 56;
  gBitColumnMask = BIT2;
  WriteFontString("SW: ");
  WriteFontString((char *)VERSION);
  WriteFontString(" (");
  WriteFontString((char *)BUILD);
  WriteFontCharacter('.');

  tVersion Version = GetWrapperVersion();
  WriteFontString(Version.pSwVer);
  WriteFontCharacter(')');
  
  gColumn = 2; //4;
  gRow = 65;
  gBitColumnMask = BIT2;
  WriteFontString("HW: REV ");
//  char HwVer[6] = "F";
//  if (QueryCalibrationValid()) sprintf(HwVer, "%d", HardwareVersion());
//  WriteFontString(HwVer);
  WriteFontCharacter(Version.HwRev);
  
  DrawLocalAddress(1, 80);
  
  /* display entire buffer */
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
  
  PageType = PAGE_TYPE_INFO;
  CurrentPage[PageType] = StatusPage;
}

static void ListPairedDevicesHandler(void)
{
//  /* clearn screen */
//  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
//
//  tString AddrStr[12+2+1];
//
//  gRow = 4;
//  gColumn = 0;
//  SetFont(MetaWatch7);
//
//  gColumn = 0;
//  gBitColumnMask = BIT4;
//  WriteFontString(GetConnectedDeviceName());
//
//  gRow += 12;
//  gColumn = 0;
//  gBitColumnMask = BIT4;
//  GetConnectedDeviceAddress(AddrStr);
//  WriteFontString(AddrStr);
//  gRow += 12+5;
//
//  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
//
//  PageType = PAGE_TYPE_INFO;
//  CurrentPage[PageType] = ListPairedDevicesPage;
}

/* change the parameter but don't save it into flash */
static void ConfigureDisplayHandler(tMessage* pMsg)
{
  switch (pMsg->Options)
  {
  case CONFIGURE_DISPLAY_OPTION_DONT_DISPLAY_SECONDS:
  case CONFIGURE_DISPLAY_OPTION_DISPLAY_SECONDS:
  
    nvDisplaySeconds = !nvDisplaySeconds;
    PrintStringAndDecimal("- ConfSec:", nvDisplaySeconds);
    if (!nvDisplaySeconds) UpdateHomeWidget(MSG_OPT_NONE);
    break;

  case CONFIGURE_DISPLAY_OPTION_DONT_INVERT_DISPLAY:
  case CONFIGURE_DISPLAY_OPTION_INVERT_DISPLAY:

   nvIdleBufferInvert =!nvIdleBufferInvert;
   
   if (PageType == PAGE_TYPE_IDLE) IdleUpdateHandler();
   else if (PageType == PAGE_TYPE_MENU) MenuModeHandler(0);
   else if (CurrentPage[PageType] == StatusPage) WatchStatusScreenHandler();
   break;
  }
}

static void ConfigureIdleBufferSizeHandler(tMessage* pMsg)
{
  NvIdle = pMsg->pBuffer[0] & IDLE_BUFFER_CONFIG_MASK;
}

static void ModifyTimeHandler(tMessage* pMsg)
{
  int time;
  switch (pMsg->Options)
  {
  case MODIFY_TIME_INCREMENT_HOUR:
    /*! todo - make these functions */
    time = RTCHOUR;
    time++; if ( time == 24 ) time = 0;
    RTCHOUR = time;
    break;
  case MODIFY_TIME_INCREMENT_MINUTE:
    time = RTCMIN;
    time++; if ( time == 60 ) time = 0;
    RTCMIN = time;
    break;
  case MODIFY_TIME_INCREMENT_DOW:
    /* modify the day of the week (not the day of the month) */
    time = RTCDOW;
    time++; if ( time == 7 ) time = 0;
    RTCDOW = time;
    break;
  }

  UpdateHomeWidget(MSG_OPT_NONE);
}

unsigned char ScreenControl(void)
{
  return NvIdle;
}

unsigned char InvertDisplay(void)
{
  return nvIdleBufferInvert;
}

static void InitMyBuffer(void)
{
  unsigned char row;
  unsigned char col;

  // Clear the display buffer.  Step through the rows
  for(row = STARTING_ROW; row < LCD_ROW_NUM; row++)
  {
    // clear a horizontal line
    for(col = 0; col < BYTES_PER_LINE; col++)
    {
      pMyBuffer[row].Row = row + FIRST_LCD_LINE_OFFSET;
      pMyBuffer[row].Data[col] = 0x00;
      pMyBuffer[row].Dummy = 0x00;
    }
  }
}

static void FillMyBuffer(unsigned char StartingRow,
                         unsigned char NumberOfRows,
                         unsigned char FillValue)
{
  unsigned char row = StartingRow;
  unsigned char col;

  // Clear the display buffer.  Step through the rows
  for( ; row < LCD_ROW_NUM && row < StartingRow+NumberOfRows; row++ )
  {
    pMyBuffer[row].Row = row + FIRST_LCD_LINE_OFFSET;
    // clear a horizontal line
    for(col = 0; col < BYTES_PER_LINE; col++)
    {
      pMyBuffer[row].Data[col] = FillValue;
    }
  }
}

static void SendMyBufferToLcd(unsigned char StartingRow, unsigned char NumberOfRows)
{
  unsigned char row = StartingRow;
  unsigned char col;

  /*
   * flip the bits before sending to LCD task because it will
   * dma this portion of the screen
  */
  if (nvIdleBufferInvert == NORMAL_DISPLAY)
  {
    for(; row < LCD_ROW_NUM && row < StartingRow+NumberOfRows; row++)
    {
      for (col = 0; col < BYTES_PER_LINE; col++)
      {
        pMyBuffer[row].Data[col] = ~(pMyBuffer[row].Data[col]);
      }
    }
  }

  UpdateMyDisplay((unsigned char*)&pMyBuffer[StartingRow], NumberOfRows);
}

static void CopyRowsIntoMyBuffer(unsigned char const* pImage,
                                 unsigned char StartingRow,
                                 unsigned char NumberOfRows)
{

  unsigned char DestRow = StartingRow;
  unsigned char SourceRow = 0;
  unsigned char col = 0;

  while ( DestRow < LCD_ROW_NUM && SourceRow < NumberOfRows )
  {
    pMyBuffer[DestRow].Row = DestRow + FIRST_LCD_LINE_OFFSET;
    for(col = 0; col < BYTES_PER_LINE; col++)
    {
      pMyBuffer[DestRow].Data[col] = pImage[SourceRow*BYTES_PER_LINE+col];
    }
    DestRow ++;
    SourceRow ++;
  }
}

static void CopyColumnsIntoMyBuffer(unsigned char const* pImage,
                                    unsigned char StartingRow,
                                    unsigned char NumberOfRows,
                                    unsigned char StartingColumn,
                                    unsigned char NumberOfColumns)
{
  unsigned char DestRow = StartingRow;
  unsigned char RowCounter = 0;
  unsigned char DestColumn = StartingColumn;
  unsigned char ColumnCounter = 0;
  unsigned int SourceIndex = 0;

  /* copy rows into display buffer */
  while ( DestRow < LCD_ROW_NUM && RowCounter < NumberOfRows )
  {
    DestColumn = StartingColumn;
    ColumnCounter = 0;
    pMyBuffer[DestRow].Row = DestRow + FIRST_LCD_LINE_OFFSET;
    
    while ( DestColumn < BYTES_PER_LINE && ColumnCounter < NumberOfColumns )
    {
      pMyBuffer[DestRow].Data[DestColumn] = pImage[SourceIndex];

      DestColumn ++;
      ColumnCounter ++;
      SourceIndex ++;
    }

    DestRow ++;
    RowCounter ++;
  }
}

static void UpdateHomeWidget(unsigned char Option)
{
  if (CurrentMode != IDLE_MODE || PageType != PAGE_TYPE_IDLE) return;
  if (CurrentPage[PageType] == InitPage || NvIdle == WATCH_DRAW_TOP) {DrawDateTime(); return;}

  unsigned char LayoutType = GetHomeWidgetLayout();
  if (!LayoutType) return; // no hw on current idle screen
  
  RtcUpdateEnable = 0; // block RTC  
  
  static unsigned char DoneUpd = 0;
  DoneUpd = Option ? DoneUpd | Option : 0;

//  PrintStringAndTwoHexBytes("DoneUpd: Layout:", DoneUpd, LayoutType);
  LayoutType &= ~DoneUpd; // remove drawn layout in Option from LayoutType
//  PrintStringAndHexByte("AF Layout:", LayoutType);
  
  if (!LayoutType)
  {
    DoneUpd = 0;
    CreateAndSendMessage(UpdateDisplay, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL | MSG_OPT_UPD_HWGT);
    RtcUpdateEnable = 1;
//    PrintString("- UpdHwgt done\r\n");
    return;
  }
    
  unsigned char i = 0;
  
  while (i < LAYOUT_NUM)
  {
    if (LayoutType & 1)
    {
      // for each layout: draw datetime, send buf ptr in msg to writebuf
      unsigned char k;
      for (k = 0; k < HOME_WIDGET_NUM; ++k)
      {
        if (HomeWidget[k].LayoutType == i)
        { // assume only one style widget for each layout
          DrawHomeWidget((Widget_t *)&HomeWidget[k]);
          
          tMessage Msg;
          SetupMessage(&Msg, WriteBufferMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_HOME_WGT );
          Msg.pBuffer = pDrawBuffer;
          RouteMsg(&Msg);
        }
      }
      break;
    }
    
    LayoutType >>= 1;
    i ++;
  }
}

static void DrawHomeWidget(Widget_t *pData)
{
  pDrawBuffer = (unsigned char *)pMyBuffer;
  
  unsigned int BufSize = Layout[pData->LayoutType].QuadNum * BYTES_PER_QUAD + SRAM_HEADER_LEN;
  unsigned int i;
  
  // Clear the buffer
  for (i = 0; i < BufSize; ++i) pDrawBuffer[i] = 0;
  
  // DrawBitmap needs the layout type when crossing half screen line
  *pDrawBuffer = pData->LayoutType;

  for (i = 0; i < pData->ItemNum; ++i)
  {
    pData->pDrawList[i].Draw((DrawInfo_t *)&pData->pDrawList[i].Info);
  }
}

static void DrawBitmap(const unsigned char *pBitmap, unsigned char X, unsigned char Y,
                       unsigned char W, unsigned char H, unsigned char BmpWidthInBytes)
{
// W is bitmap width in pixel
  unsigned char *pByte = Y / HALF_SCREEN_ROWS * BYTES_PER_QUAD * 2 + Y % HALF_SCREEN_ROWS * BYTES_PER_QUAD_LINE +
                         X / HALF_SCREEN_COLS * BYTES_PER_QUAD + (X % HALF_SCREEN_COLS >> 3) +
                         pDrawBuffer + SRAM_HEADER_LEN;

  if (!BmpWidthInBytes) BmpWidthInBytes = W % 8 ? (W >> 3) + 1: W >> 3;

//  PrintStringAndThreeDecimals("DrwBmp W:", W, " H:", H, "WB:", BmpWidthInBytes);

  unsigned char ColBit = 1 << X % 8;
  unsigned char MaskBit = BIT0;
  unsigned int Delta;
  unsigned char x, y;

  for (x = 0; x < W; ++x)
  {
    for(y = 0; y < H; ++y)
    {
      if (MaskBit & *(pBitmap + y * BmpWidthInBytes))
      {
        Delta = (*pDrawBuffer == LAYOUT_FULL_SCREEN) &&
                (Y < HALF_SCREEN_ROWS  && (Y + y) >= HALF_SCREEN_ROWS) ?
                BYTES_PER_QUAD : 0;
        
        *(pByte + y * BYTES_PER_QUAD_LINE + Delta) |= ColBit;
      }
    }

    MaskBit <<= 1;
    if (MaskBit == 0)
    {
      MaskBit = BIT0;
      pBitmap ++;
    }
    
    ColBit <<= 1;
    if (ColBit == 0)
    {
      ColBit = BIT0;
      pByte ++;
      // check next pixel x
      if ((X + x + 1) == HALF_SCREEN_COLS) pByte += BYTES_PER_QUAD - BYTES_PER_QUAD_LINE;
    }
  }
}

static void DrawText(unsigned char *pText, unsigned char Len, unsigned char X, unsigned char Y,
                     unsigned char Font, unsigned char EqualWidth)
{
//  int d; for (d = 0; d < Len; d++) PrintStringAndHexByte(" ", pText[d]); PrintString(CR);
  
  SetFont((etFontType)Font);
  const tFont *pFont = GetCurrentFont();
  unsigned char i;
  
  for (i = 0; i < Len; ++i)
  {
    if (pFont->Type == FONT_TYPE_TIME) pText[i] -= '0';
    
    unsigned char *pBitmap = GetCharacterBitmapPointer(pText[i]);
    unsigned char CharWidth = GetCharacterWidth(pText[i]);
    
    DrawBitmap(pBitmap, X, Y, CharWidth, pFont->Height, pFont->WidthInBytes);
    X += EqualWidth ? pFont->MaxWidth + 1 : CharWidth + 1;
  }
}

static void DrawHour(DrawInfo_t *Info)
{
  unsigned char Hour[3];
  Hour[0] = RTCHOUR;
  
  if (Get12H() == TWELVE_HOUR)
  {
    Hour[0] %= 12;
    if (Hour[0] == 0) Hour[0] = 12;
  }
  
  Hour[1] = Hour[0] % 10 + '0';
  Hour[0] /= 10;
  if(Hour[0] == 0) Hour[0] = TIME_CHARACTER_SPACE_INDEX;
  Hour[0] += '0';
  Hour[2] = Info->Opt; // separator
  
  DrawText(Hour, Info->Opt ? 3 : 2, Info->X, Info->Y, Info->Id, pdTRUE);
}

static void DrawAmPm(DrawInfo_t *Info)
{
  if (Get12H() != TWELVE_HOUR) return;
  DrawText(RTCHOUR > 11 ? "pm" : "am", 2, Info->X, Info->Y, Info->Id, pdFALSE);
}

static void DrawMin(DrawInfo_t *Info)
{
  unsigned char Min[2];
  Min[0] = RTCMIN / 10 + '0';
  Min[1] = RTCMIN % 10 + '0';
  DrawText(Min, 2, Info->X, Info->Y, Info->Id, pdTRUE);
}

static void DrawSec(DrawInfo_t *Info)
{
  if (!nvDisplaySeconds || Overlapping(Info->Opt)) return;

  unsigned char Sec[3];
  Sec[0] = DRAW_OPT_SEPARATOR;
  Sec[1] = RTCSEC / 10 + '0';
  Sec[2] = RTCSEC % 10 + '0';
  DrawText(Sec, 3, Info->X, Info->Y, Info->Id, pdFALSE);
}

static void DrawDate(DrawInfo_t *Info)
{
  if (Overlapping(Info->Opt)) return;
  
  unsigned char pDate[5];
  if (GetMonthFirst() == MONTH_FIRST)
  {
    pDate[0] = RTCMON / 10 + '0';
    pDate[1] = RTCMON % 10 + '0';
    pDate[2] = '/';
    pDate[3] = RTCDAY / 10 + '0';
    pDate[4] = RTCDAY % 10 + '0';
  }
  else
  {
    pDate[0] = RTCDAY / 10 + '0';
    pDate[1] = RTCDAY % 10 + '0';
    pDate[2] = '/';
    pDate[3] = RTCMON / 10 + '0';
    pDate[4] = RTCMON % 10 + '0';
  }
  DrawText(pDate, 5, Info->X, Info->Y, Info->Id, pdFALSE);
}

static void DrawDayofWeek(DrawInfo_t *Info)
{
  if (Overlapping(Info->Opt)) return;
  
  const char *pDow = DaysOfTheWeek[GetLanguage()][RTCDOW];
  DrawText((unsigned char *)pDow, strlen(pDow), Info->X, Info->Y, Info->Id, pdFALSE);
}

static unsigned char Overlapping(unsigned char Option)
{
  unsigned char BT = BluetoothState();
  
  return ((Option & DRAW_OPT_OVERLAP_BATTERY) &&
          (Charging() || ReadBatterySenseAverage() <= BatteryCriticalLevel(CRITICAL_WARNING)) ||
          (Option & DRAW_OPT_OVERLAP_BT) && BT != Connect ||
          (Option & DRAW_OPT_OVERLAP_SEC) && nvDisplaySeconds);
}

static void DrawBluetoothState(DrawInfo_t *Info)
{
  unsigned char Index = 0;
  
  if (!RadioOn()) Index = ICON_BLUETOOTH_OFF;
  else if (Connected(CONN_TYPE_MAIN)) Index = ICON_BLUETOOTH_CONN;
  else if (OnceConnected()) Index = ICON_BLUETOOTH_DISC;
  else Index = ICON_BLUETOOTH_ON;
  
  if (Index != ICON_BLUETOOTH_OFF && Index != ICON_BLUETOOTH_DISC) return;
  
//  PrintStringAndDecimal("- DrwBT:", Index);
//  char d; for (d = 0; d < 20; d++) PrintHex(IconInfo[Info->Id].pIconSet[Index * IconInfo[Info->Id].Width * IconInfo[Info->Id].Height + d]); PrintString(CR);
  
  DrawBitmap(IconInfo[Info->Id].pIconSet + Index * IconInfo[Info->Id].Width * IconInfo[Info->Id].Height,
             Info->X, Info->Y, IconInfo[Info->Id].Width * 8, IconInfo[Info->Id].Height, IconInfo[Info->Id].Width);
  
//  Index ++; if (Index == 6) Index = 0;
}

static const unsigned char *GetBatteryIcon(unsigned char Id)
{
  unsigned int Level = ReadBatterySenseAverage();
  unsigned char Index = 0;
  
  if (Level < BATTERY_FULL_LEVEL)
  {
    if (Level <= BatteryCriticalLevel(CRITICAL_WARNING) && !Charging()) Index = 1; // warning icon index
    else
    {
      unsigned int Empty = BatteryCriticalLevel(CRITICAL_BT_OFF);
      unsigned int Step = (BATTERY_FULL_LEVEL - Empty) / BATTERY_LEVEL_NUMBER;
      
      while (Level > (Empty + Step * Index)) Index ++;
    }
  }
  else Index = BATTERY_LEVEL_NUMBER;
  
  const unsigned char *pIcon = IconInfo[Id].pIconSet + (Index * IconInfo[Id].Width * IconInfo[Id].Height);
  if (!Charging()) pIcon += BATTERY_ICON_NUM * IconInfo[Id].Width * IconInfo[Id].Height;

//  PrintStringAndThreeDecimals("- Batt L:", Level, " I:", Index, "P:", ExtPower());
  return pIcon;
}

static void DrawBatteryStatus(DrawInfo_t *Info)
{
  if (!Charging() && ReadBatterySenseAverage() > BatteryCriticalLevel(CRITICAL_WARNING)) return;

  DrawBitmap(GetBatteryIcon(Info->Id), Info->X, Info->Y,
    IconInfo[Info->Id].Width * 8, IconInfo[Info->Id].Height, IconInfo[Info->Id].Width);
}

static void DrawTemplate(DrawInfo_t *Info)
{
// draw template to 4 quads
  unsigned char *pByte = pDrawBuffer + SRAM_HEADER_LEN;
  unsigned char TempId = Info->Id & TEMPLATE_ID_MASK;
  unsigned char i;
  
  if (Info->Id & FLASH_TEMPLATE_BIT)
  {
//    const unsigned char __data20 *pTemp20 = pWatchFace[TempId];
    
    for (i = 0; i < LCD_ROW_NUM; ++i)
    {
      if (i == HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

//      memcpy(pByte, pTemp20, BYTES_PER_QUAD_LINE);
//      memcpy(pByte + BYTES_PER_QUAD, pTemp20 + BYTES_PER_QUAD_LINE, BYTES_PER_QUAD_LINE);
//      pByte += BYTES_PER_QUAD_LINE;
//      pTemp20 += BYTES_PER_LINE;
    }
  }
  else
  {
    const unsigned char *pTemp =  pTemplate[TempId];

    for (i = 0; i < LCD_ROW_NUM; ++i)
    {
      if (i == HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

      memcpy(pByte, pTemp, BYTES_PER_QUAD_LINE);
      memcpy(pByte + BYTES_PER_QUAD, pTemp + BYTES_PER_QUAD_LINE, BYTES_PER_QUAD_LINE);
      pByte += BYTES_PER_QUAD_LINE;
      pTemp += BYTES_PER_LINE;
    }
  }
}

static void DrawDateTime()
{
  // clean date&time area
  FillMyBuffer(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM, 0x00);

  DrawHours();
  DrawMins();
  
  if (nvDisplaySeconds)
  {
    WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
    DrawSecs();
  }
  else
  {
    if (Get12H() == TWELVE_HOUR)
    {
      gRow = DEFAULT_AM_PM_ROW;
      gColumn = DEFAULT_AM_PM_COL;
      gBitColumnMask = DEFAULT_AM_PM_COL_BIT;
      SetFont(DEFAULT_AM_PM_FONT);
      WriteFontString((RTCHOUR >= 12) ? "PM" : "AM");
    }

    gRow = Get12H() == TWENTY_FOUR_HOUR ? DEFAULT_DOW_24HR_ROW : DEFAULT_DOW_12HR_ROW;
    gColumn = DEFAULT_DOW_COL;
    gBitColumnMask = DEFAULT_DOW_COL_BIT;
    SetFont(DEFAULT_DOW_FONT);
    WriteFontString((tString *)DaysOfTheWeek[GetLanguage()][RTCDOW]);

    //add year when time is in 24 hour mode
    if (Get12H() == TWENTY_FOUR_HOUR)
    {
      gRow = DEFAULT_DATE_YEAR_ROW;
      gColumn = DEFAULT_DATE_YEAR_COL;
      gBitColumnMask = DEFAULT_DATE_YEAR_COL_BIT;
      SetFont(DEFAULT_DATE_YEAR_FONT);

      int year = RTCYEAR;
      WriteFontCharacter(year/1000+'0');
      year %= 1000;
      WriteFontCharacter(year/100+'0');
      year %= 100;
      WriteFontCharacter(year/10+'0');
      year %= 10;
      WriteFontCharacter(year+'0');
    }

    //Display month and day
    //Watch controls time - use default date position
    unsigned char MMDD = (GetMonthFirst() == MONTH_FIRST);

    gRow = DEFAULT_DATE_FIRST_ROW;
    gColumn = DEFAULT_DATE_FIRST_COL;
    gBitColumnMask = DEFAULT_DATE_FIRST_COL_BIT;
    SetFont(DEFAULT_DATE_MONTH_FONT);
    
    WriteFontCharacter((MMDD ? RTCMON : RTCDAY) / 10+'0');
    WriteFontCharacter((MMDD ? RTCMON : RTCDAY) % 10+'0');
    
    //Display separator
    SetFont(DEFAULT_DATE_SEPARATOR_FONT);
    WriteFontCharacter(MMDD ? '/' : '.');
    
    //Display day second
    gRow = DEFAULT_DATE_SECOND_ROW;
    gColumn = DEFAULT_DATE_SECOND_COL;
    gBitColumnMask = DEFAULT_DATE_SECOND_COL_BIT;
    SetFont(DEFAULT_DATE_DAY_FONT);
    
    WriteFontCharacter((MMDD ? RTCDAY : RTCMON) / 10+'0');
    WriteFontCharacter((MMDD ? RTCDAY : RTCMON) % 10+'0');
  }
  SendMyBufferToLcd(STARTING_ROW, WATCH_DRAW_SCREEN_ROW_NUM);
}

static void DrawHours(void)
{  
  unsigned char msd;
  unsigned char lsd;
  
  /* display hour */
  int Hour = RTCHOUR;

  /* if required convert to twelve hour format */
  if ( Get12H() == TWELVE_HOUR )
  {
    Hour %= 12;
    if (Hour == 0) Hour = 12;
  }
  
  msd = Hour / 10;
  lsd = Hour % 10;
  
  //Watch controls time - use default position
  gRow = DEFAULT_HOURS_ROW;
  gColumn = DEFAULT_HOURS_COL;
  gBitColumnMask = DEFAULT_HOURS_COL_BIT;
  SetFont(DEFAULT_HOURS_FONT);
  
  /* if first digit is zero then leave location blank */
  if ( msd == 0 && Get12H() == TWELVE_HOUR )
  {
    WriteFontCharacter(TIME_CHARACTER_SPACE_INDEX);
  }
  else
  {
    WriteFontCharacter(msd);
  }

  WriteFontCharacter(lsd);
  
  //Draw colon char to separate HH and MM (uses same font has HH, but 
  //can have different position
  //Watch controls time - use current position (location after
  //writing HH, which updates gRow and gColumn and gColumnBitMask)
  SetFont(DEFAULT_TIME_SEPARATOR_FONT);
  
  WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
}

static void DrawMins(void)
{  
  //Watch controls time - use default position
  gRow = DEFAULT_MINS_ROW;
  gColumn = DEFAULT_MINS_COL;
  gBitColumnMask = DEFAULT_MINS_COL_BIT;
  SetFont(DEFAULT_MINS_FONT);
  
/* display minutes */
  WriteFontCharacter(RTCMIN / 10);
  WriteFontCharacter(RTCMIN % 10);
}

static void DrawSecs(void)
{
  //Watch controls time - use default position
  gRow = DEFAULT_SECS_ROW;
  gColumn = DEFAULT_SECS_COL;
  gBitColumnMask = DEFAULT_SECS_COL_BIT;
  SetFont(DEFAULT_SECS_FONT);
  
  //Draw colon separator - only needed when seconds are displayed
  WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
      
  WriteFontCharacter(RTCSEC / 10);
  WriteFontCharacter(RTCSEC % 10);
}

static void InitializeIdleBufferConfig(void)
{
  NvIdle = WATCH_DRAW_TOP ;
  OsalNvItemInit(NVID_IDLE_BUFFER_CONFIGURATION, sizeof(NvIdle), &NvIdle);
}

static void InitializeIdleBufferInvert(void)
{
  nvIdleBufferInvert = 0;
  OsalNvItemInit(NVID_IDLE_BUFFER_INVERT,
                 sizeof(nvIdleBufferInvert),
                 &nvIdleBufferInvert);
}

static void InitializeDisplaySeconds(void)
{
  nvDisplaySeconds = 0;
  OsalNvItemInit(NVID_DISPLAY_SECONDS,
                 sizeof(nvDisplaySeconds),
                 &nvDisplaySeconds);
}

/******************************************************************************/

static unsigned int CharacterMask;
static unsigned char CharacterRows;
static unsigned char CharacterWidth;
static unsigned int bitmap[MAX_FONT_ROWS];

/* fonts can be up to 16 bits wide */
static void WriteFontCharacter(unsigned char Character)
{
  CharacterMask = BIT0;
  CharacterRows = GetFontHeight();
  CharacterWidth = GetCharacterWidth(Character);
  GetCharacterBitmap(Character,(unsigned int*)&bitmap);

  if ( gRow + CharacterRows > LCD_ROW_NUM )
  {
    PrintString("# WriteFontChar\r\n");
    return;
  }

  /* do things bit by bit */
  unsigned char i;
  unsigned char row;

  for (i = 0 ; i < CharacterWidth && gColumn < BYTES_PER_LINE; i++ )
  {
  	for(row = 0; row < CharacterRows; row++)
    {
      if ( (CharacterMask & bitmap[row]) != 0 )
      {
        pMyBuffer[gRow+row].Data[gColumn] |= gBitColumnMask;
      }
    }

    /* the shift direction seems backwards... */
    CharacterMask = CharacterMask << 1;
    gBitColumnMask = gBitColumnMask << 1;
    if ( gBitColumnMask == 0 )
    {
      gBitColumnMask = BIT0;
      gColumn++;
    }
  }

  /* add spacing between characters */
  if(gColumn < BYTES_PER_LINE)
  {
    unsigned char FontSpacing = GetFontSpacing();
    for(i = 0; i < FontSpacing; i++)
    {
      gBitColumnMask = gBitColumnMask << 1;
      if ( gBitColumnMask == 0 )
      {
        gBitColumnMask = BIT0;
        gColumn++;
      }
    }
  }
}

void WriteFontString(tString *pString)
{
  unsigned char i = 0;

  while (pString[i] != 0 && gColumn < BYTES_PER_LINE)
  {
    WriteFontCharacter(pString[i++]);
  }
}

void ShowNotification(tString *pString, unsigned char Type)
{
  static tString CallerId[15] = "";
  tMessage Msg;

  if (Type == SHOW_NOTIF_CALLER_ID) strcpy(CallerId, pString);
  
  if (Type == SHOW_NOTIF_CALLER_NAME && *CallerId)
  {
    PrintString3("- Caller:", pString, CR);
  
    SetupMessageAndAllocateBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);
    *(tSetVibrateModePayload *)Msg.pBuffer = RingTone;
    RouteMsg(&Msg);
    
    FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
    CopyRowsIntoMyBuffer(pCallNotif, CALL_NOTIF_START_ROW, CALL_NOTIF_ROWS);

    gRow = 70;
    gColumn = 1;
    SetFont(MetaWatch7);
    gBitColumnMask = BIT4;
    WriteFontString(CallerId);

    gRow = 48;
    SetFont(MetaWatch16);

    // align center
    unsigned char i = 0;
    unsigned char NameWidth = 0;
    for (; i < strlen(pString); ++i) NameWidth += GetCharacterWidth(pString[i]);
    NameWidth += GetFontSpacing() * strlen(pString);
    
    gColumn = ((LCD_COL_NUM - NameWidth) >> 1) / 8;
    gBitColumnMask = 1 << ((LCD_COL_NUM - NameWidth) >> 1) % 8;

//    PrintStringAndThreeDecimals("W:", NameWidth, "C:", gColumn, "M:", gBitColumnMask);

    WriteFontString(pString);
    SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
    
    PageType = PAGE_TYPE_INFO;
    CurrentPage[PageType] = CallPage;

    // set a 5s timer for switching back to idle screen
    SetupOneSecondTimer(DisplayTimerId, 5, NO_REPEAT, DISPLAY_QINDEX, CallerNameMsg, SHOW_NOTIF_END);
    StartOneSecondTimer(DisplayTimerId);
  }
  else if (Type == SHOW_NOTIF_END || Type == SHOW_NOTIF_REJECT_CALL)
  {
    PrintString2("- Call Notif End", CR);
    
    *CallerId = NULL;
    StopOneSecondTimer(DisplayTimerId);

    PageType = PAGE_TYPE_IDLE;
    SendMessage(&Msg, UpdateDisplay, CurrentMode | MSG_OPT_UPD_INTERNAL |
                (CurrentMode == IDLE_MODE ? MSG_OPT_NEWUI : 0));

    if (Type == SHOW_NOTIF_REJECT_CALL) SendMessage(&Msg, HfpMsg, MSG_OPT_HFP_HANGUP);
  }
}

static void HandleMusicPlayStateChange(unsigned char State)
{
  // music icon fits into one message
  PrintStringAndDecimal("- HdlMusicPlyChg:", State);
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, WriteBufferMsg,
    MUSIC_MODE | MSG_OPT_WRTBUF_MULTILINE | ICON_MUSIC_WIDTH << 3);
  
  Msg.pBuffer[0] = MUSIC_STATE_START_ROW;
  Msg.Length = ICON_MUSIC_WIDTH * ICON_MUSIC_HEIGHT + 1;
  PrintStringAndTwoDecimals("- start:", Msg.pBuffer[0], "L:", Msg.Length);
  
  unsigned char i = 1;
  for (; i < Msg.Length; ++i) Msg.pBuffer[i] = pIconMusicState[1 - State][i - 1];
  RouteMsg(&Msg);
  
  SendMessage(&Msg, UpdateDisplay, MUSIC_MODE);
}

void EraseTemplateHandler(tMessage *pMsg)
{
  unsigned char __data20* addr = 0;
  unsigned char cnt = TEMPLATE_NUM_FLASH_PAGES;
  
  addr = (unsigned char __data20 *)&pWatchFace[*pMsg->pBuffer-TEMPLATE_1][0];
    
  //Each template is 3 Flash pages long
  do {
    flashErasePageData20(addr);
    addr += HAL_FLASH_PAGE_SIZE;
  } while (--cnt);
}

void WriteToTemplateHandler(tMessage *pMsg)
{
  unsigned char __data20* baseTempAddr = 0;
  
  /* map the payload */
  tWriteToTemplateMsgPayload* pData = (tWriteToTemplateMsgPayload*)pMsg->pBuffer;
  
  unsigned char RowA = pData->RowSelectA;
  unsigned char RowB = pData->RowSelectB;
  
  baseTempAddr = (unsigned char __data20*)&pWatchFace[pData->TemplateSelect-TEMPLATE_1][0]; 
  
  //Get address of 1st row
  unsigned char __data20 *addr = baseTempAddr + (RowA * BYTES_PER_LINE);
  flashWriteData20(addr, BYTES_PER_LINE, pData->pLineA);
  
  //Check if writing 1 or 2 lines.
  if ((pMsg->Options & MSG_OPT_WRTBUF_1_LINE))
  {
    //Writing 2 lines
    
    //Get address of 2nd row
    unsigned char __data20 *addr = baseTempAddr + (RowB * BYTES_PER_LINE);
    flashWriteData20(addr, BYTES_PER_LINE, pData->pLineB);
  }
}

unsigned char LcdRtcUpdateHandlerIsr(void)
{
  /* send a message every second or once a minute */
  if (RtcUpdateEnable && CurrentMode == IDLE_MODE && PageType == PAGE_TYPE_IDLE &&
     (nvDisplaySeconds || lastMin != RTCMIN))
  {
    lastMin = RTCMIN;
    
    tMessage Msg;
    SetupMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
    SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
    return pdTRUE;
  }
  else return pdFALSE;
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
    ENABLE_LCD_LED();
    LedOn = 1;
    StartOneSecondTimer(LedTimerId);
    break;

  case LED_TOGGLE_OPTION:
    if ( LedOn )
    {
      StopOneSecondTimer(LedTimerId);
      DISABLE_LCD_LED();
      LedOn = 0;
    }
    else
    {
      ENABLE_LCD_LED();
      LedOn = 1;
      StartOneSecondTimer(LedTimerId);
    }
    break;

  case LED_OFF_OPTION:
  default:
    StopOneSecondTimer(LedTimerId);
    DISABLE_LCD_LED();
    LedOn = 0;
    break;
  }
}

unsigned char BackLightOn(void)
{
  return LedOn;
}

unsigned char CurrentIdlePage(void)
{
  return CurrentPage[PageType];
}

static void HandleVersionInfo(void)
{
  tMessage Msg;
  tVersion  Version;
  
  SetupMessageAndAllocateBuffer(&Msg, GetInfoStringResponse, MSG_OPT_NONE);

  strcpy((char *)Msg.pBuffer, BUILD);
  Msg.Length += strlen(BUILD);
  
  Version = GetWrapperVersion();
  strcpy((char *)Msg.pBuffer + Msg.Length, Version.pSwVer);
  Msg.Length += strlen(Version.pSwVer);
  
  *(Msg.pBuffer + Msg.Length++) = (unsigned char)atoi(VERSION);
  
  unsigned char i = 0;
  while (VERSION[i++] != '.');
  *(Msg.pBuffer + Msg.Length++) = atoi(VERSION + i);
  *(Msg.pBuffer + Msg.Length++) = NULL;
  
//  while (VERSION[i++] != '.');
//  *(Msg.pBuffer + Msg.Length++) = atoi(VERSION + i);
//
  RouteMsg(&Msg);
  
  PrintString("-Ver:"); for (i = 6; i < Msg.Length; ++i) PrintHex(Msg.pBuffer[i]);
}

/*! Read the voltage of the battery. This provides power good, battery charging,
 * battery voltage, and battery voltage average.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
static void ReadBatteryVoltageHandler(void)
{
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, ReadBatteryVoltageResponse, MSG_OPT_NONE);

  /* if the battery is not present then these values are meaningless */
  Msg.pBuffer[0] = ExtPower();
  Msg.pBuffer[1] = Charging();

  unsigned int bv = ReadBatterySense();
  Msg.pBuffer[2] = bv & 0xFF;
  Msg.pBuffer[3] = (bv >> 8 ) & 0xFF;

  bv = ReadBatterySenseAverage();
  Msg.pBuffer[4] = bv & 0xFF;
  Msg.pBuffer[5] = (bv >> 8 ) & 0xFF;
  Msg.Length = 6;

  RouteMsg(&Msg);

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
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, ReadLightSensorResponse, MSG_OPT_NONE);

  /* instantaneous value */
  unsigned int lv = ReadLightSense();
  Msg.pBuffer[0] = lv & 0xFF;
  Msg.pBuffer[1] = (lv >> 8 ) & 0xFF;

  /* average value */
  lv = ReadLightSenseAverage();
  Msg.pBuffer[2] = lv & 0xFF;
  Msg.pBuffer[3] = (lv >> 8 ) & 0xFF;
  Msg.Length = 4;
  RouteMsg(&Msg);
}

/*! Setup the battery monitor interval - only happens at startup */
static void InitializeBatteryMonitorInterval(void)
{
  nvBatteryMonitorIntervalInSeconds = INTERVAL_SEC_BATTERY_MONITOR;

  OsalNvItemInit(NVID_BATTERY_SENSE_INTERVAL,
                 sizeof(nvBatteryMonitorIntervalInSeconds),
                 &nvBatteryMonitorIntervalInSeconds);

}

static void InitializeRstNmiConfiguration(void)
{
  unsigned char RstControl;
  
#ifdef HW_DEVBOARD_V2
   RstControl = RST_PIN_ENABLED;
#else
   RstControl = RST_PIN_DISABLED;
#endif

  OsalNvItemInit(NVID_RSTNMI_CONFIGURATION, sizeof(RstControl), &RstControl);
  ConfigResetPin(RstControl);
}

/* choose whether or not to do a master reset (reset non-volatile values) */
static void SoftwareResetHandler(tMessage* pMsg)
{
  if (pMsg->Options == MASTER_RESET_OPTION) WriteMasterResetKey();
  SoftwareReset();
}

static void NvalOperationHandler(tMessage* pMsg)
{

  /* overlay */
  tNvalOperationPayload* pNvPayload = (tNvalOperationPayload*)pMsg->pBuffer;

  /* create the outgoing message */
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, NvalOperationResponseMsg, NV_FAILURE);

  /* add identifier to outgoing message */
  tWordByteUnion Identifier;
  Identifier.word = pNvPayload->NvalIdentifier;
  Msg.pBuffer[0] = Identifier.Bytes.byte0;
  Msg.pBuffer[1] = Identifier.Bytes.byte1;
  Msg.Length = 2;

  PrintStringAndTwoHexBytes("- Nval Id:Val 0x", Identifier.word, pNvPayload->DataStartByte);
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
    Msg.Options = OsalNvRead(pNvPayload->NvalIdentifier,
                                     NV_ZERO_OFFSET,
                                     pNvPayload->Size,
                                     &Msg.pBuffer[2]);

    Msg.Length += pNvPayload->Size;

    break;

  case NVAL_WRITE_OPERATION:

    /* check that the size matches (otherwise NV_FAILURE is sent) */
    if (OsalNvItemLength(pNvPayload->NvalIdentifier) == pNvPayload->Size)
    {
      /* update the copy in ram */
      NvUpdater(pNvPayload);
      
      Msg.Options = OsalNvWrite(pNvPayload->NvalIdentifier,
                                NV_ZERO_OFFSET,
                                pNvPayload->Size,
                                (void*)(&pNvPayload->DataStartByte));
    }

    break;

  default:
    break;
  }

  RouteMsg(&Msg);
}

/******************************************************************************/

/* The value in RAM must be updated if the phone writes the value in
 * flash (until the code is changed to read the value from flash)
 */
static void NvUpdater(tNvalOperationPayload *pNval)
{
  tMessage Msg;
  
  switch (pNval->NvalIdentifier)
  {
    case NVID_TIME_FORMAT:
      Set12H(pNval->DataStartByte);
      SendMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
      break;

    case NVID_DATE_FORMAT:
      SetMonthFirst(pNval->DataStartByte);
      SendMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
      break;

    case NVID_DISPLAY_SECONDS:
      nvDisplaySeconds = pNval->DataStartByte;
      SendMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
      break;

    case NVID_IDLE_BUFFER_INVERT:
      nvIdleBufferInvert = pNval->DataStartByte;
      SendMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
      break;

    case NVID_IDLE_BUFFER_CONFIGURATION:
      NvIdle = pNval->DataStartByte; //watch_control_top
      break;
      
    case NVID_IDLE_MODE_TIMEOUT:
    case NVID_APP_MODE_TIMEOUT:
    case NVID_NOTIF_MODE_TIMEOUT:
    case NVID_MUSIC_MODE_TIMEOUT:
      InitModeTimeout();
      break;

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
      InitLinkAlarmEnable();
      break;

    case NVID_LINK_ALARM_DURATION:
      break;

    case NVID_PAIRING_MODE_DURATION:
      /* not for phone control - reset watch */
      break;

#ifdef ANALOG
    case NVID_IDLE_DISPLAY_TIMEOUT:
    case NVID_APPLICATION_DISPLAY_TIMEOUT:
    case NVID_NOTIFICATION_DISPLAY_TIMEOUT:
    case NVID_RESERVED_DISPLAY_TIMEOUT:
      InitializeDisplayTimeouts();
      break;

    case NVID_TOP_OLED_CONTRAST_DAY:
    case NVID_BOTTOM_OLED_CONTRAST_DAY:
    case NVID_TOP_OLED_CONTRAST_NIGHT:
    case NVID_BOTTOM_OLED_CONTRAST_NIGHT:
      InitializeContrastValues();
      break;
#endif

    default:
      PrintStringAndDecimal("# Unkwn nval:", pNval->NvalIdentifier);
      break;
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
  if (   BluetoothState() == Connect
      && QuerySniffState() == Sniff )
  {
    tMessage Msg;
    SetupMessage(&Msg, RateTestMsg, MSG_OPT_NONE);
    SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
    ExitLpm = 1;
  }

  return ExitLpm;
}
#endif

unsigned char BatteryPercentage(void)
{
  int BattVal = ReadBatterySenseAverage();
  if (BattVal > BATTERY_FULL_LEVEL) BattVal = BATTERY_FULL_LEVEL;
  BattVal -= BatteryCriticalLevel(CRITICAL_BT_OFF);
  BattVal = BattVal > 0 ? BattVal * 10 /
    ((BATTERY_FULL_LEVEL - BatteryCriticalLevel(CRITICAL_BT_OFF)) / 10) : 0;
    
//  PrintStringAndTwoDecimals("- Batt%:", BattVal,  "Aver:", ReadBatterySenseAverage());
  return (unsigned char)BattVal;
}

/******************************************************************************/
/*
 * this writes a signature to the EnterBoot RAM location
 * and generates a reset.  The signature tells the bootloader
 * to stay in bootload mode to accept bootload messaging.
 */
static void EnterBootloader(void)
{
  PrintString("- Entering Bootloader Mode\r\n");

  FillMyBuffer(STARTING_ROW, LCD_ROW_NUM, 0x00);
  CopyRowsIntoMyBuffer(pBootloader, BOOTLOADER_START_ROW, BOOTLOADER_ROWS);
  
  // Draw version
  gColumn = 4;
  gRow = 68;
  gBitColumnMask = BIT5;
  SetFont(MetaWatch5);
  WriteFontString("V ");
  WriteFontString((char *)VERSION);

  DrawLocalAddress(1, 80);
  
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
  
  __disable_interrupt();

  /* disable RAM alternate interrupt vectors */
  SYSCTL &= ~SYSRIVECT;
  SetBootloaderSignature();
  SoftwareReset();
}

/******************************************************************************/
/* This originally was for battery charge control, but now it handles
 * other things that must be periodically checked such as 
 * mux control and task check in (watchdog)
 *
 */
static void BatteryChargeControlHandler(void)
{
  tMessage Msg;

  /* change the mux settings accordingly */
  ChangeMuxMode();

  /* enable/disable serial commands */
  TestModeControl();

  /* enable the charge circuit if power is good
   *
   * update the screen if there has been a change in charging status 
   */
  /* periodically check if the charging clip is present */
  unsigned char Power = ExtPower();

//  PrintString3("- ChrgCtrl: Pwr ", Power ? "Good" : "Bad", CR);

  if (BatteryChargingControl(Power))
  {
    PrintString2("= BattChgd", CR);
    if (CurrentPage[PageType] == StatusPage) WatchStatusScreenHandler();
    UpdateHomeWidget(MSG_OPT_NONE); // must be last to avoid screen mess-up
  }

  BatterySenseCycle();
  LowBatteryMonitor(Power);

  /* Watchdog requires each task to check in.
   * If the background task reaches here then it can check in.
   * Send a message to the other tasks so that they must check in.
   */
  TaskCheckIn(eDisplayTaskCheckInId);

  SendMessage(&Msg, WrapperTaskCheckInMsg,  MSG_OPT_NONE);
  SendMessage(&Msg, QueryMemoryMsg, MSG_OPT_NONE);

#if TASK_DEBUG
  UTL_FreeRtosTaskStackCheck();
#endif

#if 0
  LightSenseCycle();
#endif
}
