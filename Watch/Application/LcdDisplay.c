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
/*! \file LcdDisplay.c
 *
 */
/******************************************************************************/

#include "FreeRTOS.h"           
#include "task.h"               
#include "queue.h"              

#include "Messages.h"

#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_battery.h"
#include "hal_lcd.h"
#include "hal_lpm.h"
       
#include "DebugUart.h"      
#include "Messages.h"
#include "Utilities.h"
#include "LcdDriver.h"
#include "SerialProfile.h"
#include "MessageQueues.h"
#include "SerialRam.h"
#include "OneSecondTimers.h"
#include "Adc.h"
#include "Buttons.h"
#include "Statistics.h"
#include "OSAL_Nv.h"
#include "Background.h"
#include "NvIds.h"
#include "Icons.h"
#include "Fonts.h"
#include "Display.h"
#include "LcdDisplay.h"   

#define DISPLAY_TASK_QUEUE_LENGTH 8
#define DISPLAY_TASK_STACK_DEPTH	(configMINIMAL_STACK_DEPTH + 90)    
#define DISPLAY_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

xTaskHandle DisplayHandle;

static void DisplayTask(void *pvParameters);

static void DisplayQueueMessageHandler(tMessage* pMsg);
static void SendMyBufferToLcd(unsigned char TotalRows);

static tMessage DisplayMsg;

static tTimerId DisplayTimerId;
static unsigned char RtcUpdateEnable;

/* Message handlers */

static void IdleUpdateHandler(void);
static void ChangeModeHandler(tMessage* pMsg);
static void ModeTimeoutHandler(tMessage* pMsg);
static void WatchStatusScreenHandler(void);
static void BarCodeHandler(tMessage* pMsg);
static void ListPairedDevicesHandler(void);
static void ConfigureDisplayHandler(tMessage* pMsg);
static void ConfigureIdleBuferSizeHandler(tMessage* pMsg);
static void ModifyTimeHandler(tMessage* pMsg);
static void MenuModeHandler(unsigned char MsgOptions);
static void MenuButtonHandler(unsigned char MsgOptions);
static void ToggleSecondsHandler(unsigned char MsgOptions);
static void ConnectionStateChangeHandler(void);

/******************************************************************************/
static void DrawIdleScreen(void);
static void DrawSimpleIdleScreen(void);
static void DrawConnectionScreen(void);
static void InitMyBuffer(void);
static void DisplayStartupScreen(void);
static void SetupSplashScreenTimeout(void);
static void AllocateDisplayTimers(void);
static void StopDisplayTimer(void);
static void DetermineIdlePage(void);

static void DrawMenu1(void);
static void DrawMenu2(void);
static void DrawMenu3(void);
static void DrawCommonMenuIcons(void);

static void FillMyBuffer(unsigned char StartingRow,
                         unsigned char NumberOfRows,
                         unsigned char FillValue);

static void PrepareMyBufferForLcd(unsigned char StartingRow,
                                  unsigned char NumberOfRows);


static void CopyRowsIntoMyBuffer(unsigned char const* pImage, 
                                 unsigned char StartingRow,
                                 unsigned char NumberOfRows);

static void CopyColumnsIntoMyBuffer(unsigned char const* pImage,
                                    unsigned char StartingRow,
                                    unsigned char NumberOfRows,
                                    unsigned char StartingColumn,
                                    unsigned char NumberOfColumns);

static void WriteFoo(unsigned char const * pFoo,
                     unsigned char RowOffset,
                     unsigned char ColumnOffset);

static void DisplayAmPm(void);
static void DisplayDayOfWeek(void);
static void DisplayDate(void);

/* the internal buffer */
#define STARTING_ROW                  ( 0 )
#define WATCH_DRAWN_IDLE_BUFFER_ROWS  ( 30 )
#define PHONE_IDLE_BUFFER_ROWS        ( 66 )

static tLcdLine pMyBuffer[NUM_LCD_ROWS];

/******************************************************************************/

static unsigned char nvIdleBufferConfig;
static unsigned char nvIdleBufferInvert;

static void SaveIdleBufferInvert(void);

/******************************************************************************/

unsigned char nvDisplaySeconds = 0;
static void SaveDisplaySeconds(void);

/******************************************************************************/


/******************************************************************************/

typedef enum
{
  ReservedPage,
  NormalPage,
  /* the next three are only used on power-up */
  RadioOnWithPairingInfoPage,
  RadioOnWithoutPairingInfoPage,
  BluetoothOffPage,
  Menu1Page,
  Menu2Page,
  Menu3Page,
  ListPairedDevicesPage,
  WatchStatusPage,
  QrCodePage
  
} etIdlePageMode;

static etIdlePageMode CurrentIdlePage;
static etIdlePageMode LastIdlePage = ReservedPage;

static unsigned char AllowConnectionStateChangeToUpdateScreen;

static void ConfigureIdleUserInterfaceButtons(void);

static void DontChangeButtonConfiguration(void);
static void DefaultApplicationAndNotificationButtonConfiguration(void);
static void SetupNormalIdleScreenButtons(void);

/******************************************************************************/

//
const unsigned char pBarCodeImage[NUM_LCD_ROWS*NUM_LCD_COL_BYTES];
const unsigned char pMetaWatchSplash[NUM_LCD_ROWS*NUM_LCD_COL_BYTES];
const unsigned char Am[10*4];
const unsigned char Pm[10*4];
const unsigned char DaysOfWeek[7][10*4];

/******************************************************************************/

static unsigned char LastMode = IDLE_MODE;
static unsigned char CurrentMode = IDLE_MODE;

//static unsigned char ReturnToApplicationMode;


/******************************************************************************/

static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;

static void WriteFontCharacter(unsigned char Character);
static void WriteFontString(tString* pString);

/******************************************************************************/

/*! Initialize the LCD display task
 *
 * Initializes the display driver, clears the display buffer and starts the
 * display task
 *
 * \return none, result is to start the display task
 */
void InitializeDisplayTask(void)
{
  InitMyBuffer();

  QueueHandles[DISPLAY_QINDEX] = 
    xQueueCreate( DISPLAY_TASK_QUEUE_LENGTH, MESSAGE_QUEUE_ITEM_SIZE  );
  
  // task function, task name, stack len , task params, priority, task handle
  xTaskCreate(DisplayTask, 
              (const signed char *)"DISPLAY", 
              DISPLAY_TASK_STACK_DEPTH, 
              NULL, 
              DISPLAY_TASK_PRIORITY, 
              &DisplayHandle);
  
    
  ClearShippingModeFlag();

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
      
  LcdPeripheralInit();
  
  DisplayStartupScreen();
  
  SerialRamInit();
  
  InitializeIdleBufferConfig();
  InitializeIdleBufferInvert();
  InitializeDisplaySeconds();
  InitializeLinkAlarmEnable();
  InitializeModeTimeouts();
  InitializeTimeFormat();
  InitializeDateFormat();
  AllocateDisplayTimers();
  SetupSplashScreenTimeout();

  DontChangeButtonConfiguration();
  DefaultApplicationAndNotificationButtonConfiguration();
  SetupNormalIdleScreenButtons();
  
#if 1  
  /* turn the radio on; initialize the serial port profile */
  tMessage Msg;
  SetupMessage(&Msg,TurnRadioOnMsg,NO_MSG_OPTIONS);
  RouteMsg(&Msg);
#endif
  
  for(;;)
  {
    if( pdTRUE == xQueueReceive(QueueHandles[DISPLAY_QINDEX], 
                                &DisplayMsg, portMAX_DELAY) )
    {
      PrintMessageType(&DisplayMsg);
       
      DisplayQueueMessageHandler(&DisplayMsg);
      
      SendToFreeQueue(&DisplayMsg);
      
      CheckStackUsage(DisplayHandle,"Display");
      
      CheckQueueUsage(QueueHandles[DISPLAY_QINDEX]);
      
    }
  }
}
  
/*! Display the startup image or Splash Screen */
static void DisplayStartupScreen(void)
{
  /* draw metawatch logo */
  CopyRowsIntoMyBuffer(pMetaWatchSplash,STARTING_ROW,NUM_LCD_ROWS);
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);
}

/*! Handle the messages routed to the display queue */
static void DisplayQueueMessageHandler(tMessage* pMsg)
{
  unsigned char Type = pMsg->Type;
      
  switch(Type)
  {
    
  case WriteBuffer:
    WriteBufferHandler(pMsg);
    break;

  case LoadTemplate:
    LoadTemplateHandler(pMsg);
    break;
  
  case UpdateDisplay:
    UpdateDisplayHandler(pMsg);
    break;
  
  case IdleUpdate:
    IdleUpdateHandler(); 
    break;

  case ChangeModeMsg:
    ChangeModeHandler(pMsg);
    break;
    
  case ModeTimeoutMsg:
    ModeTimeoutHandler(pMsg);
    break;

  case WatchStatusMsg:
    WatchStatusScreenHandler();
    break;
 
  case BarCode:
    BarCodeHandler(pMsg);
    break;
    
  case ListPairedDevicesMsg:
    ListPairedDevicesHandler();
    break;
  
  case WatchDrawnScreenTimeout:
    IdleUpdateHandler();
    break;
    
  case ConfigureDisplay:
    ConfigureDisplayHandler(pMsg);
    break;
  
  case ConfigureIdleBufferSize:
    ConfigureIdleBuferSizeHandler(pMsg);
    break;

  case ConnectionStateChangeMsg:
    ConnectionStateChangeHandler();
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
      
  case ToggleSecondsMsg:
    ToggleSecondsHandler(pMsg->Options);
    break;
  
  case SplashTimeoutMsg:
    AllowConnectionStateChangeToUpdateScreen = 1;
    IdleUpdateHandler();
    break;
    
  case LowBatteryWarningMsg:
  case LowBatteryBtOffMsg:
    break;
    
  case LinkAlarmMsg:
    if ( QueryLinkAlarmEnable() )
    {
      GenerateLinkAlarm();  
    }
    break;
    
  case RamTestMsg:
    RamTestHandler(pMsg);
    break;
    
  default:
    PrintStringAndHex("<<Unhandled Message>> in Lcd Display Task: Type 0x", Type);
    break;
  }

}

/*! Allocate ids and setup timers for the display modes */
static void AllocateDisplayTimers(void)
{
  DisplayTimerId = AllocateOneSecondTimer();
}

static void SetupSplashScreenTimeout(void)
{
  SetupOneSecondTimer(DisplayTimerId,
                      ONE_SECOND*3,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      SplashTimeoutMsg,
                      NO_MSG_OPTIONS);

  StartOneSecondTimer(DisplayTimerId);
  
  AllowConnectionStateChangeToUpdateScreen = 0;
  
}

static inline void StopDisplayTimer(void)
{
  RtcUpdateEnable = 0;
  StopOneSecondTimer(DisplayTimerId);
}

/*! Draw the Idle screen and cause the remainder of the display to be updated
 * also
 */
static void IdleUpdateHandler(void)
{
  StopDisplayTimer();
  
  /* allow rtc to send IdleUpdate every minute (or second) */
  RtcUpdateEnable = 1;
  
  /* determine if the bottom of the screen should be drawn by the watch */  
  if ( QueryFirstContact() )
  {
    /* 
     * immediately update the screen
     */ 
    if ( nvIdleBufferConfig == WATCH_CONTROLS_TOP )
    {
      /* only draw watch part */
      FillMyBuffer(STARTING_ROW,WATCH_DRAWN_IDLE_BUFFER_ROWS,0x00);
      DrawIdleScreen();
      PrepareMyBufferForLcd(STARTING_ROW,WATCH_DRAWN_IDLE_BUFFER_ROWS);
      SendMyBufferToLcd(WATCH_DRAWN_IDLE_BUFFER_ROWS);
    }
  
    /* now update the remainder of the display */
    /*! make a dirty flag for the idle page drawn by the phone
     * set it whenever watch uses whole screen
     */
    tMessage OutgoingMsg;
    SetupMessage(&OutgoingMsg,
                 UpdateDisplay,
                 (IDLE_MODE | DONT_ACTIVATE_DRAW_BUFFER));
    RouteMsg(&OutgoingMsg);
   
    CurrentIdlePage = NormalPage;
    ConfigureIdleUserInterfaceButtons();
    
  }
  else
  {
    DetermineIdlePage();

    FillMyBuffer(STARTING_ROW,NUM_LCD_ROWS,0x00);
    DrawSimpleIdleScreen();
    DrawConnectionScreen();
    
    PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
    SendMyBufferToLcd(NUM_LCD_ROWS);
    
    ConfigureIdleUserInterfaceButtons();

  }
   
}

static void DetermineIdlePage(void)
{
  etConnectionState cs = QueryConnectionState(); 
  
  switch (cs) 
  {
  case Initializing:       CurrentIdlePage = BluetoothOffPage;              break;
  case ServerFailure:      CurrentIdlePage = BluetoothOffPage;              break;
  case RadioOn:            CurrentIdlePage = RadioOnWithoutPairingInfoPage; break;
  case Paired:             CurrentIdlePage = RadioOnWithPairingInfoPage;    break;
  case Connected:          CurrentIdlePage = NormalPage;                    break;
  case RadioOff:           CurrentIdlePage = BluetoothOffPage;              break;
  case RadioOffLowBattery: CurrentIdlePage = BluetoothOffPage;              break;
  case ShippingMode:       CurrentIdlePage = BluetoothOffPage;              break;
  default:                 CurrentIdlePage = BluetoothOffPage;              break;  
  }
  
  /* if the radio is on but hasn't paired yet then don't show the pairing icon */
  if ( CurrentIdlePage == RadioOnWithoutPairingInfoPage )
  {
    if ( QueryValidPairingInfo() )
    {
      CurrentIdlePage = RadioOnWithPairingInfoPage;  
    }
  }
  
}


static void ConnectionStateChangeHandler(void)
{
  if ( AllowConnectionStateChangeToUpdateScreen )
  {
    /* certain pages should not be exited when a change in the 
     * connection state has occurred 
     */
    switch ( CurrentIdlePage )
    {
    case ReservedPage:
    case NormalPage:
    case RadioOnWithPairingInfoPage:
    case RadioOnWithoutPairingInfoPage:
    case BluetoothOffPage:
      IdleUpdateHandler();
      break;
    
    case Menu1Page:
    case Menu2Page:
    case Menu3Page:
      MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
      break;
    
    case ListPairedDevicesPage:
      ListPairedDevicesHandler();
      break;
    
    case WatchStatusPage:
      WatchStatusScreenHandler();
      break;
    
    case QrCodePage:
      break;
      
    default:
      break;
    }
  }
}

unsigned char QueryButtonMode(void)
{
  unsigned char result;
  
  switch (CurrentMode)
  {
    
  case IDLE_MODE:
    if ( CurrentIdlePage == NormalPage )
    {
      result = NORMAL_IDLE_SCREEN_BUTTON_MODE;
    }
    else
    {
      result = WATCH_DRAWN_SCREEN_BUTTON_MODE;  
    }
    break;
    
  case APPLICATION_MODE:
    result = APPLICATION_SCREEN_BUTTON_MODE;
    break;
  
  case NOTIFICATION_MODE:
    result = NOTIFICATION_BUTTON_MODE;
    break;
  
  case SCROLL_MODE:
    result = SCROLL_MODE;
    break;
  
  }
  
  return result;  
}
static void ChangeModeHandler(tMessage* pMsg)
{
  LastMode = CurrentMode;    
  CurrentMode = (pMsg->Options & MODE_MASK);
  
  unsigned int timeout;
  
  switch ( CurrentMode )
  {
  
  case IDLE_MODE:

    /* this check is so that the watch apps don't mess up the timer */
    if ( LastMode != CurrentMode )
    {
      /* idle update handler will stop display timer */
      IdleUpdateHandler();
      PrintString("Changing mode to Idle\r\n");
    }
    else
    {
      PrintString("Already in Idle mode\r\n");
    }
    break;
  
  case APPLICATION_MODE:
    
    StopDisplayTimer();
    
    timeout = QueryApplicationModeTimeout();
    
    /* don't start the timer if the timeout == 0 
     * this invites things that look like lock ups...
     * it is preferred to make this a large value
     */
    if ( timeout )
    {
      SetupOneSecondTimer(DisplayTimerId,
                          timeout,
                          NO_REPEAT,
                          DISPLAY_QINDEX,
                          ModeTimeoutMsg,
                          APPLICATION_MODE);
          
      StartOneSecondTimer(DisplayTimerId);
    }
    
    PrintString("Changing mode to Application\r\n");
    break;
  
  case NOTIFICATION_MODE:
    
    StopDisplayTimer();
    
    timeout = QueryNotificationModeTimeout();
        
    if ( timeout )
    {
      SetupOneSecondTimer(DisplayTimerId,
                          timeout,
                          NO_REPEAT,
                          DISPLAY_QINDEX,
                          ModeTimeoutMsg,
                          NOTIFICATION_MODE);
      
      StartOneSecondTimer(DisplayTimerId);
    }
    
    PrintString("Changing mode to Notification\r\n");
    break;
  
  default:
    break;
  }
  
  /* 
   * send a message to the Host indicating buffer update / mode change 
   * has completed (don't send message if it is just watch updating time ).
   */
  if ( LastMode != CurrentMode )
  {
    tMessage OutgoingMsg;
    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  StatusChangeEvent,
                                  IDLE_MODE);
    
    OutgoingMsg.pBuffer[0] = (unsigned char)eScUpdateComplete;
    OutgoingMsg.Length = 1;
    RouteMsg(&OutgoingMsg);
    
//    if ( LastMode == APPLICATION_MODE )
//    {    
//      ReturnToApplicationMode = 1;
//    }
//    else
//    {
//      ReturnToApplicationMode = 0;  
//    }
  }
  
}

static void ModeTimeoutHandler(tMessage* pMsg)
{
  switch ( CurrentMode )
  {
  
  case IDLE_MODE:
    break;
  
  case APPLICATION_MODE:
  case NOTIFICATION_MODE:
  case SCROLL_MODE: 
    /* go back to idle mode */
    IdleUpdateHandler();
    break;

  default:
    break;
  }  
  
  /* send a message to the host indicating that a timeout occurred */
  tMessage OutgoingMsg;
  SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                StatusChangeEvent,
                                CurrentMode);
    
  OutgoingMsg.pBuffer[0] = (unsigned char)eScModeTimeout;
  OutgoingMsg.Length = 1;
  RouteMsg(&OutgoingMsg);
    
}
  

static void WatchStatusScreenHandler(void)
{
  StopDisplayTimer();
  
  FillMyBuffer(STARTING_ROW,NUM_LCD_ROWS,0x00);
    
  /*
   * Add Status Icons
   */
  unsigned char const * pIcon;
  
  if ( QueryBluetoothOn() )
  {
    pIcon = pBluetoothOnStatusScreenIcon;
  }
  else
  {
    pIcon = pBluetoothOffStatusScreenIcon;  
  }
  
  CopyColumnsIntoMyBuffer(pIcon,
                          0,
                          STATUS_ICON_SIZE_IN_ROWS,
                          LEFT_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);  
    
  
  if ( QueryPhoneConnected() )
  {
    pIcon = pPhoneConnectedStatusScreenIcon;
  }
  else
  {
    pIcon = pPhoneDisconnectedStatusScreenIcon;  
  }
  
  CopyColumnsIntoMyBuffer(pIcon,
                          0,
                          STATUS_ICON_SIZE_IN_ROWS,
                          CENTER_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);  
  
  unsigned int bV = ReadBatterySenseAverage();
  
  if ( QueryBatteryCharging() )
  {
    pIcon = pBatteryChargingStatusScreenIcon;
  }
  else
  {
    if ( bV > 4000 )
    {
      pIcon = pBatteryFullStatusScreenIcon;
    }
    else if ( bV < 3500 )
    {
      pIcon = pBatteryLowStatusScreenIcon;  
    }
    else
    {
      pIcon = pBatteryMediumStatusScreenIcon;  
    }
  }
  
  
  CopyColumnsIntoMyBuffer(pIcon,
                          0,
                          STATUS_ICON_SIZE_IN_ROWS,
                          RIGHT_STATUS_ICON_COLUMN,
                          STATUS_ICON_SIZE_IN_COLUMNS);  

  /* display battery voltage */
  unsigned char msd = 0;
  
  gRow = 27+2;
  gColumn = 8;
  gBitColumnMask = BIT6;
  SetFont(MetaWatch7);
  

  msd = bV / 1000;
  bV = bV % 1000;
  WriteFontCharacter(msd+'0');
  WriteFontCharacter('.');
  
  msd = bV / 100;
  bV = bV % 100;
  WriteFontCharacter(msd+'0');
  
  msd = bV / 10;
  bV = bV % 10;
  WriteFontCharacter(msd+'0');
  WriteFontCharacter(bV+'0'); 
   
  /*
   * Add Wavy line
   */
  gRow += 12;
  CopyRowsIntoMyBuffer(pWavyLine,gRow,NUMBER_OF_ROWS_IN_WAVY_LINE);
  
  
  /*
   * Add details
   */

  /* add MAC address */
  gRow += NUMBER_OF_ROWS_IN_WAVY_LINE+2;
  gColumn = 0;
  gBitColumnMask = BIT4;
  WriteFontString(GetLocalBluetoothAddressString());

  /* add the firmware version */
  gRow += 12;
  gColumn = 0;
  gBitColumnMask = BIT4;
  WriteFontString("App ");
  WriteFontString(VERSION_STRING);

  /* stack version */
  gRow += 12;
  gColumn = 0;
  gBitColumnMask = BIT4;
  WriteFontString("Stack ");
  WriteFontString(GetStackVersion());

  /* add msp430 revision */
  gRow +=12;
  gColumn = 0;
  gBitColumnMask = BIT4;
  WriteFontString("MSP430 Rev ");
  WriteFontCharacter(GetMsp430HardwareRevision());
 
  /* display entire buffer */
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);

  CurrentIdlePage = WatchStatusPage;
  ConfigureIdleUserInterfaceButtons();
  
  /* refresh the status page once a minute */  
  SetupOneSecondTimer(DisplayTimerId,
                      ONE_SECOND*60,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      WatchStatusMsg,
                      NO_MSG_OPTIONS);
  
  StartOneSecondTimer(DisplayTimerId);
  
}


/* the bar code should remain displayed until the button is pressed again
 * or another mode is started 
 */
static void BarCodeHandler(tMessage* pMsg)
{
  StopDisplayTimer();
    
  FillMyBuffer(STARTING_ROW,NUM_LCD_ROWS,0x00);
  
  CopyRowsIntoMyBuffer(pBarCodeImage,STARTING_ROW,NUM_LCD_ROWS);

  /* display entire buffer */
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);
  
  CurrentIdlePage = QrCodePage;
  ConfigureIdleUserInterfaceButtons();
  
}

static void ListPairedDevicesHandler(void)
{  
  StopDisplayTimer();
  
  /* draw entire region */
  FillMyBuffer(STARTING_ROW,NUM_LCD_ROWS,0x00);
  
  tString pBluetoothAddress[12+1];
  tString pBluetoothName[12+1];

  gRow = 4;
  gColumn = 0;
  SetFont(MetaWatch7);
  
  unsigned char i;
  for( i = 0; i < 3; i++)
  {

    unsigned char j;
    pBluetoothName[0] = 'D';
    pBluetoothName[1] = 'e';
    pBluetoothName[2] = 'v';
    pBluetoothName[3] = 'i';
    pBluetoothName[4] = 'c';
    pBluetoothName[5] = 'e';
    pBluetoothName[6] = ' ';
    pBluetoothName[7] = 'N';
    pBluetoothName[8] = 'a';
    pBluetoothName[9] = 'm';
    pBluetoothName[10] = 'e';
    pBluetoothName[11] = '1' + i;
    
    for(j = 0; j < sizeof(pBluetoothAddress); j++)
    {
      pBluetoothAddress[j] = '0';
    }
    
    QueryLinkKeys(i,pBluetoothAddress,pBluetoothName,12);
    
    gColumn = 0;
    gBitColumnMask = BIT4;
    WriteFontString(pBluetoothName);
    gRow += 12;
    
    gColumn = 0;
    gBitColumnMask = BIT4;
    WriteFontString(pBluetoothAddress);
    gRow += 12+5;
      
  }
  
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);

  CurrentIdlePage = ListPairedDevicesPage;
  ConfigureIdleUserInterfaceButtons();

}


static void DrawConnectionScreen()
{
  
  /* this is part of the idle update
   * timing is controlled by the idle update timer
   * buffer was already cleared when drawing the time
   */
  
  unsigned char const* pSwash;
  
  switch (CurrentIdlePage)
  {
  case RadioOnWithPairingInfoPage:     
    pSwash = pBootPageConnectionSwash;  
    break;
  case RadioOnWithoutPairingInfoPage:
    pSwash = pBootPagePairingSwash;
    break;
  case BluetoothOffPage:
    pSwash = pBootPageBluetoothOffSwash;
    break;
  default:
    pSwash = pBootPageUnknownSwash;
    break;
  
  }
  
  CopyRowsIntoMyBuffer(pSwash,WATCH_DRAWN_IDLE_BUFFER_ROWS+1,32);
  
  /* local bluetooth address */
  gRow = 65;
  gColumn = 0;
  gBitColumnMask = BIT4;
  SetFont(MetaWatch7);
  WriteFontString(GetLocalBluetoothAddressString());

  /* add the firmware version */
  gRow = 75;
  gColumn = 0;
  gBitColumnMask = BIT4;
  WriteFontString("App ");
  WriteFontString(VERSION_STRING);
  
  /* and the stack version */
  gRow = 85;
  gColumn = 0;
  gBitColumnMask = BIT4;
  WriteFontString("Stack ");
  WriteFontString(GetStackVersion());

}

/* change the parameter but don't save it into flash */
static void ConfigureDisplayHandler(tMessage* pMsg)
{
  switch (pMsg->Options)
  {
  case CONFIGURE_DISPLAY_OPTION_DONT_DISPLAY_SECONDS:
    nvDisplaySeconds = 0x00;
    break;
  case CONFIGURE_DISPLAY_OPTION_DISPLAY_SECONDS:
    nvDisplaySeconds = 0x01;
    break;
  case CONFIGURE_DISPLAY_OPTION_DONT_INVERT_DISPLAY:
    nvIdleBufferInvert = 0x00;
    break;
  case CONFIGURE_DISPLAY_OPTION_INVERT_DISPLAY:
    nvIdleBufferInvert = 0x01;
    break;
  }

  if ( CurrentMode == IDLE_MODE )
  {
    IdleUpdateHandler();  
  }
  
}
    

static void ConfigureIdleBuferSizeHandler(tMessage* pMsg)
{
  nvIdleBufferConfig = pMsg->pBuffer[0] & IDLE_BUFFER_CONFIG_MASK;
  
  if ( CurrentMode == IDLE_MODE )
  {
    if ( nvIdleBufferConfig == WATCH_CONTROLS_TOP )
    {
      IdleUpdateHandler();
    }
  }
  
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
  
  /* now redraw the screen */
  IdleUpdateHandler();
  
}

unsigned char GetIdleBufferConfiguration(void)
{
  return nvIdleBufferConfig;  
}


static void SendMyBufferToLcd(unsigned char TotalRows)
{
  UpdateMyDisplay((unsigned char*)pMyBuffer,TotalRows);
}


static void InitMyBuffer(void)
{
  int row;
  int col;
  
  // Clear the display buffer.  Step through the rows
  for(row = STARTING_ROW; row < NUM_LCD_ROWS; row++)
  {
      // clear a horizontal line
      for(col = 0; col < NUM_LCD_COL_BYTES; col++)
      {
        pMyBuffer[row].Row = row+FIRST_LCD_LINE_OFFSET;
        pMyBuffer[row].Data[col] = 0x00;
        pMyBuffer[row].Dummy = 0x00;
        
      }
  }
    
  
}


static void FillMyBuffer(unsigned char StartingRow,
                         unsigned char NumberOfRows,
                         unsigned char FillValue)
{
  int row = StartingRow;
  int col;
  
  // Clear the display buffer.  Step through the rows
  for( ; row < NUM_LCD_ROWS && row < StartingRow+NumberOfRows; row++ )
  {
    // clear a horizontal line
    for(col = 0; col < NUM_LCD_COL_BYTES; col++)
    {
      pMyBuffer[row].Data[col] = FillValue;
    }
  }
  
}

static void PrepareMyBufferForLcd(unsigned char StartingRow,
                                  unsigned char NumberOfRows)
{
  int row = StartingRow;
  int col;
  
  /* 
   * flip the bits before sending to LCD task because it will 
   * dma this portion of the screen
  */
  if ( QueryInvertDisplay() == NORMAL_DISPLAY )
  {
    for( ; row < NUM_LCD_ROWS && row < StartingRow+NumberOfRows; row++)
    {
      for(col = 0; col < NUM_LCD_COL_BYTES; col++)
      {
        pMyBuffer[row].Data[col] = ~(pMyBuffer[row].Data[col]);
      }
    }
  }
    
}

    

static void CopyRowsIntoMyBuffer(unsigned char const* pImage, 
                                 unsigned char StartingRow,
                                 unsigned char NumberOfRows)
{

  unsigned char DestRow = StartingRow;
  unsigned char SourceRow = 0;
  unsigned char col = 0;
  
  while ( DestRow < NUM_LCD_ROWS && SourceRow < NumberOfRows )
  {
    for(col = 0; col < NUM_LCD_COL_BYTES; col++)
    {
      pMyBuffer[DestRow].Data[col] = pImage[SourceRow*NUM_LCD_COL_BYTES+col];
    }
    
    DestRow++;
    SourceRow++;
    
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
  while ( DestRow < NUM_LCD_ROWS && RowCounter < NumberOfRows )
  {
    DestColumn = StartingColumn;
    ColumnCounter = 0;
    while ( DestColumn < NUM_LCD_COL_BYTES && ColumnCounter < NumberOfColumns )
    {
      pMyBuffer[DestRow].Data[DestColumn] = pImage[SourceIndex];
      
      DestColumn++;
      ColumnCounter++;
      SourceIndex++;
    }
    
    DestRow++;
    RowCounter++;    
  }

}

static void DrawIdleScreen(void)
{
  unsigned char msd;
  unsigned char lsd;
    
  /* display hour */
  int Hour = RTCHOUR;
  
  /* if required convert to twelve hour format */
  if ( GetTimeFormat() == TWELVE_HOUR )
  {    
    if ( Hour == 0 )
    {
      Hour = 12;  
    }
    else if ( Hour > 12 )
    {
      Hour -= 12;  
    }
  }
  
  msd = Hour / 10;
  lsd = Hour % 10;

  gRow = 6;
  gColumn = 0;
  gBitColumnMask = BIT4;
  SetFont(MetaWatchTime);
  
  /* if first digit is zero then leave location blank */
  if ( msd == 0 && GetTimeFormat() == TWELVE_HOUR )
  {  
    WriteFontCharacter(TIME_CHARACTER_SPACE_INDEX);  
  }
  else
  {
    WriteFontCharacter(msd);
  }
  
  WriteFontCharacter(lsd);
  
  WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
  
  /* display minutes */
  int Minutes = RTCMIN;
  msd = Minutes / 10;
  lsd = Minutes % 10;
  WriteFontCharacter(msd);
  WriteFontCharacter(lsd);
    
  if ( nvDisplaySeconds )
  {
    int Seconds = RTCSEC;
    msd = Seconds / 10;
    lsd = Seconds % 10;    
    
    WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
    WriteFontCharacter(msd);
    WriteFontCharacter(lsd);
    
  }
  else /* now things starting getting fun....*/
  {
    DisplayAmPm();
    
    if ( QueryBluetoothOn() == 0 )
    {
      CopyColumnsIntoMyBuffer(pBluetoothOffIdlePageIcon,
                              IDLE_PAGE_ICON_STARTING_ROW,                                    
                              IDLE_PAGE_ICON_SIZE_IN_ROWS,
                              IDLE_PAGE_ICON_STARTING_COL,
                              IDLE_PAGE_ICON_SIZE_IN_COLS);
    }
    else if ( QueryPhoneConnected() == 0 )
    {
      CopyColumnsIntoMyBuffer(pPhoneDisconnectedIdlePageIcon,
                              IDLE_PAGE_ICON_STARTING_ROW,                                    
                              IDLE_PAGE_ICON_SIZE_IN_ROWS,
                              IDLE_PAGE_ICON_STARTING_COL,
                              IDLE_PAGE_ICON_SIZE_IN_COLS);  
    }
    else
    {
      if ( QueryBatteryCharging() )
      {
        CopyColumnsIntoMyBuffer(pBatteryChargingIdlePageIconType2,
                                IDLE_PAGE_ICON2_STARTING_ROW,                                    
                                IDLE_PAGE_ICON2_SIZE_IN_ROWS,
                                IDLE_PAGE_ICON2_STARTING_COL,
                                IDLE_PAGE_ICON2_SIZE_IN_COLS);
      }
      else
      {
        unsigned int bV = ReadBatterySenseAverage();
      
        if ( bV < 3500 )
        {
          CopyColumnsIntoMyBuffer(pLowBatteryIdlePageIconType2,
                                  IDLE_PAGE_ICON2_STARTING_ROW,                                    
                                  IDLE_PAGE_ICON2_SIZE_IN_ROWS,
                                  IDLE_PAGE_ICON2_STARTING_COL,
                                  IDLE_PAGE_ICON2_SIZE_IN_COLS);
        }
        else
        {
          DisplayDayOfWeek();
          DisplayDate();
        }
      }
    }
  }
 
}

static void DrawSimpleIdleScreen(void)
{
  unsigned char msd;
  unsigned char lsd;
    
  /* display hour */
  int Hour = RTCHOUR;
  
  /* if required convert to twelve hour format */
  if ( GetTimeFormat() == TWELVE_HOUR )
  {    
    if ( Hour == 0 )
    {
      Hour = 12;  
    }
    else if ( Hour > 12 )
    {
      Hour -= 12;  
    }
  }
  
  msd = Hour / 10;
  lsd = Hour % 10;

  gRow = 6;
  gColumn = 0;
  gBitColumnMask = BIT4;
  SetFont(MetaWatchTime);
  
  /* if first digit is zero then leave location blank */
  if ( msd == 0 && GetTimeFormat() == TWELVE_HOUR )
  {  
    WriteFontCharacter(TIME_CHARACTER_SPACE_INDEX);  
  }
  else
  {
    WriteFontCharacter(msd);
  }
  WriteFontCharacter(lsd);
  
  WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
  
  /* display minutes */
  int Minutes = RTCMIN;
  msd = Minutes / 10;
  lsd = Minutes % 10;
  WriteFontCharacter(msd);
  WriteFontCharacter(lsd);
    
  if ( nvDisplaySeconds )
  {
    int Seconds = RTCSEC;
    msd = Seconds / 10;
    lsd = Seconds % 10;    
    
    WriteFontCharacter(TIME_CHARACTER_COLON_INDEX);
    WriteFontCharacter(msd);
    WriteFontCharacter(lsd);
    
  }
  else 
  {
    DisplayAmPm();
    DisplayDayOfWeek();
    DisplayDate();

  }
 
}

static void MenuModeHandler(unsigned char MsgOptions)
{
  StopDisplayTimer();
  
  /* draw entire region */
  FillMyBuffer(STARTING_ROW,PHONE_IDLE_BUFFER_ROWS,0x00);

  switch (MsgOptions)
  {
  
  case MENU_MODE_OPTION_PAGE1:
    DrawMenu1();
    CurrentIdlePage = Menu1Page;
    ConfigureIdleUserInterfaceButtons();
    break;
  
  case MENU_MODE_OPTION_PAGE2:
    DrawMenu2();
    CurrentIdlePage = Menu2Page;
    ConfigureIdleUserInterfaceButtons();
    break;
  
  case MENU_MODE_OPTION_PAGE3:
    DrawMenu3();
    CurrentIdlePage = Menu3Page;
    ConfigureIdleUserInterfaceButtons();
    break;
  
  case MENU_MODE_OPTION_UPDATE_CURRENT_PAGE:
  
  default:
    switch ( CurrentIdlePage )
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
      PrintString("Menu Mode Screen Selection Error\r\n");
      break;
    }
    break;
  }
  
  /* these icons are common to all menus */
  DrawCommonMenuIcons();
  
  /* only invert the part that was just drawn */
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);

  /* MENU MODE DOES NOT TIMEOUT */
  
}

static void DrawMenu1(void)
{
  unsigned char const * pIcon;
  
  if ( QueryConnectionState() == Initializing )
  {
    pIcon = pPairableInitIcon;
  }
  else if ( QueryDiscoverable() )
  {
    pIcon = pPairableIcon;
  }
  else
  {
    pIcon = pUnpairableIcon;
  }

  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS); 
  
  /***************************************************************************/
  
  if ( QueryConnectionState() == Initializing )
  {
    pIcon = pBluetoothInitIcon;
  }
  else if ( QueryBluetoothOn() )
  {
    pIcon = pBluetoothOnIcon;
  }
  else
  {
    pIcon = pBluetoothOffIcon;  
  }
                          
  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);  

  /***************************************************************************/
  
  if ( QueryLinkAlarmEnable() )
  {
    pIcon = pLinkAlarmOnIcon;
  }
  else
  {
    pIcon = pLinkAlarmOffIcon; 
  }
  
  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS); 
}

static void DrawMenu2(void)
{
  /* top button is always soft reset */
  CopyColumnsIntoMyBuffer(pResetButtonIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
  
  unsigned char const * pIcon;
  
  if ( QueryRstPinEnabled() )
  {
    pIcon = pRstPinIcon;
  }
  else
  {
    pIcon = pNmiPinIcon;
  }
  
  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS); 
  
  /***************************************************************************/
  
  if ( QueryConnectionState() == Initializing )
  {
    pIcon = pSspInitIcon;
  }
  else if ( QuerySecureSimplePairingEnabled() )
  {
    pIcon = pSspEnabledIcon;
  }
  else
  { 
    pIcon = pSspDisabledIcon;
  }

  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);   
  
}

static void DrawMenu3(void)
{
  unsigned char const * pIcon;
  
  pIcon = pNormalDisplayMenuIcon;

  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS); 
  /***************************************************************************/
  
#if 0
  /* shipping mode was removed for now */
  CopyColumnsIntoMyBuffer(pShippingModeIcon,
                          BUTTON_ICON_A_F_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);  
#endif
  /***************************************************************************/
  
  if ( nvDisplaySeconds )
  {
    pIcon = pSecondsOnMenuIcon;
  }
  else
  { 
    pIcon = pSecondsOffMenuIcon;
  }

  CopyColumnsIntoMyBuffer(pIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);   
  
}

static void DrawCommonMenuIcons(void)
{
  CopyColumnsIntoMyBuffer(pNextIcon,
                          BUTTON_ICON_B_E_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
         
  CopyColumnsIntoMyBuffer(pLedIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          LEFT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);
  
  CopyColumnsIntoMyBuffer(pExitIcon,
                          BUTTON_ICON_C_D_ROW,
                          BUTTON_ICON_SIZE_IN_ROWS,
                          RIGHT_BUTTON_COLUMN,
                          BUTTON_ICON_SIZE_IN_COLUMNS);  
}

static void MenuButtonHandler(unsigned char MsgOptions)
{
  StopDisplayTimer();

  tMessage OutgoingMsg;
  
  switch (MsgOptions)
  {
  case MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY:
    
    if ( QueryConnectionState() != Initializing )
    {
      SetupMessage(&OutgoingMsg,PairingControlMsg,NO_MSG_OPTIONS);
        
      if ( QueryDiscoverable() )
      {
        OutgoingMsg.Options = PAIRING_CONTROL_OPTION_DISABLE_PAIRING;
      }
      else
      {
        OutgoingMsg.Options = PAIRING_CONTROL_OPTION_ENABLE_PAIRING;  
      }
      
      RouteMsg(&OutgoingMsg);
    }
    /* screen will be updated with a message from spp */
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM:
    ToggleLinkAlarmEnable();
    MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;
  
  case MENU_BUTTON_OPTION_EXIT:          
    
    /* save all of the non-volatile items */
    SetupMessage(&OutgoingMsg,PairingControlMsg,PAIRING_CONTROL_OPTION_SAVE_SPP);
    RouteMsg(&OutgoingMsg);
    
    SaveLinkAlarmEnable();
    SaveRstNmiConfiguration();
    SaveIdleBufferInvert();
    SaveDisplaySeconds();
      
    /* go back to the normal idle screen */
    SetupMessage(&OutgoingMsg,IdleUpdate,NO_MSG_OPTIONS);
    RouteMsg(&OutgoingMsg);

    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH:
    
    if ( QueryConnectionState() != Initializing )
    {
      if ( QueryBluetoothOn() )
      {
        SetupMessage(&OutgoingMsg,TurnRadioOffMsg,NO_MSG_OPTIONS);
      }
      else
      {
        SetupMessage(&OutgoingMsg,TurnRadioOnMsg,NO_MSG_OPTIONS);
      }
      
      RouteMsg(&OutgoingMsg);
    }
    /* screen will be updated with a message from spp */
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_SECURE_SIMPLE_PAIRING:
    if ( QueryConnectionState() != Initializing )
    {
      SetupMessage(&OutgoingMsg,PairingControlMsg,PAIRING_CONTROL_OPTION_TOGGLE_SSP);
      RouteMsg(&OutgoingMsg);
    }
    /* screen will be updated with a message from spp */
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN:
    if ( QueryRstPinEnabled() )
    {
      DisableRstPin();
    }
    else
    {
      EnableRstPin(); 
    }
    MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;
    
  case MENU_BUTTON_OPTION_DISPLAY_SECONDS:
    ToggleSecondsHandler(TOGGLE_SECONDS_OPTIONS_DONT_UPDATE_IDLE);
    MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;
    
  case MENU_BUTTON_OPTION_INVERT_DISPLAY:
    if ( nvIdleBufferInvert == 1 )
    {
      nvIdleBufferInvert = 0;
    }
    else
    {
      nvIdleBufferInvert = 1;
    }
    MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;
    
  default:
    break;
  }
   
}

static void ToggleSecondsHandler(unsigned char Options)
{
  if ( nvDisplaySeconds == 0 )
  {
    nvDisplaySeconds = 1;  
  }
  else
  {
    nvDisplaySeconds = 0;  
  }

  if ( Options == TOGGLE_SECONDS_OPTIONS_UPDATE_IDLE )
  {
    IdleUpdateHandler();
  }
  
}

static void DisplayAmPm(void)
{  
  /* don't display am/pm in 24 hour mode */
  if ( GetTimeFormat() == TWELVE_HOUR ) 
  {
    int Hour = RTCHOUR;
    
    unsigned char const *pFoo;
    
    if ( Hour >= 12 )
    {
      pFoo = Pm;  
    }
    else
    {
      pFoo = Am;  
    }
    
    WriteFoo(pFoo,0,8);
  }
  
}

static void DisplayDayOfWeek(void)
{
  int DayOfWeek = RTCDOW;
  
  unsigned char const *pFoo = DaysOfWeek[DayOfWeek];
    
  WriteFoo(pFoo,10,8);
  
}

static void DisplayDate(void)
{
  if ( QueryFirstContact() )
  {
    int First;
    int Second;
    
    /* determine if month or day is displayed first */
    if ( GetDateFormat() == MONTH_FIRST )
    {
      First = RTCMON;
      Second = RTCDAY;
    }
    else
    {
      First = RTCDAY;
      Second = RTCMON;
    }
    
    /* make it line up with AM/PM and Day of Week */
    gRow = 22;
    gColumn = 8;
    gBitColumnMask = BIT1;
    SetFont(MetaWatch7);
    WriteFontCharacter(First/10+'0');
    WriteFontCharacter(First%10+'0');
    WriteFontCharacter('/');
    WriteFontCharacter(Second/10+'0');
    WriteFontCharacter(Second%10+'0');
    
  }
}

/* these items are 4w*10h */
static void WriteFoo(unsigned char const * pFoo,
                     unsigned char RowOffset,
                     unsigned char ColumnOffset)
{
  
  /* copy digit into correct position */
  unsigned char RowNumber;
  unsigned char Column;
  
  for ( Column = 0; Column < 4; Column++ )
  {
    for ( RowNumber = 0; RowNumber < 10; RowNumber++ )
    {
      pMyBuffer[RowNumber+RowOffset].Data[Column+ColumnOffset] = 
        pFoo[RowNumber+(Column*10)];
    }
  }
  
}

unsigned char* GetTemplatePointer(unsigned char TemplateSelect)
{
  return NULL;
}


const unsigned char pBarCodeImage[NUM_LCD_ROWS*NUM_LCD_COL_BYTES] = 
{
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0xFC,0xCF,0xFF,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0xFC,0xCF,0xFF,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0xF0,0xC0,0x00,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0xF0,0xC0,0x00,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xCC,0xCC,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xCC,0xCC,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0x3C,0xC0,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0x3C,0xC0,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xFC,0xCF,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xFC,0xCF,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0x00,0xCF,0x00,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0x00,0xCF,0x00,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0xCC,0xCC,0xFF,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0xCC,0xCC,0xFF,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0xF0,0x0C,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0xF0,0x0C,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xC3,0xCC,0x3F,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xC3,0xCC,0x3F,0xFC,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xF0,0x33,0x0C,0xFF,0x0C,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xF0,0x33,0x0C,0xFF,0x0C,0x0C,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFC,0xF0,0xCF,0xF0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFC,0xF0,0xCF,0xF0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x30,0xF3,0x03,0x33,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x30,0xF3,0x03,0x33,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xF3,0x0C,0x00,0x03,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xF3,0x0C,0x00,0x03,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0xFC,0xFF,0x03,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0xFC,0xFF,0x03,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0x30,0xCC,0x30,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0x30,0xCC,0x30,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0xF0,0x33,0x3F,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0xF0,0x33,0x3F,0x0F,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0x30,0x30,0xCC,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0x30,0x30,0xCC,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xCC,0xCF,0xC0,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xCC,0xCF,0xC0,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xFC,0x33,0xF3,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xCC,0xCF,0xFC,0x33,0xF3,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0x3C,0xCF,0xC3,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x0C,0xC0,0x3C,0xCF,0xC3,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0x3C,0x03,0xF3,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0xFC,0xFF,0x3C,0x03,0xF3,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const unsigned char pMetaWatchSplash[NUM_LCD_ROWS*NUM_LCD_COL_BYTES] = 
{
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x30,0x60,0x80,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x30,0x60,0xC0,0x01,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x70,0x70,0xC0,0x01,0xE0,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x70,0xF0,0x40,0xE1,0xFF,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xD8,0xD8,0x60,0x63,0xE0,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xD8,0xD8,0x60,0x63,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xC8,0x58,0x34,0x26,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x8C,0x0D,0x36,0x36,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x0E,0x8C,0x0D,0x36,0x36,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xFE,0x0F,0x05,0x1E,0x1C,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x0E,0x00,0x07,0x1C,0x1C,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x07,0x0C,0x18,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x02,0x0C,0x18,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x30,0x18,0xFC,0xFC,0x70,0x04,0x00,0x31,0xFC,0xE1,0x83,0x40,
0x30,0x18,0xFC,0xFC,0x70,0x04,0x02,0x31,0x20,0x18,0x8C,0x40,
0x70,0x1C,0x0C,0x30,0x70,0x08,0x82,0x30,0x20,0x04,0x88,0x40,
0x78,0x3C,0x0C,0x30,0xD8,0x08,0x85,0x48,0x20,0x04,0x80,0x40,
0xD8,0x36,0x0C,0x30,0xD8,0x08,0x85,0x48,0x20,0x02,0x80,0x40,
0xD8,0x36,0xFC,0x30,0x8C,0x91,0x48,0xCC,0x20,0x02,0x80,0x7F,
0xDC,0x76,0xFC,0x30,0x8C,0x91,0x48,0x84,0x20,0x02,0x80,0x40,
0x8C,0x63,0x0C,0x30,0xFC,0x91,0x48,0x84,0x20,0x02,0x80,0x40,
0x8C,0x63,0x0C,0x30,0xFE,0xA3,0x28,0xFE,0x21,0x04,0x80,0x40,
0x86,0xC3,0x0C,0x30,0x06,0xA3,0x28,0x02,0x21,0x04,0x88,0x40,
0x06,0xC1,0xFC,0x30,0x03,0x46,0x10,0x01,0x22,0x18,0x8C,0x40,
0x06,0xC1,0xFC,0x30,0x03,0x46,0x10,0x01,0x22,0xE0,0x83,0x40,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



const unsigned char Am[10*4] =
{
0x00,0x00,0x9C,0xA2,0xA2,0xA2,0xBE,0xA2,0xA2,0x00,
0x00,0x00,0x08,0x0D,0x0A,0x08,0x08,0x08,0x08,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const unsigned char Pm[10*4] = 
{
0x00,0x00,0x9E,0xA2,0xA2,0x9E,0x82,0x82,0x82,0x00,
0x00,0x00,0x08,0x0D,0x0A,0x08,0x08,0x08,0x08,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const unsigned char DaysOfWeek[7][10*4] =
{
0x00,0x00,0x9C,0xA2,0x82,0x9C,0xA0,0xA2,0x1C,0x00,
0x00,0x00,0x28,0x68,0xA8,0x28,0x28,0x28,0x27,0x00,
0x00,0x00,0x02,0x02,0x02,0x03,0x02,0x02,0x02,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x22,0xB6,0xAA,0xA2,0xA2,0xA2,0x22,0x00,
0x00,0x00,0x27,0x68,0xA8,0x28,0x28,0x28,0x27,0x00,
0x00,0x00,0x02,0x02,0x02,0x03,0x02,0x02,0x02,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xBE,0x88,0x88,0x88,0x88,0x88,0x08,0x00,
0x00,0x00,0xE8,0x28,0x28,0xE8,0x28,0x28,0xE7,0x00,
0x00,0x00,0x03,0x00,0x00,0x01,0x00,0x00,0x03,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xA2,0xA2,0xAA,0xAA,0xAA,0xAA,0x94,0x00,
0x00,0x00,0xEF,0x20,0x20,0x27,0x20,0x20,0xEF,0x00,
0x00,0x00,0x01,0x02,0x02,0x02,0x02,0x02,0x01,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xBE,0x88,0x88,0x88,0x88,0x88,0x88,0x00,
0x00,0x00,0x28,0x28,0x28,0x2F,0x28,0x28,0xC8,0x00,
0x00,0x00,0x7A,0x8A,0x8A,0x7A,0x4A,0x8A,0x89,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xBE,0x82,0x82,0x9E,0x82,0x82,0x82,0x00,
0x00,0x00,0xC7,0x88,0x88,0x87,0x84,0x88,0xC8,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x1C,0xA2,0x82,0x9C,0xA0,0xA2,0x9C,0x00,
0x00,0x00,0xE7,0x88,0x88,0x88,0x8F,0x88,0x88,0x00,
0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


static void DontChangeButtonConfiguration(void)
{
  /* assign LED button to all modes */
  unsigned char i;
  for ( i = 0; i < NUMBER_OF_BUTTON_MODES; i++ )
  {
    /* turn off led 3 seconds after button has been released */
    EnableButtonAction(i,
                       SW_D_INDEX,
                       BUTTON_STATE_PRESSED,
                       LedChange,
                       LED_START_OFF_TIMER);
    
    /* turn on led immediately when button is pressed */
    EnableButtonAction(i,
                       SW_D_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       LedChange,
                       LED_ON_OPTION);
    
    /* software reset is available in all modes */    
    EnableButtonAction(i,
                       SW_F_INDEX,
                       BUTTON_STATE_LONG_HOLD,
                       SoftwareResetMsg,
                       MASTER_RESET_OPTION);
  
  }

}

static void SetupNormalIdleScreenButtons(void)
{
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_F_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     WatchStatusMsg,
                     RESET_DISPLAY_TIMER);
  
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_E_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     ListPairedDevicesMsg,
                     NO_MSG_OPTIONS);
   
  /* led is already assigned */
  
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_C_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     MenuModeMsg,
                     MENU_MODE_OPTION_PAGE1);
  
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_B_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     ToggleSecondsMsg,
                     TOGGLE_SECONDS_OPTIONS_UPDATE_IDLE);
  
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_A_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     BarCode,
                     RESET_DISPLAY_TIMER);  
}

static void ConfigureIdleUserInterfaceButtons(void)
{
  if ( CurrentIdlePage != LastIdlePage )
  {
    LastIdlePage = CurrentIdlePage;
  
    /* only allow reset on one of the pages */
    DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                        SW_F_INDEX,
                        BUTTON_STATE_PRESSED);
    
    switch ( CurrentIdlePage )
    {
    case NormalPage:
      /* do nothing */
      break;
      
    case RadioOnWithPairingInfoPage:
          
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         WatchStatusMsg,
                         RESET_DISPLAY_TIMER);
    
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ListPairedDevicesMsg,
                         NO_MSG_OPTIONS);
       
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE1);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ToggleSecondsMsg,
                         TOGGLE_SECONDS_OPTIONS_UPDATE_IDLE);
   
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         BarCode,
                         RESET_DISPLAY_TIMER);
      
      break;
      
    case BluetoothOffPage:
    case RadioOnWithoutPairingInfoPage:
   
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ModifyTimeMsg,
                         MODIFY_TIME_INCREMENT_HOUR);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ListPairedDevicesMsg,
                         NO_MSG_OPTIONS);
      
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE1);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ModifyTimeMsg,
                         MODIFY_TIME_INCREMENT_DOW);
   
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ModifyTimeMsg,
                         MODIFY_TIME_INCREMENT_MINUTE);
      break;
      
      
      
    case Menu1Page:
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM);
      
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_EXIT);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE2);
   
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH);
      
      break;
      
    case Menu2Page:
      
      /* this cannot be immediate because Master Reset is on this button also */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_PRESSED,
                         SoftwareResetMsg,
                         NO_MSG_OPTIONS);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_TOGGLE_SECURE_SIMPLE_PAIRING);
      
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_EXIT);
            
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE3);
            
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN);
      
      break;
    
      
    case Menu3Page:
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_INVERT_DISPLAY);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_DISPLAY_SECONDS);
      
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_EXIT);
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE1);
            
#if 0
      /* shipping mode is disabled for now */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         EnterShippingModeMsg,
                         NO_MSG_OPTIONS);
#else
      DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                          SW_A_INDEX,
                          BUTTON_STATE_IMMEDIATE);
#endif      
      break;
      
    case ListPairedDevicesPage:
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         WatchStatusMsg,
                         RESET_DISPLAY_TIMER);
    
      /* map this mode's entry button to go back to the idle mode */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         IdleUpdate,
                         NO_MSG_OPTIONS);
       
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE1);
      
      DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                          SW_B_INDEX,
                          BUTTON_STATE_IMMEDIATE);
                          
      /* map this mode's entry button to go back to the idle mode */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         BarCode,
                         RESET_DISPLAY_TIMER);
      break;

    case WatchStatusPage:
      
      /* map this mode's entry button to go back to the idle mode */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         IdleUpdate,
                         RESET_DISPLAY_TIMER);
    
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ListPairedDevicesMsg,
                         NO_MSG_OPTIONS);
       
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE1);
      
      DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                          SW_B_INDEX,
                          BUTTON_STATE_IMMEDIATE);
                          
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         BarCode,
                         RESET_DISPLAY_TIMER);
      break;
  
    case QrCodePage:
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_F_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         WatchStatusMsg,
                         RESET_DISPLAY_TIMER);
    
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_E_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         ListPairedDevicesMsg,
                         NO_MSG_OPTIONS);
       
      /* led is already assigned */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         MENU_MODE_OPTION_PAGE1);
      
      DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                          SW_B_INDEX,
                          BUTTON_STATE_IMMEDIATE);
                          
      /* map this mode's entry button to go back to the idle mode */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         IdleUpdate,
                         RESET_DISPLAY_TIMER);
      
      break;
      
      
    }
  }
}

/* the default is for all simple button presses to be sent to the phone */
static void DefaultApplicationAndNotificationButtonConfiguration(void)
{  
  unsigned char index = 0;
  
  /* 
   * this will configure the pull switch even though it does not exist 
   * on the watch
   */
  for(index = 0; index < NUMBER_OF_BUTTONS; index++)
  {
    if ( index == SW_UNUSED_INDEX )
    {
      index++;
    }
   
    EnableButtonAction(APPLICATION_SCREEN_BUTTON_MODE,
                       index,
                       BUTTON_STATE_PRESSED,
                       ButtonEventMsg,
                       NO_MSG_OPTIONS);

    EnableButtonAction(NOTIFICATION_BUTTON_MODE,
                       index,
                       BUTTON_STATE_PRESSED,
                       ButtonEventMsg,
                       NO_MSG_OPTIONS);
    
    EnableButtonAction(SCROLL_BUTTON_MODE,
                       index,
                       BUTTON_STATE_PRESSED,
                       ButtonEventMsg,
                       NO_MSG_OPTIONS);
  
  }

}


/******************************************************************************/

void InitializeIdleBufferConfig(void)
{
  nvIdleBufferConfig = WATCH_CONTROLS_TOP;
  OsalNvItemInit(NVID_IDLE_BUFFER_CONFIGURATION, 
                 sizeof(nvIdleBufferConfig), 
                 &nvIdleBufferConfig);    
}

void InitializeIdleBufferInvert(void)
{
  nvIdleBufferInvert = 0;
  OsalNvItemInit(NVID_IDLE_BUFFER_INVERT, 
                 sizeof(nvIdleBufferInvert), 
                 &nvIdleBufferInvert);   
}

void InitializeDisplaySeconds(void)
{
  nvDisplaySeconds = 0;
  OsalNvItemInit(NVID_DISPLAY_SECONDS, 
                 sizeof(nvDisplaySeconds), 
                 &nvDisplaySeconds);
}

#if 0
static void SaveIdleBufferConfig(void)
{
  osal_nv_write(NVID_IDLE_BUFFER_CONFIGURATION, 
                NV_ZERO_OFFSET,
                sizeof(nvIdleBufferConfig), 
                &nvIdleBufferConfig);    
}
#endif

static void SaveIdleBufferInvert(void)
{
  OsalNvWrite(NVID_IDLE_BUFFER_INVERT,
              NV_ZERO_OFFSET,
              sizeof(nvIdleBufferInvert), 
              &nvIdleBufferInvert);   
}

static void SaveDisplaySeconds(void)
{
  OsalNvWrite(NVID_DISPLAY_SECONDS,
              NV_ZERO_OFFSET,
              sizeof(nvDisplaySeconds),
              &nvDisplaySeconds);
}

unsigned char QueryDisplaySeconds(void)
{
  return nvDisplaySeconds;
}

unsigned char QueryInvertDisplay(void)
{
  return nvIdleBufferInvert; 
}

static unsigned int CharacterMask;
static unsigned char CharacterRows;
static unsigned char CharacterWidth;
static unsigned int bitmap[MAX_FONT_ROWS];

/* fonts can be up to 16 bits wide */
static void WriteFontCharacter(unsigned char Character)
{
  CharacterMask = BIT0;
  CharacterRows = GetCharacterHeight();
  CharacterWidth = GetCharacterWidth(Character);
  GetCharacterBitmap(Character,(unsigned int*)&bitmap);
  
  if ( gRow + CharacterRows > NUM_LCD_ROWS )
  {
    PrintString("Not enough rows to display character\r\n");
    return;  
  }
  
  /* do things bit by bit */
  unsigned char i;
  unsigned char row;
 
  for (i = 0 ; i < CharacterWidth && gColumn < NUM_LCD_COL_BYTES; i++ )
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

void WriteFontString(tString *pString)
{
  unsigned char i = 0;
   
  while (pString[i] != 0 && gColumn < NUM_LCD_COL_BYTES)
  {
    WriteFontCharacter(pString[i++]);    
  }

}



unsigned char QueryIdlePageNormal(void)
{
  if ( CurrentIdlePage == NormalPage )
  {
    return 1;
  }
  else
  {
    return 0;  
  };
  
}

unsigned char LcdRtcUpdateHandlerIsr(void)
{
  unsigned char ExitLpm = 0;
  
  unsigned int RtcSeconds = RTCSEC;
  
  if ( RtcUpdateEnable )
  {
    /* send a message every second or once a minute */
    if (   QueryDisplaySeconds()
        || RtcSeconds == 0 )
    {
      tMessage Msg;
      SetupMessage(&Msg,IdleUpdate,NO_MSG_OPTIONS);
      SendMessageToQueueFromIsr(DISPLAY_QINDEX,&Msg);
      ExitLpm = 1;  
    }    
  } 

  return ExitLpm;
  
}
