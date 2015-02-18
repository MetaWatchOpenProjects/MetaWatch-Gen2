//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Messages.h"
#include "MessageInfo.h"
#include "DebugUart.h"
#include "Log.h"
#include "hal_boot.h"
#include "hal_rtc.h" // To12H
#include "Property.h"

static char const * const ResetReason[] = {
  "NoIntrptPending",
  "BOR:Highest",
  "BOR:RST/NMI",
  "BOR:PMMSW",
  "BOR:Wakeup frm LPMx.5",
  "BOR:Security violation",
  "POR:SVSL",
  "POR:SVSH",
  "POR:SVML_OVP",
  "POR:SVMH_OVP",
  "POR:PMMSW",
  "PUC:WDT",
  "PUC:WDT pwd violation",
  "PUC:Flash pwd violation",
  "Reserved",
  "PUC:PERF Invld ISR",
  "PUC:PMM pwd violation",
  "Reserved"
};

static char const * const ResetCode[] = {
  "Unknown",
  "StackOflw",
  "TaskFail",
  "WrpInit",
  "Button",
  "Bootldr",
  "ShipMode",
  "HciLL",
  "UART",
  "TermMode",
  "HciHw",
  "Alloc",
  "FreeMem",
  "Wdt"
};

static char const * const ConnType[] = {
  "Tunnl", "Spp", "Hfp", "Map", "Mwm", "Ancs", "N/A"
};

static char const * const Bluetooth[] = {
  "Unknown", "Init", "RdOn", "Conn", "Dscn", "RdOff", "Ship"
};

static char const * const MuxMode[] = {
  "Gnd", "Serial", "SBW", "Off"
};

static char const * const Property[] = {
  "24H", "DDMM", "Sec", "Grid", "LED", "Full", "Invt"
};

typedef struct
{
  unsigned int Timestamp;
  unsigned char MsgType;
  unsigned char MsgOpt;
} ResetLog_t;
#define RESET_LOG_SIZE      (sizeof(ResetLog_t))

typedef struct
{
  unsigned char Index;
  unsigned char Num;
} LogBuffer_t;

#define RESET_LOG_NUM         18 // 18x4 = 72
#define TOTAL_LOG_SIZE        (RESET_LOG_SIZE * RESET_LOG_NUM)

#if __IAR_SYSTEMS_ICC__
__no_init __root LogBuffer_t niLogBuffer @ LOG_BUFFER_ADDR;
__no_init __root ResetLog_t niLog[RESET_LOG_NUM] @ RESET_LOG_ADDR;
#else
extern LogBuffer_t niLogBuffer;
extern ResetLog_t niLog[RESET_LOG_NUM];
#endif

typedef struct
{
  unsigned int ResetSource;
  unsigned char ResetType;
  unsigned char ResetCode;
  unsigned char ResetValue;
  unsigned char RadioReadyToSleep;
  unsigned char DisplayQueue;
  unsigned char WrapperQueue;
  unsigned char WdtNum;
  unsigned char MuxMode;
  unsigned char Property;
  unsigned char Bluetooth;
  unsigned char ConnType;
  unsigned char Disconnects;
  unsigned char Battery;
  unsigned char SrvChgdHandle;
} State_t;
#define STATE_INFO_SIZE     (sizeof(State_t))

#define STATE_ADDR          (0x1C00) // 16

extern unsigned char niWdtNum;
extern unsigned char niResetCode;
extern unsigned char niProperty;

static unsigned char *pStateLog;
static unsigned char SavedLogNum = 0;

void InitStateLog(void)
{
  niLogBuffer.Index = 0;
  niLogBuffer.Num = 0;
  niWdtNum = 0;
  niProperty = PROP_DEFAULT;
  niResetCode = RESET_UNKNOWN;

  memset((unsigned char *)&niLog, 0, TOTAL_LOG_SIZE);
}

void SaveStateLog(void)
{
  volatile State_t *pState = (volatile State_t *)STATE_ADDR;

  PrintF("State Addr:x%04X size:%u", pState, sizeof(State_t));
  PrintQ((unsigned char *)pState, STATE_INFO_SIZE);
  PrintF("LgIdx:%u LgNm:%u", niLogBuffer.Index, niLogBuffer.Num);
  
  if (niLogBuffer.Num > RESET_LOG_NUM) niLogBuffer.Num = niLogBuffer.Index;
  SavedLogNum = niLogBuffer.Num;

  /* save old state and log to heap for reading */
  pStateLog = (unsigned char *)pvPortMalloc(STATE_INFO_SIZE + niLogBuffer.Num * RESET_LOG_SIZE);

  if (pStateLog)
  {
    // fill heap with stateInfo and whole log
    memcpy(pStateLog, (unsigned char *)pState, STATE_INFO_SIZE);

    ResetLog_t *pLog = (ResetLog_t *)(pStateLog + STATE_INFO_SIZE);

    while (niLogBuffer.Num)
    {
      if (!niLog[niLogBuffer.Index].MsgType) niLogBuffer.Index = 0;
      memcpy(pLog++, (unsigned char *)&niLog[niLogBuffer.Index++], RESET_LOG_SIZE);
      if (niLogBuffer.Index == RESET_LOG_NUM) niLogBuffer.Index = 0;
      niLogBuffer.Num --;
    }
  }
}

void UpdateLog(unsigned char MsgType, unsigned char MsgOpt)
{
  if (MsgType == ButtonStateMsg || MsgType == WriteBufferMsg) return;
  
  // get rtc, convert to log format
  unsigned int Timestamp = ToBin(To12H(RTCHOUR)) << 12;
  Timestamp += ToBin(RTCMIN) << 6;
  Timestamp += ToBin(RTCSEC);
//  PrintE("Log I:%u %u:%u:%u", niLogIndex, Timestamp >> 12, (Timestamp & 0x0FC0) >> 6, Timestamp & 0x003F);

  niLog[niLogBuffer.Index].Timestamp = Timestamp;
  niLog[niLogBuffer.Index].MsgType = MsgType;
  niLog[niLogBuffer.Index].MsgOpt = MsgOpt;
//  PrintF("%s %02X", MsgInfo[MsgType].MsgStr, MsgOpt);

  if (niLogBuffer.Num < RESET_LOG_NUM) niLogBuffer.Num ++;
  if (++niLogBuffer.Index >= RESET_LOG_NUM) niLogBuffer.Index = 0;
}

// show to mux
void ShowStateLog(void)
{
  if (!pStateLog) return;

  ShowStateInfo();

  PrintS(" Time   Message  Options");
  PrintS("--------------------------");

  ResetLog_t *pLog = (ResetLog_t *)(pStateLog + STATE_INFO_SIZE);
  unsigned char i;

  for (i = 0; i < SavedLogNum; ++i)
  {
    PrintF("%u:%u:%u %s x%02X",
      pLog[i].Timestamp >> 12, (pLog[i].Timestamp & 0x0FC0) >> 6, pLog[i].Timestamp & 0x003F,
      MsgInfo[pLog[i].MsgType].MsgStr, pLog[i].MsgOpt);
  }

  vPortFree(pStateLog);
  pStateLog = NULL;
}

void ShowStateInfo(void)
{
  if (!pStateLog) return;
  State_t *pState = (State_t *)pStateLog;

  PrintS("---- State before reset ----");
  if (pState->ResetSource > 0x22) pState->ResetSource = 0x22;
  PrintS(ResetReason[pState->ResetSource >> 1]);

  PrintF("ResetCode:%s %u", ResetCode[pState->ResetCode], pState->ResetValue);
  PrintF("RadioIdle:%u", pState->RadioReadyToSleep);
  PrintF("DisplayQueue:%u", pState->DisplayQueue);
  PrintF("WrapperQueue:%u", pState->WrapperQueue);

  if (pState->ResetSource == SYSRSTIV_WDTTO || pState->ResetSource == SYSRSTIV_WDTKEY)
  {
    PrintF("# WDT %s", pState->ResetSource == SYSRSTIV_WDTTO ? "Failsafe" : "Forced");
    pState->WdtNum ++;
  }

  PrintF("Total WDT:%u", pState->WdtNum);
  PrintF("MuxMode:%s", MuxMode[pState->MuxMode]);

  unsigned char i;

  PrintW("Property:");
  for (i = 0; i < 7; ++i)
  {
    if (pState->Property & (1 << i))
      {PrintW(Property[i]); PrintC(SPACE);}
  }
  PrintR();
  
  PrintF("State:%s", Bluetooth[pState->Bluetooth]);

  PrintW("Conn:");
  for (i = 0; i < 7; ++i)
  {
    if (pState->ConnType & (1 << i))
      {PrintW(ConnType[i]); PrintC(SPACE);}
  }
  PrintR();

  PrintF("Dscnn:%u", pState->Disconnects);
  PrintF("Battery:%u%c", pState->Battery, PERCENT);
  PrintF("SrvChgdHandle:%u", pState->SrvChgdHandle);
  PrintS("-------- State end --------");
}

#define MAX_LOG_PAYLOAD_LEN   (48 - MSG_OVERHEAD_LENGTH)

/* send log to remote */
void SendStateLog(void)
{
  if (!pStateLog) return;

  unsigned char *pData = pStateLog;
  unsigned char Len = STATE_INFO_SIZE + SavedLogNum * RESET_LOG_SIZE;
  tMessage Msg ={0, LogMsg, MSG_OPT_NONE, NULL};

  while (Len)
  {
    Msg.Length = Len > MAX_LOG_PAYLOAD_LEN ? MAX_LOG_PAYLOAD_LEN : Len;

    if (CreateMessage(&Msg))
    {
      memcpy(Msg.pBuffer, pData, Msg.Length);
      pData += Msg.Length;
      Len -= Msg.Length;
      RouteMsg(&Msg);
    }
  }

  vPortFree(pStateLog);
  pStateLog = NULL;
}
