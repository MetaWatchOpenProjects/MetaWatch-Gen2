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
/*! \file Display.h
 *
 * Handle functions common to both types of display (OLED and LCD).
 */
/******************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

/******************************************************************************/

/*! turn extra prints on/off for entering/exiting sniff mode */
#define SNIFF_DEBUG_DEFAULT      ( 0 )

/*! turn extra prints on/off for displaying battery voltage and charging info */
#define BATTERY_DEBUG_DEFAULT    ( 0 ) 

/*! turn extra prints on/off for troubleshooting BT connection */
#define CONNECTION_DEBUG_DEFAULT ( 0 ) 

/*! if 0 then don't ever time out 
 * set a time limit for how long the watch remains in pairing mode
 */
#define PAIRING_MODE_TIMEOUT_IN_SECONDS ( 600 )

/*! when 0 pairing information is not saved into Nval.  Set this to 0 when
 * using a BT sniffer 
 */
#define SAVE_PAIRING_INFO_DEFAULT   ( 1 )

/*! when 1 then the watch will try to enter sniff mode */
#define ENABLE_SNIFF_ENTRY_DEFAULT  ( 1 )

/*! When 0 the stack wrapper will not request to exit sniff mode when a 
 * message is received.
 */
#define EXIT_SNIFF_ON_RECEIVE_DEFAULT  ( 1 )

/******************************************************************************/

/*! Copies the local bluetooth address string (from SerialProfile) 
 * so that the display task can put it on the screen
 */
void SetLocalBluetoothAddressString(unsigned char* pData);

/*! Copies the remote bluetooth address string so that the display task can put 
 * it on the screen
 */
void SetRemoteBluetoothAddressString(unsigned char* pData);

/*! Returns a pointer to the Local Bluetooth Address 
 * 
 */
tString * GetLocalBluetoothAddressString(void);

/*! Returns a pointer to the Remote Bluetooth Address 
 *
 */
tString * GetRemoteBluetoothAddressString(void);

/*! Find out what button configuration to use
 *
 *\return Returns a button mode as defined in messages.h
 */
unsigned char QueryButtonMode(void);

/*! Reads the hardware revision register in the MSP430
 *
 * \return Character representation of revision (Example: 'E')
 */
unsigned char GetMsp430HardwareRevision(void);

/*! Query the connection state of the bluetooth serial port
 * 
 * \return String representation of the current state of serial port.
 */
unsigned char * QueryConnectionStateAndGetString(void);


/*! This is called by the stack to set the FirstContact variable.  This 
 * variable is used to determine if the phone has connected for the first time
 */
void SetFirstContact(void);

/*! Clear the first contact flag */
void ClearFirstContact(void);


/*! Query the FirstContact Flag
 *
 * \return 1 = phone has been connected, 0 otherwise
 */
unsigned char QueryFirstContact(void);

/*! Strings for days of the week in the same order as RTC */
extern const tString DaysOfTheWeek[][7];

/*! Strings for months of the year in the same format as RTC */
extern const tString MonthsOfYear[][13];

/******************************************************************************/

/*! Twelve hour mode for TimeFormat */
#define TWELVE_HOUR      ( 0 )
/*! 24 hour mode for TimeFormat */
#define TWENTY_FOUR_HOUR ( 1 )

/*! Display the month first */
#define MONTH_FIRST ( 0 )

/*! Display the day before the month */
#define DAY_FIRST   ( 1 )

/*! Initaliaze the non-volatile item that holds the time format (which is 12 or
 * 24 hour )
*/
void InitializeTimeFormat(void);

/*! Initaliaze the non-volatile item that holds the date format (which is day
 * or month first )
*/
void InitializeDateFormat(void);

/*! \return Time Format TWELVE_HOUR = 0, TWENTY_FOUR_HOUR = 1 */
unsigned char GetTimeFormat(void);

/*! \return date format MONTH_FIRST = 0, DAY_FIRST = 1 */
unsigned char GetDateFormat(void);

/******************************************************************************/

/*! \return 1 if the link alarm is enabled, 0 if it is not */
unsigned char QueryLinkAlarmEnable(void);

/*! Toggle the state of the link Alarm control bit */
void ToggleLinkAlarmEnable(void);

/*! Initialize the nv item that determines if vibration should occur when the
 * link is lost 
*/
void InitializeLinkAlarmEnable(void);

/*! Save the link alarm control bit into nval */
void SaveLinkAlarmEnable(void);

/*! Send a vibration message to the vibration state machine */
void GenerateLinkAlarm(void);


/******************************************************************************/

/*! Initialize the nv items that hold the application and notification mode timeouts */
void InitializeModeTimeouts(void);

/*! \return Application mode timeout in seconds */
unsigned int QueryApplicationModeTimeout(void);

/*! \return Notification mode timeout in seconds */
unsigned int QueryNotificationModeTimeout(void);

/******************************************************************************/


/*! Initialize flags stored in nval used for debugging (controlling print statements */
void InitializeDebugFlags(void);

/* \return 1 when sniff debug messages should be printed */
unsigned char QuerySniffDebug(void);

/* \return 1 when battery debug messages should be printed */
unsigned char QueryBatteryDebug(void);

/* \return 1 when connection debug messages should be printed */
unsigned char QueryConnectionDebug(void);

/******************************************************************************/

/* ! Initialize nv value that determines how long watch should stay in pairing
 * mode.
 *
 * \note Called by stack wrapper 
 */
void InitializePairingModeDuration(void);

/* \return pairing duration in seconds, 0 means pair forever */
unsigned int GetPairingModeDurationInSeconds(void);

/******************************************************************************/

/*! Initialize nv value that determines if pairing information should be saved
 * 
 * \note Called by stack wrapper 
 */
void InitializeSavePairingInfo(void);

/*! \return 1 when pairing information should be saved */
unsigned char QuerySavePairingInfo(void);

/******************************************************************************/

/*! Initialize nv value that determines if sniff entry is enabled 
 *
 * \note Called by stack wrapper 
 *
 */
void InitializeEnableSniffEntry(void);

/*! \return 1 when sniff entry is enabled */
unsigned char QueryEnableSniffEntry(void);

/******************************************************************************/

/******************************************************************************/

/*! Initialize nv value that determines if sniff should be exited when receiving
 * a message
 * 
 * \note Called by stack wrapper 
 *
 */
void InitializeExitSniffOnReceive(void);

/*! \return 1 when sniff entry is enabled */
unsigned char QueryExitSniffOnReceive(void);

/******************************************************************************/

/*! \return 1 if this is the analog watch, 0 otherwise */
unsigned char QueryAnalogWatch(void);

/******************************************************************************/

#endif /* DISPLAY_H */
