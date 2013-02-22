//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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
#include "Vibration.h"
#include "SerialRam.h"
#include "Icons.h"
#include "Fonts.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "IdleTask.h"
#include "TermMode.h"
#include "BufferPool.h"
#include "MuxMode.h"
#include "Property.h"

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

#define DRAW_OPT_SEPARATOR            (':')
#define DRAW_OPT_OVERLAP_BT           (1)
#define DRAW_OPT_OVERLAP_BATTERY      (2)
#define DRAW_OPT_OVERLAP_SEC          (4)

#define MUSIC_STATE_START_ROW         (43)
#define BATTERY_MONITOR_INTERVAL      (10) //second

/*! Languages */ 
#define LANG_EN (0)
#define LANG_FI (1)
#define LANG_DE (2)

extern const char BUILD[];
extern const char VERSION[];
extern unsigned int niReset;
extern char niBuild[];
/*
 * days of week are 0-6 and months are 1-12
 */
static const tString DaysOfTheWeek[3][7][4] =
{
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  {"su", "ma", "ti", "ke", "to", "pe", "la"},
  {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"}
};

const tString MonthsOfYear[3][13][7] =
{
  {"???","Jan","Feb","Mar","Apr","May","June",
  "July","Aug","Sep","Oct","Nov","Dec"},
  {"???","tami", "helmi", "maalis", "huhti", "touko", "kesä",
   "heinä", "elo", "syys", "loka", "marras", "joulu"},
  {"???","Jan","Feb","Mar","Apr","Mai","Jun",
   "Jul","Aug","Sep","Okt","Nov","Dez"}
};

/* background.c */
static unsigned char LedOn;
static tTimerId BatteryTimerId;
static tTimerId LedTimerId = UNASSIGNED_ID;
static tTimerId DisplayTimerId = UNASSIGNED_ID;

xTaskHandle DisplayHandle;

static unsigned char RtcUpdateEnable = 0;
static unsigned char lastMin = 61;

static tLcdLine pMyBuffer[LCD_ROW_NUM];

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static eIdleModePage CurrentPage[PAGE_TYPE_NUM];

static const unsigned int ModeTimeout[] = {65535, 600, 30, 600}; // seconds

static unsigned char Splashing = pdTRUE;
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
static void ModifyTimeHandler(tMessage* pMsg);
static void MenuModeHandler(unsigned char Option);
static void MenuButtonHandler(unsigned char MsgOptions);
static void ServiceMenuHandler(void);
static void ToggleSerialGndSbw(void);
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

static void ShowNotification(tString *pString, unsigned char Type);

static void LedChangeHandler(tMessage* pMsg);
static void ReadBatteryVoltageHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void SoftwareResetHandler(tMessage* pMsg);
static void MonitorBattery(void);
static void HandleVersionInfo(void);
static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font);
static void HandleSecInvert(unsigned char Val);

/******************************************************************************/

#define DISPLAY_TASK_QUEUE_LENGTH 30
#define DISPLAY_TASK_STACK_SIZE  	(configMINIMAL_STACK_SIZE + 100) //total 48-88
#define DISPLAY_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

void CreateDisplayTask(void)
{
  QueueHandles[DISPLAY_QINDEX] =
    xQueueCreate(DISPLAY_TASK_QUEUE_LENGTH, MESSAGE_QUEUE_ITEM_SIZE);

  if (QueueHandles[DISPLAY_QINDEX] == 0) SoftwareReset();

  // task function, task name, stack len, task params, priority, task handle
  xTaskCreate(DisplayTask, "DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &DisplayHandle);
}

/*! LCD Task Main Loop */
static void DisplayTask(void *pvParameters)
{
  static tMessage Msg;
  
  Init();

  for(;;)
  {
    if (xQueueReceive(QueueHandles[DISPLAY_QINDEX], &Msg, portMAX_DELAY))
    {
      PrintMessageType(&Msg);
      DisplayQueueMessageHandler(&Msg);
      SendToFreeQueue(&Msg);
      CheckStackUsage(DisplayHandle, "~DspStk ");
      CheckQueueUsage(QueueHandles[DISPLAY_QINDEX]);
    }
  }
}

void Init(void)
{  
  __disable_interrupt();

  ENABLE_LCD_LED();
  DISABLE_LCD_POWER();

  /* clear shipping mode, if set to allow configuration */
  PMMCTL0_H = PMMPW_H;
  PM5CTL0 &= ~LOCKLPM5;  
  PMMCTL0_H = 0x00;
  
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
#else
  SaveResetSource();
#endif
  
  SetupClockAndPowerManagementModule();
  
  CheckResetCode();
  if (niReset != NO_RESET_CODE) InitProperty();

  InitBufferPool(); // message queue

  InitBattery();
  CheckClip();

  PrintString2("\r\n*** ", niReset == FLASH_RESET_CODE ? "FLASH" :
    (niReset == MASTER_RESET_CODE ? "MASTER" : "NORMAL"));
  PrintString3(":", niBuild, CR);
  
  ShowWatchdogInfo();
  WhoAmI();

  /* timer for battery checking at a regular frequency. */
  BatteryTimerId = AllocateOneSecondTimer();
  SetupOneSecondTimer(BatteryTimerId,
                      BATTERY_MONITOR_INTERVAL,
                      REPEAT_FOREVER,
                      DISPLAY_QINDEX,
                      MonitorBatteryMsg,
                      MSG_OPT_NONE);
  StartOneSecondTimer(BatteryTimerId);

  InitVibration();
  InitRealTimeClock(); // enable rtc interrupt

  LcdPeripheralInit();
  InitMyBuffer();
  DisplayStartupScreen();
  SerialRamInit();

  DisplayTimerId = AllocateOneSecondTimer();

  /* turn the radio on; initialize the serial port profile or BLE/GATT */
  CreateAndSendMessage(TurnRadioOnMsg, MSG_OPT_NONE);

  DISABLE_LCD_LED();
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
  
  switch (pMsg->Type)
  {
  case WriteBufferMsg:
    WriteBufferHandler(pMsg);
    break;

  case SetWidgetListMsg:
    SetWidgetList(pMsg);
    break;
  
  case UpdateDisplayMsg:
    UpdateDisplayHandler(pMsg);
    break;
    
  case UpdateHomeWidgetMsg:
    UpdateHomeWidget(pMsg->Options);
    break;
    
  case MonitorBatteryMsg:
    MonitorBattery();
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
    SetProperty(PROP_PHONE_DRAW_TOP, *pMsg->pBuffer ? PROP_PHONE_DRAW_TOP : 0);
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

  case DevTypeMsg:
    SetupMessageAndAllocateBuffer(&Msg, DevTypeRespMsg, MSG_OPT_NONE);
    Msg.pBuffer[0] = BOARD_TYPE;

#ifdef WATCH
    if (GetMsp430HardwareRevision() < 'F') Msg.pBuffer[0] = DIGITAL_WATCH_TYPE;
#endif

    Msg.Length = 1;
    RouteMsg(&Msg);
    PrintStringAndDecimal("- DevTypeResp:", Msg.pBuffer[0]);
    break;

  case VerInfoMsg:
    HandleVersionInfo();
    break;
    
  case SetVibrateMode:
    SetVibrateModeHandler(pMsg);
    break;

  case SetRtcMsg:
    halRtcSet((tRtcHostMsgPayload*)pMsg->pBuffer);

#ifdef DIGITAL
    SendMessage(&Msg, UpdateHomeWidgetMsg, MSG_OPT_NONE);
#endif
    break;

  case GetRtcMsg:
    SetupMessageAndAllocateBuffer(&Msg, RtcRespMsg, MSG_OPT_NONE);
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

  case SecInvertMsg:
    HandleSecInvert(pMsg->Options);
    break;

  case LoadTemplate:
    LoadTemplateHandler(pMsg);
    break;

  case LinkAlarmMsg:
    if (!GetProperty(PROP_DISABLE_LINK_ALARM))
    {
      SetupMessageAndAllocateBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);
      tSetVibrateModePayload* pMsgData;
      pMsgData = (tSetVibrateModePayload *)Msg.pBuffer;
      
      pMsgData->Enable = 1;
      pMsgData->OnDurationLsb = 0xC8;
      pMsgData->OnDurationMsb = 0x00;
      pMsgData->OffDurationLsb = 0xF4;
      pMsgData->OffDurationMsb = 0x01;
      pMsgData->NumberOfCycles = 1; //3
      
      RouteMsg(&Msg);
    }
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

  case TermModeMsg:
    TermModeHandler();
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

  case EnableAccelerometerMsg:
  case DisableAccelerometerMsg:
  case AccelerometerSendDataMsg:
  case AccelerometerAccessMsg:
  case AccelerometerSetupMsg:

    HandleAccelerometer(pMsg);
    break;

#if ENABLE_LIGHT_SENSOR
  case ReadLightSensorMsg:
    ReadLightSensorHandler();
    break;
#endif

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

  if (CurrentPage[PageType] == InitPage || !GetProperty(PROP_PHONE_DRAW_TOP)) DrawDateTime();
  
  if (OnceConnected())
    CreateAndSendMessage(UpdateDisplayMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL);
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
    
  if (Mode == MUSIC_MODE) SendMessage(&Msg, UpdConnParamMsg, ShortInterval);
  
  if (Mode != IDLE_MODE)
  {
    PageType = PAGE_TYPE_IDLE;
    SetupOneSecondTimer(DisplayTimerId,
                        ModeTimeout[Mode],
                        NO_REPEAT,
                        DISPLAY_QINDEX,
                        ModeTimeoutMsg,
                        Mode);
    StartOneSecondTimer(DisplayTimerId);
    
    if (Option & MSG_OPT_UPD_INTERNAL) SendMessage(&Msg, UpdateDisplayMsg, Option);
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
  
  switch (MsgOptions)
  {
  case MENU_BUTTON_OPTION_EXIT:

    IdleUpdateHandler();
    break;

  case MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH:

    if (BluetoothState() != Initializing)
      SendMessage(&Msg, RadioOn() ? TurnRadioOffMsg : TurnRadioOnMsg, MSG_OPT_NONE);
    break;

  case MENU_BUTTON_OPTION_DISPLAY_SECONDS:
    ToggleProperty(PROP_TIME_SECOND);
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM:
    ToggleProperty(PROP_DISABLE_LINK_ALARM);
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_INVERT_DISPLAY:
    ToggleProperty(PROP_INVERT_DISPLAY);
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_MENU1:
    break;
  
  case MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN:
    ToggleProperty(PROP_RSTNMI);
    ConfigResetPin(GetProperty(PROP_RSTNMI));
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_TOGGLE_SERIAL_SBW_GND:
    ToggleSerialGndSbw();
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_ENABLE_CHARGING:
    ToggleCharging();
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE:
    EnterBootloader();
    break;

  default:
    break;
  }
}

static void ToggleSerialGndSbw(void)
{
  unsigned char MuxMode = GetMuxMode();

  if (MuxMode == MUX_MODE_SERIAL) MuxMode = MUX_MODE_SPY_BI_WIRE;
  else if (MuxMode == MUX_MODE_SPY_BI_WIRE) MuxMode = MUX_MODE_GND;
  else MuxMode = MUX_MODE_SERIAL;
  
  SetMuxMode(MuxMode);
}

static void ServiceMenuHandler(void)
{
  MenuModeHandler(Menu2Page);
}

static void DrawConnectionScreen()
{
  unsigned char const* pSwash;

  if (!RadioOn()) pSwash = pBootPageBluetoothOffSwash;
  else if (ValidAuthInfo()) pSwash = pBootPagePairedSwash;
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
  char pAddr[6 * 2 + 3]; // BT_ADDR is 6 bytes long
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

  CopyColumnsIntoMyBuffer(GetProperty(PROP_TIME_SECOND) ? pSecondsOnMenuIcon : pSecondsOffMenuIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  CopyColumnsIntoMyBuffer(!GetProperty(PROP_DISABLE_LINK_ALARM) ? pLinkAlarmOnIcon : pLinkAlarmOffIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
}

static void DrawMenu2(void)
{
  CopyColumnsIntoMyBuffer(GetProperty(PROP_RSTNMI) ? pNmiPinIcon : pRstPinIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);

  unsigned char const * pIcon;
  unsigned char MuxMode = GetMuxMode();

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
  // Connection status
  CopyColumnsIntoMyBuffer(Connected(CONN_TYPE_MAIN) ? pConnectedIcon : pDisconnectedIcon,
                          3,
                          STATUS_ICON_SIZE_IN_ROWS,
                          CENTER_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);

  gRow = 31;
  gColumn = 5;
  gBitColumnMask = BIT4;
  SetFont(MetaWatch5);

  if (PairedDeviceType() == DEVICE_TYPE_BLE) WriteFontString("BLE");
  else if (PairedDeviceType() == DEVICE_TYPE_SPP) WriteFontString("BR");

  DrawBatteryOnIdleScreen(6, 9, MetaWatch7);

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
  WriteFontCharacter(')');
  
  gColumn = 2; //4;
  gRow = 65;
  gBitColumnMask = BIT2;
  WriteFontString("HW: REV ");
//  char HwVer[6] = "F";
//  if (ValidCalibration()) sprintf(HwVer, "%d", HardwareVersion());
//  WriteFontString(HwVer);
  WriteFontCharacter(GetMsp430HardwareRevision());
  
  DrawLocalAddress(1, 80);
  
  /* display entire buffer */
  SendMyBufferToLcd(STARTING_ROW, LCD_ROW_NUM);
  
  PageType = PAGE_TYPE_INFO;
  CurrentPage[PageType] = StatusPage;
}

static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font)
{
  // Battery
  CopyColumnsIntoMyBuffer(GetBatteryIcon(ICON_SET_BATTERY_V), Row,
    IconInfo[ICON_SET_BATTERY_V].Height, Col, IconInfo[ICON_SET_BATTERY_V].Width);

  SetFont(Font);
  gRow = Row + (Font == MetaWatch7 ? 23 : 21); //29
  gColumn = Col - 1; //8
  gBitColumnMask = (Font == MetaWatch7) ? BIT4 : BIT7;

  unsigned char BattVal = BatteryPercentage();
  
  if (BattVal < 100)
  {
    WriteFontCharacter(BattVal > 9 ? BattVal / 10 +'0' : ' ');
    WriteFontCharacter(BattVal % 10 +'0');
  }
  else WriteFontString("100");

  WriteFontCharacter('%');
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
  if (!GetProperty(PROP_INVERT_DISPLAY))
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
  if (CurrentPage[PageType] == InitPage || !GetProperty(PROP_PHONE_DRAW_TOP)) {DrawDateTime(); return;}

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
    CreateAndSendMessage(UpdateDisplayMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL | MSG_OPT_UPD_HWGT);
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
  
  if (!GetProperty(PROP_24H_TIME_FORMAT))
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
  if (GetProperty(PROP_24H_TIME_FORMAT)) return;
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
  if (!GetProperty(PROP_TIME_SECOND) || Overlapping(Info->Opt)) return;

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
  if (GetProperty(PROP_DDMM_DATE_FORMAT))
  {
    pDate[0] = RTCDAY / 10 + '0';
    pDate[1] = RTCDAY % 10 + '0';
    pDate[2] = '/';
    pDate[3] = RTCMON / 10 + '0';
    pDate[4] = RTCMON % 10 + '0';
  }
  else
  {
    pDate[0] = RTCMON / 10 + '0';
    pDate[1] = RTCMON % 10 + '0';
    pDate[2] = '/';
    pDate[3] = RTCDAY / 10 + '0';
    pDate[4] = RTCDAY % 10 + '0';
  }
  DrawText(pDate, 5, Info->X, Info->Y, Info->Id, pdFALSE);
}

static void DrawDayofWeek(DrawInfo_t *Info)
{
  if (Overlapping(Info->Opt)) return;
  
  const char *pDow = DaysOfTheWeek[LANG_EN][RTCDOW];
  DrawText((unsigned char *)pDow, strlen(pDow), Info->X, Info->Y, Info->Id, pdFALSE);
}

static unsigned char Overlapping(unsigned char Option)
{
  unsigned char BT = BluetoothState();
  
  return ((Option & DRAW_OPT_OVERLAP_BATTERY) &&
          (Charging() || BatteryLevel() <= BatteryCriticalLevel(CRITICAL_WARNING)) ||
          (Option & DRAW_OPT_OVERLAP_BT) && BT != Connect ||
          (Option & DRAW_OPT_OVERLAP_SEC) && GetProperty(PROP_TIME_SECOND));
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
  unsigned int Level = BatteryLevel();
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
  if (!Charging() && BatteryLevel() > BatteryCriticalLevel(CRITICAL_WARNING)) return;

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
  
  if (GetProperty(PROP_TIME_SECOND))
  {
    WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
    DrawSecs();
  }
  else if (Charging()) DrawBatteryOnIdleScreen(3, 9, MetaWatch5);
  else
  {
    if (!GetProperty(PROP_24H_TIME_FORMAT))
    {
      gRow = DEFAULT_AM_PM_ROW;
      gColumn = DEFAULT_AM_PM_COL;
      gBitColumnMask = DEFAULT_AM_PM_COL_BIT;
      SetFont(DEFAULT_AM_PM_FONT);
      WriteFontString((RTCHOUR >= 12) ? "PM" : "AM");
    }

    gRow = GetProperty(PROP_24H_TIME_FORMAT) ? DEFAULT_DOW_24HR_ROW : DEFAULT_DOW_12HR_ROW;
    gColumn = DEFAULT_DOW_COL;
    gBitColumnMask = DEFAULT_DOW_COL_BIT;
    SetFont(DEFAULT_DOW_FONT);
    WriteFontString((tString *)DaysOfTheWeek[LANG_EN][RTCDOW]);

    //add year when time is in 24 hour mode
    if (GetProperty(PROP_24H_TIME_FORMAT))
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
    unsigned char MMDD = (!GetProperty(PROP_DDMM_DATE_FORMAT));

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
  if (!GetProperty(PROP_24H_TIME_FORMAT))
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
  if (msd == 0 && !GetProperty(PROP_24H_TIME_FORMAT))
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

  for (i = 0 ; i < CharacterWidth && gColumn < BYTES_PER_LINE; i++)
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
    SetupOneSecondTimer(DisplayTimerId, 10, NO_REPEAT, DISPLAY_QINDEX, CallerNameMsg, SHOW_NOTIF_END);
    StartOneSecondTimer(DisplayTimerId);
  }
  else if (Type == SHOW_NOTIF_END || Type == SHOW_NOTIF_REJECT_CALL)
  {
    PrintString2("- Call Notif End", CR);
    
    *CallerId = NULL;
    StopOneSecondTimer(DisplayTimerId);

    PageType = PAGE_TYPE_IDLE;
    SendMessage(&Msg, UpdateDisplayMsg, CurrentMode | MSG_OPT_UPD_INTERNAL |
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
  
  SendMessage(&Msg, UpdateDisplayMsg, MUSIC_MODE);
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
     (GetProperty(PROP_TIME_SECOND) || lastMin != RTCMIN))
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
  if (LedTimerId == UNASSIGNED_ID)
  {
    LedTimerId = AllocateOneSecondTimer();
    SetupOneSecondTimer(LedTimerId, 5, NO_REPEAT, DISPLAY_QINDEX, LedChange, LED_OFF_OPTION);
  }
  
  switch (pMsg->Options)
  {
  case LED_ON_OPTION:
    ENABLE_LCD_LED();
    LedOn = 1;
    StartOneSecondTimer(LedTimerId);
    break;

  case LED_TOGGLE_OPTION:
    if (LedOn)
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
  SetupMessageAndAllocateBuffer(&Msg, VerInfoRespMsg, MSG_OPT_NONE);

  /* exclude middle '.' */
  strncpy((char *)Msg.pBuffer, BUILD, 3);
  strncpy((char *)Msg.pBuffer + 3, BUILD + 4, 3);
  Msg.Length += strlen(BUILD) - 1;
    
  *(Msg.pBuffer + Msg.Length++) = (unsigned char)atoi(VERSION);
  
  unsigned char i = 0;
  while (VERSION[i++] != '.');
  *(Msg.pBuffer + Msg.Length++) = atoi(VERSION + i);
  *(Msg.pBuffer + Msg.Length++) = NULL;
  
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
  Msg.pBuffer[0] = ClipOn();
  Msg.pBuffer[1] = Charging();

  unsigned int bv = BatteryLevel();
  Msg.pBuffer[2] = bv & 0xFF;
  Msg.pBuffer[3] = (bv >> 8 ) & 0xFF;

  Msg.pBuffer[4] = bv & 0xFF;
  Msg.pBuffer[5] = (bv >> 8 ) & 0xFF;
  Msg.Length = 6;

  RouteMsg(&Msg);

}

/* choose whether or not to do a master reset (reset non-volatile values) */
static void SoftwareResetHandler(tMessage* pMsg)
{
  if (pMsg->Options == MASTER_RESET_OPTION) SetMasterReset();
  SoftwareReset();
}

static void HandleSecInvert(unsigned char Val)
{
  if (Val == MSG_OPT_HIDE_SECOND && GetProperty(PROP_TIME_SECOND) ||
     (Val == MSG_OPT_SHOW_SECOND && !GetProperty(PROP_TIME_SECOND)))
  {
    ToggleProperty(PROP_TIME_SECOND);
    CreateAndSendMessage(UpdateHomeWidgetMsg, MSG_OPT_NONE);
  }

  else if (Val == MSG_OPT_NORMAL_DISPLAY && GetProperty(PROP_INVERT_DISPLAY) ||
          (Val == MSG_OPT_INVERT_DISPLAY && !GetProperty(PROP_INVERT_DISPLAY)))
  {
    ToggleProperty(PROP_INVERT_DISPLAY);

    if (PageType == PAGE_TYPE_IDLE) IdleUpdateHandler();
    else if (PageType == PAGE_TYPE_MENU) MenuModeHandler(0);
    else if (CurrentPage[PageType] == StatusPage) WatchStatusScreenHandler();
  }
}

static void HandleProperty(unsigned char Options)
{
  if (Options & PROPERTY_READ) CreateAndSendMessage(PropRespMsg, GetProperty(PROP_ALL));
  else
  {
    SetProperty(PROP_VALID, Options & PROP_VALID);
    CreateAndSendMessage(PropRespMsg, MSG_OPT_NONE);
    CreateAndSendMessage(UpdateDisplayMsg, IDLE_MODE | MSG_OPT_NEWUI);
  }
}

static void NvalOperationHandler(tMessage* pMsg)
{
  if (pMsg->pBuffer == NULL) // Property
  {
    HandleProperty(pMsg->Options);
  }
  else
  {
    tNvalOperationPayload* pNvPayload = (tNvalOperationPayload*)pMsg->pBuffer;

    /* create the outgoing message */
    tMessage Msg;
    SetupMessageAndAllocateBuffer(&Msg, PropRespMsg, MSG_OPT_NONE);

    /* add identifier to outgoing message */
    tWordByteUnion Identifier;
    Identifier.word = pNvPayload->NvalIdentifier;
    Msg.pBuffer[0] = Identifier.Bytes.byte0;
    Msg.pBuffer[1] = Identifier.Bytes.byte1;
    Msg.Length = 2;

    PrintStringAndTwoHexBytes("- Nval Id:Val 0x", Identifier.word, pNvPayload->DataStartByte);
    
    unsigned char Property = PropertyBit(pNvPayload->NvalIdentifier);

    if (pMsg->Options == NVAL_READ_OPERATION)
    {
      if (Property) Msg.pBuffer[2] = GetProperty(Property);
      Msg.Length += pNvPayload->Size;
    }
    else if (Property) // write operation
    {
      SetProperty(Property, pNvPayload->DataStartByte ? Property : 0);
      CreateAndSendMessage(UpdateHomeWidgetMsg, MSG_OPT_NONE);
    }
    RouteMsg(&Msg);
  }
}

/******************************************************************************/
/*
 * this writes a signature to the EnterBoot RAM location
 * and generates a reset.  The signature tells the bootloader
 * to stay in bootload mode to accept bootload messaging.
 */
void EnterBootloader(void)
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

/* this function probably belongs somewhere else */
void WhoAmI(void)
{
  PrintString3("Version: ", VERSION, CR);
  PrintString3("Build: ", BUILD, CR);

  PrintString2(BR_DEVICE_NAME, CR);
  PrintString("Msp430 Version ");
  PrintCharacter(GetMsp430HardwareRevision());
  PrintString(CR);
  
  PrintStringAndDecimal("HwVersion: ", HardwareVersion());
  PrintStringAndDecimal("BoardConfig: ", GetBoardConfiguration());
  PrintStringAndDecimal("Calibration: ", ValidCalibration());
  PrintStringAndDecimal("ErrataGroup1: ", QueryErrataGroup1());
}

/******************************************************************************/
/* This originally was for battery charge control, but now it handles
 * other things that must be periodically checked such as 
 * mux control and task check in (watchdog)
 */
static void MonitorBattery(void)
{
  static unsigned char LastPercent = 101;

#if CHECK_CSTACK
  if (LastPercent == 101) CheckCStack();
#endif

  unsigned char ClipChanged = CheckClip();
  CheckBattery();
  
  if (ClipChanged || Charging() || CurrentPage[PageType] == StatusPage)
  {
    unsigned char CurrPercent = BatteryPercentage();

    if (CurrPercent != LastPercent || ClipChanged)
    {
      if (CurrentPage[PageType] == StatusPage) WatchStatusScreenHandler();
      else UpdateHomeWidget(MSG_OPT_NONE); // must be the last to avoid screen mess-up
      
      LastPercent = CurrPercent;
    }
  }
  /* Watchdog requires each task to check in.
   * If the background task reaches here then it can check in.
   * Send a message to the other tasks so that they must check in.
   */
  TaskCheckIn(eDisplayTaskCheckInId);
  CreateAndSendMessage(WrapperTaskCheckInMsg, MSG_OPT_NONE);

#if TASK_DEBUG
  UTL_FreeRtosTaskStackCheck();
#endif

#if ENABLE_LIGHT_SENSOR
  LightSenseCycle();
#endif
}
