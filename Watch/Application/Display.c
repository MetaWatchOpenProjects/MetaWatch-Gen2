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
#include "Wrapper.h"
#include "OneSecondTimers.h"

/******************************************************************************/

/* 
 * these are setup to match RTC 
 * days of week are 0-6 and months are 1-12 
 */
const tString DaysOfTheWeek[3][7][4] =
{
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
  {"su", "ma", "ti", "ke", "to", "pe", "la"},
  {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"}
};

const tString MonthsOfYear[3][13][7] =
{
  {"???","Jan","Feb","Mar","Apr","May","June",
  "July","Aug","Sep","Oct","Nov","Dec"},
  {"???","tami", "helmi", "maalis", "huhti", "touko", "kesä",
   "heinä", "elo", "syys", "loka", "marras", "joulu"},
  {"???","Jan","Feb","Mar","Apr","Mai","Jun",
   "Jul","Aug","Sep","Okt","Nov","Dez"}
};

/******************************************************************************/

static unsigned char nvTimeFormat;
static unsigned char nvDateFormat;
static unsigned char nvLanguage;

void Init12H(void)
{
  nvTimeFormat = TWELVE_HOUR;
  OsalNvItemInit(NVID_TIME_FORMAT, sizeof(nvTimeFormat), &nvTimeFormat);
}

unsigned char Get12H(void)
{
  return nvTimeFormat;  
}

void Set12H(unsigned char Fmt)
{
  nvTimeFormat = Fmt;
}

void InitMonthFirst(void)
{
  nvDateFormat = MONTH_FIRST;
  OsalNvItemInit(NVID_DATE_FORMAT, sizeof(nvDateFormat), &nvDateFormat);
}

unsigned char GetMonthFirst(void)
{
  return nvDateFormat;
}

void SetMonthFirst(unsigned char Mon1st)
{
  nvDateFormat = Mon1st;
}

void InitLanguage(void)
{
  nvLanguage = LANG_EN;
  OsalNvItemInit(NVID_LANGUAGE, sizeof(nvLanguage), &nvLanguage);
}

unsigned char GetLanguage(void)
{
  return nvLanguage;
}

/******************************************************************************/

static unsigned char nvLinkAlarmEnable;

unsigned char LinkAlarmEnable(void)
{
  return nvLinkAlarmEnable;  
}

void ToggleLinkAlarmEnable(void)
{
  nvLinkAlarmEnable = !nvLinkAlarmEnable;
}

void InitLinkAlarmEnable(void)
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
  
  SetupMessageAndAllocateBuffer(&Msg,SetVibrateMode,MSG_OPT_NONE);
  tSetVibrateModePayload* pMsgData;
  pMsgData = (tSetVibrateModePayload*) Msg.pBuffer;
  
  pMsgData->Enable = 1;
  pMsgData->OnDurationLsb = 0xC8;
  pMsgData->OnDurationMsb = 0x00;
  pMsgData->OffDurationLsb = 0xF4;
  pMsgData->OffDurationMsb = 0x01;
  pMsgData->NumberOfCycles = 3;
  
  RouteMsg(&Msg);
}

/******************************************************************************/

static unsigned int nvApplicationModeTimeout;
static unsigned int nvNotificationModeTimeout;
static unsigned int nvMusicModeTimeout;


void InitModeTimeout(void)
{
  
#ifdef ANALOG
  nvApplicationModeTimeout  = ONE_SECOND * 60 * 10;
  nvNotificationModeTimeout = ONE_SECOND * 30;
#else
  nvApplicationModeTimeout  = ONE_SECOND * 60 * 10;
  nvNotificationModeTimeout = ONE_SECOND * 30;
  nvMusicModeTimeout = ONE_SECOND * 60 * 10;
#endif
  
  OsalNvItemInit(NVID_APP_MODE_TIMEOUT,
                 sizeof(nvApplicationModeTimeout),
                 &nvApplicationModeTimeout);
  
  OsalNvItemInit(NVID_NOTIF_MODE_TIMEOUT,
                 sizeof(nvNotificationModeTimeout),
                 &nvNotificationModeTimeout);

  OsalNvItemInit(NVID_MUSIC_MODE_TIMEOUT,
                 sizeof(nvMusicModeTimeout),
                 &nvMusicModeTimeout);
}

unsigned int QueryModeTimeout(unsigned char Mode)
{
  if (Mode == MUSIC_MODE) return nvMusicModeTimeout;
  if (Mode == NOTIF_MODE) return nvNotificationModeTimeout;
  return nvApplicationModeTimeout;
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

unsigned int nvRadioOffTimeout;

void InitRadioOffTimeout(void)
{
  nvRadioOffTimeout = PAIRING_MODE_TIMEOUT_IN_SECONDS;
  OsalNvItemInit(NVID_PAIRING_MODE_DURATION, 
                 sizeof(nvRadioOffTimeout), 
                 &nvRadioOffTimeout);
}

unsigned int RadioOffTimeout(void)
{
  return nvRadioOffTimeout;  
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

unsigned char SavedPairingInfo(void)
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

static unsigned char nvExitSniffOnReceive;

void InitializeExitSniffOnReceive(void)
{
  nvExitSniffOnReceive = EXIT_SNIFF_ON_RECEIVE_DEFAULT;
  OsalNvItemInit(NVID_EXIT_SNIFF_ON_RECEIVE, 
                 sizeof(nvExitSniffOnReceive), 
                 &nvExitSniffOnReceive);
}

unsigned char QueryExitSniffOnReceive(void)
{
  return nvExitSniffOnReceive;  
}

/******************************************************************************/

unsigned char QueryAnalogWatch(void)
{
  unsigned char result = 0;
  
#if defined(WATCH)
  #if defined(ANALOG)
    result = 1;
  #endif
#endif
    
  return result;
    
}
