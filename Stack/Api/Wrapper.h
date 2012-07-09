//==============================================================================
//  Copyright 2012 Meta Watch Ltd. - http://www.MetaWatch.org/
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
/*! \file Wrapper.h
*
*/
/******************************************************************************/

#ifndef WRAPPER_H
#define WRAPPER_H

/*! Initaliaze the serial port profile task.  This should be called from main.
*
* This task opens the stack which in turn creates 3 more tasks that create and 
* handle the bluetooth serial port connection.
*/
void InitializeWrapperTask(void);


/*! Query the serial port profile task if it is okay to put the part into LPM3
*
* This function is called by the idle task.  The idle task puts the 
* MSP430 into sleep mode.
* 
* \return 0 if micro cannot go into LPM3; 1 if micro can go into LPM3
*/
unsigned char SerialPortReadyToSleep(void);

typedef struct
{
  char *pSwVer;
  char *pHwVer;
  char *pBtVer;
} tVersion;

/*! Return a pointer to the wrapper version string */
tVersion GetWrapperVersion(void);

/******************************************************************************/

/*! The current state of the bluetooth serial port.
*
* Radio on also means that the stack is up.  Radio Off also means that the stack 
* is closed. As do RadioOffLowBattery and ShippingMode.
*/
typedef enum
{
  Unknown = 0,
  Initializing,
  ServerFailure,
  RadioOn,
  Paired,
  BRConnected,
  LEConnected,
  RadioOff,
  RadioOffLowBattery,
  ShippingMode  
} etConnectionState;

#define NUMBER_OF_CONNECTION_STATES  (10)

/*! Determine if the bluetooth link is in the connected state.  
* When the phone and watch are connected then data can be sent.
*
* \return 0 when not connected, 1 when connected
*/
unsigned char QueryPhoneConnected(void);

/*! This function is used to determine if the radio is on and will return 1 when
* initialization has successfully completed, but the radio may or may not be 
* paired or connected.
*
* \return 1 when stack is in the connected, paired, or radio on state, 0 otherwise
*/
unsigned char QueryBluetoothOn(void);

/*! This function is used to determine if the connection state of the 
 * bluetooth serial port.
 *
 * \return etConnectionState
 */
etConnectionState QueryConnectionState(void);

/*! Query Bluetooth Discoverability
 *
 * \return 0 when not discoverable, 1 when discoverable
 */
unsigned char QueryDiscoverable(void);

/*! Query Bluetooth Connectability
 *
 * \return 0 when not connectable, 1 when connectable
 */
unsigned char QueryConnectable(void);

/*! Query Bluetooth Secure Simple Pairing Setting
 *
 * \return 0 when SSP is disabled, 1 when SSP is enabled
 */
unsigned char QuerySecureSimplePairingEnabled(void);

/*! Query Bluetooth pairing information
 *
 * \return 0 when valid pairing does not exist, 1 when valid pairing information
 * exists
 */
unsigned char QueryValidPairingInfo(void);


/*! Query Link Key Information
 *
 * Fills parameters with values if the Index is valid
 *
 * \param Index is an index into the bluetooth link key structure
 * \param pBluetoothAddress is a pointer to the bluetooth address (13 bytes)
 * \param pBluetoothName is a pointer to the bluetooth name 
 * \param BluetoothNameSize is the size of string pBluetoothName points to 
 */
void QueryLinkKeys(unsigned char Index,
                   tString *pBluetoothAddress,
                   tString *pBluetoothName,
                   unsigned char BluetoothNameSize);

/******************************************************************************/

/*! Query the state of the AutoSniffEnabled register 
 * \return 1 if Sniff is Enabled, 0 otherwise 
 */
unsigned char QueryAutoSniffEnabled(void);

/*! 
 * \param DelayMs is the delay for entering sniff mode 
 */
void SetSniffModeEntryDelay(unsigned int DelayMs);


/* these are actually the HCI Current Mode Types + some housekeeping */
typedef enum
{
  Active = 0,             
  Hold = 1,
  Sniff = 2,                  
  Park = 3,
  PhoneNotConnected = 4,
  SniffToActive = 5,
  ActiveToSniff = 6
  
} etSniffState;

etSniffState QuerySniffState(void);

/******************************************************************************/

/*! Convert milliseconds to baseband slots
 * \note stack handles making input an even number per spec requirements
*/
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)   ((_x) / (0.625))

/*! maximum sniff slot interval */
#define SNIFF_MAX_INTERVAL_SLOTS_LEVEL ( MILLISECONDS_TO_BASEBAND_SLOTS(1280) )

/*! minimum sniff slot interval */
#define SNIFF_MIN_INTERVAL_SLOTS_LEVEL ( MILLISECONDS_TO_BASEBAND_SLOTS(1270) )

/*! number of slots that the slave must listen */
#define SNIFF_ATTEMPT_SLOTS         ( 0x0002 )

/*! number of additional slots the slave must listen */
#define SNIFF_ATTEMPT_TIMEOUT_SLOTS ( 0x0001 )

/*! These are the four sniff slot parameters that can be set by user */
typedef enum
{
  MaxInterval = 0,
  MinInterval = 1,
  Attempt = 2,
  AttemptTimeout = 3

} eSniffSlotType;

/*! Allow access to the 4 sniff parameters
 *
 * \param SniffSlotType is the parameter type
 * \param Slots is the new parameter value in slots
 */
void SetSniffSlotParameter(eSniffSlotType SniffSlotType,unsigned int Slots);


/*! \return The sniff Slot parameter
 *
 * \param SniffSlotType is the parameter type
 */
unsigned int QuerySniffSlotParameter(eSniffSlotType SniffSlotType);

/******************************************************************************/


#endif /* WRAPPER_H */

