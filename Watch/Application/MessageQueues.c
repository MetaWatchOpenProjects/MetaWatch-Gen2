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
/*! \file MessageQueues.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"           
#include "queue.h"              
#include "task.h"
#include "macro.h"

#include "Messages.h"
#include "BufferPool.h"
#include "MessageQueues.h"
#include "DebugUart.h"

typedef enum
{
  Normal,
  NormalWait,
  FromIsr,
  
} eRouteType;

static unsigned char AllQueuesReady = 0;
static void AllQueuesReadyCheck(void);

static void SendMessageToQueue(unsigned char Qindex, tHostMsg** ppMsg);
static void SendMessageToQueueWait(unsigned char Qindex, tHostMsg** ppMsg);
static void SendMessageToQueueFromIsr(unsigned char Qindex, tHostMsg** ppMsg);
static void Route(tHostMsg* pMsg, eRouteType RouteType);
void PrintMessageType(tHostMsg* pMsg);

xQueueHandle QueueHandles[TOTAL_QUEUES];

void SendToFreeQueue(tHostMsg** ppMsg)
{
  unsigned char result = xQueueSend(QueueHandles[FREE_QINDEX], ppMsg, 0);
  
  if (result == errQUEUE_FULL )
  {
    PrintString("Free Queue is full\r\n");
  }  
}

void SendToFreeQueueIsr(tHostMsg** ppMsg)
{
  unsigned char result = xQueueSendFromISR(QueueHandles[FREE_QINDEX], ppMsg, 0);
  
  if (result == errQUEUE_FULL )
  {
    PrintString("Free Queue is full (Isr)\r\n");
  }  
}

static void AllQueuesReadyCheck(void)
{
  if ( AllQueuesReady == 0 )
  {
    unsigned char Result = 1;
    
#ifdef DIGITAL
    for ( unsigned char i = 0; i < TOTAL_QUEUES; i++ )
    {
      if ( QueueHandles[i] == NULL )
      {
        Result &= 0; 
      }
    }
#else
    if (   QueueHandles[FREE_QINDEX] == 0
        || QueueHandles[BACKGROUND_QINDEX] == 0
        || QueueHandles[DISPLAY_QINDEX] == 0
        || QueueHandles[SPP_TASK_QINDEX] == 0 )
    {
      Result = 0;  
    }
#endif
  
    if ( Result )
    {
      AllQueuesReady = 1;  
    }
  }
}

void PrintQueueNameIsFull(unsigned char Qindex)
{
  switch(Qindex)
  {
  
  case FREE_QINDEX:        PrintString("Free Q is full\r\n");       break;
  case BACKGROUND_QINDEX:  PrintString("Background Q is full\r\n"); break;
  case DISPLAY_QINDEX:     PrintString("Display Q is full\r\n");    break;
  case LCD_TASK_QINDEX:    PrintString("LcdTask Q is full\r\n");    break;
  case SRAM_QINDEX:        PrintString("SerialRam Q is full\r\n");  break;
  case SPP_TASK_QINDEX:    PrintString("Spp Task Q is full\r\n");   break;
  default:                 PrintString("Unknown Q is full\r\n");    break;
  }

}
  
static void SendMessageToQueue(unsigned char Qindex, tHostMsg** ppMsg)
{
  /* if the queue is full, don't wait */
  if ( xQueueSend(QueueHandles[Qindex], ppMsg, 0) == errQUEUE_FULL )
  { 
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(ppMsg);
  }
  
}

static void SendMessageToQueueWait(unsigned char Qindex, tHostMsg** ppMsg)
{
  /* wait */
  if ( xQueueSend(QueueHandles[Qindex], ppMsg, portMAX_DELAY) == errQUEUE_FULL )
  { 
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(ppMsg);
  }

}

static void SendMessageToQueueFromIsr(unsigned char Qindex, tHostMsg** ppMsg)
{
  if ( xQueueSendFromISR(QueueHandles[Qindex], ppMsg, 0) == errQUEUE_FULL )
  {
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueueIsr(ppMsg);
  }

}

static unsigned char LastMessageType = 0;
static unsigned char AddNewline = 0;

static void PrintMessageType(tHostMsg* pMsg)
{

  unsigned char MessageType = pMsg->Type;
  
  if ( MessageType == LastMessageType )
  {
    PrintString("*");
    AddNewline = 1;
    return;
  }
  else
  {
    if ( AddNewline )
    {
      PrintString("\r\n");   
    }
  
    AddNewline = 0;
  }
  LastMessageType = MessageType;
  
  

  /* else 
   * 
   * this does not follow 80 character width rule because it is easier to
   * maintain this way 
  */
  switch (MessageType)
  {
  case InvalidMessage:             PrintStringAndHex("InvalidMessage 0x",MessageType);         break;
  case GetDeviceType:              PrintStringAndHex("GetDeviceType 0x",MessageType);          break;
  case GetDeviceTypeResponse:      PrintStringAndHex("GetDeviceTypeResponse 0x",MessageType);  break;
  case GetInfoString:              PrintStringAndHex("GetInfoString 0x",MessageType);          break;
  case GetInfoStringResponse:      PrintStringAndHex("GetInfoStringResponse 0x",MessageType);  break;
  case DiagnosticLoopback:         PrintStringAndHex("DiagnosticLoopback 0x",MessageType);     break;
  case EnterShippingModeMsg:       PrintStringAndHex("EnterShippingModeMsg 0x",MessageType);   break;
  case SoftwareResetMsg:           PrintStringAndHex("SoftwareResetMsg 0x",MessageType);       break;
  case ConnectionTimeoutMsg:       PrintStringAndHex("ConnectionTimeoutMsg 0x",MessageType);   break;
  case TurnRadioOnMsg:             PrintStringAndHex("TurnRadioOnMsg 0x",MessageType);         break;
  case TurnRadioOffMsg:            PrintStringAndHex("TurnRadioOffMsg 0x",MessageType);        break;
  case SppReserved:                PrintStringAndHex("SppReserved 0x",MessageType);            break;
  case PariringControlMsg:         PrintStringAndHex("PariringControlMsg 0x",MessageType);     break;
  case EnterSniffModeMsg:          PrintStringAndHex("EnterSniffModeMsg 0x",MessageType);      break;
  case LinkAlarmMsg:               PrintStringAndHex("LinkAlarmMsg 0x",MessageType);           break;
  case OledWriteBufferMsg:         PrintStringAndHex("OledWriteBufferMsg 0x",MessageType);        break;
  case OledConfigureModeMsg:       PrintStringAndHex("OledConfigureModeMsg 0x",MessageType);      break;
  case OledChangeModeMsg:          PrintStringAndHex("OledChangeModeMsg 0x",MessageType);         break;
  case OledWriteScrollBufferMsg:   PrintStringAndHex("OledWriteScrollBufferMsg 0x",MessageType);  break;
  case OledScrollMsg:              PrintStringAndHex("OledScrollMsg 0x",MessageType);          break;
  case OledShowIdleBufferMsg:      PrintStringAndHex("OledShowIdleBufferMsg 0x",MessageType);  break;
  case OledCrownMenuMsg:           PrintStringAndHex("OledCrownMenuMsg 0x",MessageType);       break;
  case OledCrownMenuButtonMsg:     PrintStringAndHex("OledCrownMenuButtonMsg 0x",MessageType); break;
  case AdvanceWatchHandsMsg:       PrintStringAndHex("AdvanceWatchHandsMsg 0x",MessageType);   break;
  case SetVibrateMode:             PrintStringAndHex("SetVibrateMode 0x",MessageType);         break;
  case SetRealTimeClock:           PrintStringAndHex("SetRealTimeClock 0x",MessageType);       break;
  case GetRealTimeClock:           PrintStringAndHex("GetRealTimeClock 0x",MessageType);       break;
  case GetRealTimeClockResponse:   PrintStringAndHex("GetRealTimeClockResponse 0x",MessageType);break;
  case StatusChangeEvent:          PrintStringAndHex("StatusChangeEvent 0x",MessageType);       break;
  case NvalOperationMsg:           PrintStringAndHex("NvalOperationMsg 0x",MessageType);        break;
  case NvalOperationResponseMsg:   PrintStringAndHex("NvalOperationResponseMsg 0x",MessageType); break;
  case GeneralPurposePhoneMsg:     PrintStringAndHex("GeneralPurposePhoneMsg 0x",MessageType); break;
  case GeneralPurposeWatchMsg:     PrintStringAndHex("GeneralPurposeWatchMsg 0x",MessageType); break;
  case ButtonEventMsg:             PrintStringAndHex("ButtonEventMsg 0x",MessageType);         break;
  case WriteBuffer:                PrintStringAndHex("WriteBuffer 0x",MessageType);            break;
  case ConfigureMode:              PrintStringAndHex("ConfigureMode 0x",MessageType);          break;
  case ConfigureIdleBufferSize:    PrintStringAndHex("ConfigureIdleBufferSize 0x",MessageType);break;
  case UpdateDisplay:              PrintStringAndHex("UpdateDisplay 0x",MessageType);          break;
  case LoadTemplate:               PrintStringAndHex("LoadTemplate 0x",MessageType);           break;
  case UpdateMyDisplaySram:        PrintStringAndHex("UpdateMyDisplaySram 0x",MessageType);    break;
  case UpdateMyDisplayLcd:         PrintStringAndHex("UpdateMyDisplayLcd 0x",MessageType);     break;
  case EnableButtonMsg:            PrintStringAndHex("EnableButtonMsg 0x",MessageType);        break;
  case DisableButtonMsg:           PrintStringAndHex("DisableButtonMsg 0x",MessageType);       break;
  case ReadButtonConfigMsg:        PrintStringAndHex("ReadButtonConfigMsg 0x",MessageType);    break;
  case ReadButtonConfigResponse:   PrintStringAndHex("ReadButtonConfigResponse 0x",MessageType);break;
  case BatteryChargeControl:       PrintStringAndHex("BatteryChargeControl 0x",MessageType);   break;
  case IdleUpdate:                 PrintStringAndHex("IdleUpdate 0x",MessageType);             break;
  case WatchDrawnScreenTimeout:    PrintStringAndHex("WatchDrawnScreenTimeout 0x",MessageType);break;
  case WriteLcd:                   PrintStringAndHex("WriteLcd 0x",MessageType);               break;
  case ClearLcd:                   PrintStringAndHex("ClearLcd 0x",MessageType);               break;
  case ClearLcdSpecial:            PrintStringAndHex("ClearLcdSpecial 0x",MessageType);        break;
  case ChangeModeMsg:              PrintStringAndHex("ChangeModeMsg 0x",MessageType);             break;
  case ModeTimeoutMsg:             PrintStringAndHex("ModeTimeoutMsg 0x",MessageType);            break;
  case WatchStatusMsg:             PrintStringAndHex("WatchStatusMsg 0x",MessageType);         break;
  case MenuModeMsg:                PrintStringAndHex("MenuModeMsg 0x",MessageType);            break;
  case BarCode:                    PrintStringAndHex("BarCode 0x",MessageType);                break;
  case ListPairedDevicesMsg:       PrintStringAndHex("ListPairedDevicesMsg 0x",MessageType);   break;
  case ConnectionStateChangeMsg:   PrintStringAndHex("ConnectionStateChangeMsg 0x",MessageType);  break;
  case ModifyTimeMsg:              PrintStringAndHex("ModifyTimeMsg 0x",MessageType);          break;
  case MenuButtonMsg:              PrintStringAndHex("MenuButtonMsg 0x",MessageType);          break;
  case ToggleSecondsMsg:           PrintStringAndHex("ToggleSecondsMsg 0x",MessageType);       break;
  case SplashTimeoutMsg:           PrintStringAndHex("SplashTimeoutMsg 0x",MessageType);       break;
  case LedChange:                  PrintStringAndHex("LedChange 0x",MessageType);              break;
  case AccelerometerRawData:       PrintStringAndHex("AccelerometerRawData 0x",MessageType);   break;           
  case QueryMemoryMsg:             PrintStringAndHex("QueryMemoryMsg 0x",MessageType);         break;
  case BatteryConfigMsg:           PrintStringAndHex("BatteryConfigMsg 0x",MessageType);       break;
  case LowBatteryWarningMsgHost:   PrintStringAndHex("LowBatteryWarningMsgHost 0x",MessageType);   break; 
  case LowBatteryBtOffMsgHost:     PrintStringAndHex("LowBatteryBtOffMsgHost 0x",MessageType);     break; 
  case ReadBatteryVoltageMsg:      PrintStringAndHex("ReadBatteryVoltageMsg 0x",MessageType);      break;
  case ReadBatteryVoltageResponse: PrintStringAndHex("ReadBatteryVoltageResponse 0x",MessageType); break;
  case ReadLightSensorMsg:         PrintStringAndHex("ReadLightSensorMsg 0x",MessageType);      break;
  case ReadLightSensorResponse:    PrintStringAndHex("ReadLightSensorResponse 0x",MessageType); break;
  case LowBatteryWarningMsg:       PrintStringAndHex("LowBatteryWarningMsg 0x",MessageType);   break; 
  case LowBatteryBtOffMsg:         PrintStringAndHex("LowBatteryBtOffMsg 0x",MessageType);     break; 
  
  default:                         PrintStringAndHex("Unknown Message Type 0x",MessageType);   break;
  }  
  
}

typedef void (*pfSendMessageType) (unsigned char,tHostMsg**);

pfSendMessageType SendMsgToQ;

void RouteMsg(tHostMsg** ppMsg)
{
  Route(*ppMsg,Normal); 
}

void RouteMsgBlocking(tHostMsg** ppMsg)
{
  Route(*ppMsg,NormalWait); 
}

void RouteMsgFromIsr(tHostMsg** ppMsg)
{
  Route(*ppMsg,FromIsr);
}


static void Route(tHostMsg* pMsg, eRouteType RouteType)
{
  /* pick which function to use */
  switch (RouteType)
  {
  case NormalWait:
    SendMsgToQ = SendMessageToQueueWait;
    break;
  case FromIsr:
    SendMsgToQ = SendMessageToQueueFromIsr;
    break;
  case Normal:
  default:
    SendMsgToQ = SendMessageToQueue;
    break;
  }

  AllQueuesReadyCheck();
  
  if ( pMsg == 0 )
  {
    PrintString("ERROR: Invalid message passed to Route function\r\n");  
  }
  else if ( AllQueuesReady == 0 )
  {
    PrintString("ERROR: Cannot Route when all queues are not ready\r\n");
    SendMsgToQ(FREE_QINDEX,&pMsg);
  }
  else
  {
    PrintMessageType(pMsg);
  
    switch (pMsg->Type)
    {
    case InvalidMessage:                SendMsgToQ(FREE_QINDEX,&pMsg); break;
    case GetDeviceType:                 SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case GetDeviceTypeResponse:         SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case GetInfoString:                 SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case GetInfoStringResponse:         SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case DiagnosticLoopback:            SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case EnterShippingModeMsg:          SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case SoftwareResetMsg:              SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case ConnectionTimeoutMsg:          SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case TurnRadioOnMsg:                SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case TurnRadioOffMsg:               SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case SppReserved:                   SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case PariringControlMsg:            SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case EnterSniffModeMsg:             SendMsgToQ(SPP_TASK_QINDEX,&pMsg); break;
    case LinkAlarmMsg:                  SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledWriteBufferMsg:            SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledConfigureModeMsg:          SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledChangeModeMsg:             SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledWriteScrollBufferMsg:      SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledScrollMsg:                 SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledShowIdleBufferMsg:         SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case OledCrownMenuMsg:              SendMsgToQ(DISPLAY_QINDEX,&pMsg);    break;
    case OledCrownMenuButtonMsg:        SendMsgToQ(DISPLAY_QINDEX,&pMsg);    break;
    case AdvanceWatchHandsMsg:          SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case SetVibrateMode:                SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case SetRealTimeClock:              SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case GetRealTimeClock:              SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case GetRealTimeClockResponse:      SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case StatusChangeEvent:             SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case NvalOperationMsg:              SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case NvalOperationResponseMsg:      SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case GeneralPurposePhoneMsg:        SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case GeneralPurposeWatchMsg:        SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case ButtonEventMsg:                SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case WriteBuffer:                   SendMsgToQ(SRAM_QINDEX,&pMsg);       break;
    case ConfigureMode:                 SendMsgToQ(DISPLAY_QINDEX,&pMsg);    break;
    case ConfigureIdleBufferSize:       SendMsgToQ(DISPLAY_QINDEX,&pMsg);    break;
    case UpdateDisplay:                 SendMsgToQ(SRAM_QINDEX,&pMsg); break;
    case LoadTemplate:                  SendMsgToQ(SRAM_QINDEX,&pMsg); break;
    case UpdateMyDisplaySram:           SendMsgToQ(SRAM_QINDEX,&pMsg); break;
    case UpdateMyDisplayLcd:            SendMsgToQ(LCD_TASK_QINDEX,&pMsg);   break;
    case EnableButtonMsg:               SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case DisableButtonMsg:              SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case ReadButtonConfigMsg:           SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case ReadButtonConfigResponse:      SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case BatteryChargeControl:          SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case IdleUpdate:                    SendMsgToQ(DISPLAY_QINDEX,&pMsg);  break;
    case WatchDrawnScreenTimeout:       SendMsgToQ(DISPLAY_QINDEX,&pMsg);  break;
    case WriteLcd:                      SendMsgToQ(LCD_TASK_QINDEX,&pMsg); break;
    case ClearLcd:                      SendMsgToQ(LCD_TASK_QINDEX,&pMsg); break;
    case ClearLcdSpecial:               SendMsgToQ(SRAM_QINDEX,&pMsg);    break;
    case ChangeModeMsg:                 SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case ModeTimeoutMsg:                SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case WatchStatusMsg:                SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case MenuModeMsg:                   SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case BarCode:                       SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case ListPairedDevicesMsg:          SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case ConnectionStateChangeMsg:      SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case ModifyTimeMsg:                 SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case MenuButtonMsg:                 SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case ToggleSecondsMsg:              SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case SplashTimeoutMsg:              SendMsgToQ(DISPLAY_QINDEX,&pMsg); break;
    case LedChange:                     SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case AccelerometerRawData:          SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case QueryMemoryMsg:                SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case BatteryConfigMsg:              SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case LowBatteryWarningMsgHost:      SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case LowBatteryBtOffMsgHost:        SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case ReadBatteryVoltageMsg:         SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case ReadBatteryVoltageResponse:    SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case ReadLightSensorMsg:            SendMsgToQ(BACKGROUND_QINDEX,&pMsg); break;
    case ReadLightSensorResponse:       SendMsgToQ(SPP_TASK_QINDEX,&pMsg);   break;
    case LowBatteryWarningMsg:          SendMsgToQ(DISPLAY_QINDEX,&pMsg);    break;
    case LowBatteryBtOffMsg:            SendMsgToQ(DISPLAY_QINDEX,&pMsg);    break;
    default:                            SendMsgToQ(FREE_QINDEX,&pMsg);       break;
    }
  }
  
}