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
#include "BufferPool.h"         

#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_battery.h"
#include "hal_lcd.h"
#include "hal_lpm.h"
       
#include "DebugUart.h"      
#include "Messages.h"
#include "Utilities.h"
#include "LcdTask.h"
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

static void DisplayQueueMessageHandler(tHostMsg* pMsg);
static void SendMyBufferToLcd(unsigned char TotalRows);

static tHostMsg* pDisplayMsg;

static tTimerId IdleModeTimerId;
static tTimerId ApplicationModeTimerId;
static tTimerId NotificationModeTimerId;

/* Message handlers */

static void IdleUpdateHandler(void);
static void ChangeModeHandler(tHostMsg* pMsg);
static void ModeTimeoutHandler(tHostMsg* pMsg);
static void WatchStatusScreenHandler(void);
static void BarCodeHandler(tHostMsg* pMsg);
static void ListPairedDevicesHandler(void);
static void ConfigureIdleBuferSizeHandler(tHostMsg* pMsg);
static void ModifyTimeHandler(tHostMsg* pMsg);
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
static void StopAllDisplayTimers(void);
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

static void AddDecimalPoint8w10h(unsigned char RowOffset,
                                 unsigned char ColumnOffset);

static unsigned char const* GetSpritePointerForChar(unsigned char CharIn);
static unsigned char const* GetSpritePointerForDigit(unsigned char Digit);
static unsigned char const* GetPointerForTimeDigit(unsigned char Digit,
                                                   unsigned char Offset);

unsigned char WriteString(unsigned char* pString,
                          unsigned char RowOffset,
                          unsigned char ColumnOffset,
                          unsigned char AddSpace);

#define ADD_SPACE_AT_END      ( 1 )
#define DONT_ADD_SPACE_AT_END ( 0 )

static void WriteSpriteChar(unsigned char Char,
                            unsigned char RowOffset,
                            unsigned char ColumnOffset);

static void WriteSpriteDigit(unsigned char Digit,
                             unsigned char RowOffset,
                             unsigned char ColumnOffset,
                             signed char ShiftAmount);


static void WriteTimeDigit(unsigned char Digit,
                           unsigned char RowOffset,
                           unsigned char ColumnOffset,
                           unsigned char JustificationOffset);

static void WriteTimeColon(unsigned char RowOffset,
                           unsigned char ColumnOffset,
                           unsigned char Justification);

static void WriteFoo(unsigned char const * pFoo,
                     unsigned char RowOffset,
                     unsigned char ColumnOffset);

static void DisplayAmPm(void);
static void DisplayDayOfWeek(void);
static void DisplayDate(void);

static void DisplayDataSeparator(unsigned char RowOffset,
                                 unsigned char ColumnOffset);

/* the internal buffer */
#define STARTING_ROW                  ( 0 )
#define WATCH_DRAWN_IDLE_BUFFER_ROWS  ( 30 )
#define PHONE_IDLE_BUFFER_ROWS        ( 66 )

static tLcdLine pMyBuffer[NUM_LCD_ROWS];

/******************************************************************************/

static unsigned char nvIdleBufferConfig;
static unsigned char nvIdleBufferInvert;

static void InitialiazeIdleBufferConfig(void);
static void InitializeIdleBufferInvert(void);

static void SaveIdleBufferInvert(void);

/******************************************************************************/

unsigned char nvDisplaySeconds = 0;
static void InitializeDisplaySeconds(void);
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
  QrCodePage,
  
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
const unsigned char Alphabet8w10h[96][10];
const unsigned char pTimeDigit[10*2][19*2];
const unsigned char pTimeColonR[1][19*1];
const unsigned char pTimeColonL[1][19*1];
const unsigned char Am[10*4];
const unsigned char Pm[10*4];
const unsigned char DaysOfWeek[7][10*4];

#define TIME_DIGIT_HEIGHT ( 19 )
/* actual width is 11 but they are stored in 16 bit container */
#define TIME_DIGIT_WIDTH       ( 16 )
#define LEFT_JUSTIFIED         ( 0 )
#define RIGHT_JUSTIFIED        ( 10 )

/******************************************************************************/

static unsigned char LastMode = IDLE_MODE;
static unsigned char CurrentMode = IDLE_MODE;

//static unsigned char ReturnToApplicationMode;

/******************************************************************************/

static unsigned char pBluetoothAddress[12+1];
static unsigned char pBluetoothName[12+1];

/******************************************************************************/

#ifdef FONT_TESTING
static unsigned char gBitColumnMask;
static unsigned char gColumn;
static unsigned char gRow;

static void WriteFontCharacter(unsigned char Character);
static void WriteFontString(unsigned char* pString);
#endif

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
              "DISPLAY", 
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
  
  DisplayStartupScreen();
  
  InitialiazeIdleBufferConfig();
  InitializeIdleBufferInvert();
  InitializeDisplaySeconds();
  InitializeLinkAlarmEnable();
  InitializeTimeFormat();
  InitializeDateFormat();
  AllocateDisplayTimers();
  SetupSplashScreenTimeout();

  DontChangeButtonConfiguration();
  DefaultApplicationAndNotificationButtonConfiguration();
  SetupNormalIdleScreenButtons();
  
  for(;;)
  {
    if( pdTRUE == xQueueReceive(QueueHandles[DISPLAY_QINDEX], 
                                &pDisplayMsg, portMAX_DELAY) )
    {
      DisplayQueueMessageHandler(pDisplayMsg);
      
      BPL_FreeMessageBuffer(&pDisplayMsg);
      
      CheckStackUsage(DisplayHandle,"Display");
    }
  }
}
  
/*! Display the startup image or Splash Screen */
static void DisplayStartupScreen(void)
{
  CopyRowsIntoMyBuffer(pMetaWatchSplash,STARTING_ROW,NUM_LCD_ROWS);
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);
}

/*! Handle the messages routed to the display queue */
static void DisplayQueueMessageHandler(tHostMsg* pMsg)
{
  unsigned char Type = pMsg->Type;
      
  switch(Type)
  {
  
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
    
  case ConfigureMode:
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
    
  case LinkAlarmMsg:
    if ( QueryLinkAlarmEnable() )
    {
      GenerateLinkAlarm();  
    }
    break;
  default:
    PrintStringAndHex("<<Unhandled Message>> in Lcd Display Task: Type 0x", Type);
    break;
  }

}

/*! Allocate ids and setup timers for the display modes */
static void AllocateDisplayTimers(void)
{
  IdleModeTimerId = AllocateOneSecondTimer();

  ApplicationModeTimerId = AllocateOneSecondTimer();

  NotificationModeTimerId = AllocateOneSecondTimer();

}

static void SetupSplashScreenTimeout(void)
{
  SetupOneSecondTimer(IdleModeTimerId,
                      ONE_SECOND*3,
                      NO_REPEAT,
                      SplashTimeoutMsg,
                      NO_MSG_OPTIONS);

  StartOneSecondTimer(IdleModeTimerId);
  
  AllowConnectionStateChangeToUpdateScreen = 0;
  
}

static void StopAllDisplayTimers(void)
{
  StopOneSecondTimer(IdleModeTimerId);
  StopOneSecondTimer(ApplicationModeTimerId);
  StopOneSecondTimer(NotificationModeTimerId);
   
}

/*! Draw the Idle screen and cause the remainder of the display to be updated
 * also
 */
static void IdleUpdateHandler(void)
{
  StopAllDisplayTimers();
  
  /* select between 1 second and 1 minute */
  int IdleUpdateTime;
  if ( nvDisplaySeconds )
  {
    IdleUpdateTime = ONE_SECOND;
  }
  else
  {
    IdleUpdateTime = 60 - GetRTCSEC();
  }
  
  /* setup a timer to determine when to draw the screen again */
  SetupOneSecondTimer(IdleModeTimerId,
                      IdleUpdateTime,
                      REPEAT_FOREVER,
                      IdleUpdate,
                      NO_MSG_OPTIONS);
  
  StartOneSecondTimer(IdleModeTimerId);

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
    tHostMsg* pOutgoingMsg;
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    pOutgoingMsg->Type = UpdateDisplay;
    pOutgoingMsg->Options = IDLE_MODE | DONT_ACTIVATE_DRAW_BUFFER;
    RouteMsg(&pOutgoingMsg);
   
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

static void ChangeModeHandler(tHostMsg* pMsg)
{
  LastMode = CurrentMode;    
  CurrentMode = (pMsg->Options & MODE_MASK);
  
  switch ( CurrentMode )
  {
  
  case IDLE_MODE:

    /* this check is so that the watch apps don't mess up the timer */
    if ( LastMode != CurrentMode )
    {
      /* idle update handler will stop all display clocks */
      IdleUpdateHandler();
      PrintString("Changing mode to Idle\r\n");
    }
    else
    {
      PrintString("Already in Idle mode\r\n");
    }
    break;
  
  case APPLICATION_MODE:
    
    StopAllDisplayTimers();
    
    SetupOneSecondTimer(ApplicationModeTimerId,
                        QueryApplicationModeTimeout(),
                        NO_REPEAT,
                        ModeTimeoutMsg,
                        APPLICATION_MODE);
    
    /* don't start the timer if the timeout == 0 
     * this invites things that look like lock ups...
     * it is preferred to make this a large value
     */
    if ( QueryApplicationModeTimeout() )
    {
      StartOneSecondTimer(ApplicationModeTimerId);
    }
    
    PrintString("Changing mode to Application\r\n");
    break;
  
  case NOTIFICATION_MODE:
    StopAllDisplayTimers();
    
    SetupOneSecondTimer(NotificationModeTimerId,
                        QueryNotificationModeTimeout(),
                        NO_REPEAT,
                        ModeTimeoutMsg,
                        NOTIFICATION_MODE);
        
    if ( QueryNotificationModeTimeout() )
    {
      StartOneSecondTimer(NotificationModeTimerId);
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
    tHostMsg* pOutgoingMsg;
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    unsigned char data = (unsigned char)eScUpdateComplete;
    UTL_BuildHstMsg(pOutgoingMsg, StatusChangeEvent, IDLE_MODE, &data, sizeof(data));
    RouteMsg(&pOutgoingMsg);
    
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

static void ModeTimeoutHandler(tHostMsg* pMsg)
{
  tHostMsg* pOutgoingMsg;
  
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
  
  /* send a message to the host */
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  unsigned char data = (unsigned char)eScModeTimeout;
  UTL_BuildHstMsg(pOutgoingMsg, 
                  StatusChangeEvent, 
                  NO_MSG_OPTIONS, 
                  &data, sizeof(data));
  
  RouteMsg(&pOutgoingMsg);
  
}
  

static void WatchStatusScreenHandler(void)
{
  StopAllDisplayTimers();
  
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
      
  unsigned char row = 27;
  unsigned char col = 8;
  unsigned char msd = 0;

  msd = bV / 1000;
  bV = bV % 1000;
  WriteSpriteDigit(msd,row,col++,0);
  
  msd = bV / 100;
  bV = bV % 100;
  WriteSpriteDigit(msd,row,col++,0);
  AddDecimalPoint8w10h(row,col-2);
  
  msd = bV / 10;
  bV = bV % 10;
  WriteSpriteDigit(msd,row,col++,0); 
  
  WriteSpriteDigit(bV,row,col++,0);
   
  /*
   * Add Wavy line
   */
  row += 12;
  CopyRowsIntoMyBuffer(pWavyLine,row,NUMBER_OF_ROWS_IN_WAVY_LINE);
  
  
  /*
   * Add details
   */

  /* add MAC address */
  row += NUMBER_OF_ROWS_IN_WAVY_LINE+2;
  col = 0;
  WriteString(GetLocalBluetoothAddressString(),row,col,DONT_ADD_SPACE_AT_END);

  /* add the firmware version */
  row += 12;
  col = 0;
  col = WriteString("App",row,col,ADD_SPACE_AT_END);
  col = WriteString(VERSION_STRING,row,col,ADD_SPACE_AT_END);

  /* stack version */
  row += 12;
  col = 0;
  col = WriteString("Stack",row,col,ADD_SPACE_AT_END);
  col = WriteString(GetStackVersion(),row,col,ADD_SPACE_AT_END);

  /* add msp430 revision */
  row +=12;
  col = 0;
  col = WriteString("MSP430 Rev",row,col,DONT_ADD_SPACE_AT_END);
  WriteSpriteChar(GetHardwareRevision(),row,col++);
 
  /* display entire buffer */
  PrepareMyBufferForLcd(STARTING_ROW,NUM_LCD_ROWS);
  SendMyBufferToLcd(NUM_LCD_ROWS);

  CurrentIdlePage = WatchStatusPage;
  ConfigureIdleUserInterfaceButtons();
  
  /* refresh the status page once a minute */  
  SetupOneSecondTimer(IdleModeTimerId,
                      ONE_SECOND*60,
                      NO_REPEAT,
                      WatchStatusMsg,
                      NO_MSG_OPTIONS);
  
  StartOneSecondTimer(IdleModeTimerId);
  
  
  
}


/* the bar code should remain displayed until the button is pressed again
 * or another mode is started */

static void BarCodeHandler(tHostMsg* pMsg)
{
  StopAllDisplayTimers();
    
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
  StopAllDisplayTimers();
  
  unsigned char row = 0;
  unsigned char col = 0;
  
  /* draw entire region */
  FillMyBuffer(STARTING_ROW,NUM_LCD_ROWS,0x00);
  
  for(unsigned char i = 0; i < 3; i++)
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
    
    WriteString(pBluetoothName,row,col,DONT_ADD_SPACE_AT_END);
    row += 12;
    
    WriteString(pBluetoothAddress,row,col,DONT_ADD_SPACE_AT_END);
    row += 12+5;
      
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
    
#ifdef FONT_TESTING
  
  gRow = 65;
  gColumn = 0;
  gBitColumnMask = BIT0;
  
  SetFont(MetaWatch5);
  WriteFontString("Peanut Butter");
  
  gRow = 72;
  gColumn = 0;
  gBitColumnMask = BIT0;
  
  SetFont(MetaWatch7);
  //WriteFontString("ABCDEFGHIJKLMNOP");
  WriteFontString("Peanut Butter W");
  
  gRow = 80;
  gColumn = 0;
  gBitColumnMask = BIT0;
  SetFont(MetaWatch16);
  WriteFontString("ABC pqr StuVw");

#else
  
  unsigned char row;
  unsigned char col;

  /* characters are 10h then add space of 2 lines */
  row = 65;
  col = 0;
  col = WriteString(GetLocalBluetoothAddressString(),row,col,DONT_ADD_SPACE_AT_END);

  /* add the firmware version */
  row = 75;
  col = 0;
  col = WriteString("App",row,col,ADD_SPACE_AT_END);
  col = WriteString(VERSION_STRING,row,col,ADD_SPACE_AT_END);
  
  /* and the stack version */
  row = 85;
  col = 0;
  col = WriteString("Stack",row,col,ADD_SPACE_AT_END);
  col = WriteString(GetStackVersion(),row,col,ADD_SPACE_AT_END);

#endif
  
}

static void ConfigureIdleBuferSizeHandler(tHostMsg* pMsg)
{
  nvIdleBufferConfig = pMsg->pPayload[0] & IDLE_BUFFER_CONFIG_MASK;
  
  if ( nvIdleBufferConfig == WATCH_CONTROLS_TOP )
  {
    IdleUpdateHandler();
  }
  
}

static void ModifyTimeHandler(tHostMsg* pMsg)
{
  int time;
  switch (pMsg->Options)
  {
  case MODIFY_TIME_INCREMENT_HOUR:
    /*! todo - make these functions */
    time = GetRTCHOUR();
    time++; if ( time == 24 ) time = 0;
    SetRTCHOUR(time);
    break;
  case MODIFY_TIME_INCREMENT_MINUTE:
    time = GetRTCMIN();
    time++; if ( time == 60 ) time = 0;
    SetRTCMIN(time);
    break;
  case MODIFY_TIME_INCREMENT_DOW:
    /* modify the day of the week (not the day of the month) */
    time = GetRTCDOW();
    time++; if ( time == 7 ) time = 0;
    SetRTCDOW(time);
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
  tHostMsg* pOutgoingMsg;
  
#if 0
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  ((tUpdateMyDisplayMsg*)pOutgoingMsg)->Type = ClearLcdSpecial;
  RouteMsg(&pOutgoingMsg);
#endif
  
  
  /* 
   * since my buffer is in MSP430 memory it can go directly to the Lcd BUT
   * to preserve draw order it goes to the sram queue first
   */
  BPL_AllocMessageBuffer(&pOutgoingMsg);
  ((tUpdateMyDisplayMsg*)pOutgoingMsg)->Type = UpdateMyDisplaySram;
  ((tUpdateMyDisplayMsg*)pOutgoingMsg)->TotalLines = TotalRows;
  ((tUpdateMyDisplayMsg*)pOutgoingMsg)->pMyDisplay = (unsigned char*)pMyBuffer;
  RouteMsg(&pOutgoingMsg);
}


static void InitMyBuffer(void)
{
  int row;
  int col;
  
  // Clear the display buffer.  Step through the rows
  for(row = STARTING_ROW; 
      ((row < NUM_LCD_ROWS) && (row < NUM_LCD_ROWS)); row++)
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


/* sprites are in the form row,row,row */
static void WriteSpriteChar(unsigned char Char,
                            unsigned char RowOffset,
                            unsigned char ColumnOffset)
{
  
  /* copy Char into correct position */
  unsigned char const * pChar = GetSpritePointerForChar(Char);
  unsigned char RowNumber;
  unsigned char left = 0;
  
  for ( RowNumber = 0; RowNumber < 10; RowNumber++ )
  {
    pMyBuffer[RowNumber+RowOffset].Data[left+ColumnOffset] = pChar[RowNumber];
  }
  
}

/* sprites are in the form row,row,row */

/* shift only works for blank pixels 
 * a real shift will require an OR operation
 */
static void WriteSpriteDigit(unsigned char Digit,
                             unsigned char RowOffset,
                             unsigned char ColumnOffset,
                             signed char ShiftAmount)
{
  
  /* copy digit into correct position */
  unsigned char const * pDigit = GetSpritePointerForDigit(Digit);
  unsigned char RowNumber;
  unsigned char left = 0;
  unsigned char BitMap;
  
  for ( RowNumber = 0; RowNumber < 10; RowNumber++ )
  {
    if ( ShiftAmount >= 0 )
    {
      BitMap = (pDigit[RowNumber] << ShiftAmount);
    }
    else
    {
      BitMap = (pDigit[RowNumber] >> (ShiftAmount*-1));
    }
    
    pMyBuffer[RowNumber+RowOffset].Data[left+ColumnOffset] = BitMap;
  }
  
}


/* 
 * the system font for the digits used for time are 19 high
 */
static void WriteTimeDigit(unsigned char Digit,
                           unsigned char RowOffset,
                           unsigned char ColumnOffset,
                           unsigned char JustificationOffset)
{
  
  /* copy digit into correct position */
  unsigned char const * pDigit = GetPointerForTimeDigit(Digit,JustificationOffset);
  unsigned char RowNumber;
  unsigned char Column;
  
  for ( Column = 0; Column < 2; Column++ )
  {
    /* RowNumber is the row in the pDigit */
    for ( RowNumber = 0; RowNumber < TIME_DIGIT_HEIGHT; RowNumber++ )
    {
      /* data is ORED in because the 11 bit wide letters are right or left
       * justified in 16 bit containers */
      pMyBuffer[RowNumber+RowOffset].Data[Column+ColumnOffset] |= 
        pDigit[RowNumber+(Column*TIME_DIGIT_HEIGHT)];
    }
  }
  
}

static void WriteTimeColon(unsigned char RowOffset,
                           unsigned char ColumnOffset,
                           unsigned char Justification)
{
  
  /* copy digit into correct position */
  unsigned char const * pDigit;
  if ( Justification == LEFT_JUSTIFIED )
  {
    pDigit = pTimeColonL[0];
  }
  else
  {
    pDigit = pTimeColonR[0];
  }
  
  unsigned char RowNumber;
  unsigned char Column = 0;
  
  /* colon has only one column of non-zero data*/
  for ( RowNumber = 0; RowNumber < TIME_DIGIT_HEIGHT; RowNumber++ )
  {
    /* data is ORED in because the 11 bit wide letters are right or left
     * justified in 16 bit containers */
    pMyBuffer[RowNumber+RowOffset].Data[Column+ColumnOffset] |= 
      pDigit[RowNumber+(Column*TIME_DIGIT_HEIGHT)];
  }
  
}

static void DrawIdleScreen(void)
{
  unsigned char msd;
  unsigned char lsd;
    
  unsigned char Row = 6;
  unsigned char Col = 0;
  
  /* display hour */
  int Hour = GetRTCHOUR();
  
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

  /* if first digit is zero then leave location blank */
  if ( msd != 0 )
  {  
    WriteTimeDigit(msd,Row,Col,LEFT_JUSTIFIED);
  }
  Col += 1;
  WriteTimeDigit(lsd,Row,Col,RIGHT_JUSTIFIED);
  Col += 2;
  
  /* the colon takes the first 5 bits on the byte*/
  WriteTimeColon(Row,Col,RIGHT_JUSTIFIED);
  Col+=1;
  
  /* display minutes */
  int Minutes = GetRTCMIN();
  msd = Minutes / 10;
  lsd = Minutes % 10;
  WriteTimeDigit(msd,Row,Col,RIGHT_JUSTIFIED);
  Col += 2;
  WriteTimeDigit(lsd,Row,Col,LEFT_JUSTIFIED);
    
  if ( nvDisplaySeconds )
  {
    /* the final colon's spacing isn't quite the same */
    int Seconds = GetRTCSEC();
    msd = Seconds / 10;
    lsd = Seconds % 10;    
    
    Col +=2;
    WriteTimeColon(Row,Col,LEFT_JUSTIFIED);
    Col += 1;
    WriteTimeDigit(msd,Row,Col,LEFT_JUSTIFIED);
    Col += 1;
    WriteTimeDigit(lsd,Row,Col,RIGHT_JUSTIFIED);    
    
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
    
  unsigned char Row = 6;
  unsigned char Col = 0;
  
  /* display hour */
  int Hour = GetRTCHOUR();
  
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

  /* if first digit is zero then leave location blank */
  if ( msd != 0 )
  {  
    WriteTimeDigit(msd,Row,Col,LEFT_JUSTIFIED);
  }
  Col += 1;
  WriteTimeDigit(lsd,Row,Col,RIGHT_JUSTIFIED);
  Col += 2;
  
  /* the colon takes the first 5 bits on the byte*/
  WriteTimeColon(Row,Col,RIGHT_JUSTIFIED);
  Col+=1;
  
  /* display minutes */
  int Minutes = GetRTCMIN();
  msd = Minutes / 10;
  lsd = Minutes % 10;
  WriteTimeDigit(msd,Row,Col,RIGHT_JUSTIFIED);
  Col += 2;
  WriteTimeDigit(lsd,Row,Col,LEFT_JUSTIFIED);
    
  if ( nvDisplaySeconds )
  {
    /* the final colon's spacing isn't quite the same */
    int Seconds = GetRTCSEC();
    msd = Seconds / 10;
    lsd = Seconds % 10;    
    
    Col +=2;
    WriteTimeColon(Row,Col,LEFT_JUSTIFIED);
    Col += 1;
    WriteTimeDigit(msd,Row,Col,LEFT_JUSTIFIED);
    Col += 1;
    WriteTimeDigit(lsd,Row,Col,RIGHT_JUSTIFIED);    
    
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
  StopAllDisplayTimers();
  
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
  StopAllDisplayTimers();

  tHostMsg* pOutgoingMsg;
  
  switch (MsgOptions)
  {
  case MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY:
    
    if ( QueryConnectionState() != Initializing )
    {
      BPL_AllocMessageBuffer(&pOutgoingMsg);
      pOutgoingMsg->Type = PariringControlMsg;
        
      if ( QueryDiscoverable() )
      {
        pOutgoingMsg->Options = PAIRING_CONTROL_OPTION_DISABLE_PAIRING;
      }
      else
      {
        pOutgoingMsg->Options = PAIRING_CONTROL_OPTION_ENABLE_PAIRING;  
      }
      
      RouteMsg(&pOutgoingMsg);
    }
    /* screen will be updated with a message from spp */
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM:
    ToggleLinkAlarmEnable();
    MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;
  
  case MENU_BUTTON_OPTION_EXIT:          
    
    /* save all of the non-volatile items */
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    pOutgoingMsg->Type = PariringControlMsg;
    pOutgoingMsg->Options = PAIRING_CONTROL_OPTION_SAVE_SPP;
    RouteMsg(&pOutgoingMsg);
     
    SaveLinkAlarmEnable();
    SaveRstNmiConfiguration();
    SaveIdleBufferInvert();
    SaveDisplaySeconds();
    
    /* go back to the normal idle screen */
    BPL_AllocMessageBuffer(&pOutgoingMsg);
    pOutgoingMsg->Type = IdleUpdate;
    RouteMsg(&pOutgoingMsg);
    
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH:
    
    if ( QueryConnectionState() != Initializing )
    {
      BPL_AllocMessageBuffer(&pOutgoingMsg);
        
      if ( QueryBluetoothOn() )
      {
        pOutgoingMsg->Type = TurnRadioOffMsg;
      }
      else
      {
        pOutgoingMsg->Type = TurnRadioOnMsg;
      }
      
      RouteMsg(&pOutgoingMsg);
    }
    /* screen will be updated with a message from spp */
    break;
    
  case MENU_BUTTON_OPTION_TOGGLE_SECURE_SIMPLE_PAIRING:
    if ( QueryConnectionState() != Initializing )
    {
      BPL_AllocMessageBuffer(&pOutgoingMsg);
      pOutgoingMsg->Type = PariringControlMsg;
      pOutgoingMsg->Options = PAIRING_CONTROL_OPTION_TOGGLE_SSP;
      RouteMsg(&pOutgoingMsg);
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

static void AddDecimalPoint8w10h(unsigned char RowOffset,
                                 unsigned char ColumnOffset)
{
  /* right most pixel is most significant bit */
  pMyBuffer[RowOffset+8].Data[ColumnOffset+1] |= 0x01;
  
}

static unsigned char const* GetSpritePointerForChar(unsigned char CharIn)
{
  return (Alphabet8w10h[MapCharacterToIndex(CharIn)]);
}

static unsigned char const* GetSpritePointerForDigit(unsigned char Digit)
{
  return (Alphabet8w10h[MapDigitToIndex(Digit)]);
}

static unsigned char const* GetPointerForTimeDigit(unsigned char Digit,
                                                   unsigned char Offset)
{
  if ( Digit > 10 )
  {
    PrintString("Invalid input to GetPointerForTimeDigit\r\n");
  }
                
  return (pTimeDigit[Digit+Offset]); 
}

static void DisplayAmPm(void)
{  
  /* don't display am/pm in 24 hour mode */
  if ( GetTimeFormat() == TWELVE_HOUR ) 
  {
    int Hour = GetRTCHOUR();
    
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
  int DayOfWeek = GetRTCDOW();
  
  unsigned char const *pFoo = DaysOfWeek[DayOfWeek];
    
  WriteFoo(pFoo,10,8);
  
}

/* add a '/' between the month and day */
static void DisplayDataSeparator(unsigned char RowOffset,
                                 unsigned char ColumnOffset)
{
  /* right most pixel is most significant bit */
  pMyBuffer[RowOffset+0].Data[ColumnOffset+1] |= 0x04;
  pMyBuffer[RowOffset+1].Data[ColumnOffset+1] |= 0x02;
  pMyBuffer[RowOffset+2].Data[ColumnOffset+1] |= 0x02;
  pMyBuffer[RowOffset+3].Data[ColumnOffset+1] |= 0x02;
  pMyBuffer[RowOffset+4].Data[ColumnOffset+1] |= 0x02;
  pMyBuffer[RowOffset+5].Data[ColumnOffset+1] |= 0x01;
  pMyBuffer[RowOffset+6].Data[ColumnOffset+1] |= 0x01;
  pMyBuffer[RowOffset+7].Data[ColumnOffset+1] |= 0x01;
  pMyBuffer[RowOffset+8].Data[ColumnOffset+1] |= 0x01;
  pMyBuffer[RowOffset+9].Data[ColumnOffset]   |= 0x80;
  
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
      First = GetRTCMON();
      Second = GetRTCDAY();
    }
    else
    {
      First = GetRTCDAY();
      Second = GetRTCMON();
    }
    
    /* shift bit so that it lines up with AM/PM and Day of Week */
    WriteSpriteDigit(First/10,20,8,-1);
    /* shift the bits so we can fit a / in the middle */
    WriteSpriteDigit(First%10,20,9,-1);
    WriteSpriteDigit(Second/10,20,10,1);
    WriteSpriteDigit(Second%10,20,11,0);
    DisplayDataSeparator(20,9);
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

const unsigned char Alphabet8w10h[96][10] =  
{
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x10,0x38,0x38,0x10,0x10,0x00,0x10,0x00,
0x00,0x00,0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x28,0x7C,0x28,0x28,0x7C,0x28,0x00,
0x00,0x00,0x10,0x70,0x08,0x30,0x40,0x38,0x20,0x00,
0x00,0x00,0x4C,0x4C,0x20,0x10,0x08,0x64,0x64,0x00,
0x00,0x00,0x08,0x14,0x14,0x08,0x54,0x24,0x58,0x00,
0x00,0x00,0x18,0x18,0x10,0x08,0x00,0x00,0x00,0x00,
0x00,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
0x00,0x00,0x10,0x20,0x20,0x20,0x20,0x20,0x10,0x00,
0x00,0x00,0x92,0x54,0x38,0xFE,0x38,0x54,0x92,0x00,
0x00,0x00,0x00,0x10,0x10,0x7C,0x10,0x10,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x10,0x08,
0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,
0x00,0x00,0x00,0x40,0x20,0x10,0x08,0x04,0x00,0x00,
0x00,0x00,0x38,0x44,0x64,0x54,0x4C,0x44,0x38,0x00,
0x00,0x00,0x10,0x18,0x10,0x10,0x10,0x10,0x38,0x00,
0x00,0x00,0x38,0x44,0x40,0x30,0x08,0x04,0x7C,0x00,
0x00,0x00,0x38,0x44,0x40,0x38,0x40,0x44,0x38,0x00,
0x00,0x00,0x20,0x30,0x28,0x24,0x7C,0x20,0x20,0x00,
0x00,0x00,0x7C,0x04,0x04,0x3C,0x40,0x44,0x38,0x00,
0x00,0x00,0x30,0x08,0x04,0x3C,0x44,0x44,0x38,0x00,
0x00,0x00,0x7C,0x40,0x20,0x10,0x08,0x08,0x08,0x00,
0x00,0x00,0x38,0x44,0x44,0x38,0x44,0x44,0x38,0x00,
0x00,0x00,0x38,0x44,0x44,0x78,0x40,0x20,0x18,0x00,
0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00,
0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x10,
0x00,0x00,0x20,0x10,0x08,0x04,0x08,0x10,0x20,0x00,
0x00,0x00,0x00,0x00,0x7C,0x00,0x00,0x7C,0x00,0x00,
0x00,0x00,0x08,0x10,0x20,0x40,0x20,0x10,0x08,0x00,
0x00,0x00,0x38,0x44,0x40,0x30,0x10,0x00,0x10,0x00,
0x00,0x00,0x38,0x44,0x74,0x54,0x74,0x04,0x38,0x00,
0x00,0x00,0x38,0x44,0x44,0x44,0x7C,0x44,0x44,0x00,
0x00,0x00,0x3C,0x44,0x44,0x3C,0x44,0x44,0x3C,0x00,
0x00,0x00,0x38,0x44,0x04,0x04,0x04,0x44,0x38,0x00,
0x00,0x00,0x3C,0x44,0x44,0x44,0x44,0x44,0x3C,0x00,
0x00,0x00,0x7C,0x04,0x04,0x3C,0x04,0x04,0x7C,0x00,
0x00,0x00,0x7C,0x04,0x04,0x3C,0x04,0x04,0x04,0x00,
0x00,0x00,0x38,0x44,0x04,0x74,0x44,0x44,0x78,0x00,
0x00,0x00,0x44,0x44,0x44,0x7C,0x44,0x44,0x44,0x00,
0x00,0x00,0x38,0x10,0x10,0x10,0x10,0x10,0x38,0x00,
0x00,0x00,0x40,0x40,0x40,0x40,0x44,0x44,0x38,0x00,
0x00,0x00,0x44,0x24,0x14,0x0C,0x14,0x24,0x44,0x00,
0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x7C,0x00,
0x00,0x00,0x44,0x6C,0x54,0x44,0x44,0x44,0x44,0x00,
0x00,0x00,0x44,0x4C,0x54,0x64,0x44,0x44,0x44,0x00,
0x00,0x00,0x38,0x44,0x44,0x44,0x44,0x44,0x38,0x00,
0x00,0x00,0x3C,0x44,0x44,0x3C,0x04,0x04,0x04,0x00,
0x00,0x00,0x38,0x44,0x44,0x44,0x54,0x24,0x58,0x00,
0x00,0x00,0x3C,0x44,0x44,0x3C,0x24,0x44,0x44,0x00,
0x00,0x00,0x38,0x44,0x04,0x38,0x40,0x44,0x38,0x00,
0x00,0x00,0x7C,0x10,0x10,0x10,0x10,0x10,0x10,0x00,
0x00,0x00,0x44,0x44,0x44,0x44,0x44,0x44,0x38,0x00,
0x00,0x00,0x44,0x44,0x44,0x44,0x44,0x28,0x10,0x00,
0x00,0x00,0x44,0x44,0x54,0x54,0x54,0x54,0x28,0x00,
0x00,0x00,0x44,0x44,0x28,0x10,0x28,0x44,0x44,0x00,
0x00,0x00,0x44,0x44,0x44,0x28,0x10,0x10,0x10,0x00,
0x00,0x00,0x7C,0x40,0x20,0x10,0x08,0x04,0x7C,0x00,
0x00,0x00,0x38,0x08,0x08,0x08,0x08,0x08,0x38,0x00,
0x00,0x00,0x00,0x04,0x08,0x10,0x20,0x40,0x00,0x00,
0x00,0x00,0x38,0x20,0x20,0x20,0x20,0x20,0x38,0x00,
0x00,0x00,0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00,
0x00,0x18,0x18,0x08,0x10,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x38,0x40,0x78,0x44,0x78,0x00,
0x00,0x00,0x04,0x04,0x3C,0x44,0x44,0x44,0x3C,0x00,
0x00,0x00,0x00,0x00,0x38,0x44,0x04,0x44,0x38,0x00,
0x00,0x00,0x40,0x40,0x78,0x44,0x44,0x44,0x78,0x00,
0x00,0x00,0x00,0x00,0x38,0x44,0x3C,0x04,0x38,0x00,
0x00,0x00,0x60,0x10,0x10,0x78,0x10,0x10,0x10,0x00,
0x00,0x00,0x00,0x78,0x44,0x44,0x78,0x40,0x38,0x00,
0x00,0x00,0x08,0x08,0x38,0x48,0x48,0x48,0x48,0x00,
0x00,0x00,0x10,0x00,0x10,0x10,0x10,0x10,0x10,0x00,
0x00,0x00,0x40,0x00,0x60,0x40,0x40,0x40,0x48,0x30,
0x00,0x00,0x08,0x08,0x48,0x28,0x18,0x28,0x48,0x00,
0x00,0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,
0x00,0x00,0x00,0x00,0x2C,0x54,0x54,0x44,0x44,0x00,
0x00,0x00,0x00,0x00,0x38,0x48,0x48,0x48,0x48,0x00,
0x00,0x00,0x00,0x00,0x38,0x44,0x44,0x44,0x38,0x00,
0x00,0x00,0x00,0x00,0x3C,0x44,0x44,0x3C,0x04,0x04,
0x00,0x00,0x00,0x00,0x78,0x44,0x44,0x78,0x40,0x40,
0x00,0x00,0x00,0x00,0x34,0x48,0x08,0x08,0x1C,0x00,
0x00,0x00,0x00,0x00,0x38,0x04,0x38,0x40,0x38,0x00,
0x00,0x00,0x00,0x10,0x78,0x10,0x10,0x50,0x20,0x00,
0x00,0x00,0x00,0x00,0x48,0x48,0x48,0x68,0x50,0x00,
0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x28,0x10,0x00,
0x00,0x00,0x00,0x00,0x44,0x44,0x54,0x7C,0x28,0x00,
0x00,0x00,0x00,0x00,0x44,0x28,0x10,0x28,0x44,0x00,
0x00,0x00,0x00,0x00,0x48,0x48,0x48,0x70,0x40,0x70,
0x00,0x00,0x00,0x00,0x78,0x40,0x30,0x08,0x78,0x00,
0x00,0x00,0x30,0x08,0x08,0x0C,0x08,0x08,0x30,0x00,
0x00,0x00,0x10,0x10,0x10,0x00,0x10,0x10,0x10,0x00,
0x00,0x00,0x18,0x20,0x20,0x60,0x20,0x20,0x18,0x00,
0x00,0x00,0x00,0x50,0x28,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x48,0x00,0x00,0x48,0x30,0x00,0x00,
};

/* This is the system font for the time
 * The characters are 11w*19h but they are
 * in 16 bit containers the first are 
 * left justified and the second group are right justified.
 * 10*2*2 = 40 rows of 19
 */
const unsigned char pTimeDigit[10*2][19*2] =
{
0xFC,0xFE,0xFF,0xFF,0x8F,0x8F,0x8F,0x8F,0x8F,0x8F,0x8F,0x8F,0x8F,0x8F,0x8F,0xFF,0xFF,0xFE,0xFC,
0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x01,
0xE0,0xF0,0xFC,0xFC,0xFC,0xFC,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xFC,0xFE,0xFF,0xFF,0x8F,0x8F,0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0x7E,0x3F,0x1F,0xFF,0xFF,0xFF,0xFF,
0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x07,0x07,0x07,0x07,
0xFC,0xFE,0xFF,0xFF,0x8F,0x8F,0x80,0xC0,0xF0,0xF0,0xF0,0xC0,0x80,0x8F,0x8F,0xFF,0xFF,0xFE,0xFC,
0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x01,
0x3C,0xBC,0xBC,0x9E,0x9E,0x8F,0x8F,0xFF,0xFF,0xFF,0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,
0x00,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x00,
0xFF,0xFF,0xFF,0xFF,0x0F,0x0F,0x0F,0xFF,0xFF,0xFF,0xFF,0x80,0x80,0x80,0xC0,0xFF,0xFF,0xFF,0xFF,
0x07,0x07,0x07,0x07,0x00,0x00,0x00,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x03,0x00,
0xF0,0xFC,0xFE,0xFE,0x1F,0x0F,0x0F,0xFF,0xFF,0xFF,0xFF,0x8F,0x8F,0x8F,0x8F,0xFF,0xFF,0xFE,0xFC,
0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x01,
0xFF,0xFF,0xFF,0xFF,0x80,0xC0,0xC0,0xE0,0xE0,0xF0,0xF0,0xF8,0x78,0x78,0x78,0x78,0x78,0x78,0x78,
0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x03,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xFC,0xFE,0xFF,0xFF,0x8F,0x8F,0x8F,0xFF,0xFF,0xFE,0xFF,0x8F,0x8F,0x8F,0x8F,0xFF,0xFF,0xFE,0xFC,
0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x01,
0xFC,0xFE,0xFF,0xFF,0x8F,0x8F,0x8F,0x8F,0xFF,0xFF,0xFE,0xFC,0x80,0x80,0xC0,0xFC,0xFC,0xFC,0x7C,
0x01,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x03,0x03,0x01,0x00,
0xC0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xE0,0xC0,
0x1F,0x3F,0x7F,0x7F,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x7F,0x7F,0x3F,0x1F,
0x00,0x00,0xC0,0xC0,0xC0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
0xC0,0xE0,0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
0x1F,0x3F,0x7F,0x7F,0x78,0x78,0x78,0x7C,0x7E,0x3F,0x1F,0x0F,0x07,0x03,0x01,0x7F,0x7F,0x7F,0x7F,
0xC0,0xE0,0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0xE0,0xC0,
0x1F,0x3F,0x7F,0x7F,0x78,0x78,0x78,0x7C,0x3F,0x1F,0x3F,0x7C,0x78,0x78,0x78,0x7F,0x7F,0x3F,0x1F,
0xC0,0xC0,0xC0,0xC0,0xE0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x03,0x03,0x7B,0x7B,0x79,0x79,0x78,0x78,0x7F,0x7F,0x7F,0x7F,0x78,0x78,0x78,0x78,0x78,0x78,0x78,
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0,
0x7F,0x7F,0x7F,0x7F,0x00,0x00,0x00,0x3F,0x7F,0x7F,0x7F,0x78,0x78,0x78,0x7C,0x7F,0x3F,0x3F,0x0F,
0x00,0xC0,0xE0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xE0,0xC0,
0x1F,0x1F,0x1F,0x1F,0x01,0x00,0x00,0x1F,0x3F,0x7F,0x7F,0x78,0x78,0x78,0x78,0x7F,0x7F,0x3F,0x1F,
0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x7F,0x7F,0x7F,0x7F,0x78,0x7C,0x3C,0x3E,0x1E,0x1F,0x0F,0x0F,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
0xC0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xE0,0xC0,
0x1F,0x3F,0x7F,0x7F,0x78,0x78,0x78,0x7F,0x7F,0x3F,0x7F,0x78,0x78,0x78,0x78,0x7F,0x7F,0x3F,0x1F,
0xC0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xE0,0xC0,0x00,0x00,0x00,0xC0,0xC0,0xC0,0xC0,
0x1F,0x3F,0x7F,0x7F,0x78,0x78,0x78,0x78,0x7F,0x7F,0x7F,0x7F,0x78,0x78,0x7C,0x3F,0x3F,0x1F,0x07,
};

const unsigned char pTimeColonR[1][19*1] = 
{
0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,0x00,0x00,0x00,0x30,0x78,0x78,0x30,0x00,0x00,0x00,0x00,
};

const unsigned char pTimeColonL[1][19*1] = 
{
0x00,0x00,0x00,0x00,0x0C,0x1E,0x1E,0x0C,0x00,0x00,0x00,0x0C,0x1E,0x1E,0x0C,0x00,0x00,0x00,0x00,
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
  for ( unsigned char i = 0; i < NUMBER_OF_BUTTON_MODES; i++ )
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

unsigned char WriteString(unsigned char *pString,
                          unsigned char RowOffset,
                          unsigned char ColumnOffset,
                          unsigned char AddSpace)
{
  unsigned char i = 0;
  unsigned char character = pString[i];
  
  while (   ((i+ColumnOffset) < NUM_LCD_COL_BYTES)
         && character != 0 )
  {
    WriteSpriteChar(pString[i],RowOffset,ColumnOffset+i);  
     
    character = pString[i++];
  }

  return ColumnOffset+i+AddSpace-1;
  
}

/******************************************************************************/

static void InitialiazeIdleBufferConfig(void)
{
  nvIdleBufferConfig = WATCH_CONTROLS_TOP;
  OsalNvItemInit(NVID_IDLE_BUFFER_CONFIGURATION, 
                 sizeof(nvIdleBufferConfig), 
                 &nvIdleBufferConfig);    
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

unsigned char QueryInvertDisplay(void)
{
  return nvIdleBufferInvert; 
}

#ifdef FONT_TESTING
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
  unsigned char i = 0;
  for ( ; i < CharacterWidth && gColumn < NUM_LCD_COL_BYTES; i++ )
  {
    for(unsigned char row = 0; row < CharacterRows; row++)
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

void WriteFontString(unsigned char *pString)
{
  unsigned char i = 0;
   
  while (pString[i] != 0 && gColumn < NUM_LCD_COL_BYTES)
  {
    WriteFontCharacter(pString[i++]);    
  }

}
#endif


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
