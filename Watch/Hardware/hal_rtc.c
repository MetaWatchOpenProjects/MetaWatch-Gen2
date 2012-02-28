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
/*! \file hal_rtc.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "Messages.h"
#include "MessageQueues.h"

#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_lpm.h"
#include "hal_calibration.h"

#include "DebugUart.h"
#include "Utilities.h"
#include "Statistics.h"
#include "OneSecondTimers.h"
#include "SerialProfile.h"
#include "Vibration.h"
#include "LcdDisplay.h"

/** Real Time Clock interrupt Flag definitions */
#define RTC_NO_INTERRUPT      ( 0 )
#define RTC_RDY_IFG           ( 2 )
#define RTC_EV_IFG            ( 4 )
#define RTC_A_IFG             ( 6 )
#define RTC_PRESCALE_ZERO_IFG ( 8 )
#define RTC_PRESCALE_ONE_IFG  ( 10 )

#define RTCCAL_VALUE_MASK ( 0x3f )

static unsigned char RtcInUseMask;

void InitializeRealTimeClock( void )
{
  RtcInUseMask = 0;
  
  // stop it
  RTCCTL01 = RTCHOLD;

  // use calibration data to adjust real time clock frequency
  if ( QueryCalibrationValid() )
  {
    signed char RtcCalibrationValue = GetRtcCalibrationValue();
    
    if ( RtcCalibrationValue < 0 )
    {
      RtcCalibrationValue = -1 * RtcCalibrationValue;
      RTCCTL2 = RtcCalibrationValue & RTCCAL_VALUE_MASK;
    }
    else
    {
      RTCCTL2 = RtcCalibrationValue & RTCCAL_VALUE_MASK;
      /* adjust up */
      RTCCTL2 |= RTCCALS;
    }
    
  }
  
  
  // Set the counter for RTC mode
  RTCCTL01 |= RTCMODE;

  // set 128 Hz rate for prescale 0 interrupt
  RTCPS0CTL |= RT0IP_7;

  // enable 1 pulse per second interrupt using prescale 1
  RTCPS1CTL |= RT1IP_6 | RT1PSIE;

  // 1 Hz calibration output
  RTCCTL23 |= RTCCALF_3;

  // setting the peripheral selection bit makes the other I/O control a don't care
  // P2.4 = 1 Hz RTC calibration output
  // Direction needs to be set as output
  RTC_1HZ_PORT_SEL |= RTC_1HZ_BIT;  
  RTC_1HZ_PORT_DIR |= RTC_1HZ_BIT;  

  RTCYEAR = (unsigned int)0x07db;
  RTCMON = (unsigned int)5;
  RTCDAY = (unsigned int)23;
  RTCDOW = (unsigned int)5;
  RTCHOUR = (unsigned int)23;
  RTCMIN = (unsigned int)58;
  RTCSEC = (unsigned int)0;


  // Enable the RTC
  RTCCTL01 &= ~RTCHOLD;  
  
}




void halRtcSet(tRtcHostMsgPayload* pRtcData)
{
  // Stop the RTC
  RTCCTL01 |= RTCHOLD;    

  // These calls are to asm level patch functions provided by TI for the MSP430F5438
  tWordByteUnion temp;
  temp.Bytes.byte0 = pRtcData->YearLsb; 
  temp.Bytes.byte1 = pRtcData->YearMsb;
  RTCYEAR = (unsigned int) temp.word;
  RTCMON = (unsigned int) pRtcData->Month;
  RTCDAY = (unsigned int) pRtcData->DayOfMonth;
  RTCDOW = (unsigned int) pRtcData->DayOfWeek;
  RTCHOUR = (unsigned int) pRtcData->Hour;
  RTCMIN = (unsigned int) pRtcData->Minute;
  RTCSEC = (unsigned int) pRtcData->Second;
  
  // Enable the RTC
  RTCCTL01 &= ~RTCHOLD;  
}

void halRtcGet(tRtcHostMsgPayload* pRtcData)
{
  tWordByteUnion temp;
  temp.word = RTCYEAR;
  pRtcData->YearLsb = temp.Bytes.byte0;
  pRtcData->YearMsb = temp.Bytes.byte1;
  pRtcData->Month = RTCMON;
  pRtcData->DayOfMonth = RTCDAY; 
  pRtcData->DayOfWeek = RTCDOW;
  pRtcData->Hour = RTCHOUR;
  pRtcData->Minute = RTCMIN;
  pRtcData->Second = RTCSEC;

}



void EnableRtcPrescaleInterruptUser(unsigned char UserMask)
{
  portENTER_CRITICAL();
   
  // If not currently enabled, enable the timer ISR
  if( RtcInUseMask == 0 )
  {
    RTCPS0CTL |= RT0PSIE;
  }
  
  RtcInUseMask |= UserMask;
   
  portEXIT_CRITICAL();

}

void DisableRtcPrescaleInterruptUser(unsigned char UserMask)
{

  portENTER_CRITICAL();
  
  RtcInUseMask &= ~UserMask;
      
  // If there are no more users, disable the interrupt
  if( RtcInUseMask == 0 )
  {
    RTCPS0CTL &= ~RT0PSIE;
  }
  
  portEXIT_CRITICAL();

}

unsigned char QueryRtcUserActive(unsigned char UserMask)
{
  return RtcInUseMask & UserMask;
}


static unsigned char DivideByFour = 0;

/*! Real Time Clock interrupt handler function.
 *
 *  Used for system timing.  When the processor is in low power mode, the RTC
 *  is the only timer running so we use it as a system timer.  There are two
 *  different timers.  RTC prescale one occurs at a 32 Hz rate and is enabled
 *  as needed.  The RTC Ready event occurs at 1 ppS and is always enabled.
 *
 * don't exit LPM3 unless it is required
 */
#pragma vector = RTC_VECTOR
__interrupt void RTC_ISR(void)
{
  P6OUT |= BIT7;   /* debug4_high */
  
  unsigned char ExitLpm = 0;
  tMessage Msg;
        
  // compiler intrinsic, value must be even, and in the range of 0 to 10
  switch(__even_in_range(RTCIV,10))
  {
  case RTC_NO_INTERRUPT: break;
  case RTC_RDY_IFG:      break;
  case RTC_EV_IFG:       break;
  case RTC_A_IFG:        break;

  case RTC_PRESCALE_ZERO_IFG:

    // divide by four to get 32 Hz
    if ( DivideByFour >= 4-1 )
    {
      DivideByFour = 0;
           
      if( QueryRtcUserActive(RTC_TIMER_VIBRATION) )
      {
        VibrationMotorStateMachineIsr();
      }

      if( QueryRtcUserActive(RTC_TIMER_BUTTON) )
      {
        SetupMessage(&Msg,ButtonStateMsg,NO_MSG_OPTIONS);
        SendMessageToQueueFromIsr(BACKGROUND_QINDEX,&Msg); 
        
        ExitLpm = 1;
      }

      if ( QueryRtcUserActive(RTC_TIMER_USER_DEBUG_UART) )
      {
        DisableUartSmClkIsr();
        ExitLpm = 1;
      }

    }
    else
    {
      DivideByFour++;
    }
    break;

  case RTC_PRESCALE_ONE_IFG:
    
#ifdef DIGITAL
    ExitLpm |= LcdRtcUpdateHandlerIsr();
#endif
    
    IncrementUpTime();
    ExitLpm |= OneSecondTimerHandlerIsr();
    
    break;
  
  default:
    break;
  }

  if ( ExitLpm )
  {
    EXIT_LPM_ISR();  
  }
  
  P6OUT &= ~BIT7;       /* debug4_low */
  
}


/******************************************************************************/



