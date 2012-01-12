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

#include "hal_board_type.h"

#include "Messages.h"
#include "BufferPool.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Statistics.h"

typedef enum
{
  Normal,
  NormalWait,
  FromIsr,
  
} eRouteType;

static unsigned char AllQueuesReady = 0;
static void AllQueuesReadyCheck(void);

static void SendMessageToQueue(unsigned char Qindex, tMessage* pMsg);
static void SendMessageToQueueWait(unsigned char Qindex, tMessage* pMsg);
static void SendMessageToQueueFromIsr(unsigned char Qindex, tMessage* pMsg);
static void Route(tMessage* pMsg, eRouteType RouteType);

xQueueHandle QueueHandles[TOTAL_QUEUES];

#define NORMAL_CONTEXT ( 0 )
#define ISR_CONTEXT    ( 1 )

/* most messages do not have a buffer so there really isn't anything to free */
void SendToFreeQueue(tMessage* pMsg)
{
  if ( pMsg->pBuffer != 0 )
  {
    /* The free queue is different.  
     * It holds pointers to buffers not messaages 
     */
    BPL_FreeMessageBuffer(pMsg->pBuffer);
  }
  
}
  
void SendToFreeQueueIsr(tMessage* pMsg)
  {
  if ( pMsg->pBuffer != 0 )
  {
    BPL_FreeMessageBufferFromIsr(pMsg->pBuffer);
  }  
}

static void SendToFreeQueueContext(tMessage* pMsg, eRouteType RouteType)
{
  if ( RouteType == Normal )
{
    SendToFreeQueue(pMsg);  
  }
  else
  {
    SendToFreeQueueIsr(pMsg);  
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
  gAppStats.QueueOverflow = 1;
  
  switch(Qindex)
  {
  
  case FREE_QINDEX:        PrintString("Shoud not get here\r\n");   break;
  case BACKGROUND_QINDEX:  PrintString("Background Q is full\r\n"); break;
  case DISPLAY_QINDEX:     PrintString("Display Q is full\r\n");    break;
  case SPP_TASK_QINDEX:    PrintString("Spp Task Q is full\r\n");   break;
  default:                 PrintString("Unknown Q is full\r\n");    break;
  }

}
  

/* if the queue is full, don't wait */
static void SendMessageToQueue(unsigned char Qindex, tMessage* pMsg)
{
  if ( Qindex == FREE_QINDEX )
  {
    SendToFreeQueue(pMsg);  
  }
  else if ( errQUEUE_FULL ==  xQueueSend(QueueHandles[Qindex],pMsg,DONT_WAIT) )
  { 
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(pMsg);
  }
  
}

static void SendMessageToQueueWait(unsigned char Qindex, tMessage* pMsg)
{
  /* wait */
  if ( xQueueSend(QueueHandles[Qindex], pMsg, portMAX_DELAY) == errQUEUE_FULL )
  { 
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(pMsg);
  }

}

static void SendMessageToQueueFromIsr(unsigned char Qindex, tMessage* pMsg)
{
  signed portBASE_TYPE HigherPriorityTaskWoken;
  
  if ( Qindex == FREE_QINDEX )
  {
    SendToFreeQueueIsr(pMsg);  
  }
  else if ( errQUEUE_FULL == xQueueSendFromISR(QueueHandles[Qindex],
                                               pMsg,
                                               &HigherPriorityTaskWoken))
  {
    PrintQueueNameIsFull(Qindex);
      SendToFreeQueueIsr(pMsg);
  }
  
  if ( HigherPriorityTaskWoken == pdTRUE )
  {
    portYIELD();
  }

}

static unsigned char LastMessageType = 0;
static unsigned char AddNewline = 0;

void PrintMessageType(tMessage* pMsg)
{

  unsigned char MessageType = pMsg->Type;
  
  /* if the message is the same as the last then print an asterix */
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
  
  

  /*
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
  case ReadRssiMsg:                PrintStringAndHex("ReadRssiMsg 0x",MessageType);                break;
  case PairingControlMsg:          PrintStringAndHex("PairingControlMsg 0x",MessageType);          break;
  case ReadRssiResponseMsg:        PrintStringAndHex("ReadRssiResponseMsg 0x",MessageType);        break;
  case SniffControlMsg:            PrintStringAndHex("SniffControlMsg 0x",MessageType);            break;
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
  case ButtonStateMsg:             PrintStringAndHex("ButtonStateMsg 0x",MessageType);             break;
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
  case ConfigureDisplay:           PrintStringAndHex("ConfigureDisplay 0x",MessageType);           break;
  case ConfigureIdleBufferSize:    PrintStringAndHex("ConfigureIdleBufferSize 0x",MessageType);break;
  case UpdateDisplay:              PrintStringAndHex("UpdateDisplay 0x",MessageType);          break;
  case LoadTemplate:               PrintStringAndHex("LoadTemplate 0x",MessageType);           break;
  case EnableButtonMsg:            PrintStringAndHex("EnableButtonMsg 0x",MessageType);        break;
  case DisableButtonMsg:           PrintStringAndHex("DisableButtonMsg 0x",MessageType);       break;
  case ReadButtonConfigMsg:        PrintStringAndHex("ReadButtonConfigMsg 0x",MessageType);    break;
  case ReadButtonConfigResponse:   PrintStringAndHex("ReadButtonConfigResponse 0x",MessageType);break;
  case BatteryChargeControl:       PrintStringAndHex("BatteryChargeControl 0x",MessageType);   break;
  case IdleUpdate:                 PrintStringAndHex("IdleUpdate 0x",MessageType);             break;
  case WatchDrawnScreenTimeout:    PrintStringAndHex("WatchDrawnScreenTimeout 0x",MessageType);break;
  case SplashTimeoutMsg:           PrintStringAndHex("SplashTimeoutMsg 0x",MessageType);           break;
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
  case LedChange:                  PrintStringAndHex("LedChange 0x",MessageType);              break;
  case AccelerometerHostMsg:       PrintStringAndHex("AccelerometerHostMsg 0x",MessageType);       break;           
  case AccelerometerEnableMsg:     PrintStringAndHex("AccelerometerEnableMsg 0x",MessageType);     break;           
  case AccelerometerDisableMsg:    PrintStringAndHex("AccelerometerDisableMsg 0x",MessageType);    break;           
  case AccelerometerSendDataMsg:   PrintStringAndHex("AccelerometerSendDataMsg 0x",MessageType);   break;           
  case AccelerometerAccessMsg:     PrintStringAndHex("AccelerometerAccessMsg 0x",MessageType);     break;           
  case AccelerometerResponseMsg:   PrintStringAndHex("AccelerometerResponseMsg 0x",MessageType);   break;           
  case AccelerometerSetupMsg:      PrintStringAndHex("AccelerometerSetupMsg 0x",MessageType);      break;
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
  case SniffControlAckMsg:         PrintStringAndHex("SniffControlAckMsg 0x",MessageType);         break; 
  case SniffStateChangeMsg:        PrintStringAndHex("SniffStateChangeMsg 0x",MessageType);        break; 
  
  default:                         PrintStringAndHex("Unknown Message Type 0x",MessageType);   break;
  }  
  
}

typedef void (*pfSendMessageType) (unsigned char,tMessage*);

static pfSendMessageType SendMsgToQ;

void RouteMsg(tMessage* pMsg)
{
  Route(pMsg,Normal); 
}

void RouteMsgBlocking(tMessage* pMsg)
{
  Route(pMsg,NormalWait); 
}

void RouteMsgFromIsr(tMessage* pMsg)
{
  Route(pMsg,FromIsr);
}


static void Route(tMessage* pMsg, eRouteType RouteType)
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
    /* if this happens in ISR context that is OK - the system is broke anyway */
    PrintString("ERROR: Invalid message passed to Route function\r\n");  
  }
  else if ( AllQueuesReady == 0 )
  {
    PrintString("ERROR: Cannot Route when all queues are not ready\r\n");
    SendToFreeQueueContext(pMsg,RouteType);  
  }
  else
  {
  
    switch (pMsg->Type)
    {
    case InvalidMessage:                SendMsgToQ(FREE_QINDEX,pMsg);       break;
    case GetDeviceType:                 SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case GetDeviceTypeResponse:         SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case GetInfoString:                 SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case GetInfoStringResponse:         SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case DiagnosticLoopback:            SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case EnterShippingModeMsg:          SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case SoftwareResetMsg:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ConnectionTimeoutMsg:          SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case TurnRadioOnMsg:                SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case TurnRadioOffMsg:               SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case ReadRssiMsg:                   SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case PairingControlMsg:             SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case ReadRssiResponseMsg:           SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case SniffControlMsg:               SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case LinkAlarmMsg:                  SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledWriteBufferMsg:            SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledConfigureModeMsg:          SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledChangeModeMsg:             SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledWriteScrollBufferMsg:      SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledScrollMsg:                 SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledShowIdleBufferMsg:         SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledCrownMenuMsg:              SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case OledCrownMenuButtonMsg:        SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case AdvanceWatchHandsMsg:          SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case SetVibrateMode:                SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ButtonStateMsg:                SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case SetRealTimeClock:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case GetRealTimeClock:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case GetRealTimeClockResponse:      SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case StatusChangeEvent:             SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case NvalOperationMsg:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case NvalOperationResponseMsg:      SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case GeneralPurposePhoneMsg:        SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case GeneralPurposeWatchMsg:        SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ButtonEventMsg:                SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case WriteBuffer:                   SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ConfigureDisplay:              SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ConfigureIdleBufferSize:       SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case UpdateDisplay:                 SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case LoadTemplate:                  SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case EnableButtonMsg:               SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case DisableButtonMsg:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ReadButtonConfigMsg:           SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ReadButtonConfigResponse:      SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case BatteryChargeControl:          SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case IdleUpdate:                    SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case WatchDrawnScreenTimeout:       SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case SplashTimeoutMsg:              SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ChangeModeMsg:                 SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ModeTimeoutMsg:                SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case WatchStatusMsg:                SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case MenuModeMsg:                   SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case BarCode:                       SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ListPairedDevicesMsg:          SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ConnectionStateChangeMsg:      SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ModifyTimeMsg:                 SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case MenuButtonMsg:                 SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case ToggleSecondsMsg:              SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case LedChange:                     SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case AccelerometerHostMsg:          SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case AccelerometerEnableMsg:        SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case AccelerometerDisableMsg:       SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;       
    case AccelerometerSendDataMsg:      SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;        
    case AccelerometerAccessMsg:        SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;        
    case AccelerometerResponseMsg:      SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;        
    case AccelerometerSetupMsg:         SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case QueryMemoryMsg:                SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case BatteryConfigMsg:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case LowBatteryWarningMsgHost:      SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case LowBatteryBtOffMsgHost:        SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case ReadBatteryVoltageMsg:         SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ReadBatteryVoltageResponse:    SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case ReadLightSensorMsg:            SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ReadLightSensorResponse:       SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case LowBatteryWarningMsg:          SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case LowBatteryBtOffMsg:            SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case SniffControlAckMsg:            SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case SniffStateChangeMsg:           SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    default:                            SendMsgToQ(FREE_QINDEX,pMsg);       break;
    }
  }
  
}

void AssignSppQueueHandle(xQueueHandle SppHandle)
{
  QueueHandles[SPP_TASK_QINDEX] = SppHandle;  
}


void SetupMessage(tMessage* pMsg,
                  unsigned char Type,
                  unsigned char Options)
{
  pMsg->Length = 0;
  pMsg->Type = Type;
  pMsg->Options = Options;
  pMsg->pBuffer = NULL;  
}

void SetupMessageAndAllocateBuffer(tMessage* pMsg,
                                   unsigned char Type,
                                   unsigned char Options)
{
  pMsg->Length = 0;
  pMsg->Type = Type;
  pMsg->Options = Options;
  pMsg->pBuffer = BPL_AllocMessageBuffer();

}

/* \return 1 when all task queues are empty and the part can go into sleep 
 * mode 
 */
#ifdef DIGITAL
unsigned char AllTaskQueuesEmpty(void)
{
  unsigned char result = 0;  
  
  ENTER_CRITICAL_REGION_QUICK();
      //QueueHandles[FREE_QINDEX]->uxMessagesWaiting == NUM_MSG_BUFFERS 
      //&& 
  if (   QueueHandles[BACKGROUND_QINDEX]->uxMessagesWaiting == 0 
      && QueueHandles[DISPLAY_QINDEX]->uxMessagesWaiting == 0 
      && QueueHandles[SPP_TASK_QINDEX]->uxMessagesWaiting == 0 )
  {  
    result = 1; 
  }
  
  LEAVE_CRITICAL_REGION_QUICK();
       
  return result;
  
}
#else
unsigned char AllTaskQueuesEmpty(void)
{
  unsigned char result = 0;  
  
  ENTER_CRITICAL_REGION_QUICK();
      
  if (   QueueHandles[BACKGROUND_QINDEX]->uxMessagesWaiting == 0 
      && QueueHandles[DISPLAY_QINDEX]->uxMessagesWaiting == 0 
      && QueueHandles[SPP_TASK_QINDEX]->uxMessagesWaiting == 0 )
  {  
    result = 1; 
  }
  
  LEAVE_CRITICAL_REGION_QUICK();
       
  return result;
  
}
#endif
 