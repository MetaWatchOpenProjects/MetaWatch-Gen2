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
/*! \file Display.c
 *
 * Handle functions common to both types of display
 */
/******************************************************************************/

#include "FreeRTOS.h"
#include "queue.h"
#include "portmacro.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Display.h"
#include "OSAL_Nv.h"
#include "NvIds.h"
#include "SerialProfile.h"
#include "OneSecondTimers.h"

/* these have the null character added at the end */
tString pLocalBluetoothAddressString[] = "000000000000";
tString pRemoteBluetoothAddressString[] = "000000000000";

/* LocalBluetoothAddress == watch */
void SetLocalBluetoothAddressString(unsigned char* pData)
{
  unsigned char i = 0;
  
  while ( pLocalBluetoothAddressString[i] != 0 && pData[i] != 0 )
  {
    pLocalBluetoothAddressString[i] = pData[i];
    i++;
  }
  
}


/* RemoteBluetoothAddress == phone */
void SetRemoteBluetoothAddressString(unsigned char* pData)
{
  unsigned char i = 0;
  
  while ( pRemoteBluetoothAddressString[i] != 0 && pData[i] != 0 )
  {
    pRemoteBluetoothAddressString[i] = pData[i];
    i++;
  }
  
}

tString* GetLocalBluetoothAddressString(void)
{
  return pLocalBluetoothAddressString;
}

tString* GetRemoteBluetoothAddressString(void)
{
  return pRemoteBluetoothAddressString;  
}

/******************************************************************************/

#define HARDWARE_REVISION_ADDRESS (0x1a07)

unsigned char GetMsp430HardwareRevision(void)
{
  unsigned char *pDeviceType = 
    (unsigned char *)(unsigned char *)HARDWARE_REVISION_ADDRESS;
  
  return pDeviceType[0]+'1';                         
}


/******************************************************************************/

unsigned char * QueryConnectionStateAndGetString(void)
{
  etConnectionState cs = QueryConnectionState();
  char * pString;
  
  /* Initializing is the longest word that can fit on the LCD */
  switch (cs) 
  {
  case Initializing:       pString = "Initializing"; break;
  case ServerFailure:      pString = "ServerFail";   break;
  case RadioOn:            pString = "RadioOn";      break;
  case Paired:             pString = "Paired";       break;
  case Connected:          pString = "Connected";    break;
  case RadioOff:           pString = "RadioOff";     break;
  case RadioOffLowBattery: pString = "RadioOff";     break;
  case ShippingMode:       pString = "ShippingMode"; break;
  default:                 pString = "Unknown";      break;  
  }
  
  return (unsigned char*)pString;
}

/******************************************************************************/

unsigned char FirstContact;

void SetFirstContact(void)
{
  FirstContact = 1;
}

void ClearFirstContact(void)
{
  FirstContact = 0;  
}

unsigned char QueryFirstContact(void)
{
  return FirstContact;  
}

/******************************************************************************/

/* 
 * these are setup to match RTC 
 * days of week are 0-6 and months are 1-12 
 */
const tString DaysOfTheWeek[][7] = 
{
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

const tString MonthsOfYear[][13] = 
{
  "???","Jan","Feb","Mar","Apr","May","June",
  "July","Aug","Sep","Oct","Nov","Dec"
};

/******************************************************************************/

static unsigned char nvTimeFormat;
static unsigned char nvDateFormat;

void InitializeTimeFormat(void)
{
  nvTimeFormat = TWELVE_HOUR;
  
  OsalNvItemInit(NVID_TIME_FORMAT, 
                 sizeof(nvTimeFormat), 
                 &nvTimeFormat);


}

void InitializeDateFormat(void)
{
  nvDateFormat = MONTH_FIRST;
  
  OsalNvItemInit(NVID_DATE_FORMAT, 
                 sizeof(nvDateFormat), 
                 &nvDateFormat);
  
}

unsigned char GetTimeFormat(void)
{
  return nvTimeFormat;  
}

unsigned char GetDateFormat(void)
{
  return nvDateFormat;
}

/******************************************************************************/

static unsigned char nvLinkAlarmEnable;

unsigned char QueryLinkAlarmEnable(void)
{
  return nvLinkAlarmEnable;  
}

void ToggleLinkAlarmEnable(void)
{
  if ( nvLinkAlarmEnable == 1 )
  {
    nvLinkAlarmEnable = 0;
  }
  else
  {
    nvLinkAlarmEnable = 1;  
  }
}

void InitializeLinkAlarmEnable(void)
{
  nvLinkAlarmEnable = 1;
  OsalNvItemInit(NVID_LINK_ALARM_ENABLE, 
                 sizeof(nvLinkAlarmEnable), 
                 &nvLinkAlarmEnable);
    
}

void SaveLinkAlarmEnable(void)
{
  OsalNvWrite(NVID_LINK_ALARM_ENABLE,
              NV_ZERO_OFFSET,
              sizeof(nvLinkAlarmEnable),
              &nvLinkAlarmEnable);
}


/* send a vibration to the wearer */
void GenerateLinkAlarm(void)
{
  tMessage Msg;
  
  SetupMessageAndAllocateBuffer(&Msg,SetVibrateMode,NO_MSG_OPTIONS);
  
  tSetVibrateModePayload* pMsgData;
  pMsgData = (tSetVibrateModePayload*) Msg.pBuffer;
  
  pMsgData->Enable = 1;
  pMsgData->OnDurationLsb = 0x00;
  pMsgData->OnDurationMsb = 0x01;
  pMsgData->OffDurationLsb = 0x00;
  pMsgData->OffDurationMsb = 0x01;
  pMsgData->NumberOfCycles = 5;
  
  RouteMsg(&Msg);
}

/******************************************************************************/

static unsigned int nvApplicationModeTimeout;
static unsigned int nvNotificationModeTimeout;

void InitializeModeTimeouts(void)
{
  
#ifdef ANALOG
  nvApplicationModeTimeout  = ONE_SECOND*60*10;
  nvNotificationModeTimeout = ONE_SECOND*30;
#else
  nvApplicationModeTimeout  = ONE_SECOND*60*10;
  nvNotificationModeTimeout = ONE_SECOND*30;  
#endif
  
  OsalNvItemInit(NVID_APPLICATION_MODE_TIMEOUT,
                 sizeof(nvApplicationModeTimeout),
                 &nvApplicationModeTimeout);
  
  OsalNvItemInit(NVID_NOTIFICATION_MODE_TIMEOUT,
                 sizeof(nvNotificationModeTimeout),
                 &nvNotificationModeTimeout);
 
}

unsigned int QueryApplicationModeTimeout(void)
{
  return nvApplicationModeTimeout;  
}

unsigned int QueryNotificationModeTimeout(void)
{
  return nvNotificationModeTimeout;  
}

/******************************************************************************/

static unsigned char nvSniffDebug;
static unsigned char nvBatteryDebug;
static unsigned char nvConnectionDebug;

void InitializeDebugFlags(void)
{
  nvSniffDebug = SNIFF_DEBUG_DEFAULT;
  OsalNvItemInit(NVID_SNIFF_DEBUG,
                 sizeof(nvSniffDebug),
                 &nvSniffDebug);
  
  nvBatteryDebug = BATTERY_DEBUG_DEFAULT;
  OsalNvItemInit(NVID_BATTERY_DEBUG,
                 sizeof(nvBatteryDebug),
                 &nvBatteryDebug);
    
  nvConnectionDebug = CONNECTION_DEBUG_DEFAULT;
  OsalNvItemInit(NVID_CONNECTION_DEBUG,
                 sizeof(nvConnectionDebug),
                 &nvConnectionDebug);
 
}

unsigned char QuerySniffDebug(void)
{
  return nvSniffDebug;  
}

unsigned char QueryBatteryDebug(void)
{
  return nvBatteryDebug; 
}

unsigned char QueryConnectionDebug(void)
{
  return nvConnectionDebug;  
}

/******************************************************************************/

unsigned int nvPairingModeDurationInSeconds;

void InitializePairingModeDuration(void)
{
  nvPairingModeDurationInSeconds = PAIRING_MODE_TIMEOUT_IN_SECONDS;
  OsalNvItemInit(NVID_PAIRING_MODE_DURATION, 
                 sizeof(nvPairingModeDurationInSeconds), 
                 &nvPairingModeDurationInSeconds);
  
  
}

unsigned int GetPairingModeDurationInSeconds(void)
{
  return nvPairingModeDurationInSeconds;  
}

/******************************************************************************/

static unsigned char nvSavePairingInfo;

void InitializeSavePairingInfo(void)
{
  nvSavePairingInfo = SAVE_PAIRING_INFO_DEFAULT;
  OsalNvItemInit(NVID_SAVE_PAIRING_INFO, 
                 sizeof(nvSavePairingInfo), 
                 &nvSavePairingInfo);
}

unsigned char QuerySavePairingInfo(void)
{
  return nvSavePairingInfo;  
}

/******************************************************************************/

static unsigned char nvEnableSniffEntry;

void InitializeEnableSniffEntry(void)
{
  nvEnableSniffEntry = ENABLE_SNIFF_ENTRY_DEFAULT;
  OsalNvItemInit(NVID_ENABLE_SNIFF_ENTRY, 
                 sizeof(nvEnableSniffEntry), 
                 &nvEnableSniffEntry);
}

unsigned char QueryEnableSniffEntry(void)
{
  return nvEnableSniffEntry;  
}

/******************************************************************************/
