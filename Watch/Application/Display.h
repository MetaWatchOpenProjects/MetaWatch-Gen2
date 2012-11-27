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

#define BT_ADDR_LEN (6)

/*! Strings for days of the week in the same order as RTC */
extern const tString DaysOfTheWeek[][7][4];

/*! Strings for months of the year in the same format as RTC */
extern const tString MonthsOfYear[][13][7];

/******************************************************************************/

/*! Twelve hour mode for TimeFormat */
#define TWELVE_HOUR      ( 0 )
/*! 24 hour mode for TimeFormat */
#define TWENTY_FOUR_HOUR ( 1 )

/*! Display the month first */
#define MONTH_FIRST ( 0 )

/*! Display the day before the month */
#define DAY_FIRST   ( 1 )

/*! Languages */ 
#define LANG_EN (0)
#define LANG_FI (1)
#define LANG_DE (2)

/*! Initaliaze the non-volatile item that holds the time format (which is 12 or
 * 24 hour )
*/
void Init12H(void);

/*! \return Time Format TWELVE_HOUR = 0, TWENTY_FOUR_HOUR = 1 */
unsigned char Get12H(void);
void Set12H(unsigned char Fmt);

/*! Initaliaze the non-volatile item that holds the date format (which is day
 * or month first )
*/
void InitMonthFirst(void);

/*! \return date format MONTH_FIRST = 0, DAY_FIRST = 1 */
unsigned char GetMonthFirst(void);
void SetMonthFirst(unsigned char Mon1st);

void InitLanguage(void);
unsigned char GetLanguage(void);

/******************************************************************************/

/*! \return 1 if the link alarm is enabled, 0 if it is not */
unsigned char QueryLinkAlarmEnable(void);

/*! Toggle the state of the link Alarm control bit */
void ToggleLinkAlarmEnable(void);

/*! Initialize the nv item that determines if vibration should occur when the
 * link is lost 
*/
void InitLinkAlarmEnable(void);

/*! Save the link alarm control bit into nval */
void SaveLinkAlarmEnable(void);

/*! Send a vibration message to the vibration state machine */
void GenerateLinkAlarm(void);


/******************************************************************************/

/*! Initialize the nv items that hold the application and notification mode timeouts */
void InitModeTimeout(void);

/*! \return mode's timeout in seconds */
unsigned int QueryModeTimeout(unsigned char Mode);

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
void InitRadioOffTimeout(void);

/* \return pairing duration in seconds, 0 means pair forever */
unsigned int RadioOffTimeout(void);

/******************************************************************************/

/*! Initialize nv value that determines if pairing information should be saved
 * 
 * \note Called by stack wrapper 
 */
void InitializeSavePairingInfo(void);

/*! \return 1 when pairing information should be saved */
unsigned char SavedPairingInfo(void);

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
