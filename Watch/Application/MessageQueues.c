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

#include "hal_board_type.h"

#include "Messages.h"
#include "BufferPool.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Statistics.h"

#ifdef MESSAGE_QUEUE_DEBUG
static unsigned char AllQueuesReady = 0;
static void AllQueuesReadyCheck(void);
#endif

static void SendMsgToQ(unsigned char Qindex, tMessage* pMsg);

xQueueHandle QueueHandles[TOTAL_QUEUES];

/* most messages do not have a buffer so there really isn't anything to free */
void SendToFreeQueue(tMessage* pMsg)
{
  if ( pMsg->pBuffer != 0 )
  {
    /* The free queue is different.  
     * It holds pointers to buffers not messages 
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

#ifdef MESSAGE_QUEUE_DEBUG
static void AllQueuesReadyCheck(void)
{
  if ( AllQueuesReady == 0 )
  {
    unsigned char Result = 1;
    
    unsigned char i;
    for (i = 0; i < TOTAL_QUEUES; i++ )
    {
      if ( QueueHandles[i] == NULL )
      {
        Result &= 0; 
      }
    }
  
    if ( Result )
    {
      AllQueuesReady = 1;  
    }
  }
}
#endif

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
static void SendMsgToQ(unsigned char Qindex, tMessage* pMsg)
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

#if 0
/* this was kept for reference */
static void SendMessageToQueueWait(unsigned char Qindex, tMessage* pMsg)
{
  /* wait */
  if ( xQueueSend(QueueHandles[Qindex], pMsg, portMAX_DELAY) == errQUEUE_FULL )
  { 
    PrintQueueNameIsFull(Qindex);
    SendToFreeQueue(pMsg);
  }
  
}
#endif

/* send a message to a specific queue from an isr 
 * routing requires 25 us
 * putting a message into a queue (in interrupt context) requires 28 us
 * this requires 1/2 the time of using route
 */
void SendMessageToQueueFromIsr(unsigned char Qindex, tMessage* pMsg)
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
  
  
#if 0
  /* This is not used
   *
   * we don't want to task switch when in sleep mode
   * The ISR will exit sleep mode and then the RTOS will run
   * 
   */
  if ( HigherPriorityTaskWoken == pdTRUE )
  {
    portYIELD();
  }
#endif
  
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
  case InvalidMessage:             PrintStringAndHexByte("InvalidMessage 0x",MessageType);         break;
  case GetDeviceType:              PrintStringAndHexByte("GetDeviceType 0x",MessageType);          break;
  case GetDeviceTypeResponse:      PrintStringAndHexByte("GetDeviceTypeResponse 0x",MessageType);  break;
  case GetInfoString:              PrintStringAndHexByte("GetInfoString 0x",MessageType);          break;
  case GetInfoStringResponse:      PrintStringAndHexByte("GetInfoStringResponse 0x",MessageType);  break;
  case DiagnosticLoopback:         PrintStringAndHexByte("DiagnosticLoopback 0x",MessageType);     break;
  case EnterShippingModeMsg:       PrintStringAndHexByte("EnterShippingModeMsg 0x",MessageType);   break;
  case SoftwareResetMsg:           PrintStringAndHexByte("SoftwareResetMsg 0x",MessageType);       break;
  case ConnectionTimeoutMsg:       PrintStringAndHexByte("ConnectionTimeoutMsg 0x",MessageType);   break;
  case TurnRadioOnMsg:             PrintStringAndHexByte("TurnRadioOnMsg 0x",MessageType);         break;
  case TurnRadioOffMsg:            PrintStringAndHexByte("TurnRadioOffMsg 0x",MessageType);        break;
  case ReadRssiMsg:                PrintStringAndHexByte("ReadRssiMsg 0x",MessageType);               break;
  case PairingControlMsg:          PrintStringAndHexByte("PairingControlMsg 0x",MessageType);         break;
  case ReadRssiResponseMsg:        PrintStringAndHexByte("ReadRssiResponseMsg 0x",MessageType);       break;
  case SniffControlMsg:            PrintStringAndHexByte("SniffControlMsg 0x",MessageType);           break;
  case LinkAlarmMsg:               PrintStringAndHexByte("LinkAlarmMsg 0x",MessageType);              break;
  case OledWriteBufferMsg:         PrintStringAndHexByte("OledWriteBufferMsg 0x",MessageType);        break;
  case OledConfigureModeMsg:       PrintStringAndHexByte("OledConfigureModeMsg 0x",MessageType);      break;
  case OledChangeModeMsg:          PrintStringAndHexByte("OledChangeModeMsg 0x",MessageType);         break;
  case OledWriteScrollBufferMsg:   PrintStringAndHexByte("OledWriteScrollBufferMsg 0x",MessageType);  break;
  case OledScrollMsg:              PrintStringAndHexByte("OledScrollMsg 0x",MessageType);          break;
  case OledShowIdleBufferMsg:      PrintStringAndHexByte("OledShowIdleBufferMsg 0x",MessageType);  break;
  case OledCrownMenuMsg:           PrintStringAndHexByte("OledCrownMenuMsg 0x",MessageType);       break;
  case OledCrownMenuButtonMsg:     PrintStringAndHexByte("OledCrownMenuButtonMsg 0x",MessageType); break;
  case AdvanceWatchHandsMsg:       PrintStringAndHexByte("AdvanceWatchHandsMsg 0x",MessageType);   break;
  case SetVibrateMode:             PrintStringAndHexByte("SetVibrateMode 0x",MessageType);         break;
  case ButtonStateMsg:             PrintStringAndHexByte("ButtonStateMsg 0x",MessageType);         break;
  case SetRealTimeClock:           PrintStringAndHexByte("SetRealTimeClock 0x",MessageType);       break;
  case GetRealTimeClock:           PrintStringAndHexByte("GetRealTimeClock 0x",MessageType);       break;
  case GetRealTimeClockResponse:   PrintStringAndHexByte("GetRealTimeClockResponse 0x",MessageType);break;
  case StatusChangeEvent:          PrintStringAndHexByte("StatusChangeEvent 0x",MessageType);       break;
  case NvalOperationMsg:           PrintStringAndHexByte("NvalOperationMsg 0x",MessageType);        break;
  case NvalOperationResponseMsg:   PrintStringAndHexByte("NvalOperationResponseMsg 0x",MessageType); break;
  case GeneralPurposePhoneMsg:     PrintStringAndHexByte("GeneralPurposePhoneMsg 0x",MessageType); break;
  case GeneralPurposeWatchMsg:     PrintStringAndHexByte("GeneralPurposeWatchMsg 0x",MessageType); break;
  case ButtonEventMsg:             PrintStringAndHexByte("ButtonEventMsg 0x",MessageType);         break;
  case WriteBuffer:                PrintStringAndHexByte("WriteBuffer 0x",MessageType);            break;
  case ConfigureDisplay:           PrintStringAndHexByte("ConfigureDisplay 0x",MessageType);       break;
  case ConfigureIdleBufferSize:    PrintStringAndHexByte("ConfigureIdleBufferSize 0x",MessageType);break;
  case UpdateDisplay:              PrintStringAndHexByte("UpdateDisplay 0x",MessageType);          break;
  case LoadTemplate:               PrintStringAndHexByte("LoadTemplate 0x",MessageType);           break;
  case EnableButtonMsg:            PrintStringAndHexByte("EnableButtonMsg 0x",MessageType);        break;
  case DisableButtonMsg:           PrintStringAndHexByte("DisableButtonMsg 0x",MessageType);       break;
  case ReadButtonConfigMsg:        PrintStringAndHexByte("ReadButtonConfigMsg 0x",MessageType);    break;
  case ReadButtonConfigResponse:   PrintStringAndHexByte("ReadButtonConfigResponse 0x",MessageType);break;
  case BatteryChargeControl:       PrintStringAndHexByte("BatteryChargeControl 0x",MessageType);   break;
  case IdleUpdate:                 PrintStringAndHexByte("IdleUpdate 0x",MessageType);             break;
  case WatchDrawnScreenTimeout:    PrintStringAndHexByte("WatchDrawnScreenTimeout 0x",MessageType);break;
  case SplashTimeoutMsg:           PrintStringAndHexByte("SplashTimeoutMsg 0x",MessageType);       break;
  case ChangeModeMsg:              PrintStringAndHexByte("ChangeModeMsg 0x",MessageType);          break;
  case ModeTimeoutMsg:             PrintStringAndHexByte("ModeTimeoutMsg 0x",MessageType);         break;
  case WatchStatusMsg:             PrintStringAndHexByte("WatchStatusMsg 0x",MessageType);         break;
  case MenuModeMsg:                PrintStringAndHexByte("MenuModeMsg 0x",MessageType);            break;
  case BarCode:                    PrintStringAndHexByte("BarCode 0x",MessageType);                break;
  case ListPairedDevicesMsg:       PrintStringAndHexByte("ListPairedDevicesMsg 0x",MessageType);   break;
  case ConnectionStateChangeMsg:   PrintStringAndHexByte("ConnectionStateChangeMsg 0x",MessageType);  break;
  case ModifyTimeMsg:              PrintStringAndHexByte("ModifyTimeMsg 0x",MessageType);          break;
  case MenuButtonMsg:              PrintStringAndHexByte("MenuButtonMsg 0x",MessageType);          break;
  case ToggleSecondsMsg:           PrintStringAndHexByte("ToggleSecondsMsg 0x",MessageType);       break;
  case LedChange:                  PrintStringAndHexByte("LedChange 0x",MessageType);              break;
  //case AccelerometerHostMsg:       PrintStringAndHexByte("AccelerometerHostMsg 0x",MessageType);       break;           
  case AccelerometerHostMsg:       PrintString("-");       break;           
  case AccelerometerEnableMsg:     PrintStringAndHexByte("AccelerometerEnableMsg 0x",MessageType);     break;           
  case AccelerometerDisableMsg:    PrintStringAndHexByte("AccelerometerDisableMsg 0x",MessageType);    break;           
  //case AccelerometerSendDataMsg:   PrintStringAndHexByte("AccelerometerSendDataMsg 0x",MessageType);   break;           
  case AccelerometerSendDataMsg:   PrintString("_");   break;           
  case AccelerometerAccessMsg:     PrintStringAndHexByte("AccelerometerAccessMsg 0x",MessageType);     break;           
  case AccelerometerResponseMsg:   PrintStringAndHexByte("AccelerometerResponseMsg 0x",MessageType);   break;           
  case AccelerometerSetupMsg:      PrintStringAndHexByte("AccelerometerSetupMsg 0x",MessageType);      break;
  case QueryMemoryMsg:             PrintStringAndHexByte("QueryMemoryMsg 0x",MessageType);             break;
  case RamTestMsg:                 PrintStringAndHexByte("RamTestMsg 0x",MessageType);                 break;
  case RateTestMsg:                PrintStringAndHexByte("RateTestMsg 0x",MessageType);                break;
  case BatteryConfigMsg:           PrintStringAndHexByte("BatteryConfigMsg 0x",MessageType);           break;
  case LowBatteryWarningMsgHost:   PrintStringAndHexByte("LowBatteryWarningMsgHost 0x",MessageType);   break; 
  case LowBatteryBtOffMsgHost:     PrintStringAndHexByte("LowBatteryBtOffMsgHost 0x",MessageType);     break; 
  case ReadBatteryVoltageMsg:      PrintStringAndHexByte("ReadBatteryVoltageMsg 0x",MessageType);      break;
  case ReadBatteryVoltageResponse: PrintStringAndHexByte("ReadBatteryVoltageResponse 0x",MessageType); break;
  case ReadLightSensorMsg:         PrintStringAndHexByte("ReadLightSensorMsg 0x",MessageType);         break;
  case ReadLightSensorResponse:    PrintStringAndHexByte("ReadLightSensorResponse 0x",MessageType);    break;
  case LowBatteryWarningMsg:       PrintStringAndHexByte("LowBatteryWarningMsg 0x",MessageType);       break; 
  case LowBatteryBtOffMsg:         PrintStringAndHexByte("LowBatteryBtOffMsg 0x",MessageType);         break; 
  case RadioPowerControlMsg:       PrintStringAndHexByte("RadioPowerControlMsg 0x",MessageType);       break;
  case AdvertisingDataMsg:         PrintStringAndHexByte("AdvertisingDataMsg 0x",MessageType);         break;
  case CallbackTimeoutMsg:         PrintStringAndHexByte("CallbackTimeoutMsg 0x",MessageType);         break;
  case SetCallbackTimerMsg:        PrintStringAndHexByte("SetCallbackTimerMsg 0x",MessageType);        break;
  default:                         PrintStringAndHexByte("Unknown Message Type 0x",MessageType);       break;
  }  
  
}

void RouteMsg(tMessage* pMsg)
{
#ifdef MESSAGE_QUEUE_DEBUG
  AllQueuesReadyCheck();
  
  if ( pMsg == 0 )
  {
    PrintString("ERROR: Invalid message passed to Route function\r\n");  
  }
  else if ( AllQueuesReady == 0 )
  {
    PrintString("ERROR: Cannot Route when all queues are not ready\r\n");
    SendToFreeQueueContext(pMsg,RouteType);  
  }
  else
#endif
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
    case RamTestMsg:                    SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case RateTestMsg:                   SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case BatteryConfigMsg:              SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case LowBatteryWarningMsgHost:      SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case LowBatteryBtOffMsgHost:        SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case ReadBatteryVoltageMsg:         SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ReadBatteryVoltageResponse:    SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case ReadLightSensorMsg:            SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case ReadLightSensorResponse:       SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case LowBatteryWarningMsg:          SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case LowBatteryBtOffMsg:            SendMsgToQ(DISPLAY_QINDEX,pMsg);    break;
    case AdvertisingDataMsg:            SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case CallbackTimeoutMsg:            SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    case SetCallbackTimerMsg:           SendMsgToQ(BACKGROUND_QINDEX,pMsg); break;
    case RadioPowerControlMsg:          SendMsgToQ(SPP_TASK_QINDEX,pMsg);   break;
    default:                            SendMsgToQ(FREE_QINDEX,pMsg);       break;
    }
  }
  
}

void AssignWrapperQueueHandle(xQueueHandle WrapperHandle)
{
  QueueHandles[SPP_TASK_QINDEX] = WrapperHandle;  
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
unsigned char AllTaskQueuesEmpty(void)
{
  unsigned char result = 0;  
  
  portENTER_CRITICAL();
      //QueueHandles[FREE_QINDEX]->uxMessagesWaiting == NUM_MSG_BUFFERS 
      //&& 
  if (   QueueHandles[BACKGROUND_QINDEX]->uxMessagesWaiting == 0 
      && QueueHandles[DISPLAY_QINDEX]->uxMessagesWaiting == 0 
      && QueueHandles[SPP_TASK_QINDEX]->uxMessagesWaiting == 0 )
  {  
    result = 1; 
  }
  
  portEXIT_CRITICAL();
       
  return result;
  
}
