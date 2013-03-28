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
#include "LcdBuffer.h"

#define PAGE_TYPE_NUM                 (3)
#define MUSIC_STATE_START_ROW         (43)
#define BATTERY_MONITOR_INTERVAL      (10) //second

extern const char BUILD[];
extern const char VERSION[];
extern unsigned int niReset;
extern char niBuild[];


xTaskHandle DisplayHandle;

static unsigned char RtcUpdateEnabled = pdFALSE;
static unsigned char lastMin = 61;

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static eIdleModePage CurrentPage[PAGE_TYPE_NUM];

static const unsigned int ModeTimeout[] = {65535, 600, 30, 600}; // seconds
static const tSetVibrateModePayload RingTone = {1, 0x00, 0x01, 0x00, 0x01, 2};

static tTimerId ModeTimerId = UNASSIGNED;

static unsigned char Splashing = pdTRUE;
static unsigned char LedOn;
static unsigned char LinkAlarmEnable = pdTRUE; // temporary

/******************************************************************************/
static void DisplayTask(void *pvParameters);
static void DisplayQueueMessageHandler(tMessage* pMsg);

/* Message handlers */
static void IdleUpdateHandler();
static void DetermineIdlePage(void);
static void ChangeModeHandler(unsigned char Option);
static void ModeTimeoutHandler();
static void ModifyTimeHandler(tMessage* pMsg);
static void MenuModeHandler(unsigned char Option);
static void MenuButtonHandler(unsigned char MsgOptions);
static void ServiceMenuHandler(void);
static void ToggleSerialGndSbw(void);
static void BluetoothStateChangeHandler(tMessage *pMsg);
static void UpdateClock(void);

static void HandleMusicPlayStateChange(unsigned char State);
static void ShowCall(tString *pString, unsigned char Type);
static void SetBacklight(unsigned char Value);
static void ReadBatteryVoltageHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void SoftwareResetHandler(tMessage* pMsg);
static void MonitorBattery(void);
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

  PrintF("*** %s:%s", niReset == FLASH_RESET_CODE ? "FLASH" :
    (niReset == MASTER_RESET_CODE ? "MASTER" : "NORMAL"), niBuild);
  
  ShowWatchdogInfo();
  WhoAmI();

  /* timer for battery checking at a regular frequency. */
  StartTimer(BATTERY_MONITOR_INTERVAL, REPEAT_FOREVER, DISPLAY_QINDEX, MonitorBatteryMsg, MSG_OPT_NONE);

  InitVibration();
  InitRealTimeClock(); // enable rtc interrupt

  LcdPeripheralInit();
  DrawStartupScreen();
  SerialRamInit();

  /* turn the radio on; initialize the serial port profile or BLE/GATT */
  CreateAndSendMessage(TurnRadioOnMsg, MSG_OPT_NONE);

  DISABLE_LCD_LED();
}

/*! Handle the messages routed to the display queue */
static void DisplayQueueMessageHandler(tMessage* pMsg)
{
  tMessage Msg;
  unsigned char i = 0;

  switch (pMsg->Type)
  {
  case WriteBufferMsg:
    WriteBufferHandler(pMsg);
    break;

  case SetWidgetListMsg:
    SetWidgetList(pMsg);
    break;
  
  case UpdateDisplayMsg:
    if ((!(pMsg->Options & MSG_OPT_UPD_INTERNAL) &&
          (pMsg->Options & MODE_MASK) == NOTIF_MODE) &&
        GetProperty(PROP_AUTO_BACKLIGHT))
      SendMessage(&Msg, AutoBacklightMsg, MSG_OPT_NONE);

    UpdateDisplayHandler(pMsg);
    break;
    
  case UpdateClockMsg:
    UpdateClock();
    break;
    
  case DrawClockWidgetMsg:
    DrawClockWidget(pMsg->Options);
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
    pMsg->pBuffer[pMsg->Length] = NULL;
    ShowCall((char *)pMsg->pBuffer, pMsg->Options);
    break;
    
  case CallerNameMsg:
    if (pMsg->Length)
    {
      pMsg->pBuffer[pMsg->Length] = NULL;
      ShowCall((char *)pMsg->pBuffer, SHOW_NOTIF_CALLER_NAME);

      if (GetProperty(PROP_AUTO_BACKLIGHT)) SendMessage(&Msg, AutoBacklightMsg, MSG_OPT_NONE);
    }
    else ShowCall("", pMsg->Options);
    break;

  case MusicPlayStateMsg:
    HandleMusicPlayStateChange(pMsg->Options);
    break;
    
  case ChangeModeMsg:
    ChangeModeHandler(pMsg->Options);
    break;
  
  case ControlFullScreenMsg:
    SetProperty(PROP_PHONE_DRAW_TOP, pMsg->Options || *pMsg->pBuffer ? PROP_PHONE_DRAW_TOP : 0);
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
    Msg.Options = BOARD_TYPE; //default G2
    Msg.pBuffer[0] = BOARD_TYPE; // backward compatible

#ifdef WATCH
    if (GetMsp430HardwareRevision() < 'F')
    {
      Msg.Options = DIGITAL_WATCH_TYPE_G1;
      Msg.pBuffer[0] = DIGITAL_WATCH_TYPE_G1; // backward compatible
    }
#endif

    Msg.Length = 1;
    RouteMsg(&Msg);
    PrintF("- DevTypeResp:%d", Msg.pBuffer[0]);
    break;

  case VerInfoMsg:
    SetupMessageAndAllocateBuffer(&Msg, VerInfoRespMsg, MSG_OPT_NONE);

    /* exclude middle '.' */
    strncpy((char *)Msg.pBuffer, BUILD, 3);
    strncpy((char *)Msg.pBuffer + 3, BUILD + 4, 3);
    Msg.Length += strlen(BUILD) - 1;
      
    *(Msg.pBuffer + Msg.Length++) = (unsigned char)atoi(VERSION);
    
    while (VERSION[i++] != '.');
    *(Msg.pBuffer + Msg.Length++) = atoi(VERSION + i);
    *(Msg.pBuffer + Msg.Length++) = NULL;
    
    RouteMsg(&Msg);
    
    PrintS("-Ver:"); for (i = 6; i < Msg.Length; ++i) PrintH(Msg.pBuffer[i]);
    break;
    
  case SetVibrateMode:
    SetVibrateModeHandler(pMsg);
    break;

  case SetRtcMsg:
    halRtcSet((tRtcHostMsgPayload*)pMsg->pBuffer);

#ifdef DIGITAL
    UpdateClock();
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

  case SetBacklightMsg:
    SetBacklight(pMsg->Options);
    break;

  case AutoBacklightMsg:
    if (LightSenseCycle() < DARK_LEVEL) SetBacklight(LED_ON_OPTION);
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

  case LoadTemplateMsg:
    LoadTemplateHandler(pMsg);
    break;

  case LinkAlarmMsg:
    if (LinkAlarmEnable)
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
    PageType = PAGE_TYPE_INFO;
    CurrentPage[PageType] = StatusPage;
    DrawWatchStatusScreen();
    break;

//  case ListPairedDevicesMsg:
//    ListPairedDevicesHandler();
//    break;

  case WatchDrawnScreenTimeout:
    IdleUpdateHandler();
    break;

  case TermModeMsg:
    TermModeHandler();
    break;
    
  case LowBatteryWarningMsg:
    break;
    
  case LowBatteryBtOffMsg:
    UpdateClock();
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
    PrintF("# Disp Msg: 0x%02x", pMsg->Type);
    break;
  }
}

/*! Switch from other-mode/menu page back to idle type page
 */
static void IdleUpdateHandler(void)
{
  PageType = PAGE_TYPE_IDLE;

  UpdateClock();

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

  PrintF("- ChgModInd:0x%02x", Msg.Options);
  
  if (Option & MSG_OPT_CHGMOD_IND) return; // ask for current idle page only
    
  if (Mode == MUSIC_MODE) SendMessage(&Msg, UpdConnParamMsg, ShortInterval);
  
  StopTimer(ModeTimerId);

  if (Mode != IDLE_MODE)
  {
    PageType = PAGE_TYPE_IDLE;
    ModeTimerId = StartTimer(ModeTimeout[Mode], NO_REPEAT, DISPLAY_QINDEX, ModeTimeoutMsg, Mode);
    if (Option & MSG_OPT_UPD_INTERNAL) SendMessage(&Msg, UpdateDisplayMsg, Option);
  }
  else
  {
    IdleUpdateHandler();
  }
  
  CurrentMode = Mode;
}

static void ModeTimeoutHandler()
{
  /* send a message to the host indicating that a timeout occurred */
  PrintS("- ModChgTout");
  
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, ModeChangeIndMsg, CurrentMode);
  Msg.pBuffer[0] = eScModeTimeout;
  Msg.Length = 1;
  RouteMsg(&Msg);
  
  ChangeModeHandler(IDLE_MODE);
}

void ResetModeTimer(void)
{
  ResetTimer(ModeTimerId);
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
        else UpdateClock();
      }
      else if (PageType == PAGE_TYPE_MENU) MenuModeHandler(0);
      else if (CurrentPage[PAGE_TYPE_INFO] == StatusPage) DrawWatchStatusScreen();
    }
  }
}

static void DetermineIdlePage(void)
{
  if (Connected(CONN_TYPE_MAIN)) CurrentPage[PAGE_TYPE_IDLE] = ConnectedPage;
  else CurrentPage[PAGE_TYPE_IDLE] = OnceConnected() ? DisconnectedPage: InitPage;
  
  PrintF("- Idle Pg:%d", CurrentPage[PAGE_TYPE_IDLE]);
}

static void MenuModeHandler(unsigned char Option)
{
  PageType = PAGE_TYPE_MENU;

  if (Option) CurrentPage[PageType] = (eIdleModePage)Option;  
  DrawMenu(CurrentPage[PageType]);
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
    LinkAlarmEnable = !LinkAlarmEnable;
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_INVERT_DISPLAY:
    ToggleProperty(PROP_INVERT_DISPLAY);
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_MENU1:
    break;
  
  case MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN:
    ConfigResetPin(ResetPin() ? 1 : 0);
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
//  UpdateClockWidget(MSG_OPT_NONE);
}

static void HandleMusicPlayStateChange(unsigned char State)
{
  // music icon fits into one message
  PrintF("- HdlMusicPlyChg:%d", State);
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, WriteBufferMsg,
    MUSIC_MODE | MSG_OPT_WRTBUF_MULTILINE | ICON_MUSIC_WIDTH << 3);
  
  Msg.pBuffer[0] = MUSIC_STATE_START_ROW;
  Msg.Length = ICON_MUSIC_WIDTH * ICON_MUSIC_HEIGHT + 1;
  PrintF("- start:%d Len: %d", Msg.pBuffer[0], Msg.Length);
  
  unsigned char i = 1;
  for (; i < Msg.Length; ++i) Msg.pBuffer[i] = pIconMusicState[1 - State][i - 1];
  RouteMsg(&Msg);
  
  SendMessage(&Msg, UpdateDisplayMsg, MUSIC_MODE);
}

static void ShowCall(tString *pString, unsigned char Type)
{
  static tString CallerId[15] = "";
  static tTimerId NotifyTimerId = UNASSIGNED;
  tMessage Msg;

  if (Type == SHOW_NOTIF_CALLER_ID) strcpy(CallerId, pString);
  
  if (Type == SHOW_NOTIF_CALLER_NAME && *CallerId)
  {
    PrintF("- Caller:%s", pString);
  
    SetupMessageAndAllocateBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);
    *(tSetVibrateModePayload *)Msg.pBuffer = RingTone;
    RouteMsg(&Msg);
    
    PageType = PAGE_TYPE_INFO;
    CurrentPage[PageType] = CallPage;
    DrawCallScreen(CallerId, pString);

    // set a 5s timer for switching back to idle screen
    NotifyTimerId = StartTimer(10, NO_REPEAT, DISPLAY_QINDEX, CallerNameMsg, SHOW_NOTIF_END);
  }
  else if (Type == SHOW_NOTIF_END || Type == SHOW_NOTIF_REJECT_CALL)
  {
    PrintF("- Call Notif End");
    
    *CallerId = NULL;
    StopTimer(NotifyTimerId);

    PageType = PAGE_TYPE_IDLE;
    SendMessage(&Msg, UpdateDisplayMsg, CurrentMode | MSG_OPT_UPD_INTERNAL |
                (CurrentMode == IDLE_MODE ? MSG_OPT_NEWUI : 0));

    if (Type == SHOW_NOTIF_REJECT_CALL) SendMessage(&Msg, HfpMsg, MSG_OPT_HFP_HANGUP);
  }
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
    SetupMessage(&Msg, UpdateClockMsg, MSG_OPT_NONE);
    SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
    return pdTRUE;
  }
  else return pdFALSE;
}

static void UpdateClock(void)
{
  if (CurrentMode == IDLE_MODE && PageType == PAGE_TYPE_IDLE)
  {
    if (CurrentPage[PageType] == InitPage) DrawDateTime();
    else
    {
      if (GetProperty(PROP_PHONE_DRAW_TOP)) UpdateClockWidgets();
      else DrawDateTime();
    }
  }
}

/*! Led Change Handler
 *
 * \param tHostMsg* pMsg The message options contain the type of operation that
 * should be performed on the LED outout.
 */
static void SetBacklight(unsigned char Value)
{
  static tTimerId LedTimerId = UNASSIGNED;

  if (Value == LED_OFF_OPTION && !LedOn) return;  
  if (Value == LED_TOGGLE_OPTION) Value = LedOn ? LED_OFF_OPTION : LED_ON_OPTION;
  
  StopTimer(LedTimerId);

  if (Value == LED_ON_OPTION)
  {
    ENABLE_LCD_LED();
    LedOn = pdTRUE;
    LedTimerId = StartTimer(5, NO_REPEAT, DISPLAY_QINDEX, SetBacklightMsg, LED_OFF_OPTION);
  }
  else
  {
    DISABLE_LCD_LED();
    LedOn = pdFALSE;
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

unsigned char LinkAlarmEnabled(void)
{
  return LinkAlarmEnable;
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
  SetupMessageAndAllocateBuffer(&Msg, VBatRespMsg, MSG_OPT_NONE);
  Msg.Length = 6;

  Msg.pBuffer[0] = ClipOn();
  Msg.pBuffer[1] = Charging();
  Msg.pBuffer[2] = BatteryPercentage();

  unsigned int bv = Read(BATTERY);
  Msg.pBuffer[4] = bv & 0xFF;
  Msg.pBuffer[5] = (bv >> 8) & 0xFF;

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
    UpdateClock();
  }

  else if (Val == MSG_OPT_NORMAL_DISPLAY && GetProperty(PROP_INVERT_DISPLAY) ||
          (Val == MSG_OPT_INVERT_DISPLAY && !GetProperty(PROP_INVERT_DISPLAY)))
  {
    ToggleProperty(PROP_INVERT_DISPLAY);

    if (PageType == PAGE_TYPE_IDLE) IdleUpdateHandler();
    else if (PageType == PAGE_TYPE_MENU) MenuModeHandler(0);
    else if (CurrentPage[PageType] == StatusPage) DrawWatchStatusScreen();
  }
}

static void HandleProperty(unsigned char Options)
{
  if (Options & PROPERTY_READ) CreateAndSendMessage(PropRespMsg, GetProperty(PROP_VALID));
  else
  {
    PrintF(">SetProp:0x%02x", Options);
    SetProperty(PROP_VALID, Options & PROP_VALID);
    CreateAndSendMessage(PropRespMsg, MSG_OPT_NONE);
    CreateAndSendMessage(UpdateDisplayMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL);
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

    PrintF(">Nval %04x:%d", Identifier.word, pNvPayload->DataStartByte);
    
    unsigned char Property = PropertyBit(pNvPayload->NvalIdentifier);

    if (pMsg->Options == NVAL_READ_OPERATION)
    {
      if (Property) Msg.pBuffer[2] = GetProperty(Property);
      Msg.Length += pNvPayload->Size;
    }
    else if (Property) // write operation
    {
      SetProperty(Property, pNvPayload->DataStartByte ? Property : 0);
      UpdateClock();
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
  PrintS("- Entering Bootloader Mode");
  DrawBootloaderScreen();
  
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
      if (CurrentPage[PageType] == StatusPage) DrawWatchStatusScreen();
      else UpdateClock(); // must be the last to avoid screen mess-up
      
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
}
