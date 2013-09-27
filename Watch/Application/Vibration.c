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
/*! \file Vibration.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "Messages.h"
#include "MessageQueues.h"

#include "hal_board_type.h"
#include "hal_vibe.h"
#include "hal_rtc.h"

#include "DebugUart.h"
#include "LcdDisplay.h"

#include "Vibration.h"

/******************************************************************************/
static tSetVibrateModePayload const VibraPattern[] =
{
  {1, 0x00, 0x01, 0x00, 0x01, 2}, // 512ms per cycle, 58 cycles for 30s
  {1, 0x80, 0x01, 0x80, 0x00, 3},
  {1, 0xC8, 0x00, 0xF4, 0x01, 1},
  {1, 0x00, 0x02, 0x00, 0x02, 5},
  {1, 0x00, 0x01, 0x00, 0x01, 5},
  {1, 0xE8, 0x03, 0xE8, 0x03, 1} // 1s on, 1s off
};

static unsigned char VibeEventActive;  
static unsigned char motorOn;          
static unsigned char CycleCount;       

// count the number of interrupts to get vibe on/off timing
static unsigned int VibeEventTimerCount;

static unsigned int timeOn;           
static unsigned int timeOff;          
static unsigned int nextActionTime;   

/******************************************************************************/


void InitVibration(void)
{
  // Make sure the vibe motor is off
  SetVibeMotorState(FALSE);
  
  // Initialize the timer for the vibe motor with the right PWM params.
  SetupVibrationMotorTimerAndPwm();

  // Vibe motor duration timer.
  VibeEventTimerCount = 0;    
}

/* Handle the message from the host that starts a vibration event */
void SetVibrateModeHandler(tMessage *pMsg)
{
  // overlay a structure pointer on the data section
  tSetVibrateModePayload const *pMsgData;

  if (pMsg->Options != MSG_OPT_VIBRA_OFF)
  {
    pMsgData = !pMsg->Length ? &VibraPattern[pMsg->Options] : (tSetVibrateModePayload *)pMsg->pBuffer;

    // set it active or cancel it
    VibeEventActive = pMsgData->Enable;
  }
  else VibeEventActive = FALSE;

  motorOn = VibeEventActive;
  // Start or stop the RTC PS timer based on whether the motor is being turned
  // on or off
  if (VibeEventActive)
  {
    tWordByteUnion temp;
    temp.Bytes.byte0 = pMsgData->OnDurationLsb; 
    temp.Bytes.byte1 = pMsgData->OnDurationMsb;
    timeOn = temp.word / RTC_TIMER_MS_PER_TICK;

    temp.Bytes.byte0 = pMsgData->OffDurationLsb; 
    temp.Bytes.byte1 = pMsgData->OffDurationMsb;
    timeOff = temp.word / RTC_TIMER_MS_PER_TICK;

    CycleCount = pMsgData->NumberOfCycles;

    EnableRtcPrescaleInterruptUser(RTC_TIMER_VIBRATION);
    EnableVibratorPwm();
  }
  else
  {
    DisableRtcPrescaleInterruptUser(RTC_TIMER_VIBRATION);
    DisableVibratorPwm();
  }

  // Use a separate timer that counts Rtc_Prescale_Timer_Static messages to
  // get the time for the vibe motor.
  if (VibeEventActive)
  {
    VibeEventTimerCount = 0;
  }

  // the next event is to turn
  nextActionTime = timeOn;

  // Set/clear  the port bit that controls the motor
  SetVibeMotorState(motorOn);
}

/* 
 * Once the phone has started a vibration event this controls the pulsing
 * on and off.
 * 
 * This requires < 7 us and is done in the ISR
*/
void VibrationMotorStateMachineIsr(void)
{
  VibeEventTimerCount ++;
  
  // If we have an active event
  if (VibeEventActive)
  {
    // is it time for the next action
    if (VibeEventTimerCount >= nextActionTime)
    {
      // if the motor is currently on
      if (motorOn)
      {
        motorOn = FALSE;
        nextActionTime += timeOff;
        
        if (CycleCount > 1)
        {
          CycleCount --;
        }
        else /* last cycle */
        {
          VibeEventActive = FALSE;
          DisableRtcPrescaleInterruptUser(RTC_TIMER_VIBRATION);
          DisableVibratorPwm();
        }
      
      }
      else
      {
        motorOn = TRUE;
        nextActionTime += timeOn;
      }
    }
  
    // Set/clean the port bit that controls the motor
    SetVibeMotorState(motorOn);
  }
  else
  {
    DisableRtcPrescaleInterruptUser(RTC_TIMER_VIBRATION);
    DisableVibratorPwm();   
  }
}
