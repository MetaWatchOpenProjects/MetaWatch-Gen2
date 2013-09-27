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
#include "hal_clock_control.h"
#include "hal_boot.h"
#include "DebugUart.h"

#include "LcdDriver.h"
#include "Wrapper.h"
#include "OneSecondTimers.h"
#include "Adc.h"
#include "Accelerometer.h"
#include "Buttons.h"
#include "Vibration.h"
#include "DrawMsgHandler.h"
#include "ClockWidget.h"
#include "SerialRam.h"
#include "Icons.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "IdleTask.h"
#include "TermMode.h"
#include "BufferPool.h"
#include "MuxMode.h"
#include "Property.h"
#include "Fonts.h"
#include "LcdBuffer.h"
#include "CallNotifier.h"

#if COUNTDOWN_TIMER
#include "Countdown.h"
#endif

#define PAGE_TYPE_NUM                 3
#define MUSIC_STATE_START_ROW         43

#define DEV_TYPE_EN_ACK               0x80
#define BUILD_LENGTH                  7

extern char const BUILD[];
extern char const VERSION[];
extern unsigned int niReset;
extern char niBuild[];
extern unsigned char Clocking;

xTaskHandle DisplayHandle;

static unsigned char RtcUpdateEnabled = FALSE;

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static eIdleModePage CurrentPage[PAGE_TYPE_NUM];

static const unsigned int ModeTimeOut[] = {TOUT_IDLE_MODE, TOUT_APP_MODE, TOUT_NOTIF_MODE, TOUT_MUSIC_MODE};

static unsigned char Splashing = TRUE;
static unsigned char LedOn = 0;
static unsigned char LinkAlarmEnable = TRUE; // temporary

/******************************************************************************/
static void DisplayTask(void *pvParameters);
static void DisplayQueueMessageHandler(tMessage* pMsg);

/* Message handlers */
static void IdleUpdateHandler();
static void DetermineIdlePage(void);
static void ModeTimeoutHandler();
static void ModifyTimeHandler(tMessage* pMsg);
static void MenuModeHandler(unsigned char Option);
static void MenuButtonHandler(unsigned char MsgOptions);
static void ServiceMenuHandler(void);
static void ToggleSerialGndSbw(void);
static void BluetoothStateChangeHandler(tMessage *pMsg);
static void UpdateClock(void);

static void HandleMusicPlayStateChange(unsigned char State);
static void SetBacklight(unsigned char Value);
static void ReadBatteryVoltageHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void SoftwareResetHandler(tMessage* pMsg);
static void MonitorBattery(void);
static void HandleSecInvert(unsigned char Val);

#if __IAR_SYSTEMS_ICC__
//static void WriteToTemplateHandler(tMessage *pMsg);
//static void EraseTemplateHandler(tMessage *pMsg);
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
      CheckStackAndQueueUsage(DISPLAY_QINDEX);
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
  if (niReset == MASTER_RESET_CODE)
  {
    InitProperty();
#if COUNTDOWN_TIMER
    InitCountdown();
#endif
  }
  
  InitBufferPool(); // message queue

  InitBattery();
  CheckClip();
  __enable_interrupt();
  
  PrintF("*** %s:%s ***", niReset == FLASH_RESET_CODE ? "FLASH" :
    (niReset == MASTER_RESET_CODE ? "MASTER" : "NORMAL"), niBuild);
  
  WhoAmI();
  ShowWatchdogInfo();

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
static void DisplayQueueMessageHandler(tMessage *pMsg)
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
  
    if ((!(pMsg->Options & MSG_OPT_UPD_INTERNAL) &&
        (pMsg->Options & MODE_MASK) == NOTIF_MODE) &&
        GetProperty(PROP_AUTO_BACKLIGHT))
      SendMessage(&Msg, AutoBacklightMsg, MSG_OPT_NONE);

    UpdateDisplayHandler(pMsg);
    break;

  case MonitorBatteryMsg:
    MonitorBattery();
    break;

  case DrawMsg:
  
    DrawMsgHandler(pMsg);
    break;
    
  case UpdateClockMsg:
    UpdateClock();
    break;
    
  case DrawClockWidgetMsg:
    DrawClockWidget(pMsg->Options);
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

  case ShowCallMsg:

    HandleCallNotification(pMsg->Options, pMsg->pBuffer, pMsg->Length);
    break;

  case MusicControlMsg:
    HandleMusicPlayStateChange(pMsg->Options);
    break;
    
  case ChangeModeMsg:
    ChangeMode(pMsg->Options);
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

      Msg.Options |= DEV_TYPE_EN_ACK; // support ACK
      RouteMsg(&Msg);
    }

    PrintF("- DevTypeResp:%d", Msg.Options);

    // set ACK and HFP/MAP bits
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
        
      *(Msg.pBuffer + Msg.Length++) = VERSION[0] - ZERO;
      *(Msg.pBuffer + Msg.Length++) = VERSION[2] - ZERO;
      *(Msg.pBuffer + Msg.Length++) = VERSION[4] - ZERO;
      *(Msg.pBuffer + Msg.Length++) = GetMsp430HardwareRevision();
      RouteMsg(&Msg);
    }
    PrintW("-Ver:"); PrintQ(Msg.pBuffer, Msg.Length);
    break;
    
  case VibrateMsg:
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

  case FieldTestMsg:
    HandleFieldTestMode(pMsg->Options);
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
    if (LightSenseCycle() < DARK_LEVEL) SetBacklight(LED_ON);
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
    if (LinkAlarmEnable) SendMessage(&Msg, VibrateMsg, VIBRA_PATTERN_LNKALM);
    break;

  case ModeTimeoutMsg:
    ModeTimeoutHandler();
    break;

  case WatchStatusMsg:
    PageType = PAGE_TYPE_INFO;
    CurrentPage[PageType] = StatusPage;
    DrawWatchStatusScreen(TRUE);
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
    UpdateClock();
    break;

#if __IAR_SYSTEMS_ICC__
  case EraseTemplateMsg:
//    EraseTemplateHandler(pMsg);
    break;
    
  case WriteToTemplateMsg:
//    WriteToTemplateHandler(pMsg);
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
    PrintF("# Disp Msg:x%02X", pMsg->Type);
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

void ChangeMode(unsigned char Option)
{
//  PrintF("- ChgModInd:0x%02x", Option);
//  PrintF(" PgTp %d Pg: %d", PageType, CurrentPage[PageType]);

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
    
  if (Mode == MUSIC_MODE || Mode == APP_MODE) SendMessage(&Msg, UpdConnParamMsg, SHORT);

  CurrentMode = Mode;

  if (Mode == IDLE_MODE)
  {
#if COUNTDOWN_TIMER
    if (CurrentPage[PageType] == CountdownPage) SendMessage(&Msg, CountDownMsg, MSG_OPT_NONE);
    else
#endif
    {
      StopTimer(ModeTimer);
      if (Ringing()) SendMessage(&Msg, ShowCallMsg, CALL_REJECTED);

      IdleUpdateHandler();
    }
  }
  else
  {
    ResetTimer(ModeTimer, ModeTimeOut[Mode], TOUT_ONCE);
    
    if (Option & MSG_OPT_UPD_INTERNAL) SendMessage(&Msg, UpdateDisplayMsg, Option);
  }
}

static void ModeTimeoutHandler()
{
  PrintS("- ModChgTout");
  ChangeMode(IDLE_MODE);
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
      Splashing = FALSE;
      RtcUpdateEnabled = TRUE;
      DetermineIdlePage();
      IdleUpdateHandler();
      
      pMsg->Options = MUSIC_MODE | (TMPL_MUSIC_MODE << 4);
      LoadTemplateHandler(pMsg);

      pMsg->Options = NOTIF_MODE | (TMPL_NOTIF_EMPTY << 4);
      LoadTemplateHandler(pMsg);

      InitButton();
    }
  }
  else
  {
    if (BluetoothState() == Disconnect) ResetButtonAction();
    
    //decide which idle page to be
    DetermineIdlePage();
    
    if (CurrentMode == IDLE_MODE)
    {
      if (PageType == PAGE_TYPE_IDLE)
      {
        if (OnceConnected()) UpdateClock();
        else DrawConnectionScreen();
      }
      else if (PageType == PAGE_TYPE_MENU) MenuModeHandler(0);
      else if (CurrentPage[PAGE_TYPE_INFO] == StatusPage) DrawWatchStatusScreen(FALSE);
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
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE:
    EnterBootloader();
    break;

  case MENU_BUTTON_OPTION_TEST:
  
    SendMessage(&Msg, VibrateMsg, VIBRA_PATTERN_TEST);
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

#if __IAR_SYSTEMS_ICC__
//Watch Faces Flash memory declaration
extern __data20 const unsigned char pWatchFace[][1]; //TEMPLATE_FLASH_SIZE];
#endif

void EnableRtcUpdate(unsigned char Enable)
{
  RtcUpdateEnabled = Enable;
}

unsigned char LcdRtcUpdateHandlerIsr(void)
{
  static unsigned char lastMin = 61;
  unsigned char ExitLpm = FALSE;
  
  /* send a message every second or once a minute */
  if (RtcUpdateEnabled)
  {
    tMessage Msg;
    unsigned char Minute = RTCMIN;
    
    if (GetProperty(PROP_TIME_SECOND) || Minute != lastMin)
    {
      SetupMessage(&Msg, UpdateClockMsg, MSG_OPT_NONE);
      SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
      lastMin = Minute;
      ExitLpm = TRUE;
    }
#if COUNTDOWN_TIMER
    else if (CurrentPage[PageType] == CountdownPage)
    {
      if (CountdownMode() == COUNTING && Minute != lastMin)
      {
        SetupMessage(&Msg, CountDownMsg, MSG_OPT_CNTDWN_TIME);
        SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);
        lastMin = Minute;
        ExitLpm = TRUE;
      }
    }
#endif
  }

  return ExitLpm;
}

static void UpdateClock(void)
{
  if (CurrentMode == IDLE_MODE)
  {
    if (PageType == PAGE_TYPE_IDLE)
    {
      if (CurrentPage[PageType] == InitPage) DrawDateTime();
      else
      {
        if (GetProperty(PROP_PHONE_DRAW_TOP))
        {
          UpdateClockWidgets();
          DrawStatusBar();
        }
        else DrawDateTime();
      }
    }
  }
  else DrawStatusBar();
}

/*! Led Change Handler
 *
 * \param tHostMsg* pMsg The message options contain the type of operation that
 * should be performed on the LED outout.
 */
static void SetBacklight(unsigned char Value)
{
  if (Value == LED_ON && LedOn) StartTimer(BacklightTimer);
  else if (Value == LED_OFF && !LedOn) return;
  else
  {
    LedOn = !LedOn;

    if (LedOn == LED_ON)
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

    unsigned int bv = Msg.pBuffer[2] * LEVEL_RANGE_ONEPERCENT + BATTERY_CRITICAL_LEVEL;
    Msg.pBuffer[4] = bv & 0xFF;
    Msg.pBuffer[5] = (bv >> 8) & 0xFF;

    PrintF("RdBatt:%u%c %u", Msg.pBuffer[2], bv);
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
    else if (CurrentPage[PageType] == StatusPage) DrawWatchStatusScreen(FALSE);
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
  static unsigned char LastPercent = BATTERY_UNKNOWN;

#if CHECK_CSTACK
  if (LastPercent == BATTERY_UNKNOWN) CheckCStack();
#endif

  unsigned char ClipChanged = CheckClip();
  CheckBattery();
  
  if (ClipChanged || Charging() || CurrentPage[PageType] == StatusPage ||
      LastPercent == BATTERY_UNKNOWN)
  {
    unsigned char CurrPercent = BatteryPercentage();

    if (CurrPercent != LastPercent || ClipChanged)
    {
      if (CurrentMode == IDLE_MODE && CurrentPage[PageType] == StatusPage)
        DrawWatchStatusScreen(FALSE);
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
}
