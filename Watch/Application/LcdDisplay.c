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
#include "ClockWidget.h"

#define IDLE_FULL_UPDATE   (0)
#define DATE_TIME_ONLY     (1)

#define SPLASH_START_ROW   (41)

#define PAGE_TYPE_NUM      (3)

/* the internal buffer */
#define STARTING_ROW                  ( 0 )
#define PHONE_DRAW_SCREEN_ROW_NUM     ( 66 )

#define MUSIC_STATE_START_ROW         (43)
#define BATTERY_MONITOR_INTERVAL      (10) //second

extern const char BUILD[];
extern const char VERSION[];
extern unsigned int niReset;
extern char niBuild[];

/*
 * days of week are 0-6 and months are 1-12
 */
const char DaysOfTheWeek[][7][4] =
{
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  {"su", "ma", "ti", "ke", "to", "pe", "la"},
  {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"}
};

const char MonthsOfYear[][13][7] =
{
  {"???","Jan","Feb","Mar","Apr","May","June",
  "July","Aug","Sep","Oct","Nov","Dec"},
  {"???","tami", "helmi", "maalis", "huhti", "touko", "kesä",
   "heinä", "elo", "syys", "loka", "marras", "joulu"},
  {"???","Jan","Feb","Mar","Apr","Mai","Jun",
   "Jul","Aug","Sep","Okt","Nov","Dez"}
};

static unsigned char LedOn;
static tTimerId BatteryTimerId;
static tTimerId LedTimerId = UNASSIGNED_ID;
static tTimerId DisplayTimerId = UNASSIGNED_ID;

xTaskHandle DisplayHandle;

static unsigned char RtcUpdateEnabled = pdFALSE;
static unsigned char lastMin = 61;

tLcdLine pMyBuffer[LCD_ROW_NUM];

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static eIdleModePage CurrentPage[PAGE_TYPE_NUM];

static const unsigned int ModeTimeout[] = {65535, 600, 30, 600}; // seconds

static unsigned char Splashing = pdTRUE;
static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;

static const tSetVibrateModePayload RingTone = {1, 0x00, 0x01, 0x00, 0x01, 2};

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

static void DrawSecs(void);
static void DrawMins(void);
static void DrawHours(void);
static void WriteFontCharacter(unsigned char Character);
static void WriteFontString(tString* pString);
static void DrawLocalAddress(unsigned char Col, unsigned Row);
static void HandleMusicPlayStateChange(unsigned char State);

static void ShowNotification(tString *pString, unsigned char Type);

static void LedChangeHandler(tMessage* pMsg);
static void ReadBatteryVoltageHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void SoftwareResetHandler(tMessage* pMsg);
static void MonitorBattery(void);
static void HandleVersionInfo(void);
static void DrawBatteryOnIdleScreen(unsigned char Row, unsigned char Col, etFontType Font);
static void HandleSecInvert(unsigned char Val);

#if __IAR_SYSTEMS_ICC__
static void WriteToTemplateHandler(tMessage *pMsg);
static void EraseTemplateHandler(tMessage *pMsg);
#endif

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

#if __IAR_SYSTEMS_ICC__
  case EraseTemplateMsg:
    EraseTemplateHandler(pMsg);
    break;
    
  case WriteToTemplateMsg:
    WriteToTemplateHandler(pMsg);
    break;
#endif

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
      RtcUpdateEnabled = pdTRUE;
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

const unsigned char *GetBatteryIcon(unsigned char Id)
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
  switch (pMsg->Options)
  {
  case MODIFY_TIME_INCREMENT_HOUR:
    RTCHOUR = (RTCHOUR == 23) ? 0 : RTCHOUR + 1;
    break;
    
  case MODIFY_TIME_INCREMENT_MINUTE:
    RTCMIN = (RTCMIN == 59) ? 0 : RTCMIN + 1;
    break;
    
  case MODIFY_TIME_INCREMENT_DOW:
    RTCDOW = (RTCDOW == 6) ? 0 : RTCDOW + 1;
    break;
  }
  
  ResetWatchdog();
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

void DrawDateTime()
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
  	for (row = 0; row < CharacterRows; row++)
    {
      if ((CharacterMask & bitmap[row]))
        pMyBuffer[gRow+row].Data[gColumn] |= gBitColumnMask;
//      else pMyBuffer[gRow+row].Data[gColumn] &= ~gBitColumnMask;
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

#if __IAR_SYSTEMS_ICC__
//Watch Faces Flash memory declaration
extern __data20 const unsigned char pWatchFace[][1]; //TEMPLATE_FLASH_SIZE];

static void EraseTemplateHandler(tMessage *pMsg)
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

static void WriteToTemplateHandler(tMessage *pMsg)
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
#endif

void EnableRtcUpdate(unsigned char Enable)
{
  RtcUpdateEnabled = Enable;
}

unsigned char LcdRtcUpdateHandlerIsr(void)
{
  /* send a message every second or once a minute */
  if (RtcUpdateEnabled && CurrentMode == IDLE_MODE && PageType == PAGE_TYPE_IDLE &&
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

void *GetDrawBuffer(void)
{
  return (void *)pMyBuffer;
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
  Msg.Length = 5;

  Msg.pBuffer[0] = ClipOn();
  Msg.pBuffer[1] = Charging();

  unsigned int bv = BatteryLevel();
  Msg.pBuffer[2] = bv & 0xFF;
  Msg.pBuffer[3] = (bv >> 8) & 0xFF;

  Msg.pBuffer[4] = BatteryPercentage();

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
