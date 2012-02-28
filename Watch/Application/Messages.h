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
/*! \file Messages.h
 *
 */
/******************************************************************************/

#ifndef MESSAGES_H
#define MESSAGES_H

/*******************************************************************************
    Description:  Constants related to the host packet format
*******************************************************************************/
#define HOST_MSG_BUFFER_LENGTH  ( 32 )
#define HOST_MSG_HEADER_LENGTH  ( 4 )
#define HOST_MSG_CRC_LENGTH     ( 2 )
#define HOST_MSG_START_FLAG     ( 0x01 )    
/* 26 */
#define HOST_MSG_MAX_PAYLOAD_LENGTH \
  (HOST_MSG_BUFFER_LENGTH - HOST_MSG_HEADER_LENGTH - HOST_MSG_CRC_LENGTH)


/*! Message Format
 *
 * \param Length is the length of the message.  This is only used for messages
 * sent to the host
 * \param Type is the type of the message
 * \param Options is the option byte of the message
 * \param pBuffer can point to a message buffer 
 *
 */
typedef struct
{
  unsigned char Length;
  unsigned char Type;
  unsigned char Options;
  unsigned char * pBuffer; 

} tMessage;



/*! Host Message Packet Format
 * 
 * \note This message format is also used internally but not all fields are used.
 *
 * \param startByte is always 0x01
 * \param Length is total number of bytes including start and crc
 * \param Type is the message type
 * \param Options is a byte to hold message options
 * \param pPayload is an array of bytes
 * \param crcLsb
 * \param crcMsb
 * 
 * \note
 * The CRC is CCITT 16 intialized with 0xFFFF and bit reversed generation
 * not pretty, but it's what the MSP430 hardware does. A test vector is:
 *
 * CRC({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39}) = 0x89F6
 *
 * SPP can deliver a partial packet if the link times out so bytes are needed to 
 * re-assemble/frame a message.
 * 
 * The Get Device Type message is 0x01, 0x06, 0x01, 0x00, 0x0B, 0xD9.
 *
 * \note Deprecated - This format is not used internally BUT IS STILL THE 
 * FORMAT FOR MESSAGES SENT TO THE HOST 
 */
#if 0
typedef struct
{
  unsigned char startByte;
  unsigned char Length;
  unsigned char Type;
  unsigned char Options;
  unsigned char pPayload[HOST_MSG_MAX_PAYLOAD_LENGTH];
  unsigned char crcLsb;
  unsigned char crcMsb;

} tHostMsg;

#endif

#define NO_MSG_OPTIONS      ( 0 )
#define NONZERO_MSG_OPTIONS ( 0xff )

#define HOST_MSG_TYPE_INDEX ( 2 )

/* defines for write buffer command */
#define WRITE_BUFFER_TWO_LINES     ( 0x00 )
#define WRITE_BUFFER_ONE_LINE      ( 0x10 )
#define WRITE_BUFFER_ONE_LINE_MASK ( 0x10 )

/*! The serial ram message is formatted so that it can be overlaid onto the
 * message from the host (so that a buffer allocation does not have to be
 * performed and there is one less message copy).
 *
 * \param Reserved0
 * \param Reserved1
 * \param SerialRamCommand is the command for the serial ram (read/write)
 * \param AddressMsb
 * \param AddressLsb
 * \param pLineA[12] is pixel data (and AddressMsb2 for second write)
 * \param AddressLsb2
 * \param pLineB[12]
 * \param Reserved31
 *
 */
typedef struct
{
  unsigned char RowSelectA;
  unsigned char pLineA[12];
  unsigned char RowSelectB;
  unsigned char pLineB[12];

} tSerialRamPayload;

/*! The LCD message is formatted so that it can be written directly to the LCD
 *
 * \param LcdCommand is the lcd write command for the LCD
 * \param RowNumber is the row on the LCD
 * \param pLine[12] is a line of LCD data
 * \param Dummy1
 * \param Dummy2
 *
 */
typedef struct
{
  unsigned char Reserved0;
  unsigned char Reserved1;
  unsigned char LcdCommand;
  unsigned char RowNumber;
  unsigned char pLine[12];
  unsigned char Dummy1;
  unsigned char Dummy2;
  
} tLcdMessagePayload;

#define LCD_MESSAGE_CMD_INDEX        ( 2 ) 
#define LCD_MESSAGE_ROW_NUMBER_INDEX ( 3 )
#define LCD_MESSAGE_LINE_INDEX       ( 4 )


/*! Message type enumeration 
 * 
 * for this processor the default is 16 bits for an enumeration
 */
typedef enum
{
  InvalidMessage = 0x00,
  
  GetDeviceType = 0x01,
  GetDeviceTypeResponse = 0x02,
  GetInfoString = 0x03,
  GetInfoStringResponse = 0x04,
  DiagnosticLoopback = 0x05,  
  EnterShippingModeMsg = 0x06,
  SoftwareResetMsg = 0x07,
  ConnectionTimeoutMsg = 0x08,
  TurnRadioOnMsg = 0x09,
  TurnRadioOffMsg = 0x0a,
  ReadRssiMsg = 0x0b,
  PairingControlMsg = 0x0c,
  ReadRssiResponseMsg = 0x0d,
  SniffControlMsg = 0x0e,
  LinkAlarmMsg = 0x0f,
  
  /*
   * OLED display related commands
   */
  OledWriteBufferMsg = 0x10,
  OledConfigureModeMsg = 0x11,
  OledChangeModeMsg = 0x12,
  OledWriteScrollBufferMsg = 0x13,
  OledScrollMsg = 0x14,
  OledShowIdleBufferMsg = 0x15,
  OledCrownMenuMsg = 0x16, 
  OledCrownMenuButtonMsg = 0x17,
  
  /* 
   * Status and control
   */
   
  /* move the hands hours, mins and seconds */
  AdvanceWatchHandsMsg = 0x20,  

  /* config and (dis)enable vibrate */
  SetVibrateMode = 0x23,
  
  ButtonStateMsg = 0x24,
  
  /* Sets the RTC */
  SetRealTimeClock = 0x26,  
  GetRealTimeClock = 0x27,
  GetRealTimeClockResponse = 0x28,
  
  /* osal nv */
  NvalOperationMsg = 0x30,
  NvalOperationResponseMsg = 0x31,
  
  /* status of the current display operation */  
  StatusChangeEvent = 0x33,
  
  ButtonEventMsg = 0x34,
  
  GeneralPurposePhoneMsg = 0x35,
  GeneralPurposeWatchMsg = 0x36,
  /*
   * LCD display related commands
   */ 
  WriteBuffer = 0x40,
  ConfigureDisplay = 0x41,
  ConfigureIdleBufferSize = 0x42,
  UpdateDisplay = 0x43,
  LoadTemplate = 0x44,
  Unused_0x45 = 0x45, 
  EnableButtonMsg = 0x46,
  DisableButtonMsg = 0x47,
  ReadButtonConfigMsg = 0x48,
  ReadButtonConfigResponse = 0x49,
  Unused_0x4a = 0x4a,
  
  /* */
  BatteryChargeControl = 0x52,
  BatteryConfigMsg = 0x53,
  LowBatteryWarningMsgHost = 0x54,
  LowBatteryBtOffMsgHost = 0x55,
  ReadBatteryVoltageMsg = 0x56,
  ReadBatteryVoltageResponse = 0x57,
  ReadLightSensorMsg = 0x58,
  ReadLightSensorResponse = 0x59,
  LowBatteryWarningMsg = 0x5a,
  LowBatteryBtOffMsg = 0x5b,
  
  /*****************************************************************************
   *
   * User Reserved 0x60-0x70-0x80-0x90
   *
   ****************************************************************************/
  
  /*****************************************************************************
   *
   * Watch/Internal Use Only
   *
   ****************************************************************************/
  IdleUpdate = 0xa0,
  WatchDrawnScreenTimeout = 0xa2,
  SplashTimeoutMsg = 0xa3,
  Unused_0xa4 = 0xa4,
  Unused_0xa5 = 0xa5,
  ChangeModeMsg = 0xa6,
  ModeTimeoutMsg = 0xa7,
  WatchStatusMsg = 0xa8,
  MenuModeMsg = 0xa9,
  BarCode = 0xaa,
  ListPairedDevicesMsg = 0xab,
  ConnectionStateChangeMsg = 0xac,
  ModifyTimeMsg = 0xad,
  MenuButtonMsg = 0xae,
  ToggleSecondsMsg = 0xaf,
  
  LedChange = 0xc0,
  
  QueryMemoryMsg = 0xd0,
    
  AccelerometerHostMsg = 0xe0,
  AccelerometerEnableMsg  = 0xe1,
  AccelerometerDisableMsg = 0xe2,
  AccelerometerSendDataMsg = 0xe3,
  AccelerometerAccessMsg = 0xe4,
  AccelerometerResponseMsg = 0xe5,
  AccelerometerSetupMsg = 0xe6,
  
  SniffControlAckMsg = 0xf0,
  SniffStateChangeMsg = 0xf1
  
} eMessageType;


#define LED_OFF_OPTION      ( 0x00 )
#define LED_ON_OPTION       ( 0x01 )
#define LED_TOGGLE_OPTION   ( 0x02 )
#define LED_START_OFF_TIMER ( 0x03 )

/******************************************************************************/

#define PAGE_CONTROL_MASK                 ( 0x70 )
#define PAGE_CONTROL_RESERVED             ( 0x00 )
#define PAGE_CONTROL_INVALIDATE           ( 0x10 )
#define PAGE_CONTROL_INVALIDATE_AND_CLEAR ( 0x20 )
#define PAGE_CONTROL_INVALIDATE_AND_FILL  ( 0x30 )
#define PAGE_CONTROL_ACTIVATE             ( 0x40 )

#define WRITE_OLED_BUFFER_MAX_PAYLOAD ( HOST_MSG_MAX_PAYLOAD_LENGTH - 3 )

/*!
 * 
 * \param BufferSelect is the buffer to write to (for example idle top)
 * \param Column is the starting column address
 * \param Size is the number of columns
 * \param pPayload[WRITE_OLED_BUFFER_MAX_PAYLOAD] is an array holding column data
 */
typedef struct
{
  unsigned char BufferSelect;
  unsigned char Column;
  unsigned char Size;
  unsigned char pPayload[WRITE_OLED_BUFFER_MAX_PAYLOAD];  

} tWriteOledBufferPayload;

#define WRITE_SCROLL_BUFFER_MAX_PAYLOAD ( HOST_MSG_MAX_PAYLOAD_LENGTH - 1 )

/*!  
 * \param Size is the number of columns being written
 * \param pPayload[WRITE_SCROLL_BUFFER_MAX_PAYLOAD] is the column data
 *
 * \note Only the bottom row of the bottom OLED can scroll at this time.
 */
typedef struct
{
  unsigned char Size;
  unsigned char pPayload[WRITE_SCROLL_BUFFER_MAX_PAYLOAD];  

} tWriteScrollBufferPayload;

/******************************************************************************/

/* configure mode - oled version */

#define PARAMETER_SELECT_MASK            ( 0x70 )
#define PARAMETER_SELECT_RESERVED        ( 0x10 )
#define PARAMETER_SELECT_DISPLAY_TIMEOUT ( 0x20 )
#define PARAMETER_SELECT_MODE_TIMEOUT    ( 0x30 )

/******************************************************************************/

/*! Advance Watch Hands Payload Structure
 *
 * \param Hours to advance (12 max)
 * \param Minutes to advance (60 max)
 * \param Seconds to advance (60 max)
 */
typedef struct
{
  unsigned char Hours;    
  unsigned char Minutes;  
  unsigned char Seconds; 

} tAdvanceWatchHandsPayload;

/*! Set Vibrate Mode Payload Structure
 *
 * \param Enable when > 0 disabled when == 0.                    
 * \param OnDuration is the duration in milliseconds
 * \param OffDuration is the off duration in milliseconds.
 * \param NumberOfCycles is the number of on/off cycles to perform
 *
 * \note when durations were changed to integers the on duration was correct
 * but the number of cycles became first byte of checksum (packing problem)
 */
typedef struct
{
  unsigned char Enable;                     
  unsigned char OnDurationLsb;
  unsigned char OnDurationMsb;
  unsigned char OffDurationLsb;
  unsigned char OffDurationMsb;
  unsigned char NumberOfCycles;

} tSetVibrateModePayload;

/*! 
 * \param Year is a 12 bit value
 * \param Month of the year - 1 to 12
 * \param DayOfMonth is 1 to 31
 * \param DayOfWeek is 0 to 6
 * \param Hour is 0 to 24
 * \param Minute is 0 to 59
 * \param Second is 0 to 59
 */
typedef struct
{
  unsigned char YearMsb;
  unsigned char YearLsb;
  unsigned char Month;          
  unsigned char DayOfMonth;     
  unsigned char DayOfWeek;      
  unsigned char Hour;           
  unsigned char Minute;         
  unsigned char Second;	        

} tRtcHostMsgPayload;

/*! Load Template Strucutre 
 *
 * /param TemplateSelect (the first bye of payload) selects what will be filled
 * into display memory.
 */  
typedef struct
{
  unsigned char TemplateSelect;
  
} tLoadTemplatePayload;

/* options */
#define IDLE_BUFFER_SELECT         ( 0x00 )
#define APPLICATION_BUFFER_SELECT  ( 0x01 )
#define NOTIFICATION_BUFFER_SELECT ( 0x02 )
#define SCROLL_BUFFER_SELECT       ( 0x03 )
#define BUFFER_SELECT_MASK         ( 0x0F )

/* make mode definitions the same as buffer definitions */
#define IDLE_MODE         ( IDLE_BUFFER_SELECT )
#define APPLICATION_MODE  ( APPLICATION_BUFFER_SELECT )
#define NOTIFICATION_MODE ( NOTIFICATION_BUFFER_SELECT )
#define SCROLL_MODE       ( SCROLL_BUFFER_SELECT )
#define NUMBER_OF_MODES   ( 4 )
#define MODE_MASK         ( BUFFER_SELECT_MASK )

/* these should match the display modes for idle, application, notification,
 * and scroll modes
 */
#define NORMAL_IDLE_SCREEN_BUTTON_MODE ( 0 )
#define APPLICATION_SCREEN_BUTTON_MODE ( 1 )
#define NOTIFICATION_BUTTON_MODE       ( 2 )
#define SCROLL_BUTTON_MODE             ( 3 )
#define WATCH_DRAWN_SCREEN_BUTTON_MODE ( 4 )
#define NUMBER_OF_BUTTON_MODES         ( 5 )

#define UPDATE_COPY_MASK                       ( BIT4 )
#define COPY_ACTIVE_TO_DRAW_DURING_UPDATE      ( BIT4 )
#define NO_COPY_DURING_UPDATE                  ( 0    )

#define DRAW_BUFFER_ACTIVATION_MASK ( BIT5 )
#define ACTIVATE_DRAW_BUFFER        ( 0 )
#define DONT_ACTIVATE_DRAW_BUFFER   ( BIT5 )

#define IDLE_TIMER_UPDATE_TYPE_MASK ( BIT6 )
#define RESTART_IDLE_TIMER          ( BIT6 )
#define PERIODIC_IDLE_UPDATE        ( 0 )

/* button option */
#define RESET_DISPLAY_TIMER ( 1 )

/* configure mode option */
#define SAVE_MODE_CONFIGURATION_MASK ( BIT4 )

/*! 
 * \param DisplayMode is Idle, Application, or Notification
 * \param ButtonIndex is the button index
 * \param ButtonPressType is immediate, pressed, hold, or long hold
 * \param CallbackMsgType is the callback message type for the button event
 * \param CallbackMsgOptions is the options to send with the message
 */
typedef struct
{
  unsigned char DisplayMode;
  unsigned char ButtonIndex;
  unsigned char ButtonPressType;
  unsigned char CallbackMsgType;
  unsigned char CallbackMsgOptions;

} tButtonActionPayload;

/*! Status change event types that are sent with the StatusChangeEvent message*/ 
typedef enum
{
  eScReserved = 0x00,
  eScUpdateComplete = 0x01,
  eScModeTimeout = 0x02,
  eScScrollComplete = 0x10,  
  eScScrollRequest = 0x11
  
} eStatusChangeEvents;

/* OSAL Nv operation message */
#define NVAL_INIT_OPERATION     ( 0x00 )
#define NVAL_READ_OPERATION     ( 0x01 )
#define NVAL_WRITE_OPERATION    ( 0x02 )
#define NVAL_RESERVED_OPERATION ( 0x03 )

/*! NvalOperationPayload
 *
 * \param NvalIdentifier is the 16 bit id of an NVAL item
 * \param Size is the size of the nval item
 * \param DataStartByte is the starting location of nval data
 */
#ifdef __IAR_SYSTEMS_ICC__
#pragma pack(1)
#endif

typedef struct
{
  unsigned int NvalIdentifier;
  unsigned char Size;
  unsigned char DataStartByte;

} tNvalOperationPayload;

#ifdef __IAR_SYSTEMS_ICC__
#pragma pack()
#endif

/******************************************************************************/

/* Modify Time Message Options */
#define MODIFY_TIME_INCREMENT_HOUR   ( 0x00 )
#define MODIFY_TIME_INCREMENT_MINUTE ( 0x01 ) 
#define MODIFY_TIME_INCREMENT_DOW    ( 0x02 ) 


#define MENU_MODE_OPTION_PAGE1               ( 0x01 )
#define MENU_MODE_OPTION_PAGE2               ( 0x02 )
#define MENU_MODE_OPTION_PAGE3               ( 0x03 )
#define MENU_MODE_OPTION_UPDATE_CURRENT_PAGE ( 0x04 )


/* Menu Button Message Options */
#define MENU_BUTTON_OPTION_RESERVED                     ( 0x00 )
#define MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY       ( 0x01 )
#define MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM            ( 0x02 )
#define MENU_BUTTON_OPTION_EXIT                         ( 0x03 )
#define MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH             ( 0x04 )
#define MENU_BUTTON_OPTION_TOGGLE_SECURE_SIMPLE_PAIRING ( 0x05 )
#define MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN           ( 0x06 )
#define MENU_BUTTON_OPTION_DISPLAY_SECONDS              ( 0x07 )
#define MENU_BUTTON_OPTION_INVERT_DISPLAY               ( 0x08 )

#define PAIRING_CONTROL_OPTION_DISABLE_PAIRING ( 0x01 )
#define PAIRING_CONTROL_OPTION_ENABLE_PAIRING  ( 0x02 )
#define PAIRING_CONTROL_OPTION_TOGGLE_SSP      ( 0x03 )
#define PAIRING_CONTROL_OPTION_SAVE_SPP        ( 0x04 )

#define NORMAL_SOFTWARE_RESET_OPTION ( 0x00 )
#define MASTER_RESET_OPTION          ( 0x01 )

#define TOGGLE_SECONDS_OPTIONS_UPDATE_IDLE      ( 0x01 )
#define TOGGLE_SECONDS_OPTIONS_DONT_UPDATE_IDLE ( 0x02 )

/******************************************************************************/

#define OLED_CONTRAST_OPTION_TOP_DISPLAY    ( 0x01 )
#define OLED_CONTRAST_OPTION_BOTTOM_DISPLAY ( 0x02 )
#define OLED_CONTRAST_OPTION_SAVE_SETTINGS  ( 0x03 )

/******************************************************************************/


#define SCROLL_OPTION_LAST_PACKET ( BIT0 )
#define SCROLL_OPTION_START       ( BIT1 )

#define SCROLL_OPTION_LAST_PACKET_MASK ( BIT0 )
#define SCROLL_OPTION_START_MASK       ( BIT1 )


/******************************************************************************/

/*! \param word is The unholy union of two bytes into a word */
typedef union
{
  struct
  {
    unsigned char byte0;
    unsigned char byte1;
    
  } Bytes;
  
  unsigned int word;

} tWordByteUnion;

/******************************************************************************/


#define OLED_CROWN_MENU_MODE_OPTION_ENTER               ( 0 )
#define OLED_CROWN_MENU_MODE_OPTION_NEXT_MENU           ( 1 )
#define OLED_CROWN_MENU_MODE_OPTION_UPDATE_CURRENT_PAGE ( 2 )

#define OLED_CROWN_MENU_BUTTON_OPTION_RESERVED        ( 0 )
#define OLED_CROWN_MENU_BUTTON_OPTION_EXIT            ( 1 )
#define OLED_CROWN_MENU_BUTTON_OPTION_TOP_CONTRAST    ( 2 )
#define OLED_CROWN_MENU_BUTTON_OPTION_BOTTOM_CONTRAST ( 3 )

/******************************************************************************/

/*! Overlay onto buffer */
typedef struct
{
  unsigned char GeneralPurposeType;
  unsigned char pData[HOST_MSG_MAX_PAYLOAD_LENGTH-1];

} tGeneralPurposePayload;

/******************************************************************************/

/*! Options for sniff control message
 *
 * Enable and disable sniff mode
 * Request stack to exit sniff mode
 * Request stack to disable sniff mode and disable sniff mode
 */
#define SNIFF_RESERVED_OPTION      ( 0 )
#define AUTO_SNIFF_ENABLE_OPTION   ( 1 )
#define AUTO_SNIFF_DISABLE_OPTION  ( 2 ) 
#define SNIFF_ENTER_OPTION         ( 3 )
#define SNIFF_EXIT_OPTION          ( 4 )
#define SNIFF_ENTER_FAILED_OPTION  ( 5 )
#define SNIFF_EXIT_FAILED_OPTION   ( 6 ) 

/******************************************************************************/


typedef struct
{
  unsigned char Address;
  unsigned char Size;
  unsigned char Data;
  
} tAccelerometerAccessPayload;

#define ACCELEROMETER_DATA_START_INDEX ( 2 ) 

#define ACCELEROMETER_ACCESS_WRITE_OPTION ( 0 )
#define ACCELEROMETER_ACCESS_READ_OPTION  ( 1 )

#define INTERRUPT_CONTROL_DISABLE_INTERRUPT ( 0 )
#define INTERRUPT_CONTROL_ENABLE_INTERRUPT  ( 1 )

/* send interrupt data = sid */
#define SID_CONTROL_SEND_INTERRUPT ( 0 )
#define SID_CONTROL_SEND_DATA      ( 1 )

#define ACCELEROMETER_SETUP_RESERVED_OPTION                 ( 0 )
#define ACCELEROMETER_SETUP_INTERRUPT_CONTROL_OPTION        ( 1 )
#define ACCELEROMETER_SETUP_OPMODE_OPTION                   ( 2 )
#define ACCELEROMETER_SETUP_SID_CONTROL_OPTION              ( 3 )
#define ACCELEROMETER_SETUP_SID_ADDR_OPTION                 ( 4 )
#define ACCELEROMETER_SETUP_SID_LENGTH_OPTION               ( 5 )
#define ACCELEROMETER_SETUP_INTERRUPT_ENABLE_DISABLE_OPTION ( 6 )

#define ACCELEROMETER_HOST_MSG_IS_DATA_OPTION      ( 1 )
#define ACCELEROMETER_HOST_MSG_IS_INTERRUPT_OPTION ( 2 )


/******************************************************************************/

#define READ_RSSI_SUCCESS_OPTION ( 1 )
#define READ_RSSI_FAILURE_OPTION ( 2 )


/******************************************************************************/
  
#define CONFIGURE_DISPLAY_OPTION_RESERVED             ( 0 )
#define CONFIGURE_DISPLAY_OPTION_DONT_DISPLAY_SECONDS ( 1 )
#define CONFIGURE_DISPLAY_OPTION_DISPLAY_SECONDS      ( 2 )
#define CONFIGURE_DISPLAY_OPTION_DONT_INVERT_DISPLAY  ( 3 )
#define CONFIGURE_DISPLAY_OPTION_INVERT_DISPLAY       ( 4 )

#endif  /* MESSAGES_H */
