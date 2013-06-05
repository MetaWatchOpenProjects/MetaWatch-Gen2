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
#include "hal_rtc.h"
#include "hal_battery.h"
#include "hal_miscellaneous.h"
#include "hal_lcd.h"
#include "hal_clock_control.h"
#include "hal_boot.h"
#include "DebugUart.h"
#include "Utilities.h"
#include "LcdDriver.h"
#include "Wrapper.h"
#include "OneSecondTimers.h"
#include "Adc.h"
#include "Accelerometer.h"
#include "Buttons.h"
#include "Vibration.h"
#include "SerialRam.h"
#include "Icons.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "IdleTask.h"
#include "TermMode.h"
#include "BufferPool.h"
#include "MuxMode.h"
#include "Property.h"
#include "ClockWidget.h"
#include "Fonts.h"
#include "LcdBuffer.h"

#if COUNTDOWN_TIMER
#include "Countdown.h"
#endif

#define PAGE_TYPE_NUM                 (3)
#define MUSIC_STATE_START_ROW         (43)

extern const char BUILD[];
extern const char VERSION[];
extern unsigned int niReset;
extern char niBuild[];

xTaskHandle DisplayHandle;

static unsigned char RtcUpdateEnabled = pdFALSE;

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static eIdleModePage CurrentPage[PAGE_TYPE_NUM];

static const tSetVibrateModePayload RingTone = {1, 0x00, 0x01, 0x00, 0x01, 2};
static const tSetVibrateModePayload TestTone = {1, 0x80, 0x01, 0x80, 0x00, 3};
static const tSetVibrateModePayload LnkAlmTone = {1, 0xC8, 0x00, 0xF4, 0x01, 1};
static const unsigned int ModeTimeOut[] = {TOUT_IDLE_MODE, TOUT_APP_MODE, TOUT_NOTIF_MODE, TOUT_MUSIC_MODE};

static unsigned char Splashing = pdTRUE;
static unsigned char LedOn = 0;
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
  xTaskCreate(DisplayTask, (signed char *)"DISPLAY", DISPLAY_TASK_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &DisplayHandle);
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
  if (niReset != NORMAL_RESET_CODE)
  {
    InitProperty();
#if COUNTDOWN_TIMER
    InitCountdown();
#endif
  }
  
  InitBufferPool(); // message queue

  InitBattery();
  CheckClip();

  PrintF("*** %s:%s ***", niReset == FLASH_RESET_CODE ? "FLASH" :
    (niReset == MASTER_RESET_CODE ? "MASTER" : "NORMAL"), niBuild);
  
  ShowWatchdogInfo();
  WhoAmI();

  /* timer for battery checking at a regular frequency. */
  StartTimer(BatteryTimer);

  InitVibration();
  InitRealTimeClock(); // enable rtc interrupt

  LcdPeripheralInit();
  DrawSplashScreen();
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
    pMsg->pBuffer[pMsg->Length] = NUL;
    ShowCall((char *)pMsg->pBuffer, pMsg->Options);
    break;
    
  case CallerNameMsg:
    if (pMsg->Length)
    {
      pMsg->pBuffer[pMsg->Length] = NUL;
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

    SetupMessageWithBuffer(&Msg, DevTypeRespMsg, BOARD_TYPE); //default G2
    if (Msg.pBuffer != NULL)
    {
      Msg.pBuffer[0] = BOARD_TYPE; // backward compatible
      Msg.Length = 1;

      if (GetMsp430HardwareRevision() < 'F')
      {
        Msg.Options = DIGITAL_WATCH_TYPE_G1;
        Msg.pBuffer[0] = DIGITAL_WATCH_TYPE_G1; // backward compatible
      }

      Msg.Options |= pMsg->Options & 0x80; // support ACK
      RouteMsg(&Msg);
    }

    PrintF("- DevTypeResp:%d", Msg.Options);

    // set ACK bit
    SendMessage(&Msg, ConnTypeMsg, pMsg->Options);
    break;

  case VerInfoMsg:
    SetupMessageWithBuffer(&Msg, VerInfoRespMsg, MSG_OPT_NONE);
    if (Msg.pBuffer != NULL)
    {
      /* exclude middle '.' */
      strncpy((char *)Msg.pBuffer, BUILD, 3);
      strncpy((char *)Msg.pBuffer + 3, BUILD + 4, 3);
      Msg.Length += strlen(BUILD) - 1;
        
      *(Msg.pBuffer + Msg.Length++) = (unsigned char)atoi(VERSION);
      
      while (VERSION[i++] != '.');
      *(Msg.pBuffer + Msg.Length++) = atoi(VERSION + i);
      *(Msg.pBuffer + Msg.Length++) = NULL;
      
      RouteMsg(&Msg);
    }
    PrintS("-Ver:"); for (i = 6; i < Msg.Length; ++i) PrintH(Msg.pBuffer[i]);
    break;
    
  case SetVibrateMode:
    SetVibrateModeHandler(pMsg);
    break;

  case SetRtcMsg:
    SetRtc((Rtc_t *)pMsg->pBuffer);

#if COUNTDOWN_TIMER
    if (CurrentPage[PageType] == CountdownPage && CountdownMode() == COUNTING)
      SendMessage(&Msg, CountDownMsg, MSG_OPT_CNTDWN_TIME);
#endif
    UpdateClock();
    break;
    
#if COUNTDOWN_TIMER
  case CountDownMsg:
    if (pMsg->Options == MSG_OPT_NONE)
    {
      PageType = PAGE_TYPE_INFO;
      CurrentPage[PageType] = CountdownPage;
    }
    DrawCountdownScreen(pMsg->Options);
    break;

  case SetCountdownDoneMsg:
    // for testing
//    Rtc_t Done;
//    Done.Month = 5;
//    Done.Day = 1;
//    Done.Hour = 0;
//    Done.Minute = 0;
//    SetDoneTime(&Done);

    // Valid only DoneTime is different
    if (SetDoneTime((Rtc_t *)pMsg->pBuffer)) SendMessage(&Msg, CountDownMsg, MSG_OPT_NONE);
    break;
#endif

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
    // testing
//    pMsg->Type = AccelMsg;
//    pMsg->Options = 1; //enable
//    HandleAccelerometer(pMsg);
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
      SetupMessageWithBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);
      if (Msg.pBuffer != NULL)
      {
        *(tSetVibrateModePayload *)Msg.pBuffer = LnkAlmTone;
        RouteMsg(&Msg);
      }
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

  case AccelMsg:
    HandleAccelerometer(pMsg);
    break;

  case ReadLightSensorMsg:
    ReadLightSensorHandler();
    break;

  case RateTestMsg:
    SetupMessageWithBuffer(&Msg, DiagnosticLoopback, MSG_OPT_NONE);
    if (Msg.pBuffer != NULL)
    {
      /* don't care what data is */
      Msg.Length = 10;
      RouteMsg(&Msg);
    }
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
  PrintF("- ChgModInd:0x%02x", Option);
  PrintF(" PgTp %d Pg: %d", PageType, CurrentPage[PageType]);
  
  unsigned char Mode = Option & MODE_MASK;

  tMessage Msg;
  SetupMessageWithBuffer(&Msg, ModeChangeIndMsg, (CurrentIdleScreen() << 4) | Mode);
  if (Msg.pBuffer != NULL)
  {
    Msg.pBuffer[0] = eScUpdateComplete;
    Msg.Length = 1;
    RouteMsg(&Msg);
  }

  if (Option & MSG_OPT_CHGMOD_IND) return; // ask for current idle page only
    
//  StopTimer(ModeTimer);

  if (Mode == MUSIC_MODE) SendMessage(&Msg, UpdConnParamMsg, ShortInterval);
  else if (CurrentMode == MUSIC_MODE) SendMessage(&Msg, UpdConnParamMsg, LongInterval);

  CurrentMode = Mode;
  
  if (Mode == IDLE_MODE)
  {
#if COUNTDOWN_TIMER
    if (CurrentPage[PageType] == CountdownPage) SendMessage(&Msg, CountDownMsg, MSG_OPT_NONE);
    else
#endif
    {
//    PageType = PAGE_TYPE_IDLE;
      IdleUpdateHandler();
    }
  }
  else
  {
    SetTimer(ModeTimer, ModeTimeOut[Mode], TOUT_ONCE);
    
    if (Option & MSG_OPT_UPD_INTERNAL) SendMessage(&Msg, UpdateDisplayMsg, Option);
  }
}

static void ModeTimeoutHandler()
{
  /* send a message to the host indicating that a timeout occurred */
  PrintS("- ModChgTout");
  
  tMessage Msg;
  SetupMessageWithBuffer(&Msg, ModeChangeIndMsg, CurrentMode);
  if (Msg.pBuffer != NULL)
  {
    Msg.pBuffer[0] = eScModeTimeout;
    Msg.Length = 1;
    RouteMsg(&Msg);
  }
  ChangeModeHandler(IDLE_MODE);
}

void ResetModeTimer(void)
{
  StartTimer(ModeTimer);
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
        if (OnceConnected())
        {
//#if COUNTDOWN_TIMER
//          if (Connected(CONN_TYPE_MAIN)) CreateAndSendMessage(CountDownMsg, MSG_OPT_NONE);
//#endif
          UpdateClock();
        }
        else DrawConnectionScreen();
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

  case MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN:
    if (RESET_PIN) {SET_RESET_PIN_RST();}
    else {SET_RESET_PIN_NMI();}
    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_TOGGLE_SERIAL_SBW_GND:
    ToggleSerialGndSbw();
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_ENABLE_CHARGING:
    ToggleCharging();
//    SendMessage(&Msg, AccelMsg, 0); //test accel
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE:
    EnterBootloader();
    break;

  case MENU_BUTTON_OPTION_TEST:
  
    SetupMessageWithBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);
    if (Msg.pBuffer != NULL)
    {
      *(tSetVibrateModePayload *)Msg.pBuffer = TestTone;
      RouteMsg(&Msg);
    }
    // test accelemeter MSG_OPT_ACCEL_ENABLE
//    SendMessage(&Msg, AccelMsg, 1);
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
  IncRtc(pMsg->Options);  
  DrawDateTime();
  ResetWatchdog(); //battery timer can't be fire
}

static void HandleMusicPlayStateChange(unsigned char State)
{
  // music icon fits into one message
  PrintF("- HdlMusicPlyChg:%d", State);
  tMessage Msg;
  SetupMessageWithBuffer(&Msg, WriteBufferMsg,
    MUSIC_MODE | MSG_OPT_WRTBUF_MULTILINE | ICON_MUSIC_WIDTH << 3);
  
  if (Msg.pBuffer != NULL)
  {
    Msg.pBuffer[0] = MUSIC_STATE_START_ROW;
    Msg.Length = ICON_MUSIC_WIDTH * ICON_MUSIC_HEIGHT + 1;
    PrintF("- start:%d Len: %d", Msg.pBuffer[0], Msg.Length);
    
    unsigned char i = 1;
    for (; i < Msg.Length; ++i) Msg.pBuffer[i] = pIconMusicState[1 - State][i - 1];
    RouteMsg(&Msg);
  }

  SendMessage(&Msg, UpdateDisplayMsg, MUSIC_MODE);
}

static void ShowCall(tString *pString, unsigned char Type)
{
  static tString CallerId[15] = "";
  tMessage Msg;

  if (Type == SHOW_NOTIF_CALLER_ID) strcpy(CallerId, pString);
  
  if (Type == SHOW_NOTIF_CALLER_NAME && *CallerId)
  {
    PrintF("- Caller:%s", pString);
  
    SetupMessageWithBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);
    if (Msg.pBuffer != NULL)
    {
      *(tSetVibrateModePayload *)Msg.pBuffer = RingTone;
      RouteMsg(&Msg);
    }
    PageType = PAGE_TYPE_INFO;
    CurrentPage[PageType] = CallPage;
    DrawCallScreen(CallerId, pString);

    // set a 5s timer for switching back to idle screen
    StartTimer(ShowCallTimer);
  }
  else if (Type == SHOW_NOTIF_END || Type == SHOW_NOTIF_REJECT_CALL)
  {
    PrintF("- Call Notif End");
    
    *CallerId = NULL;
    StopTimer(ShowCallTimer);

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
  static unsigned char lastMin = 61;
  unsigned char ExitLpm = pdFALSE;
  
  /* send a message every second or once a minute */
  if (RtcUpdateEnabled && CurrentMode == IDLE_MODE)
  {
    tMessage Msg;
    unsigned char Minute = RTCMIN;
    
    if (PageType == PAGE_TYPE_IDLE && (GetProperty(PROP_TIME_SECOND) || Minute != lastMin))
    {
      SetupMessage(&Msg, UpdateClockMsg, MSG_OPT_NONE);
      SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
      lastMin = Minute;
      ExitLpm = pdTRUE;
    }
#if COUNTDOWN_TIMER
    else if (CurrentPage[PageType] == CountdownPage)
    {
      if (CountdownMode() == COUNTING && Minute != lastMin)
      {
        SetupMessage(&Msg, CountDownMsg, MSG_OPT_CNTDWN_TIME);
        SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
        lastMin = Minute;
        ExitLpm = pdTRUE;
      }
    }
#endif
  }

  return ExitLpm;
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
  if (Value == LED_ON_OPTION && LedOn) StartTimer(BacklightTimer);
  else if (Value == LED_OFF_OPTION && !LedOn) return;
  else
  {
    LedOn = !LedOn;

    if (Value == LED_ON_OPTION)
    {
      ENABLE_LCD_LED();
      StartTimer(BacklightTimer);
    }
    else DISABLE_LCD_LED(); // LED_OFF only when timeout
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
  SetupMessageWithBuffer(&Msg, VBatRespMsg, MSG_OPT_NONE);
  if (Msg.pBuffer != NULL)
  {
    Msg.Length = 6;
    Msg.pBuffer[0] = ClipOn();
    Msg.pBuffer[1] = Charging();
    Msg.pBuffer[2] = BatteryPercentage();

    unsigned int bv = Read(BATTERY);
    Msg.pBuffer[4] = bv & 0xFF;
    Msg.pBuffer[5] = (bv >> 8) & 0xFF;
    RouteMsg(&Msg);
  }
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

    if (CurrentMode == IDLE_MODE && PageType == PAGE_TYPE_IDLE)
      CreateAndSendMessage(IdleUpdateMsg, MSG_OPT_NONE);
  }
}

static void NvalOperationHandler(tMessage *pMsg)
{
  if (pMsg->pBuffer == NULL) // Property
  {
    HandleProperty(pMsg->Options);
  }
  else
  {
    tNvalOperationPayload* pNvPayload = (tNvalOperationPayload *)pMsg->pBuffer;

    /* create the outgoing message */
    tMessage Msg;
    SetupMessageWithBuffer(&Msg, PropRespMsg, MSG_OPT_NONE);
    if (Msg.pBuffer != NULL)
    {
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
}

/******************************************************************************/
/*
 * this writes a signature to the EnterBoot RAM location
 * and generates a reset.  The signature tells the bootloader
 * to stay in bootload mode to accept bootload messaging.
 */
void EnterBootloader(void)
{
#if BOOTLOADER
  PrintS("- Entering Bootloader Mode");
  DrawBootloaderScreen();
#endif
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
  static unsigned char LastPercent = BATTERY_PERCENT_UNKNOWN;

#if CHECK_CSTACK
  if (LastPercent == BATTERY_PERCENT_UNKNOWN) CheckCStack();
#endif

  unsigned char ClipChanged = CheckClip();
  CheckBattery();
  
  if (ClipChanged || Charging() || CurrentPage[PageType] == StatusPage ||
      LastPercent == BATTERY_PERCENT_UNKNOWN)
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
