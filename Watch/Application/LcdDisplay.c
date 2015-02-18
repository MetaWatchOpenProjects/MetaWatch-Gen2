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
#include "DrawHandler.h"
#include "Widget.h"
#include "ClockWidget.h"
#include "SerialRam.h"
#include "Icons.h"
#include "LcdDisplay.h"
#include "BitmapData.h"
#include "IdleTask.h"
#include "TermMode.h"
#include "MuxMode.h"
#include "Property.h"
#include "Fonts.h"
#include "Countdown.h"
#include "LcdBuffer.h"
#include "CallNotifier.h"
#include "Log.h"
#include "Icons.h"

#define PAGE_TYPE_NUM                 3
#define MUSIC_STATE_START_ROW         43

#define DEV_TYPE_EN_ACK               0x80

#if BOOTLOADER
#include "bl_boot.h"
extern char const BootVersion[VERSION_LEN];
#else
static char const BootVersion[] = "0.1.7";
#endif

extern char const BUILD[];
extern char const VERSION[];

static unsigned char RtcUpdateEnabled = FALSE;

unsigned char CurrentMode = IDLE_MODE;
unsigned char PageType = PAGE_TYPE_IDLE;

static unsigned char CurrentPage[PAGE_TYPE_NUM];

static unsigned char Splashing = TRUE;
static unsigned char LedOn = 0;

/******************************************************************************/
static void DisplayTask(void *pvParameters);
static void DisplayQueueMessageHandler(tMessage* pMsg);
static void InitDisplay(void);

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

static void MusicIcon(unsigned char options);
static void HandleMusicStateChange(unsigned char State);
static void SetBacklight(unsigned char Value);
static void ReadBatteryVoltageHandler(void);
static void NvalOperationHandler(tMessage* pMsg);
static void MonitorBattery(void);
static void HandleSecInvert(unsigned char Val);

/******************************************************************************/

#define DISPLAY_STACK_SIZE      (configMINIMAL_STACK_SIZE + 200) //total 48-88
#define DISPLAY_PRIORITY        (tskIDLE_PRIORITY + 1)
extern xQueueHandle QueueHandles[];

void CreateDisplayTask(void)
{
  xTaskHandle Handle;

  QueueHandles[DISPLAY_QINDEX] = xQueueCreate(DISPLAY_QUEUE_LENGTH, MESSAGE_SIZE);
  if (!QueueHandles[DISPLAY_QINDEX]) SoftwareReset(RESET_TASK_FAIL, DISPLAY_QINDEX);

  // task function, task name, stack len, task params, priority, task handle
  xTaskCreate(DisplayTask, "DISPLAY", DISPLAY_STACK_SIZE, NULL, DISPLAY_PRIORITY, &Handle);
}

/*! LCD Task Main Loop */
static void DisplayTask(void *pvParameters)
{
  tMessage Msg;

  InitDisplay();

  for(;;)
  {
    if (xQueueReceive(QueueHandles[DISPLAY_QINDEX], &Msg, portMAX_DELAY))
    {
      ShowMessageInfo(&Msg);
      DisplayQueueMessageHandler(&Msg);

      if (Msg.pBuffer) FreeMessageBuffer(Msg.pBuffer);
      CheckStackAndQueueUsage(DISPLAY_QINDEX);
    }
  }
}

static void InitDisplay(void)
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

#if BOOTLOADER
  /*
   * enable RAM alternate interrupt vectors
   * these are defined in AltVect.s43 and copied to RAM by cstartup
   */
  SYSCTL |= SYSRIVECT;
  ClearBootloaderSignature();
#endif

  SetupClockAndPowerManagementModule();

  CheckClip(); // enable debuguart
  __enable_interrupt();
  PrintF("\r\n*** %s:%c%c%c ***", (niResetType == NORMAL_RESET ? "NORMAL" : "MASTER"),
    BUILD[0], BUILD[1], BUILD[2]);

  if (niResetType != NORMAL_RESET) InitStateLog();

  SaveStateLog();

  WhoAmI();
  ShowStateInfo();

  InitAdc();
  /* timer for battery checking at a regular frequency. */
  StartTimer(BatteryTimer);

  InitVibration();
  InitRealTimeClock(); // enable rtc interrupt

  LcdPeripheralInit();
  DrawSplashScreen();

  InitSerialRam();

  /* turn the radio on; initialize the serial port profile or BLE/GATT */
  SendMessage(TurnRadioOnMsg, MSG_OPT_NONE);

  DISABLE_LCD_LED();
}

/*! Handle the messages routed to the display queue */
static void DisplayQueueMessageHandler(tMessage *pMsg)
{
  tMessage Msg;

  switch (pMsg->Type)
  {
  case ShowCallMsg:
    HandleCallNotification(pMsg->Options, pMsg->pBuffer, pMsg->Length);
    break;

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
      SendMessage(AutoBacklightMsg, MSG_OPT_NONE);

    UpdateDisplayHandler(pMsg);
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

  case StopTimerMsg:
    StopTimer((eTimerId)pMsg->Options);
    break;

  case MonitorBatteryMsg:
    MonitorBattery();
    break;

  case MusicIconMsg:
      MusicIcon(pMsg->Options);
      break;

  case MusicStateMsg:
    HandleMusicStateChange(pMsg->Options);
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

    Msg.Length = 1;
    Msg.Type = DevTypeRespMsg;
    Msg.Options = BOARD_TYPE; //default G2

    if (CreateMessage(&Msg))
    {
      Msg.pBuffer[0] = BOARD_TYPE; // backward compatible

      if (GetMsp430HardwareRevision() < 'F')
      {
        Msg.Options = DIGITAL_WATCH_TYPE_G1;
        Msg.pBuffer[0] = DIGITAL_WATCH_TYPE_G1; // backward compatible
      }

      Msg.Options |= DEV_TYPE_EN_ACK; // support ACK
      RouteMsg(&Msg);
    }

    PrintF("- DevTypeResp:%u", Msg.Options);

    // set ACK and HFP/MAP bits
//    SendMessage(ConnTypeMsg, pMsg->Options);
    break;

  case VerInfoMsg:

    Msg.Length = BUILD_LENGTH + 4 + 3;
    Msg.Type = VerInfoRespMsg;
    Msg.Options = MSG_OPT_NONE;

    if (CreateMessage(&Msg))
    {
      GetBuildNumber(Msg.pBuffer);

      *(Msg.pBuffer + BUILD_LENGTH) = VERSION[0] - ZERO;
      *(Msg.pBuffer + BUILD_LENGTH + 1) = VERSION[2] - ZERO;
      *(Msg.pBuffer + BUILD_LENGTH + 2) = VERSION[4] - ZERO;
      *(Msg.pBuffer + BUILD_LENGTH + 3) = GetMsp430HardwareRevision();
      *(Msg.pBuffer + BUILD_LENGTH + 4) = BootVersion[0] - ZERO;
      *(Msg.pBuffer + BUILD_LENGTH + 5) = BootVersion[2] - ZERO;
      *(Msg.pBuffer + BUILD_LENGTH + 6) = BootVersion[4] - ZERO;

      RouteMsg(&Msg);
    }
    PrintE("-Ver(%u):", Msg.Length); PrintQ(Msg.pBuffer, Msg.Length);
    break;

  case LogMsg:
    SendStateLog();
    break;
    
  case VibrateMsg:
    SetVibrateModeHandler(pMsg);
    break;

  case SetRtcMsg:
    if (SetRtc((Rtc_t *)pMsg->pBuffer)) UpdateClock();
    break;
    
  case CountdownMsg:
    if (pMsg->Options == CDT_ENTER)
    {
      PageType = PAGE_TYPE_INFO;
      CurrentPage[PageType] = CountdownPage;
    }
    CountdownHandler(pMsg->Options);
    break;

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
    SoftwareReset(RESET_BUTTON_PRESS, pMsg->Options);
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
    SendMessage(VibrateMsg, VIBRA_PATTERN_LNKALM);
    break;

  case ModeTimeoutMsg:
    ModeTimeoutHandler();
    break;

  case WatchStatusMsg:
    PageType = PAGE_TYPE_INFO;
    CurrentPage[PageType] = StatusPage;
    DrawWatchStatusScreen(TRUE);
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

  case WatchDrawnScreenTimeout:
    IdleUpdateHandler();
    break;

  case RateTestMsg:
    /* don't care what data is */
    Msg.Length = 10;
    Msg.Type = DiagnosticLoopback;
    Msg.Options = MSG_OPT_NONE;
    if (CreateMessage(&Msg)) RouteMsg(&Msg);
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
    SendMessage(UpdateDisplayMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL);
  else DrawConnectionScreen();
}

#define MODE_CHANGE_TIMEOUT     2

void ChangeMode(unsigned char Option)
{
//  PrintF("- ChgModInd:0x%02x", Option);
//  PrintF(" PgTp %d Pg: %d", PageType, CurrentPage[PageType]);

  unsigned char Mode = Option & MODE_MASK;

  tMessage Msg = {1, ModeChangeIndMsg, (CurrentIdleScreen() << 4) | Mode, NULL};
  if (CreateMessage(&Msg))
  {
    Msg.pBuffer[0] = (Option >> 4) ? Option >> 4 : eScUpdateComplete;
    RouteMsg(&Msg);
  }

  if (Option & MSG_OPT_CHGMOD_IND) return; // ask for current idle page only
    
  if (Mode == MUSIC_MODE || Mode == APP_MODE) SendMessage(UpdConnParamMsg, SHORT);
  else if (Mode == IDLE_MODE) StopTimer((eTimerId)CurrentMode);

  CurrentMode = Mode;

  if (Mode == IDLE_MODE)
  {
    if (Ringing()) SendMessage(ShowCallMsg, CALL_REJECTED);
    IdleUpdateHandler();
  }
  else
  {
    StartTimer((eTimerId)Mode);
    if (Mode == MUSIC_MODE)
    {
        MusicIcon(ICON_MUSIC_PLAY);
        MusicIcon(ICON_MUSIC_NEXT);
        SendMessage(UpdateDisplayMsg, Option);
    }
    if (Option & MSG_OPT_UPD_INTERNAL) SendMessage(UpdateDisplayMsg, Option);
  }
}

static void ModeTimeoutHandler()
{
  PrintS("- ModChgTout");
  ChangeMode((eScModeTimeout << 4) | IDLE_MODE);
}

void ResetModeTimer(void)
{
  StartTimer((eTimerId)CurrentMode);
}

static void BluetoothStateChangeHandler(tMessage *pMsg)
{
  if (Splashing)
  {
    if (pMsg->Options == Initial) return;

    Splashing = FALSE;
    RtcUpdateEnabled = TRUE;
    DetermineIdlePage();
    IdleUpdateHandler();
    
    pMsg->Options = (TMPL_MUSIC_MODE << 4) | MUSIC_MODE;
    LoadTemplateHandler(pMsg);

    pMsg->Options = (TMPL_NOTIF_EMPTY << 4) | NOTIF_MODE;
    LoadTemplateHandler(pMsg);

    InitButton();
  }
  else
  {
//    unsigned char Bt = BluetoothState();
//
//    if (Bt == Initial) {DISABLE_BUTTONS();}
//    else if (Bt == On || Bt == Off) ResetButtonAction();

    if (BluetoothState() == On) ResetButtonAction();
    
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
  
//  PrintF("- Idle Pg:%d", CurrentPage[PAGE_TYPE_IDLE]);
}

static void MenuModeHandler(unsigned char Option)
{
  PageType = PAGE_TYPE_MENU;

  if (Option) CurrentPage[PageType] = Option;
  DrawMenu(CurrentPage[PageType]);
}

static void MenuButtonHandler(unsigned char MsgOptions)
{
  switch (MsgOptions)
  {
  case MENU_BUTTON_OPTION_EXIT:

    IdleUpdateHandler();
    break;

  case MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH:

    if (BluetoothState() != Initial)
      SendMessage(RadioOn() ? TurnRadioOffMsg : TurnRadioOnMsg, MSG_OPT_NONE);
    break;

  case MENU_BUTTON_OPTION_DISPLAY_SECONDS:
    ToggleProperty(PROP_TIME_SECOND);
    MenuModeHandler(0);
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM:
//    LinkAlarmEnable = !LinkAlarmEnable;
//    MenuModeHandler(0);
    break;

  case MENU_BUTTON_OPTION_INVERT_DISPLAY:

//    SendMessage(EnableAdvMsg, MSG_OPT_NONE);
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
  
    SendMessage(VibrateMsg, VIBRA_PATTERN_TEST);
    CheckLpm();
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

static void MusicIcon(unsigned char options)
{
    unsigned char i;
    unsigned char icon_type = options >> 4;
    unsigned char const* pIcon;
    tMessage Msg;
    unsigned char HID_options;
    tMessage button_event_Msg = {5, ButtonEventMsg, 0, NULL};
    unsigned char Index;

    Msg.Type = WriteBufferMsg;
    Msg.Options = MUSIC_MODE | MSG_OPT_WRTBUF_MULTILINE | ICON_MUSIC_WIDTH << 3;
    Msg.pBuffer = NULL;

    switch (options & ICON_MUSIC_TYPE_MASK)
    {
    case ICON_MUSIC_PLAY:
        pIcon = au8_music_icon_play[options & ICON_MUSIC_ENLARGE_MASK];
        Msg.Length = ICON_MUSIC_PLAY_SIZE + 2;

        HID_options = MSG_OPT_MUSIC_PLAY;
        button_event_Msg.Options = MSG_OPT_PLAY_PAUSE;
        Index = SW_E_INDEX;
        break;

    case ICON_MUSIC_NEXT:
        pIcon = au8_music_icon_next[options & ICON_MUSIC_ENLARGE_MASK];
        Msg.Length = ICON_MUSIC_NEXT_SIZE + 2;

        HID_options = MSG_OPT_MUSIC_NEXT;
        button_event_Msg.Options = MSG_OPT_NEXT_TRACK;
        Index = SW_D_INDEX;
        break;

    case ICON_MUSIC_PLUS:
        pIcon = au8_music_icon_plus[options & ICON_MUSIC_ENLARGE_MASK];
        Msg.Length = ICON_MUSIC_PLUS_SIZE + 2;

        HID_options = MSG_OPT_MUSIC_VOL_UP;
        button_event_Msg.Options = MSG_OPT_VOL_UP;
        Index = SW_B_INDEX;
        break;

    case ICON_MUSIC_MINUS:
        pIcon = au8_music_icon_minus[options & ICON_MUSIC_ENLARGE_MASK];
        Msg.Length = ICON_MUSIC_MINUS_SIZE + 2;

        HID_options = MSG_OPT_MUSIC_VOL_DOWN;
        button_event_Msg.Options = MSG_OPT_VOL_DOWN;
        Index = SW_C_INDEX;
        break;

    default:
        Msg.Length = 0;
        break;
    }

    if ((options & ICON_MUSIC_ENLARGE_MASK) == 0)
    {
        HID_options = MSG_OPT_MUSIC_CHANGE_END;
    }

    if (Msg.Length)
    {
        if (CreateMessage(&Msg))
        {
            // Set the starting row
            Msg.pBuffer[0] = au8_music_icon_positions[icon_type][1];
            Msg.pBuffer[1] = au8_music_icon_positions[icon_type][0];
            PrintF("- start:%d Len: %d", Msg.pBuffer[0], Msg.Length);

            for (i = 2; i < Msg.Length; ++i)
            {
                Msg.pBuffer[i] = pIcon[i - 2];
            }
            RouteMsg(&Msg);

            SendMessage(UpdateDisplayMsg, MUSIC_MODE);
        }

        if (BtPaired()) // Check for SPP/Android
        {
            if (options & ICON_MUSIC_ENLARGE_MASK)
            {
                if (CreateMessage(&button_event_Msg))
                {
                    button_event_Msg.pBuffer[0] = Index;
                    button_event_Msg.pBuffer[1] = MUSIC_MODE;
                    button_event_Msg.pBuffer[2] = BTN_EVT_IMDT;
                    button_event_Msg.pBuffer[3] = button_event_Msg.Type;
                    button_event_Msg.pBuffer[4] = button_event_Msg.Options;
                    RouteMsg(&button_event_Msg);
                }
            }
        }
        else
        {
            // Use HID for iOS only.
            SendMessage(HidMsg, HID_options);
        }
    }
    return;
}

static void HandleMusicStateChange(unsigned char State)
{
  PrintF("- HdlMusicPlyChg:%d", State);

  // clean the song info when stop playing
  if (State == 0)
  {
    SendMessage(LoadTemplateMsg, (TMPL_MUSIC_MODE << 4) | MUSIC_MODE);
    SendMessage(UpdateDisplayMsg, MUSIC_MODE);
  }

  // music icon fits into one message
//  tMessage Msg = {ICON_MUSIC_WIDTH * ICON_MUSIC_HEIGHT + 1, WriteBufferMsg,
//    MUSIC_MODE | MSG_OPT_WRTBUF_MULTILINE | ICON_MUSIC_WIDTH << 3, NULL};
//
//  if (CreateMessage(&Msg))
//  {
//    Msg.pBuffer[0] = MUSIC_STATE_START_ROW;
//    PrintF("- start:%d Len: %d", Msg.pBuffer[0], Msg.Length);
//
//    unsigned char i = 1;
//    for (; i < Msg.Length; ++i) Msg.pBuffer[i] = pIconMusicState[1 - State][i - 1];
//    RouteMsg(&Msg);
//  }
//
//  SendMessage(UpdateDisplayMsg, MUSIC_MODE);
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
    unsigned char Minute = RTCMIN;
    
    if (GetProperty(PROP_TIME_SECOND) || Minute != lastMin)
    {
      SendMessageIsr(UpdateClockMsg, MSG_OPT_NONE);
      lastMin = Minute;
      ExitLpm = TRUE;
    }
  }

  return ExitLpm;
}

static void UpdateClock(void)
{
  unsigned char Updated = FALSE;

  if (CurrentMode == IDLE_MODE)
  {
    if (PageType == PAGE_TYPE_IDLE)
    {
      if (CurrentPage[PageType] == InitPage) DrawDateTime();
      else
      {
        if (GetProperty(PROP_PHONE_DRAW_TOP))
        {
          Updated = UpdateClockWidget();
          if (Updated) SendMessage(UpdateDisplayMsg, MSG_OPT_NEWUI | MSG_OPT_UPD_INTERNAL | IDLE_MODE);

          DrawStatusBar();
        }
        else DrawDateTime();
      }
    }
    else if (PageType == PAGE_TYPE_INFO && CurrentPage[PageType] == CountdownPage)
    {
      DrawStatusBarToLcd();
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
    else
    {
      DISABLE_LCD_LED();
      StopTimer(BacklightTimer);
    }
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

void GetBuildNumber(unsigned char *pBuild)
{
//  memcpy(pBuild, BUILD, BUILD_LENGTH);

  memcpy(pBuild, &BUILD[1], 3);
  *(pBuild + 3) = BUILD[0];
  *(pBuild + 4) = BUILD[4];
  *(pBuild + 5) = BUILD[5];
}

/*! Read the voltage of the battery. This provides power good, battery charging,
 * battery voltage, and battery voltage average.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
static void ReadBatteryVoltageHandler(void)
{
  tMessage Msg = {6, VBatRespMsg, MSG_OPT_NONE, NULL};
  if (CreateMessage(&Msg))
  {
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
  if (Options & PROPERTY_READ) SendMessage(PropRespMsg, GetProperty(PROP_VALID));
  else
  {
    PrintF(">SetProp:0x%02x", Options);
    SetProperty(PROP_VALID, Options & PROP_VALID);
    SendMessage(PropRespMsg, MSG_OPT_NONE);

    if (CurrentMode == IDLE_MODE && PageType == PAGE_TYPE_IDLE)
      SendMessage(IdleUpdateMsg, MSG_OPT_NONE);
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
    tMessage Msg = {2, PropRespMsg, MSG_OPT_NONE, NULL};
    if (CreateMessage(&Msg))
    {
      /* add identifier to outgoing message */
      tWordByteUnion Identifier;
      Identifier.word = pNvPayload->NvalIdentifier;
      Msg.pBuffer[0] = Identifier.Bytes.byte0;
      Msg.pBuffer[1] = Identifier.Bytes.byte1;

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

/*******************************************************************************
 * this writes a signature to the EnterBoot RAM location
 * and generates a reset.  The signature tells the bootloader
 * to stay in bootload mode to accept bootload messaging.
 */
void EnterBootloader(void)
{
  PrintS("Bootloader...");
  DrawBootloaderScreen();
  __disable_interrupt();

  /* disable RAM alternate interrupt vectors */
  SYSCTL &= ~SYSRIVECT;
  SetBootloaderSignature();
  SoftwareReset(RESET_BOOTLOADER, MASTER_RESET);
}

/*******************************************************************************
 * This originally was for battery charge control, but now it handles
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
  SendMessage(WrapperTaskCheckInMsg, MSG_OPT_NONE);
}
