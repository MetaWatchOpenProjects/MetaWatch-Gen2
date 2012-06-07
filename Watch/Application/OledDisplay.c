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
/*! \file OledDisplay.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "Messages.h"
#include "MessageQueues.h"

#include "hal_rtc.h"            
#include "hal_analog_display.h"
#include "hal_board_type.h"
#include "hal_lpm.h"
#include "hal_battery.h"
#include "hal_crystal_timers.h"
#include "hal_oled.h"

#include "Buttons.h"
#include "OledDriver.h"
#include "DebugUart.h"
#include "Utilities.h"
#include "Adc.h"
#include "Wrapper.h"
#include "OneSecondTimers.h"
#include "OSAL_Nv.h"
#include "NvIds.h"
#include "Display.h"
#include "Background.h"
#include "Fonts.h"
#include "OledFonts.h"
#include "OledDisplay.h"

/*****************************************************************************/

#define IDLE_PAGE1_INDEX   ( 0 )
#define IDLE_PAGE2_INDEX   ( 1 )
#define TOTAL_IDLE_BUFFERS ( 2 )

static unsigned char IdlePageIndex = IDLE_PAGE1_INDEX;
static void GetNextActiveBufferIndex(void);

static tImageBuffer WatchPage1top;
static tImageBuffer WatchPage1bottom;
static tImageBuffer IdlePageTop[TOTAL_IDLE_BUFFERS];
static tImageBuffer IdlePageBottom[TOTAL_IDLE_BUFFERS];
static tImageBuffer ApplicationPage1top;
static tImageBuffer ApplicationPage1bottom;
static tImageBuffer NotificationPage1top;
static tImageBuffer NotificationPage1bottom;
const tImageBuffer MetaWatchLogoBuffer;

static void InitializeDisplayBuffers(void);

/*****************************************************************************/

static unsigned char TopDisplayOn;   
static unsigned char BottomDisplayOn;   

static void TurnDisplayOn(tImageBuffer* pBuffer);
static void TurnDisplayOff(etOledPosition OledPosition);

#define BLANK_PIXEL_COLUMN ( 0x00 )
#define FULL_PIXEL_COLUMN  ( 0xFF )
static void FillDisplayBuffer(tImageBuffer* pBuffer,unsigned char FillByte);

/*****************************************************************************/

static unsigned char BitReverse(unsigned char data);

/*****************************************************************************/

//static void OledPrint(etOledPosition OledPosition, unsigned char * pString);

static void StartBuildingOledScreen(etOledPosition OledPosition);
static void StartBuildingOledScreenAlternate(tImageBuffer* pBuffer);
static void BuildOledScreenChangeDisplayTimeout(unsigned int Timeout);
static void BuildOledScreenAddCharacter(unsigned char Character);
static void BuildOledScreenAddString(tString* const pString);
static void BuildOledScreenNewlineCheck(void);
static void BuildOledScreenAddNewline(void);
//static void BuildOledScreenAddSpace(void);
//static void BuildOledScreenAddSpaces(unsigned char Spaces);
static void BuildOledScreenAddInteger(unsigned int Value);
static void BuildOledScreenSendToDisplay(void);

unsigned int DisplayTimeoutInSeconds;
  
static tImageBuffer* pBuildBuffer;
static unsigned char BuildRow;
static unsigned char BuildColumn;
static unsigned char BuildBufferFull;
static unsigned char* pBuildSlice;
static tWordByteUnion pBitmap[MAX_FONT_COLUMNS];

static void CopyBufferToDisplay(tImageBuffer* pBuffer);
static void DisplayBuffer(tImageBuffer* pBuffer);
static void StopAllDisplayTimers(void);

/******************************************************************************/

static void DisplayAppAndStackVersionsOnBottomOled(void);
static void DisplayBluetoothStatusBottomOled(void);
static void DisplayBatteryVoltage(unsigned int BatteryVoltage);
static void DisplayLowBatteryIconAndVoltageOnTopOled(void);
static void DisplayToggleOnTopOled(void);

static void DisplayConnectionStatusFace(void);
static void DisplayBatteryStatusFace(void);
static void DisplayVersionsFace(void);
static void DisplayDateAndTimeFace(void);
static void DisplayLowBatteryFace(void);
static void DisplayLowBatteryBluetoothOffFace(void);
static void DisplayLinkAlarmFace(void);
static void SetupIdleFace(void);
static void DisplayChangesSavedFace(void);
static void DisplayBluetoothToggleFace(void);
static void DisplayLinkAlarmToggleFace(void);
static void DisplayPairiabilityFace(void);
static void DisplayRstNmiConfigurationFace(void);
static void DisplaySspFace(void);
static void DisplayResetWatchFace(void);
static void DisplayCrownPullFace(void);
static void DisplayTopContrastFace(void);
static void DisplayBottomContrastFace(void);
static void DisplayMasterResetFace(void);

unsigned char GetBatteryIcon(unsigned int BatteryVoltage);

/* currently each display has its own timer */
static tTimerId ScreenTimerId;
static tTimerId ModeTimerId;

unsigned char NvalSettingsChanged;

/******************************************************************************/

static void WatchStatusHandler(tMessage* pMsg);
static void WatchDrawnScreenTimeoutHandler(tMessage* pMsg);
static void WriteBufferHandler(tMessage* pMsg);
static void WriteScrollBufferHandler(tMessage* pMsg);
static void ScrollHandler(void);
static void ChangeModeHandler(unsigned char Mode);
static void ModeTimeoutHandler(unsigned char CurrentMode);
static void MenuModeHandler(unsigned char MsgOptions);
static void MenuButtonHandler(unsigned char MsgOptions);
static void ShowIdleBufferHandler(void);
static void OledCrownMenuHandler(unsigned char MsgOptions);
static void OledCrownMenuButtonHandler(unsigned char MsgOptions);

static void InitializeDisplayTimers(void);

static void MenuExitHandler(void);

/******************************************************************************/

#define DISPLAY_TASK_QUEUE_LENGTH 8
#define DISPLAY_TASK_STACK_SIZE 	(configMINIMAL_STACK_SIZE + 100)    
#define DISPLAY_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

xTaskHandle DisplayHandle;

static void DisplayTask(void *pvParameters);

static tMessage DisplayMsg;

static unsigned char CurrentMode;
static unsigned char LastMode;
static unsigned char ReturnToApplicationMode;

/******************************************************************************/

#define TOTAL_CONTRAST_VALUES ( 9 )

static unsigned char ContrastTable[TOTAL_CONTRAST_VALUES] =
{
  0,32,64,96,128,160,192,224,255,
};

static unsigned char LastTopContrastIndex;
static unsigned char LastBottomContrastIndex;
static unsigned char nvTopOledContrastIndexDay;
static unsigned char nvBottomOledContrastIndexDay;
static unsigned char nvTopOledContrastIndexNight;
static unsigned char nvBottomOledContrastIndexNight;

static void SaveContrastVales(void);

/******************************************************************************/

static unsigned int nvIdleDisplayTimeout;
static unsigned int nvApplicationDisplayTimeout;
static unsigned int nvNotificationDisplayTimeout;


/******************************************************************************/

unsigned char ScrollTimerCallbackIsr(void);
static void StartScrollTimer(void);
static void StopScrollTimer(void);

/******************************************************************************/

static unsigned char CurrentMenuPage;
static void InitializeMenuPage(void);
static void SelectNextMenuPage(void);

/******************************************************************************/

typedef enum 
{
  ReservedButtonMode,
  IdleButtonMode,
  MenuButtonMode,
  CrownMenuButtonMode
  
} tOledButtonMode;

static tOledButtonMode CurrentButtonConfiguration = ReservedButtonMode;
static tOledButtonMode LastButtonConfiguration = ReservedButtonMode;

static void DontChangeButtonConfiguration(void);
static void ChangeAnalogButtonConfiguration(tOledButtonMode NextButtonMode);
static void NormalIdleScreenButtonConfiguration(void);

/******************************************************************************/

/*! Create queue and task, setup timer, setup OLED, 
 *  and setup display buffers
 *
 * \return none
 */
void InitializeDisplayTask(void)
{
  QueueHandles[DISPLAY_QINDEX] = 
    xQueueCreate( DISPLAY_TASK_QUEUE_LENGTH, MESSAGE_QUEUE_ITEM_SIZE );
  
  // prams are: task function, task name, stack len , task params, priority, task handle
  xTaskCreate(DisplayTask, 
              (const signed char *)"DISPLAY", 
              DISPLAY_TASK_STACK_SIZE, 
              NULL, 
              DISPLAY_TASK_PRIORITY, 
              &DisplayHandle);
  
    
  ClearShippingModeFlag();
 
  /*****************************************************************************/
  
  SetupTimerForAnalogDisplay();
  InitializeContrastValues();
  InitOledI2cPeripheral();
  OledPowerUpSequence();
  InitializeDisplayBuffers();
  SetupIdleFace();
  InitializeDisplayTimeouts();
  InitializeModeTimeouts();
  InitializeTimeFormat();
  InitializeDateFormat();
  InitializeLinkAlarmEnable();
  InitializeMenuPage();
  SetFont(MetaWatch16Oled);
  
  CurrentMode = IDLE_MODE;
  
}

static void InitializeDisplayBuffers(void)
{
  WatchPage1top.OledPosition = TopOled;
  WatchPage1top.Mode = IDLE_MODE;
  
  WatchPage1bottom.OledPosition = BottomOled;
  WatchPage1bottom.Mode = IDLE_MODE;
  
  IdlePageTop[IDLE_PAGE1_INDEX].OledPosition = TopOled;
  IdlePageTop[IDLE_PAGE1_INDEX].Mode = IDLE_MODE;
  
  IdlePageBottom[IDLE_PAGE1_INDEX].OledPosition = BottomOled;
  IdlePageBottom[IDLE_PAGE1_INDEX].Mode = IDLE_MODE;
  
  IdlePageTop[IDLE_PAGE2_INDEX].OledPosition = TopOled;
  IdlePageTop[IDLE_PAGE2_INDEX].Mode = IDLE_MODE;
  
  IdlePageBottom[IDLE_PAGE2_INDEX].OledPosition = BottomOled;
  IdlePageBottom[IDLE_PAGE2_INDEX].Mode = IDLE_MODE;
  
  ApplicationPage1top.OledPosition = TopOled;
  ApplicationPage1top.Mode = APPLICATION_MODE;
  
  ApplicationPage1bottom.OledPosition = BottomOled;
  ApplicationPage1bottom.Mode = APPLICATION_MODE;
  
  NotificationPage1top.OledPosition = TopOled;
  NotificationPage1top.Mode = NOTIFICATION_MODE;
  
  NotificationPage1bottom.OledPosition = BottomOled;
  NotificationPage1bottom.Mode = NOTIFICATION_MODE;

}


static void InitializeDisplayControllers(void)
{
  /*
   * Initialize display controllers for both of the OLEDs
   */
  InitializeOledDisplayController(TopOled,ContrastTable[nvTopOledContrastIndexDay]);
  PrintString("Initialized TopOled\r\n");
  
  InitializeOledDisplayController(BottomOled,ContrastTable[nvBottomOledContrastIndexDay]);
  PrintString("Initialized BottomOled\r\n");
  
#if 0
  OledPrint(TopOled,"Testing 123456789abcdefghijklmnopq");
  OledPrint(BottomOled,"456789 & 10");
#endif
  
#if 0
  /* ? can the display scroll itself ? */
  WriteOneByteOledCommand(0x26);
  /* dummy */
  WriteOneByteOledCommand(0x00);
  /* start page */
  WriteOneByteOledCommand(0x06);
  /* time interval */
  WriteOneByteOledCommand(0x00);
  /* end page */
  WriteOneByteOledCommand(0x07);
  /* dummy byte */
  WriteOneByteOledCommand(0x00);
  /* dummy byte */
  WriteOneByteOledCommand(0xff);
  /* activate scroll */
  WriteOneByteOledCommand(0x2f);
#endif

}


static void DisplayQueueMessageHandler(tMessage* pMsg)
{
  eMessageType Type = (eMessageType)pMsg->Type;
  
  switch(Type)
  {
  case OledWriteBufferMsg:
    WriteBufferHandler(pMsg);
    break;
    
  case OledConfigureModeMsg:
    break;
    
  case OledChangeModeMsg:
    ChangeModeHandler(pMsg->Options & MODE_MASK);
    break;
   
  case OledWriteScrollBufferMsg:
    WriteScrollBufferHandler(pMsg);
    break;
    
  case OledScrollMsg:
    ScrollHandler();
    break;
    
  case OledShowIdleBufferMsg:
    ShowIdleBufferHandler();
    break;
    
  case WatchStatusMsg:
    WatchStatusHandler(pMsg);
    break;
  
  case WatchDrawnScreenTimeout:
    WatchDrawnScreenTimeoutHandler(pMsg);
    break;
   
  case ModeTimeoutMsg:
    ModeTimeoutHandler(pMsg->Options & MODE_MASK);
    break;
    
  case OledCrownMenuMsg:
    OledCrownMenuHandler(pMsg->Options);
    break;
    
  case OledCrownMenuButtonMsg:
    OledCrownMenuButtonHandler(pMsg->Options);
    break;
    
  case MenuModeMsg:
    MenuModeHandler(pMsg->Options);
    break;
  
  case MenuButtonMsg:
    MenuButtonHandler(pMsg->Options);
    break;
    
  case ConnectionStateChangeMsg:
    if ( CurrentMode == IDLE_MODE )
    {
    switch (CurrentButtonConfiguration)
    {
    case IdleButtonMode:
      /* don't know what sub-mode we are in */
      break;
    case MenuButtonMode:
      MenuModeHandler(MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
      break;
    case CrownMenuButtonMode:
      /* don't care */
      break;
    default:
      break;
    }
    }
    break;
   
  case LowBatteryWarningMsg:
    DisplayLowBatteryFace();
    break;
    
  case LowBatteryBtOffMsg:
    DisplayLowBatteryBluetoothOffFace();
    break;
    
  case LinkAlarmMsg:
    if ( QueryLinkAlarmEnable() )
    {
      DisplayLinkAlarmFace();
      GenerateLinkAlarm();  
    }
    break;
    
  default:
    PrintStringAndHex("<<Unhandled Message>> in Oled Display Task: Type 0x", Type);
    break;
  }

}

static void DisplayTask(void *pvParameters)
{
  if ( QueueHandles[DISPLAY_QINDEX] == 0 )
  {
    PrintString("Display Queue not created!\r\n");  
  }
  
  InitializeDisplayTimers();
  InitializeDisplayControllers();
  
  /* 
   * display the logo on the top screen and 
   * the application and stack version on the bottom screen
   */
  DisplayBuffer((tImageBuffer*)&MetaWatchLogoBuffer);
  DisplayAppAndStackVersionsOnBottomOled();
    
  /* turn the radio on; initialize the serial port profile */
  tMessage Msg;
  SetupMessage(&Msg,TurnRadioOnMsg,NO_MSG_OPTIONS);
  RouteMsg(&Msg);
    
  /* force startup screens to be displayed 
   * without something else overwriting them
   */
  TaskDelayLpmDisable();
  vTaskDelay(2000);
  TaskDelayLpmEnable();
  
  /* don't enable buttons until after splash screen */
  DontChangeButtonConfiguration();
  ChangeAnalogButtonConfiguration(IdleButtonMode);
  NormalIdleScreenButtonConfiguration();
  
  for(;;)
  {
    if( xQueueReceive(QueueHandles[DISPLAY_QINDEX], &DisplayMsg, portMAX_DELAY) )
    {
      DisplayQueueMessageHandler(&DisplayMsg);
      
      SendToFreeQueue(&DisplayMsg);
      
      CheckStackUsage(DisplayHandle,"Display");
      
      CheckQueueUsage(QueueHandles[DISPLAY_QINDEX]);
      
    }
  }
}


/* select the buffer based on the message options and buffer select bits */
tImageBuffer* SelectImageBuffer(unsigned char MsgOptions,
                                unsigned char BufferSelect)
{
  
  tImageBuffer * pImage = 0;
  
  switch(MsgOptions & MODE_MASK)
  {
  case IDLE_MODE:
    
    switch(BufferSelect)
    {
    case 0: pImage = &IdlePageTop[IDLE_PAGE1_INDEX];    break;
    case 1: pImage = &IdlePageBottom[IDLE_PAGE1_INDEX]; break;
    case 2: pImage = &IdlePageTop[IDLE_PAGE2_INDEX];    break;
    case 3: pImage = &IdlePageBottom[IDLE_PAGE2_INDEX]; break;
    default: 
      break;
    }
    break;
  
  case APPLICATION_MODE:
    
    switch(BufferSelect)
    {
    case 0: pImage = &ApplicationPage1top;    break;
    case 1: pImage = &ApplicationPage1bottom; break;
    default: 
      break;
    }
    break;
  
  case NOTIFICATION_MODE:
    switch(BufferSelect)
    {
    case 0: pImage = &NotificationPage1top;    break;
    case 1: pImage = &NotificationPage1bottom; break;
    default: 
      break;
    }
    break;
    
  default:
    break;
  }
 
  return pImage;
}

static void WriteBufferHandler(tMessage* pMsg)
{
  tImageBuffer * pImage;
  
  tWriteOledBufferPayload* pWriteOledBufferPayload = 
    (tWriteOledBufferPayload*)pMsg->pBuffer;

  pImage = SelectImageBuffer(pMsg->Options,
                             pWriteOledBufferPayload->BufferSelect);  
  
  /* the fill operations occur for the entire page */
  unsigned char ActivatePage = 0;
  switch(pMsg->Options & PAGE_CONTROL_MASK)
  {
  case PAGE_CONTROL_INVALIDATE:
    pImage->Valid = 0;
    break;
  case PAGE_CONTROL_INVALIDATE_AND_CLEAR:
    pImage->Valid = 0;
    FillDisplayBuffer(pImage,BLANK_PIXEL_COLUMN);
    break;
  case PAGE_CONTROL_INVALIDATE_AND_FILL:
    pImage->Valid = 0;
    FillDisplayBuffer(pImage,FULL_PIXEL_COLUMN);
    break;
  case PAGE_CONTROL_ACTIVATE: 
    pImage->Valid = 1;
    ActivatePage = 1;
    break;
  }
  
  /* row index == column */
  unsigned char Index = pWriteOledBufferPayload->Column;
  
  /* copy the data into the buffer */
  unsigned char i;
  for(i = 0; i < pWriteOledBufferPayload->Size; i++)
  {
    if ( Index < DISPLAY_BUFFER_SIZE )
    {
      /* this is reversed from what the rest of the code does
       * this is how application wants it
       */
      if ( pImage->OledPosition == BottomOled )
      {
        pImage->pPixelData[Index++] = BitReverse(pWriteOledBufferPayload->pPayload[i]);
      }
      else
      {
        pImage->pPixelData[Index++] = pWriteOledBufferPayload->pPayload[i];
      }
      
    }
  }
  
  if ( ActivatePage )
  {
    DisplayBuffer(pImage);
  }
  
}

static void StopAllDisplayTimers(void)
{
  StopOneSecondTimer(ScreenTimerId);  
  StopScrollTimer();
}

/* after the first row is written the read and write need to be offset 
 * by at least a row (buffer has more characters than phone knows about) */
#define SCROLL_BUFFER_SIZE ( 240+80 )

static unsigned char ScrollFirstRowDone;
static unsigned int ScrollWriteIndex;
static unsigned int ScrollReadIndex;
static unsigned char ScrollDisplaySize;
static unsigned int ScrollCharactersToDisplay;
static unsigned char pScrollBuffer[SCROLL_BUFFER_SIZE];
static unsigned char LastScrollPacketReceived;

static void WriteScrollBufferHandler(tMessage* pMsg)
{
  tWriteScrollBufferPayload* pWriteScrollBufferPayload = 
    (tWriteScrollBufferPayload*)pMsg->pBuffer;

  /* copy the data into the buffer */
  unsigned char i;
  for(i = 0; i < pWriteScrollBufferPayload->Size; i++)
  {
    pScrollBuffer[ScrollWriteIndex] = 
      BitReverse(pWriteScrollBufferPayload->pPayload[i]);
  
    ScrollWriteIndex++;
    if ( ScrollWriteIndex >= SCROLL_BUFFER_SIZE )
    {
      ScrollWriteIndex = 0;  
    }
    
    ScrollCharactersToDisplay++;
    
    if ( ScrollCharactersToDisplay > SCROLL_BUFFER_SIZE - ROW_SIZE )
    {
      __no_operation();  
    }
  
  }
  
  if ( (pMsg->Options & SCROLL_OPTION_LAST_PACKET_MASK) == SCROLL_OPTION_LAST_PACKET)
  {
    LastScrollPacketReceived = 1;  
  }
  
  if ( (pMsg->Options & SCROLL_OPTION_START_MASK) == SCROLL_OPTION_START)
  {
    StopAllDisplayTimers();
    StartScrollTimer();  
  }
}


/*! Copies an image buffer to the specified OLED display
 *
 * \param pImageBuffer Pointer to the image buffer with the data to write to the display
 *
 * \return none
 */
static void CopyBufferToDisplay(tImageBuffer* pBuffer)
{
  /* set the i2c address once per operation */
  SetOledDeviceAddress(pBuffer->OledPosition);

  unsigned char row;
  for(row = 0; row < NUMBER_OF_ROWS; row++)
  {
    SetRowInOled(row,pBuffer->OledPosition);
    WriteOledData((unsigned char*)&pBuffer->pPixelData[row*ROW_SIZE],ROW_SIZE);
  }
  
}


/* or CopyOffsetBufferToDisplay */
static void ScrollHandler(void)
{
  tMessage OutgoingMsg;
      
  /* shorthand */
  unsigned char *pBuf = NotificationPage1bottom.pPixelData;
  
  /* set the i2c address once per operation */
  SetOledDeviceAddress(BottomOled);

  SetRowInOled(0,BottomOled);
  WriteOledData(&pBuf[0],ROW_SIZE);
  
  SetRowInOled(1,BottomOled);
  if ( ScrollDisplaySize < 80 )
  {
    ScrollDisplaySize++;
    
    /* draw the first part of the buffer until it has been shifted out */
    unsigned FirstPieceSize = ROW_SIZE - ScrollDisplaySize;
  
    if ( FirstPieceSize )
    {
      WriteOledData(&pBuf[ROW_SIZE+ScrollReadIndex],FirstPieceSize);
    }
    
    WriteOledData(&pScrollBuffer[0],ScrollDisplaySize);    
  
  }
  else /* we have displayed the first 80 columns */
  {
    if ( ScrollReadIndex > SCROLL_BUFFER_SIZE - ROW_SIZE )
    {
      unsigned char PartOneSize = SCROLL_BUFFER_SIZE - ScrollReadIndex;
      unsigned char PartTwoSize = ROW_SIZE - PartOneSize;
      
      WriteOledData(&pScrollBuffer[ScrollReadIndex],PartOneSize);
      WriteOledData(&pScrollBuffer[0],PartTwoSize);
    }
    else
    {
      WriteOledData(&pScrollBuffer[ScrollReadIndex],ROW_SIZE);
    }
    
  }
    
  ScrollReadIndex++;
  if ( ScrollReadIndex % ROW_SIZE == 0)
  {
    /* restart counter after first row because things switch in the
     * if else above
     */
    if ( ScrollReadIndex == ROW_SIZE && ScrollFirstRowDone == 0 )
    {
      ScrollReadIndex = 0;
      ScrollFirstRowDone = 1;
    }
    else if ( ScrollReadIndex == SCROLL_BUFFER_SIZE )
    {
      ScrollReadIndex = 0;  
    }
    
    if ( LastScrollPacketReceived == 0 )
    {
      /* send a scroll request message to the host */
      SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                    StatusChangeEvent,
                                    NOTIFICATION_MODE);
      OutgoingMsg.Length = 2;
      OutgoingMsg.pBuffer[0] = (unsigned char)eScScrollRequest;

      if ( ScrollCharactersToDisplay > SCROLL_BUFFER_SIZE - ROW_SIZE )
      {
        OutgoingMsg.pBuffer[1] = 0;
      }
      else
      {
        OutgoingMsg.pBuffer[1] = SCROLL_BUFFER_SIZE - ROW_SIZE - ScrollCharactersToDisplay;
      }
      
      RouteMsg(&OutgoingMsg);  
    }
  }
  
  if ( ScrollCharactersToDisplay > 0 )
  {
    ScrollCharactersToDisplay--;  
  }
  
  if ( ScrollCharactersToDisplay != 0 ) 
  {
    StartScrollTimer();  
  }
  else
  {
    StopOneSecondTimer(ScreenTimerId);
    
    SetupOneSecondTimer(ScreenTimerId,
                        ONE_SECOND*2,
                        NO_REPEAT,
                        DISPLAY_QINDEX,
                        WatchDrawnScreenTimeout,
                        NO_MSG_OPTIONS);
      
    StartOneSecondTimer(ScreenTimerId);
    
    ScrollFirstRowDone = 0;
    ScrollWriteIndex = 0;
    ScrollReadIndex = 0;
    ScrollDisplaySize = 0;
    LastScrollPacketReceived = 0;
    
    /* send scroll done status */
    SetupMessageAndAllocateBuffer(&OutgoingMsg,
                                  StatusChangeEvent,
                                  NOTIFICATION_MODE);
          
    OutgoingMsg.pBuffer[0] = (unsigned char)eScScrollComplete;
    OutgoingMsg.Length = 1;
    
    RouteMsg(&OutgoingMsg);  
    
  }
}

static void DisplayBuffer(tImageBuffer* pBuffer)
{
  if ( pBuffer->Valid )
  {
    TurnDisplayOn(pBuffer);
    CopyBufferToDisplay(pBuffer);
  }
}

#if 0
/*!
 *  Writes a string to an OLED Display buffer
 * 
 * \return none
 */

static void OledPrint(etOledPosition OledPosition, unsigned char *pString)
{
  StartBuildingOledScreen(OledPosition);
  BuildOledScreenAddString(pString);
  BuildOledScreenSendToDisplay();
}
#endif

/* screens are assumed to be built up and sent so there isn't any 
 * provisioning for drawing into more than one buffer at a time 
 */
static void StartBuildingOledScreen(etOledPosition OledPosition)
{
  /* select the appropriate display buffer */
  if ( OledPosition == TopOled )
  {
    pBuildBuffer = &WatchPage1top;  
  }
  else
  {
    pBuildBuffer = &WatchPage1bottom;
  }
  
  /* clean the buffer */
  pBuildBuffer->Valid = 0;
  FillDisplayBuffer(pBuildBuffer,BLANK_PIXEL_COLUMN);

  BuildColumn = 0;
  BuildRow = 0;
  BuildBufferFull = 0;
  pBuildSlice = pBuildBuffer->pPixelData;
  
  DisplayTimeoutInSeconds = nvIdleDisplayTimeout;
  
}

static void BuildOledScreenChangeDisplayTimeout(unsigned int Timeout)
{
  DisplayTimeoutInSeconds = Timeout;  
}

/*
 * use this to draw in buffers other than WatchTop and WatchBottom
 */
static void StartBuildingOledScreenAlternate(tImageBuffer* pBuffer)
{
  pBuildBuffer = pBuffer;
  
  /* clean the buffer */
  pBuildBuffer->Valid = 0;
  FillDisplayBuffer(pBuildBuffer,BLANK_PIXEL_COLUMN);

  BuildColumn = 0;
  BuildRow = 0;
  BuildBufferFull = 0;
  pBuildSlice = pBuildBuffer->pPixelData;
  
  DisplayTimeoutInSeconds = nvIdleDisplayTimeout;
  
}

/*! todo - handle the case where the string goes to the next line, but the 
 * character is a space
 */
static void BuildOledScreenAddString(tString * pString)
{
  
  /* 
   * stop processing characters when a null is reached or
   * when there isn't any more room
   */
  unsigned char StringIndex = 0;
  while( pString[StringIndex] != 0 && BuildBufferFull == 0 )                      
  {
    BuildOledScreenAddCharacter(pString[StringIndex++]);
  }

}


static void BuildOledScreenAddCharacter(unsigned char Character)
{
  unsigned char width = GetCharacterWidth(Character);
  GetCharacterBitmap(Character,(unsigned int*)pBitmap);
  
  unsigned char slice;
  for(slice = 0; slice < width && BuildColumn < NUMBER_OF_COLUMNS; slice++)
  {
    
    if ( GetCharacterHeight() > COLUMN_HEIGHT )
    {
      /* 
       * the rows are known for characters that take two rows 
      */
      
      if ( pBuildBuffer->OledPosition == TopOled )
      {
        pBuildSlice[BuildColumn] = BitReverse(pBitmap[slice].Bytes.byte1);
        pBuildSlice[ROW_SIZE+BuildColumn] = BitReverse(pBitmap[slice].Bytes.byte0);
      }
      else
      {
        pBuildSlice[BuildColumn] = pBitmap[slice].Bytes.byte1; 
        pBuildSlice[ROW_SIZE+BuildColumn] = pBitmap[slice].Bytes.byte0; 
      }

    }
    else
    {
      if ( pBuildBuffer->OledPosition == TopOled )
      {
        pBuildSlice[BuildRow*ROW_SIZE+BuildColumn] = BitReverse(pBitmap[slice].Bytes.byte0);
      }
      else
      {
        pBuildSlice[BuildRow*ROW_SIZE+BuildColumn] = pBitmap[slice].Bytes.byte0; 
      }
      
    }
    
    BuildColumn++;

  }

  /* add space between characters */
  BuildColumn += GetFontSpacing();
  
  /* if a character is too long for the line then it will be chopped off */  
  BuildOledScreenNewlineCheck(); 

}

static void BuildOledScreenNewlineCheck(void)
{
  if ( BuildColumn >= NUMBER_OF_COLUMNS )
  {
    if ( GetCharacterHeight() > COLUMN_HEIGHT )
    {
      BuildBufferFull = 1; 
    }
    else if ( BuildRow == 1)
    {
      BuildBufferFull = 1;  
    }
    else
    {
      BuildColumn = 0;
      BuildRow = 1;
    }
  }  
}


static void BuildOledScreenAddNewline(void)
{
  BuildColumn = NUMBER_OF_COLUMNS;
  BuildOledScreenNewlineCheck();
}

///* does not check end of row */
//static void BuildOledScreenAddSpace(void)
//{
//  BuildColumn++;  
//}
//
//static void BuildOledScreenAddSpaces(unsigned char Spaces)
//{
//  BuildColumn += Spaces;  
//}

static void BuildOledScreenAddInteger(unsigned int Value)
{
  tString pTemporaryBuildString[6];
  ToDecimalString(Value,(char*)&pTemporaryBuildString);
  BuildOledScreenAddString(pTemporaryBuildString); 
}


static void BuildOledScreenSendToDisplay(void)
{
  pBuildBuffer->Valid = 1;
  DisplayBuffer(pBuildBuffer);
}

/*! Turns the specified OLED display on and starts display time
 *
 * \return none
 */
static void TurnDisplayOn(tImageBuffer* pBuffer)
{                 
  /* determine the display timeout */
  switch (pBuffer->Mode)
  {
  case IDLE_MODE:
    if ( CurrentButtonConfiguration == CrownMenuButtonMode )
    {
      DisplayTimeoutInSeconds = ONE_SECOND*60;
    }
    break;
  case APPLICATION_MODE:
    DisplayTimeoutInSeconds = nvApplicationDisplayTimeout;
    break;
  case NOTIFICATION_MODE:
    DisplayTimeoutInSeconds = nvNotificationDisplayTimeout;
    break;
  default:
    DisplayTimeoutInSeconds = DEFAULT_IDLE_DISPLAY_TIMEOUT;
    break;
  }
  
  /* to save power the 10V supply is disabled when the oleds are not being used
   * the 2.5V IO is also disabled.
   *
   * if both of the displays are off then turn on the supplies and
   * re-initialize them 
   */
  if ( TopDisplayOn == 0 && BottomDisplayOn == 0 )
  {
    OledPowerUpSequence();
    InitializeDisplayControllers();
  }
  
  /* turn required display on */
  switch (pBuffer->OledPosition)
  {
  case TopOled:
    if ( !TopDisplayOn )
    {
      SetOledDeviceAddress(pBuffer->OledPosition);
      WriteOneByteOledCommand(OLED_CMD_DISPLAY_ON_NORMAL_MODE);
      WriteOneByteOledCommand(OLED_CMD_DISPLAY_ON_NORMAL);
      TopDisplayOn = 1;
    }
    break;
    
  case BottomOled:
    if ( !BottomDisplayOn )
    {
      SetOledDeviceAddress(pBuffer->OledPosition);
      WriteOneByteOledCommand(OLED_CMD_DISPLAY_ON_NORMAL_MODE);
      WriteOneByteOledCommand(OLED_CMD_DISPLAY_ON_NORMAL);
      BottomDisplayOn = 1;
    }
    break;  
  }

  /* is there a condition in which more than one timer would be needed ? */
  StopOneSecondTimer(ScreenTimerId);
  
  SetupOneSecondTimer(ScreenTimerId,
                      DisplayTimeoutInSeconds,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      WatchDrawnScreenTimeout,
                      NO_MSG_OPTIONS);
  
  StartOneSecondTimer(ScreenTimerId);
    
}

/*! Turns the specified OLED display off if it is not in active mode
 * or being used for scrolling
 *
 * \param etOledPosition OledPosition select
 *
 * \return none
 */
static void TurnDisplayOff(etOledPosition OledPosition)
{

  switch (OledPosition)
  {

  case TopOled:

    if( TopDisplayOn )
    {
      SetOledDeviceAddress(TopOled);
      WriteOneByteOledCommand(OLED_CMD_DISPLAY_OFF_SLEEP);
      TopDisplayOn = 0;
    }
    break;

  case BottomOled:

    if( BottomDisplayOn )
    {
      SetOledDeviceAddress(BottomOled);
      WriteOneByteOledCommand(OLED_CMD_DISPLAY_OFF_SLEEP);
      BottomDisplayOn = 0;
    }
    break;

  default:
    PrintString("Display ? off\r\n");
    break;
  }

  if ( TopDisplayOn == 0 && BottomDisplayOn == 0 )
  {
    OledPowerDown();  
  }
  
}

static void FillDisplayBuffer(tImageBuffer* pBuffer,unsigned char FillByte)
{
  unsigned char i;	
  for( i = 0; i < DISPLAY_BUFFER_SIZE; i++ )
  {  
    pBuffer->pPixelData[i] = FillByte;
  }
}

/*******************************************************************************
Fast Bit reverse algorithm for bytes b[7] = b[0]  etc...
*******************************************************************************/
static inline unsigned char BitReverse(unsigned char data)
{
  static const unsigned char lut[] = 
  { 
    0, 8,  4, 12,
    2, 10, 6, 14,
    1, 9,  5, 13,
    3, 11, 7, 15 
  };

  return (unsigned char)(((lut[(data & 0x0F)]) << 4) + lut[((data & 0xF0) >> 4)]);
  
}

unsigned char QueryButtonMode(void)
{
  unsigned char result;
  
  switch (CurrentMode)
  {
    
  case IDLE_MODE:
    if ( CurrentButtonConfiguration == IdleButtonMode )
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

static void DontChangeButtonConfiguration(void)
{
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_A_INDEX,
                     BUTTON_STATE_LONG_HOLD,
                     SoftwareResetMsg,
                     NO_MSG_OPTIONS);

  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     OledCrownMenuMsg,
                     NO_MSG_OPTIONS);
      
  /* 
   * setup the button to generate an event when the crown is pushed back in 
   * (in == off state)
   */
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_PRESSED,
                     OledCrownMenuButtonMsg,
                     OLED_CROWN_MENU_BUTTON_OPTION_EXIT);
      
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_HOLD,
                     OledCrownMenuButtonMsg,
                     OLED_CROWN_MENU_BUTTON_OPTION_EXIT);
  
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_LONG_HOLD,
                     OledCrownMenuButtonMsg,
                     OLED_CROWN_MENU_BUTTON_OPTION_EXIT);
    
  /****************************************************************************/

  EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                     SW_A_INDEX,
                     BUTTON_STATE_LONG_HOLD,
                     SoftwareResetMsg,
                     NO_MSG_OPTIONS);

  EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_IMMEDIATE,
                     OledCrownMenuMsg,
                     NO_MSG_OPTIONS);
      
  /* 
   * setup the button to generate an event when the crown is pushed back in 
   * (in == off state)
   */
  EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_PRESSED,
                     OledCrownMenuButtonMsg,
                     OLED_CROWN_MENU_BUTTON_OPTION_EXIT);
      
  EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_HOLD,
                     OledCrownMenuButtonMsg,
                     OLED_CROWN_MENU_BUTTON_OPTION_EXIT);
  
  EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                     SW_P_INDEX,
                     BUTTON_STATE_LONG_HOLD,
                     OledCrownMenuButtonMsg,
                     OLED_CROWN_MENU_BUTTON_OPTION_EXIT);
          
}


static void NormalIdleScreenButtonConfiguration(void)
{
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                         SW_A_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         WatchStatusMsg,
                         NO_MSG_OPTIONS);
    
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         OledShowIdleBufferMsg,
                         NO_MSG_OPTIONS);
      
  EnableButtonAction(NORMAL_IDLE_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         NO_MSG_OPTIONS);
}
      
static void ChangeAnalogButtonConfiguration(tOledButtonMode NextButtonMode)
{
  LastButtonConfiguration = CurrentButtonConfiguration;
  CurrentButtonConfiguration = NextButtonMode;
      
  /* always delete the master reset action */
  DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                      SW_A_INDEX,
                      BUTTON_STATE_LONG_HOLD);
      
  if ( CurrentButtonConfiguration != LastButtonConfiguration )
  {
    switch (CurrentButtonConfiguration)
    {
    case IdleButtonMode:
      /* don't do anything - this has its own button configuration */
      break;
    
    case MenuButtonMode:
         
      /* toggle button is handled for each page/face */
      
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_B_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuButtonMsg,
                         MENU_BUTTON_OPTION_EXIT);
      
      /* go to the next page */
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         MenuModeMsg,
                         NO_MSG_OPTIONS);
      
      break;
      
    case CrownMenuButtonMode:
      
      DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                          SW_A_INDEX,
                          BUTTON_STATE_IMMEDIATE);
      
      DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                          SW_B_INDEX,
                          BUTTON_STATE_IMMEDIATE);
            
      EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                         SW_C_INDEX,
                         BUTTON_STATE_IMMEDIATE,
                         OledCrownMenuMsg,
                         OLED_CROWN_MENU_MODE_OPTION_NEXT_MENU);
      break;
      
    default:
      break;
    }
    
  }
    
}

    /* buttons d, e, and f are activated when the crown is pulled */


#define WATCH_STATUS_DATE_TIME_FACE              ( 0 )
#define WATCH_STATUS_CONNECTION_STATUS_FACE      ( 1 )
#define WATCH_STATUS_BATTERY_STATUS_FACE         ( 2 )
#define WATCH_STATUS_VERSIONS_FACE               ( 3 )
#define WATCH_STATUS_STACK_VERSION               ( 4 ) 
#define WATCH_STATUS_MSP430_VERSION              ( 5 )
#define WATCH_STATUS_TOTAL_PAGES                 ( 4 ) 

static unsigned char WatchStatusIndex;

static void WatchStatusHandler(tMessage* pMsg)
{
  /* on the first press display the time
   *
   * while active increment through the status displays
   *
   * keep track of the last menu that was used
   */
  if (   TopDisplayOn 
      && CurrentButtonConfiguration == IdleButtonMode )
  {
    WatchStatusIndex++;  
    if ( WatchStatusIndex >= WATCH_STATUS_TOTAL_PAGES )
    {
      WatchStatusIndex = 0;  
    }
  }

  /* don't display time if the phone has not been connected */  
  if (   WatchStatusIndex == WATCH_STATUS_DATE_TIME_FACE 
      && QueryFirstContact() == 0)
  {
    WatchStatusIndex++;  
  }
  
  switch (WatchStatusIndex)  
  {
  case WATCH_STATUS_DATE_TIME_FACE:
    DisplayDateAndTimeFace();
    break;
  case WATCH_STATUS_CONNECTION_STATUS_FACE:
    DisplayConnectionStatusFace();
    break;
  case WATCH_STATUS_BATTERY_STATUS_FACE:
    DisplayBatteryStatusFace();
    break;
  case WATCH_STATUS_VERSIONS_FACE:
    DisplayVersionsFace();
    break;
#if 0
  case WATCH_STATUS_STACK_VERSION:
    DisplayStackVersion();
    break;
  case WATCH_STATUS_MSP430_VERSION:
    DisplayMsp430Version();
    break;
#endif
  }
  
}

/* display timeout */
static void WatchDrawnScreenTimeoutHandler(tMessage* pMsg)
{
  TurnDisplayOff(TopOled);
  TurnDisplayOff(BottomOled);
   
  /* the menu exit handler will redraw the screen - so this must occur
   * after the display is shut off (the else-if could be made more complicated too )
   */
  if ( CurrentMode == IDLE_MODE )
  {
    /* save menu changes on display timeout
     * the exit menu button will change button mode
     * before we get here 
     */
    if ( CurrentButtonConfiguration == MenuButtonMode )
    {
      MenuExitHandler();
    }
    else
    {
      ChangeAnalogButtonConfiguration(IdleButtonMode);
    }
  }
  
  
}

#if 0
static void DisplayChargingStatus(void)
{
  StartBuildingOledScreen(TopOled);
   
  if ( QueryBatteryCharging() )
  {
    BuildOledScreenAddString("Battery Charging");  
  }
  else if ( QueryPowerGood() )
  {
    BuildOledScreenAddString("USB connected");  
  }
  else
  {
    BuildOledScreenAddString("USB not connected");  
  }
  
  BuildOledScreenSendToDisplay();
  
}
#endif

static void DisplayAppAndStackVersionsOnBottomOled(void)
{ 
  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString("App ");
  BuildOledScreenAddString(VERSION_STRING);
  BuildOledScreenAddNewline();
  BuildOledScreenAddString("Stack ");
  BuildOledScreenAddString(GetWrapperVersion());
  BuildOledScreenSendToDisplay();
}

static void DisplayVersionsFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString("MSP430 Rev ");
  BuildOledScreenAddCharacter(GetMsp430HardwareRevision());
  BuildOledScreenSendToDisplay();
  
  DisplayAppAndStackVersionsOnBottomOled();    
}

static void DisplayDateAndTimeFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch16Oled);
  BuildColumn = 15;
  BuildOledScreenAddString((tString*)DaysOfTheWeek[RTCDOW]);
  BuildOledScreenAddCharacter(' ');
  
  /* determine if month or day is displayed first */
  if ( GetDateFormat() == MONTH_FIRST )
  {
    BuildOledScreenAddInteger(RTCMON);
    BuildOledScreenAddCharacter('/');
    BuildOledScreenAddInteger(RTCDAY);
  }
  else
  {
    BuildOledScreenAddInteger(RTCDAY);
    BuildOledScreenAddCharacter('/');
    BuildOledScreenAddInteger(RTCMON);
  }
  
  BuildOledScreenSendToDisplay();  

  /****************************************************************************/
    
  SetFont(MetaWatch16Oled);
  StartBuildingOledScreen(BottomOled);
  BuildColumn = 25;
  
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
  BuildOledScreenAddInteger(Hour);
  BuildOledScreenAddCharacter(':');
  
  /* check to see if the minutes need to be padded */
  int Minutes = RTCMIN;
  if ( Minutes < 10 )
  {
    BuildOledScreenAddCharacter('0');
  }
  BuildOledScreenAddInteger(RTCMIN);
  
#if 0
  BuildOledScreenAddCharacter(':');
  BuildOledScreenAddInteger(RTCSEC);
#endif
  
  /* display am/pm if in 12 hour mode */
  SetFont(MetaWatch5Oled);
  if ( GetTimeFormat() == TWELVE_HOUR ) 
  {
    Hour = RTCHOUR;
    
    if ( Hour >= 12 )
    {
      BuildOledScreenAddString("PM");   
    }
    else
    {
      BuildOledScreenAddString("AM");   
    }
    
  }
  
  
  BuildOledScreenSendToDisplay();  
  
  
}

static void DisplayConnectionStatusFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString("MAC ADDRESS");
  
  BuildOledScreenAddNewline();
  
  SetFont(MetaWatch5Oled);
  
  /* print the string with ':' between two characters */
  tString* pBtAddr = GetLocalBluetoothAddressString();
  BuildOledScreenAddCharacter(pBtAddr[0]);
  BuildOledScreenAddCharacter(pBtAddr[1]);
  BuildOledScreenAddCharacter(':');
  BuildOledScreenAddCharacter(pBtAddr[2]);
  BuildOledScreenAddCharacter(pBtAddr[3]);
  BuildOledScreenAddCharacter(':');
  BuildOledScreenAddCharacter(pBtAddr[4]);
  BuildOledScreenAddCharacter(pBtAddr[5]);
  BuildOledScreenAddCharacter(':');
  BuildOledScreenAddCharacter(pBtAddr[6]);
  BuildOledScreenAddCharacter(pBtAddr[7]);
  BuildOledScreenAddCharacter(':');
  BuildOledScreenAddCharacter(pBtAddr[8]);
  BuildOledScreenAddCharacter(pBtAddr[9]);
  BuildOledScreenAddCharacter(':');
  BuildOledScreenAddCharacter(pBtAddr[10]);
  BuildOledScreenAddCharacter(pBtAddr[11]);
 
  BuildOledScreenSendToDisplay();
  
  /* now draw the bottom screen */
  DisplayBluetoothStatusBottomOled();
  
}

static void DisplayBluetoothStatusBottomOled(void)
{
  StartBuildingOledScreen(BottomOled);
  
  /* add phone */
  unsigned char IconIndex;
  if ( QueryPhoneConnected() )
  {
    IconIndex = CHECK_ICON_INDEX;
  }
  else
  {
    IconIndex = X_ICON_INDEX; 
  }
  BuildColumn = 5;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(IconIndex);
  BuildOledScreenAddCharacter(PHONE_ICON_INDEX);  
  
  /* add bluetooth on off */
  if ( QueryBluetoothOn() )
  {
    IconIndex = CHECK_ICON_INDEX;
  }
  else
  {
    IconIndex = X_ICON_INDEX;
  }
  BuildColumn = 26+5;
  BuildOledScreenAddCharacter(IconIndex);
  BuildOledScreenAddCharacter(BLUETOOTH_ICON_INDEX);  
  
  /* add bluetooth on off */
  if ( QueryConnectionState() == Initializing )
  {
    IconIndex = INITIALIZING_ICON_INDEX;
  }
  else if ( QueryDiscoverable() )
  {
    IconIndex = CHECK_ICON_INDEX;
  }
  else
  {
    IconIndex = X_ICON_INDEX;  
  }
  BuildColumn = 52+5;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(IconIndex);
  BuildOledScreenAddCharacter(PAIRING_ICON_INDEX);  
  
  BuildOledScreenSendToDisplay();
  
}

static void DisplayBatteryStatusFace(void)
{
  unsigned int BatteryVoltage = ReadBatterySenseAverage();
 
  StartBuildingOledScreen(TopOled);
  
  /* display a battery icon */
  BuildColumn = 5;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(GetBatteryIcon(BatteryVoltage));
  
  /* display the voltage in volts and drop the last digit */
  BuildColumn = 40;
  DisplayBatteryVoltage(BatteryVoltage);
  BuildOledScreenSendToDisplay();
  
  DisplayBluetoothStatusBottomOled();
  
}

static void DisplayBatteryVoltage(unsigned int BatteryVoltage)
{
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddInteger(BatteryVoltage/1000);
  BuildOledScreenAddCharacter('.');
  BatteryVoltage = BatteryVoltage % 1000;
  BuildOledScreenAddInteger(BatteryVoltage/100);
  BatteryVoltage = BatteryVoltage % 100;
  BuildOledScreenAddInteger(BatteryVoltage/10);
  BuildOledScreenAddString(" V");  
}


/* there wasn't much thought put into the levels */
unsigned char GetBatteryIcon(unsigned int BatteryVoltage)
{
  unsigned char BatteryIconIndex;      
  
  if ( BatteryVoltage > 4000 )
  {
    BatteryIconIndex = BATTERY_LEVEL5_ICON_INDEX;  
  }
  else if ( BatteryVoltage > 3900 )
  {
    BatteryIconIndex = BATTERY_LEVEL4_ICON_INDEX;  
  }
  else if ( BatteryVoltage > 3700 )
  {
    BatteryIconIndex = BATTERY_LEVEL3_ICON_INDEX;  
  }
  else if ( BatteryVoltage > 3500 )
  {
    BatteryIconIndex = BATTERY_LEVEL2_ICON_INDEX;
  }
  else if ( BatteryVoltage > 3300 )
  {
    BatteryIconIndex = BATTERY_LEVEL1_ICON_INDEX;  
  }
  else
  { 
    BatteryIconIndex = BATTERY_LEVEL0_ICON_INDEX;
  }
  
  /* the battery charging icons are the next six icons in the table */
  if ( QueryBatteryCharging() )
  {
    BatteryIconIndex += 6;  
  }
  
  return BatteryIconIndex;
  
}

static void DisplayLowBatteryFace(void)
{
  DisplayLowBatteryIconAndVoltageOnTopOled();
 
  SetFont(MetaWatch16Oled);
  StartBuildingOledScreen(BottomOled);
  BuildOledScreenAddString("Low Battery");
  BuildOledScreenSendToDisplay();
  
  /* force minimum display time of 2 seconds */
  TaskDelayLpmDisable();
  vTaskDelay(2000);
  TaskDelayLpmEnable();
  
}

static void DisplayLowBatteryBluetoothOffFace(void)
{
  DisplayLowBatteryIconAndVoltageOnTopOled();
  
  SetFont(MetaWatchIconOled);
  StartBuildingOledScreen(BottomOled);
  BuildColumn = 35;
  BuildOledScreenAddCharacter(X_ICON_INDEX);
  BuildOledScreenAddCharacter(BLUETOOTH_ICON_INDEX);
  BuildOledScreenSendToDisplay();
  
  /* force minimum display time of 2 seconds */
  TaskDelayLpmDisable();
  vTaskDelay(2000);
  TaskDelayLpmEnable();
  
}

static void DisplayLinkAlarmFace(void)
{
  SetFont(MetaWatch16Oled);
  StartBuildingOledScreen(TopOled);
  BuildOledScreenAddString("Link Lost!");
  BuildOledScreenSendToDisplay();

  SetFont(MetaWatchIconOled);
  StartBuildingOledScreen(BottomOled);
  BuildColumn = 35;
  BuildOledScreenAddCharacter(PHONE_X_ICON_INDEX);
  BuildOledScreenSendToDisplay();
  
}

static void DisplayLowBatteryIconAndVoltageOnTopOled(void)
{
  StartBuildingOledScreen(TopOled);

  /* display a ! battery icon */
  BuildColumn = 5;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(BATTERY_LOW_ICON_INDEX);
  
  /* display the voltage in volts and drop the last digit */
  BuildColumn = 40;
  DisplayBatteryVoltage(ReadBatterySenseAverage());
  BuildOledScreenSendToDisplay();  
}


/* Setup the default text for idle buffer #1 */
static void SetupIdleFace(void)
{
  StartBuildingOledScreenAlternate(&IdlePageTop[IDLE_PAGE1_INDEX]);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString(" Status --> ");
  /* don't send screen to display */  
  pBuildBuffer->Valid = 1;
  
  StartBuildingOledScreenAlternate(&IdlePageBottom[IDLE_PAGE1_INDEX]);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("  Menu --> ");
  /* don't send screen to display */  
  pBuildBuffer->Valid = 1;
  
}

/* display a message indicating that non-volatile values are being saved 
 * to flash 
 */
static void DisplayChangesSavedFace(void)
{
  if ( NvalSettingsChanged )
  {
    NvalSettingsChanged = 0;
    
    StartBuildingOledScreen(TopOled);
    SetFont(MetaWatch16Oled);
    BuildOledScreenAddString("   Changes");
    BuildOledScreenChangeDisplayTimeout(ONE_SECOND);
    BuildOledScreenSendToDisplay();
   
    StartBuildingOledScreen(BottomOled);
    SetFont(MetaWatch16Oled);
    BuildOledScreenAddString("    Saved");
    BuildOledScreenChangeDisplayTimeout(ONE_SECOND);
    BuildOledScreenSendToDisplay();
  }
  else
  {
    /* blank the display if we were in crown pull mode */
    StartBuildingOledScreen(TopOled);
    BuildOledScreenChangeDisplayTimeout(ONE_SECOND);
    BuildOledScreenSendToDisplay();
   
    StartBuildingOledScreen(BottomOled);
    BuildOledScreenChangeDisplayTimeout(ONE_SECOND);
    BuildOledScreenSendToDisplay();
    
  }
  
}

static void DisplayToggleOnTopOled(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString(" Toggle -->");
  BuildOledScreenSendToDisplay();
}

#define STARTING_COLUMN_FOR_CENTERED_ICON ( 30 )

static void DisplayBluetoothToggleFace(void)
{
  DisplayToggleOnTopOled();
  
  StartBuildingOledScreen(BottomOled);
  unsigned char IconIndex;
  if ( QueryConnectionState() == Initializing )
  {
    IconIndex = INITIALIZING_ICON_INDEX;
  }
  else if ( QueryBluetoothOn() )
  {
    IconIndex = CHECK_ICON_INDEX;
  }
  else
  {
    IconIndex = X_ICON_INDEX; 
  }
  BuildColumn = STARTING_COLUMN_FOR_CENTERED_ICON;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(IconIndex);
  BuildOledScreenAddCharacter(BLUETOOTH_ICON_INDEX); 
  BuildOledScreenSendToDisplay();
  
}

static void DisplayLinkAlarmToggleFace(void)
{
  DisplayToggleOnTopOled();
  
  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch7Oled);
  if ( QueryLinkAlarmEnable() )
  {
    BuildOledScreenAddString("Link Alarm On");  
  }
  else
  {
    BuildOledScreenAddString("Link Alarm Off");
  }
  BuildOledScreenSendToDisplay();
  
}

static void DisplayPairiabilityFace(void)
{
  DisplayToggleOnTopOled();

  StartBuildingOledScreen(BottomOled);
  unsigned char IconIndex;
  
  
  if ( QueryConnectionState() == Initializing )
  {
    IconIndex = INITIALIZING_ICON_INDEX;
  }
  else if ( QueryDiscoverable() )
  {
    IconIndex = CHECK_ICON_INDEX;
  }
  else
  {
    IconIndex = X_ICON_INDEX; 
  }
  BuildColumn = STARTING_COLUMN_FOR_CENTERED_ICON;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(IconIndex);
  BuildOledScreenAddCharacter(PAIRING_ICON_INDEX);
  BuildOledScreenSendToDisplay();
  
}

static void DisplayRstNmiConfigurationFace(void)
{
  DisplayToggleOnTopOled();
  
  StartBuildingOledScreen(BottomOled);
  
  unsigned char FirstIconIndex;
  unsigned char SecondIconIndex;
  
  if ( QueryRstPinEnabled() )
  {
    FirstIconIndex = CHECK_ICON_INDEX;
    SecondIconIndex = X_ICON_INDEX;
  }
  else
  {
    FirstIconIndex = X_ICON_INDEX;
    SecondIconIndex = CHECK_ICON_INDEX;  
  }

  BuildColumn = 5;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(FirstIconIndex);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("RST ");
  
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(SecondIconIndex);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("NMI");
  BuildOledScreenSendToDisplay();
  
}

static void DisplaySspFace(void)
{
  DisplayToggleOnTopOled();

  StartBuildingOledScreen(BottomOled);
  unsigned char IconIndex;
  
  if ( QueryConnectionState() == Initializing )
  {
    IconIndex = INITIALIZING_ICON_INDEX;
  }
  else if ( QuerySecureSimplePairingEnabled() )
  {
    IconIndex = CHECK_ICON_INDEX;
  }
  else
  {
    IconIndex = X_ICON_INDEX; 
  }
  BuildColumn = 5;
  SetFont(MetaWatchIconOled);
  BuildOledScreenAddCharacter(IconIndex);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("SSP (Beta)");
  BuildOledScreenSendToDisplay();  
  
}

static void DisplayResetWatchFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString(" Reset --> ");
  BuildOledScreenSendToDisplay();
  
  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("Reset Watch");
  BuildOledScreenSendToDisplay();
   
}

static void DisplayCrownPullFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString(" Turn Crown");
  BuildOledScreenAddNewline();
  SetFont(MetaWatch5Oled);
  BuildOledScreenAddString("  To Set Time");
  BuildOledScreenSendToDisplay();
  
  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString(" Next -->");
  BuildOledScreenSendToDisplay();
  
}

static void DisplayTopContrastFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("OLED Level");
  BuildOledScreenSendToDisplay();
  
  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString("OLD:");
  BuildOledScreenAddInteger(LastTopContrastIndex);
  BuildColumn = 55;
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddInteger(nvTopOledContrastIndexDay);
  BuildOledScreenSendToDisplay();
      
}

static void DisplayBottomContrastFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString("OLD:");
  BuildOledScreenAddInteger(LastBottomContrastIndex);
  BuildColumn = 55;
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddInteger(nvBottomOledContrastIndexDay);
  BuildOledScreenSendToDisplay();

  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString("OLED Level");
  BuildOledScreenSendToDisplay();       
}

static void DisplayMasterResetFace(void)
{
  StartBuildingOledScreen(TopOled);
  SetFont(MetaWatch7Oled);
  BuildOledScreenAddString("Master Reset");
  BuildOledScreenAddNewline();
  SetFont(MetaWatch5Oled);
  BuildOledScreenAddString("   (Hold)");
  BuildOledScreenSendToDisplay();
  
  StartBuildingOledScreen(BottomOled);
  SetFont(MetaWatch16Oled);
  BuildOledScreenAddString(" Next -->");
  BuildOledScreenSendToDisplay();
}

static void InitializeDisplayTimers(void)
{
  ScreenTimerId = AllocateOneSecondTimer();
  
  SetupOneSecondTimer(ScreenTimerId,
                      ONE_SECOND*2,
                      NO_REPEAT,
                      DISPLAY_QINDEX,
                      WatchDrawnScreenTimeout,
                      NO_MSG_OPTIONS);
    
}


void InitializeContrastValues(void)
{
  nvTopOledContrastIndexDay = 4;
  nvBottomOledContrastIndexDay = 4;
  nvTopOledContrastIndexNight = 4;
  nvBottomOledContrastIndexNight = 4;
  LastTopContrastIndex = nvTopOledContrastIndexDay;
  LastBottomContrastIndex = nvBottomOledContrastIndexDay;
  
  OsalNvItemInit(NVID_TOP_OLED_CONTRAST_DAY,
                 sizeof(nvTopOledContrastIndexDay),
                 &nvTopOledContrastIndexDay);
  
  OsalNvItemInit(NVID_BOTTOM_OLED_CONTRAST_DAY,
                 sizeof(nvBottomOledContrastIndexDay),
                 &nvBottomOledContrastIndexDay);
  
  OsalNvItemInit(NVID_TOP_OLED_CONTRAST_NIGHT,
                 sizeof(nvTopOledContrastIndexNight),
                 &nvTopOledContrastIndexNight);
  
  OsalNvItemInit(NVID_BOTTOM_OLED_CONTRAST_NIGHT,
                 sizeof(nvBottomOledContrastIndexNight),
                 &nvBottomOledContrastIndexNight);
  
}

static void SaveContrastVales(void)
{
  LastTopContrastIndex = nvTopOledContrastIndexDay;
  LastBottomContrastIndex = nvBottomOledContrastIndexDay;
    
  OsalNvWrite(NVID_TOP_OLED_CONTRAST_DAY,
              NV_ZERO_OFFSET,
              sizeof(nvTopOledContrastIndexDay),
              &nvTopOledContrastIndexDay);
  
  OsalNvWrite(NVID_BOTTOM_OLED_CONTRAST_DAY,
              NV_ZERO_OFFSET,
              sizeof(nvBottomOledContrastIndexDay),
              &nvBottomOledContrastIndexDay);
  
  OsalNvWrite(NVID_TOP_OLED_CONTRAST_NIGHT,
              NV_ZERO_OFFSET,
              sizeof(nvTopOledContrastIndexNight),
              &nvTopOledContrastIndexNight);
  
  OsalNvWrite(NVID_BOTTOM_OLED_CONTRAST_NIGHT,
              NV_ZERO_OFFSET,
              sizeof(nvBottomOledContrastIndexNight),
              &nvBottomOledContrastIndexNight);
}

void InitializeDisplayTimeouts(void)
{
  nvIdleDisplayTimeout = DEFAULT_IDLE_DISPLAY_TIMEOUT;
  nvApplicationDisplayTimeout = DEFAULT_APPLICATION_DISPLAY_TIMEOUT;
  nvNotificationDisplayTimeout = DEFAULT_NOTIFICATION_DISPLAY_TIMEOUT;
  
  OsalNvItemInit(NVID_IDLE_DISPLAY_TIMEOUT,
                 sizeof(nvIdleDisplayTimeout),
                 &nvIdleDisplayTimeout);
  
  OsalNvItemInit(NVID_APPLICATION_DISPLAY_TIMEOUT,
                 sizeof(nvApplicationDisplayTimeout),
                 &nvApplicationDisplayTimeout);
  
  OsalNvItemInit(NVID_NOTIFICATION_DISPLAY_TIMEOUT,
                 sizeof(nvNotificationDisplayTimeout),
                 &nvNotificationDisplayTimeout);
 
}

static void ChangeModeHandler(unsigned char Mode)
{
  LastMode = CurrentMode;
  CurrentMode = Mode;
  
  switch ( CurrentMode )
  {
  
  case IDLE_MODE:

    ReturnToApplicationMode = 0;
    
    if ( LastMode != CurrentMode )
    {
      PrintString("Changing mode to Idle\r\n");
      ChangeAnalogButtonConfiguration(IdleButtonMode);
    }
    else
    {
      PrintString("Already in Idle mode\r\n");
    }
    
    StopOneSecondTimer(ModeTimerId);
    break;
  
  case APPLICATION_MODE:
    ReturnToApplicationMode = 1;
    PrintString("Changing mode to Application\r\n");
    
    StopOneSecondTimer(ModeTimerId);
    
    SetupOneSecondTimer(ModeTimerId,
                        QueryApplicationModeTimeout(),
                        NO_REPEAT,
                        DISPLAY_QINDEX,
                        ModeTimeoutMsg,
                        APPLICATION_MODE);
    
    StartOneSecondTimer(ModeTimerId);
    
    break;
  
  case NOTIFICATION_MODE:
    
    PrintString("Changing mode to Notification\r\n");
    
    StopOneSecondTimer(ModeTimerId);
    
    SetupOneSecondTimer(ModeTimerId,
                        QueryNotificationModeTimeout(),
                        NO_REPEAT,
                        DISPLAY_QINDEX,
                        ModeTimeoutMsg,
                        NOTIFICATION_MODE);
    
    StartOneSecondTimer(ModeTimerId);
    
    break;

  default:
    break;
  }

}

static void ModeTimeoutHandler(unsigned char CurrentMode)
{
  switch ( CurrentMode )
  {
  
  case IDLE_MODE:
    break;
  
  case APPLICATION_MODE:
    ChangeModeHandler(IDLE_MODE);
    break;
    
  case NOTIFICATION_MODE:
    if ( ReturnToApplicationMode )
    {
      ReturnToApplicationMode = 0;
      ChangeModeHandler(APPLICATION_MODE);
    }
    else
    {
      ChangeModeHandler(IDLE_MODE);  
    }
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


#define TOGGLE_BLUETOOTH_PAGE             ( 0 )
#define TOGGLE_LINK_ALARM_PAGE            ( 1 )
#define TOGGLE_DISCOVERABILITY_PAGE       ( 2 )
#define TOGGLE_RST_NMI_PAGE               ( 3 )
#define TOGGLE_SECURE_SIMPLE_PAIRING_PAGE ( 4 )
#define RESET_WATCH_PAGE                  ( 5 )
#define TOTAL_PAGES                       ( 6 )

static void MenuModeHandler(unsigned char MsgOptions)
{
  if (   TopDisplayOn 
      && CurrentButtonConfiguration == MenuButtonMode )
  {
    if ( MsgOptions != MENU_MODE_OPTION_UPDATE_CURRENT_PAGE)
    { 
      SelectNextMenuPage();  
    }
  } 
  else
  {
    CurrentMenuPage = TOGGLE_BLUETOOTH_PAGE; 
  }
  
  ChangeAnalogButtonConfiguration(MenuButtonMode);
  
  switch (CurrentMenuPage)
  {
  case TOGGLE_BLUETOOTH_PAGE:
    DisplayBluetoothToggleFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       MenuButtonMsg,
                       MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH);
    break;
   
  case TOGGLE_LINK_ALARM_PAGE:
    DisplayLinkAlarmToggleFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       MenuButtonMsg,
                       MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM);
    
    break;
    
  case TOGGLE_DISCOVERABILITY_PAGE:
    DisplayPairiabilityFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       MenuButtonMsg,
                       MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY);
    
    break;
    
  case TOGGLE_RST_NMI_PAGE:
    DisplayRstNmiConfigurationFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       MenuButtonMsg,
                       MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN);
    
    break;
    
  case TOGGLE_SECURE_SIMPLE_PAIRING_PAGE:
    DisplaySspFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       MenuButtonMsg,
                       MENU_BUTTON_OPTION_TOGGLE_SECURE_SIMPLE_PAIRING);
    
    break;
    
  case RESET_WATCH_PAGE:
    DisplayResetWatchFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       SoftwareResetMsg,
                       NO_MSG_OPTIONS);
        
    break;
    
  default:
    break;
    
  }
}

static void InitializeMenuPage(void)
{
  CurrentMenuPage = TOGGLE_BLUETOOTH_PAGE;  
}

static void SelectNextMenuPage(void)
{
  CurrentMenuPage++;
  if ( CurrentMenuPage >= TOTAL_PAGES )
  {
    CurrentMenuPage = 0;  
  } 
}

static void MenuButtonHandler(unsigned char MsgOptions)
{
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
    NvalSettingsChanged = 1;
    break;
  
  case MENU_BUTTON_OPTION_EXIT:          
    MenuExitHandler();
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
      NvalSettingsChanged = 1;
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
    NvalSettingsChanged = 1;    
    break;

  default:
    break;
  }

}

static void MenuExitHandler(void)
{
  /* change the 'mode' we are in so the connection state change
   * message from the stack does not cause a menu mode screen update to 
   * occur after this function finishes
   */
  ChangeAnalogButtonConfiguration(IdleButtonMode);
    
  /* save all of the non-volatile items 
   *
  */
  tMessage OutgoingMsg;
  SetupMessage(&OutgoingMsg,PairingControlMsg,PAIRING_CONTROL_OPTION_SAVE_SPP);
  RouteMsg(&OutgoingMsg);
  
  /**/
  SaveLinkAlarmEnable();
  SaveRstNmiConfiguration();
    
  DisplayChangesSavedFace();    
}

/* Display the buffer loaded by the phone */
static void ShowIdleBufferHandler(void)
{
  GetNextActiveBufferIndex();
  
  DisplayBuffer(&IdlePageTop[IdlePageIndex]);
  DisplayBuffer(&IdlePageBottom[IdlePageIndex]);
  
}

/* determine if the idle buffer should be displayed
 * right now there are only two ...
 */
static void GetNextActiveBufferIndex(void)
{
  unsigned char i;
  for(i = 0; i < TOTAL_IDLE_BUFFERS; i++)
  {
    IdlePageIndex++;
    if ( IdlePageIndex >= TOTAL_IDLE_BUFFERS )
    {
      IdlePageIndex = 0;  
    }
    
    if ( IdlePageTop[IdlePageIndex].Valid )
    {
      break;  
    }
  }

}

#define TURN_CROWN_PAGE        ( 0 )
#define TOP_CONTRAST_PAGE      ( 1 )
#define BOTTOM_CONTRAST_PAGE   ( 2 )
#define MASTER_RESET_PAGE      ( 3 )
#define TOTAL_CROWN_MENU_PAGES ( 4 )
static unsigned char CurrentCrownMenuPage = 0;

static void OledCrownMenuHandler(unsigned char MsgOptions)
{
  if (   TopDisplayOn 
      && CurrentButtonConfiguration == CrownMenuButtonMode )
  {
    if ( MsgOptions != OLED_CROWN_MENU_MODE_OPTION_UPDATE_CURRENT_PAGE )
    {     
      CurrentCrownMenuPage++;
      if ( CurrentCrownMenuPage >= TOTAL_CROWN_MENU_PAGES )
      {
        CurrentCrownMenuPage = 0;  
      }
    }
  } 
  else
  {
    CurrentCrownMenuPage = TURN_CROWN_PAGE; 
  }
  
  ChangeAnalogButtonConfiguration(CrownMenuButtonMode);
  
  switch (CurrentCrownMenuPage)
  {
  case TURN_CROWN_PAGE:
    DisplayCrownPullFace();
    
    DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                        SW_A_INDEX,
                        BUTTON_STATE_IMMEDIATE);
    break;
   
  case TOP_CONTRAST_PAGE:
    DisplayTopContrastFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       OledCrownMenuButtonMsg,
                       OLED_CROWN_MENU_BUTTON_OPTION_TOP_CONTRAST);
    
    break;
    
       
  case BOTTOM_CONTRAST_PAGE:
    DisplayBottomContrastFace();
    
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_IMMEDIATE,
                       OledCrownMenuButtonMsg,
                       OLED_CROWN_MENU_BUTTON_OPTION_BOTTOM_CONTRAST);
    
    break;
    
  case MASTER_RESET_PAGE:
    DisplayMasterResetFace();
    
    DisableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                        SW_A_INDEX,
                        BUTTON_STATE_IMMEDIATE);
        
    EnableButtonAction(WATCH_DRAWN_SCREEN_BUTTON_MODE,
                       SW_A_INDEX,
                       BUTTON_STATE_LONG_HOLD,
                       SoftwareResetMsg,
                       MASTER_RESET_OPTION);
    break;
   
  default:
    break;
    
  }
}

static void OledCrownMenuButtonHandler(unsigned char MsgOptions)
{
  switch (MsgOptions)
  {

  case OLED_CROWN_MENU_BUTTON_OPTION_EXIT:          
    
    ChangeAnalogButtonConfiguration(IdleButtonMode);
    SaveContrastVales();
    DisplayChangesSavedFace();  
    break;
    
  case OLED_CROWN_MENU_BUTTON_OPTION_TOP_CONTRAST:
    
    nvTopOledContrastIndexDay++;
    if ( nvTopOledContrastIndexDay >= TOTAL_CONTRAST_VALUES )
    {
      nvTopOledContrastIndexDay = 0;  
    }
    SetOledDeviceAddress(TopOled);
    WriteOneByteOledCommand(OLED_CMD_CONTRAST);
    WriteOneByteOledCommand(ContrastTable[nvTopOledContrastIndexDay]);
    NvalSettingsChanged = 1;
    OledCrownMenuHandler(OLED_CROWN_MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;
    
  case OLED_CROWN_MENU_BUTTON_OPTION_BOTTOM_CONTRAST:
    
    nvBottomOledContrastIndexDay++;
    if ( nvBottomOledContrastIndexDay >= TOTAL_CONTRAST_VALUES )
    {
      nvBottomOledContrastIndexDay = 0;  
    }
    SetOledDeviceAddress(BottomOled);
    WriteOneByteOledCommand(OLED_CMD_CONTRAST);
    WriteOneByteOledCommand(ContrastTable[nvBottomOledContrastIndexDay]);
    NvalSettingsChanged = 1;
    OledCrownMenuHandler(OLED_CROWN_MENU_MODE_OPTION_UPDATE_CURRENT_PAGE);
    break;

  default:
    break;
  }

}






/* bitfontcreator settings little endian, col scan, preferred row, not packed */
const tImageBuffer MetaWatchLogoBuffer = 
{ 
  IDLE_MODE,
  1,
  TopOled,
  {
    0xFF, 0xFF, 0x3F, 0x07, 0xC7, 0x07, 0x3F, 0xFF, 
    0x3F, 0x07, 0xC7, 0x07, 0x3F, 0xFF, 0xFF, 0x07, 
    0x07, 0x67, 0x67, 0x67, 0xFF, 0xE7, 0xE7, 0x07, 
    0x07, 0xE7, 0xE7, 0xFF, 0x7F, 0x1F, 0x87, 0x87, 
    0x1F, 0x7F, 0xE7, 0x9F, 0x7F, 0xFF, 0xFF, 0xFF, 
    0x7F, 0x9F, 0xE7, 0x9F, 0x7F, 0xFF, 0xFF, 0xFF, 
    0x7F, 0x9F, 0xE7, 0x7F, 0x9F, 0xE7, 0x9F, 0x7F, 
    0xF7, 0xF7, 0xF7, 0x07, 0xF7, 0xF7, 0x37, 0xCF, 
    0xEF, 0xF7, 0xF7, 0xF7, 0xF7, 0xEF, 0xCF, 0xFF, 
    0x07, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x07, 0xFF, 
    0xFF, 0xE3, 0xE0, 0xFC, 0xFF, 0xFC, 0xE0, 0xE1, 
    0xE0, 0xFC, 0xFF, 0xFC, 0xE0, 0xE3, 0xFF, 0xE0, 
    0xE0, 0xE6, 0xE6, 0xE6, 0xFF, 0xFF, 0xFF, 0xE0, 
    0xE0, 0xFF, 0xE7, 0xE1, 0xF8, 0xF8, 0xF9, 0xF9, 
    0xF8, 0xF8, 0xE1, 0xE7, 0xFE, 0xF9, 0xE7, 0xF9, 
    0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF9, 0xE7, 0xF9, 
    0xFE, 0xE7, 0xF9, 0xFC, 0xFD, 0xFD, 0xFD, 0xFC, 
    0xF9, 0xE7, 0xFF, 0xE0, 0xFF, 0xFF, 0xFC, 0xF3, 
    0xF7, 0xEF, 0xEF, 0xEF, 0xEF, 0xF7, 0xF3, 0xFF, 
    0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0xFF,  
  }
};

unsigned char ScrollTimerCallbackIsr(void)
{
  unsigned char ExitLpm = 1;

  tMessage Msg;
  SetupMessage(&Msg,OledScrollMsg,NO_MSG_OPTIONS);
  SendMessageToQueueFromIsr(DISPLAY_QINDEX,&Msg);
  
  return ExitLpm;
}

static void StartScrollTimer(void)
{
  StartCrystalTimer(CRYSTAL_TIMER_ID2,
                    ScrollTimerCallbackIsr,
                    50);
  
  
}

static void StopScrollTimer(void)
{
  StopCrystalTimer(CRYSTAL_TIMER_ID2);
}
