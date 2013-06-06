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
#include "hal_lcd.h"

#define MSG_BUFFER_LENGTH  ( 32 )
#define MSG_HEADER_LENGTH  ( 4 )
#define MSG_CRC_LENGTH     ( 2 )
#define MSG_OVERHEAD_LENTH ( 6 )
#define MSG_PAYLOAD_LENTH  (14)
#define MSG_FRM_START      (0x01)
#define FRAME_HEADER_LEN   (2) // Flag + frame length

// see BufferPool.c
#define MSG_RELATIVE_INDEX_FLG  (-4)
#define MSG_RELATIVE_INDEX_LEN  (-3)
#define MSG_RELATIVE_INDEX_TYPE (-2)
#define MSG_RELATIVE_INDEX_OPT  (-1)

/* 26 */
#define MSG_MAX_PAYLOAD_LENGTH \
  (MSG_BUFFER_LENGTH - MSG_HEADER_LENGTH - MSG_CRC_LENGTH)


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
  unsigned char pPayload[MSG_MAX_PAYLOAD_LENGTH];
  unsigned char crcLsb;
  unsigned char crcMsb;

} tHostMsg;

#endif

/*! Message type enumeration
 *
 * for this processor the default is 16 bits for an enumeration
 */
typedef enum
{
  InvalidMsg = 0x00,

  DevTypeMsg = 0x01,
  DevTypeRespMsg = 0x02,
  VerInfoMsg = 0x03,
  VerInfoRespMsg = 0x04,
  DiagnosticLoopback = 0x05,
  ShippingModeMsg = 0x06,
  ResetMsg = 0x07,
  ConnTimeoutMsg = 0x08,
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

  /* Music Play State */
  MusicPlayStateMsg = 0x18,
  
  /*
   * Status and control
   */

  /* move the hands hours, mins and seconds */
  AdvanceWatchHandsMsg = 0x20,

  TermModeMsg = 0x21,
  
  /* config and (dis)enable vibrate */
  SetVibrateMode = 0x23,
  ButtonStateMsg = 0x24,

  /* Sets the RTC */
  SetRtcMsg = 0x26,
  GetRtcMsg = 0x27,
  RtcRespMsg = 0x28,

  /* osal nv */
  NvalOperationMsg = 0x30,
  PropRespMsg = 0x31,

  /* status of the current display operation */
  ModeChangeIndMsg = 0x33,
  ButtonEventMsg = 0x34,
  GeneralPurposePhoneMsg = 0x35,
  GeneralPurposeWatchMsg = 0x36,
  WrapperTaskCheckInMsg = 0x37,
  DisplayTaskCheckInMsg = 0x38,
  
  /*
   * LCD display related commands
   */
  WriteBufferMsg = 0x40,
  SecInvertMsg = 0x41,
  ControlFullScreenMsg = 0x42,
  UpdateDisplayMsg = 0x43,
  LoadTemplateMsg = 0x44,
  ExtAppMsg = 0x45,  
  EnableButtonMsg = 0x46,
  DisableButtonMsg = 0x47,
  ReadButtonConfigMsg = 0x48,
  ReadButtonConfigResponse = 0x49,
  ExtAppIndMsg = 0x4a,
  EraseTemplateMsg = 0x4b,
  WriteToTemplateMsg = 0x4c,
  SetClockWidgetSettingsMsg = 0x4d,
  DrawClockWidgetMsg = 0x4e,
  WriteClockWidgetDoneMsg = 0x4f,

  SetExtWidgetMsg = 0x50,
  UpdateClockMsg = 0x51,
  MonitorBatteryMsg = 0x52,
  BatteryConfigMsg = 0x53,
  LowBatteryWarningMsgHost = 0x54,
  LowBatteryBtOffMsgHost = 0x55,
  ReadBatteryVoltageMsg = 0x56,
  VBatRespMsg = 0x57,
  ReadLightSensorMsg = 0x58,
  LightSensorRespMsg = 0x59,
  LowBatteryWarningMsg = 0x5a,
  LowBatteryBtOffMsg = 0x5b,
  AutoBacklightMsg = 0x5c,
  SetBacklightMsg = 0x5d,

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
  IdleUpdateMsg = 0xa0,
  SetWidgetListMsg = 0xa1, // for new UI
  WatchDrawnScreenTimeout = 0xa2,

  ChangeModeMsg = 0xa6,
  ModeTimeoutMsg = 0xa7,
  WatchStatusMsg = 0xa8,
  MenuModeMsg = 0xa9,
  ServiceMenuMsg = 0xaa,
  ListPairedDevicesMsg = 0xab,
  BluetoothStateChangeMsg = 0xac,
  ModifyTimeMsg = 0xad,
  MenuButtonMsg = 0xae,
  ToggleSecondsMsg = 0xaf,

  /* BLE messages */
  SetHeartbeatMsg = 0xb0,
  HeartbeatTimeoutMsg = 0xb1,
  UpdConnParamMsg = 0xb2,

  /* HFP messages */
  CallerIdIndMsg = 0xb3,
  CallerNameMsg = 0xb4,
  CallerIdMsg = 0xb5,
  HfpMsg = 0xb6,
  
  /* MAP messages */
  MapMsg = 0xb7,
  MapIndMsg = 0xb8,

  ConnChangeMsg = 0xb9,
  UpdWgtIndMsg = 0xba,
  ConnParamChgIndMsg = 0xbb,
  TunnelTimeoutMsg = 0xbc,

  SppAckMsg = 0xcc,
  CountDownMsg = 0xcd,
  SetCountdownDoneMsg = 0xce,
  
  QueryMemoryMsg = 0xd0,
  ConnTypeMsg = 0xd1,
  RateTestMsg = 0xd2,
  
  AccelIndMsg = 0xe0,
  AccelMsg = 0xe1,

  RadioPowerControlMsg = 0xf0,
  
  EnableAdvMsg = 0xf1,
  SetAdvDataMsg = 0xf2,
  SetScanRespMsg = 0xf3
} eMessageType;

#define MAXIMUM_MESSAGE_TYPES      (256)

#define MSG_OPT_NONE               (0)
#define NONZERO_MSG_OPTIONS        (0xFF)
#define MSG_TYPE_INDEX             (2)

#define MSG_OPT_NEWUI              (0x80)
#define MSG_OPT_HOME_WGT           (0x40)
#define MSG_OPT_WGT_LAYOUT_MASK    (0x0C)
#define MSG_OPT_WGT_LAYOUT_SHFT    (2)

/* options for mode change */
#define MSG_OPT_CHGMOD_IND        (0x80)

/* options for UpdateDisplayMsg */
#define MSG_OPT_PAGE_NO           (0x0C)
#define MSG_OPT_TURN_PAGE         (0x0C)
#define MSG_OPT_PRV_PAGE          (0x08)
#define MSG_OPT_NXT_PAGE          (0x04)
#define MSG_OPT_SET_PAGE          (0x20)
#define MSG_OPT_UPD_HWGT          (0x20)
#define MSG_OPT_UPD_GRID_BIT      (0x40)
#define MSG_OPT_UPD_INTERNAL      (0x10)
#define SET_PAGE_SHFT             (2)

/* options */
#define NUMBER_OF_BUFFERS          ( 2 )
#define NUMBER_OF_BUFFER_STATUS    ( 4 )
#define BUFFER_CLEAN               ( 0 )
#define BUFFER_READING             ( 1 )
#define BUFFER_WRITING             ( 2 )
#define BUFFER_WRITTEN             ( 3 )
#define BUFFER_UNAVAILABLE         ( 0xFF )
#define BUFFER_WRITTEN_MASK        ( BIT5 )
#define BUFFER_TYPE_READ           ( 0 )
#define BUFFER_TYPE_WRITE          ( 1 )
#define BUFFER_TYPE_RECENT         ( 2 )
#define BUFFER_STATUS_MASK         ( 0x7F )
#define BUFFER_STATUS_RECENT       ( 0x80 )
#define NUMBER_OF_READ_BUFFER_SEL_RULES ( 2 )
#define NUMBER_OF_WRITE_BUFFER_SEL_RULES ( 3 )

/* options for the indication message */
#define MSG_OPT_IND_HEARTBEAT  (0)
#define MSG_OPT_IND_CALLERID   (1)
#define MSG_OPT_HB_MWM         (0)

/* options for the heartbeat message */
#define MSG_OPT_HEARTBEAT_MASTER (0x0)

/* options for MAP State message */
#define MSG_OPT_MAP_DISC_MAS   (0)
#define MSG_OPT_MAP_OPEN_MAS   (1)
#define MSG_OPT_MAP_OPEN_MNS   (2)
#define MSG_OPT_MAP_FLDR_LST   (3)
#define MSG_OPT_MAP_SET_FLDR   (4)
#define MSG_OPT_MAP_MESG_LST   (5)
#define MSG_OPT_MAP_GET_MESG   (6)
#define MSG_OPT_MAP_CLOSE_MAP  (7)

/* options for HFP message */
#define MSG_OPT_HFP_HANGUP     (0)
#define MSG_OPT_HFP_VRCG       (1)
#define MSG_OPT_HFP_OPEN_AG    (2)
#define MSG_OPT_HFP_RING_STOP  (3)
#define MSG_OPT_HFP_SCO_CONN   (4)

#define SHOW_NOTIF_CALLER_ID   (0)
#define SHOW_NOTIF_CALLER_NAME (1)
#define SHOW_NOTIF_END         (2)
#define SHOW_NOTIF_REJECT_CALL (3)

/* options for MAP Indication message */
#define MSG_OPT_MAP_IND_TYPE        (0)
#define MSG_OPT_MAP_IND_SENDER      (1)
#define MSG_OPT_MAP_IND_SUBJECT     (2)
#define MSG_OPT_MAP_IND_BODY        (3)
#define MSG_OPT_MAP_IND_BODY_END    (4)

/* options for Bluetooth state change */
#define MSG_OPT_BT_STATE_DISCONN    (1)
#define MSG_OPT_INIT_BONDING        (1)

/* make mode definitions the same as buffer definitions */
#define IDLE_MODE         (0)
#define APP_MODE          (1)
#define NOTIF_MODE        (2)
#define MUSIC_MODE        (3)
#define MODE_NUM          (4)
#define MODE_MASK         (0x3)

/* Timeout for OneSecondTimer */
#define TOUT_MONITOR_BATTERY          (10) //second
#define TOUT_BACKLIGHT                (5) //second
#define TOUT_IDLE_MODE                (0xFFFF)
#define TOUT_APP_MODE                 (600)
#define TOUT_NOTIF_MODE               (30)
#define TOUT_MUSIC_MODE               (600)
#define TOUT_CALL_NOTIF               (10)
#define TOUT_CONN_HFP_MAP_LONG        (10)
#define TOUT_CONN_HFP_MAP_SHORT       (1)
#define TOUT_DISCONNECT               (10)
#define TOUT_TUNNEL_CONNECTING        (10)
#define TOUT_TUNNEL_LONG              (40) //60
#define TOUT_HEARTBEAT                (1) 

/* these should match the display modes for idle, application, notification,
 * and scroll modes
 */
#define NORMAL_IDLE_SCREEN_BUTTON_MODE ( 0 )
#define WATCH_DRAWN_SCREEN_BUTTON_MODE ( 1 )
#define APPLICATION_SCREEN_BUTTON_MODE ( 4 )
#define NOTIFICATION_BUTTON_MODE       ( 2 )
#define SCROLL_BUTTON_MODE             ( 3 )

#define UPDATE_COPY_MASK                       ( BIT4 )
#define COPY_ACTIVE_TO_DRAW_DURING_UPDATE      ( BIT4 )
#define FORCE_UPDATE_MASK                      ( BIT5 )
#define NO_COPY_DURING_UPDATE                  ( 0    )

#define DRAW_BUFFER_ACTIVATION_MASK ( BIT5 )
//#define ACTIVATE_DRAW_BUFFER        ( 0 )
#define FORCE_UPDATE                ( BIT5 )

#define IDLE_TIMER_UPDATE_TYPE_MASK ( BIT6 )
#define RESTART_IDLE_TIMER          ( BIT6 )
#define PERIODIC_IDLE_UPDATE        ( 0 )

/* button option */
#define RESET_DISPLAY_TIMER ( 1 )

/* configure mode option */
#define SAVE_MODE_CONFIGURATION_MASK ( BIT4 )


/*! The serial ram message is formatted so that it can be overlaid onto the
 * message from the host (so that a buffer allocation does not have to be
 * performed and there is one less message copy).
 *
 * \param Reserved0
 * \param Reserved1
 * \param SerialRamCommand is the command for the serial ram (read/write)
 * \param AddressMsb
 * \param AddressLsb
 * \param pLineA[BYTES_PER_LINE] is pixel data (and AddressMsb2 for second write)
 * \param AddressLsb2
 * \param pLineB[BYTES_PER_LINE]
 * \param Reserved31
 *
 */
typedef struct
{
  unsigned char RowSelectA;
  unsigned char pLineA[BYTES_PER_LINE];
  unsigned char RowSelectB;
  unsigned char pLineB[BYTES_PER_LINE];

} tSerialRamPayload;

#define LCD_MESSAGE_CMD_INDEX        ( 2 )
#define LCD_MESSAGE_ROW_NUMBER_INDEX ( 3 )
#define LCD_MESSAGE_LINE_INDEX       ( 4 )

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

#define WRITE_OLED_BUFFER_MAX_PAYLOAD ( MSG_MAX_PAYLOAD_LENGTH - 3 )

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

#define WRITE_SCROLL_BUFFER_MAX_PAYLOAD ( MSG_MAX_PAYLOAD_LENGTH - 1 )

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

/*! Write To Template Strucutre
 *
 * @param TemplateSelect (the first bye of payload) selects which template will
 * be written to
 * @param RowSelectA 1st row to be written
 * @param pLineA Data written to 1st row
 * @param RowSelectB 2st row to be written (optional)
 * @param pLineB Data written to 2nd row (optional)
 */
typedef struct
{
  unsigned char TemplateSelect;
  unsigned char RowSelectA;
  unsigned char pLineA[12];
  unsigned char RowSelectB;
  unsigned char pLineB[12];

} tWriteToTemplateMsgPayload;

/*! Custom Date Time Position message payload Strucutre
 *
 * @param HoursRow - row pixel (0-95)
 * @param HoursCol - column pixel (0-95)
 * @param MinsRow - row pixel (0-95)
 * @param MinsCol - column pixel (0-95)
 * @param SecsRow - row pixel (0-95)
 * @param SecsCol - column pixel (0-95)
 * @param AmPmRow - row pixel (0-95)
 * @param AmPmCol - column pixel (0-95)
 * @param DowRow - row pixel (0-95)
 * @param DowCol - column pixel (0-95)
 * @param YearRow - row pixel (0-95)
 * @param YearCol - column pixel (0-95)
 * @param MonthRow - row pixel (0-95)
 * @param MonthCol - column pixel (0-95)
 * @param DayRow - row pixel (0-95)
 * @param DayCol - column pixel (0-95)
 * @param DateSeparatorRow - row pixel (0-95)
 * @param DateSeparatorCol - column pixel (0-95)
 *
 */
typedef struct
{
  unsigned char HoursRow;
  unsigned char HoursCol;
  unsigned char TimeSeparatorRow;
  unsigned char TimeSeparatorCol;
  unsigned char MinsRow;
  unsigned char MinsCol;
  unsigned char SecsRow;
  unsigned char SecsCol;
  unsigned char AmPmRow;
  unsigned char AmPmCol;
  unsigned char DowRow;
  unsigned char DowCol;
  unsigned char YearRow;
  unsigned char YearCol;
  unsigned char MonthRow;
  unsigned char MonthCol;
  unsigned char DayRow;
  unsigned char DayCol;
  unsigned char DateSeparatorRow;
  unsigned char DateSeparatorCol;

} tSetCustomPosMsgPayload;

/*! Custom Idle Icons Position message payload Strucutre
 *
 * @param BluetoothOffRow - row pixel (0-95)
 * @param BluetoothOffCol - column pixel (0-95)
 * @param PhoneDisconnectedRow - row pixel (0-95)
 * @param PhoneDisconnectedCol - column pixel (0-95)
 * @param BatteryChargingRow - row pixel (0-95)
 * @param BatteryChargingCol - column pixel (0-95)
 * @param LowBatteryRow - row pixel (0-95)
 * @param LowBatteryCol - column pixel (0-95)
 *
 */
typedef struct
{
  unsigned char BluetoothOffRow;
  unsigned char BluetoothOffCol;
  unsigned char PhoneDisconnectedRow;
  unsigned char PhoneDisconnectedCol;
  unsigned char BatteryChargingRow;
  unsigned char BatteryChargingCol;
  unsigned char LowBatteryRow;
  unsigned char LowBatteryCol;

} tSetCustomIdleIconsPosMsgPayload;

/*! Custom Date Time Font message payload Strucutre
 *
 * @param HoursFont - font type
 * @param MinsFont - font type
 * @param SecsFont - font type
 * @param AmPmFont - font type
 * @param DayOfWeekFont - font type
 * @param YearFont - font type
 * @param MonthFont - font type
 * @param DayFont - font type
 * @param DateSeparatorFont - font type
 *
 */
typedef struct
{
  unsigned char HoursFont;
  unsigned char TimeSeparatorFont;
  unsigned char MinsFont;
  unsigned char SecsFont;
  unsigned char AmPmFont;

  unsigned char DayOfWeekFont;
  unsigned char YearFont;
  unsigned char MonthFont;
  unsigned char DayFont;
  unsigned char DateSeparatorFont;

} tSetCustomFontMsgPayload;

/*!
 * \param DisplayMode is Idle, Application, or Notification
 * \param ButtonIndex is the button index
 * \param ButtonEvent is immediate, pressed, hold, or long hold
 * \param CallbackMsgType is the callback message type for the button event
 * \param CallbackMsgOptions is the options to send with the message
 */
typedef struct
{
  unsigned char DisplayMode;
  unsigned char ButtonIndex;
  unsigned char ButtonEvent;
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

} eModeChangeEvent;

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
#define MENU_BUTTON_OPTION_RESERVED                     (0)
#define MENU_BUTTON_OPTION_TOGGLE_DISCOVERABILITY       (1)
#define MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM            (2)
#define MENU_BUTTON_OPTION_EXIT                         (3)
#define MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH             (4)
#define MENU_BUTTON_OPTION_TEST                         (5)
#define MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN           (6)
#define MENU_BUTTON_OPTION_DISPLAY_SECONDS              (7)
#define MENU_BUTTON_OPTION_INVERT_DISPLAY               (8)
#define MENU_BUTTON_OPTION_TOGGLE_ACCEL                 (9)
#define MENU_BUTTON_OPTION_TOGGLE_ENABLE_CHARGING       (10)
#define MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE        (11)
#define MENU_BUTTON_OPTION_TOGGLE_SERIAL_SBW_GND        (12)
#define MENU_BUTTON_OPTION_TOGGLE_DELETE_PAIRING        (13)
#define MENU_BUTTON_OPTION_TOGGLE_SBW_OFF_CLIP          (14)
#define MENU_BUTTON_OPTION_MENU1                        (15)

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
  unsigned char pData[MSG_MAX_PAYLOAD_LENGTH-1];

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
#define MSG_OPT_ENTER_SNIFF         ( 3 )
#define MSG_OPT_EXIT_SNIFF          ( 4 )
#define SNIFF_ENTER_FAILED_OPTION  ( 5 )
#define SNIFF_EXIT_FAILED_OPTION   ( 6 )

/******************************************************************************/

#define READ_RSSI_SUCCESS_OPTION ( 1 )
#define READ_RSSI_FAILURE_OPTION ( 2 )

/******************************************************************************/

#define CONFIGURE_DISPLAY_OPTION_RESERVED             ( 0 )
#define MSG_OPT_HIDE_SECOND       ( 1 )
#define MSG_OPT_SHOW_SECOND       ( 2 )
#define MSG_OPT_NORMAL_DISPLAY    ( 3 )
#define MSG_OPT_INVERT_DISPLAY    ( 4 )
/******************************************************************************/

#endif  /* MESSAGES_H */
