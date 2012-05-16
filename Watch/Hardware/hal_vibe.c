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
/*! \file hal_vibe.c
*
*/
/******************************************************************************/

#include "hal_board_type.h"
#include "hal_vibe.h"

static unsigned char VibeEnableControl = 1;

void VibeEnable(void)
{
  VibeEnableControl = 1;
}

void VibeDisable(void)
{
  VibeEnableControl = 0;  
}

#ifdef HW_DEVBOARD_V2
#ifdef DIGITAL

#define START_VIBE_PWM_TIMER() { TB0CTL |= TBSSEL__ACLK | MC__UP | ID_0; }
#define STOP_VIBE_PWM_TIMER()  { TB0CTL = 0; }


void SetVibeMotorState(unsigned char motorOn)
{
  if(motorOn && VibeEnableControl)
  {
    P4SEL |= BIT3;  
  }
  else
  {
    P4SEL &= ~BIT3;
  }
}

void SetupVibrationMotorTimerAndPwm(void)
{
  // Start with P7.3 as an output
  P4OUT &= ~BIT3;   
  P4SEL &= ~BIT3;   
  P4DIR |=  BIT3;   

  STOP_VIBE_PWM_TIMER();
  
  // No expansion divide
  TB0EX0 = 0;

  // Compare channel 3 is used as an output 
  // (set on CCR3/ reset on CCR0)
  TB0CCTL3 = OUTMOD_3;         
  TB0CCR0 = 5-1;
  TB0CCR3 = 3;
}

void EnableVibratorPwm(void)
{
  START_VIBE_PWM_TIMER();  
}

void DisableVibratorPwm(void)
{
  STOP_VIBE_PWM_TIMER();
}

#else

/* todo:?! I don't understand why this mode causes a line on the LCD */

void SetVibeMotorState(unsigned char motorOn) 
{

}

void SetupVibrationMotorTimerAndPwm(void)
{
//  P4OUT &= ~BIT3;   
//  P4SEL &= ~BIT3;   
//  P4DIR |=  BIT3;
}

void EnableVibratorPwm(void)
{

}

void DisableVibratorPwm(void)
{

}

#endif  // DIGITAL

#else // not the devboard

#define START_VIBE_PWM_TIMER() { TA1CTL |= TASSEL__ACLK | MC__UPDOWN | ID_0; }
#define STOP_VIBE_PWM_TIMER()  { TA1CTL = 0; }

void SetVibeMotorState(unsigned char motorOn)
{

  if(motorOn && VibeEnableControl)
  {
    // P7.3 is TA1.2 CCR output which is configured as a PWM output
    // P7 option select
    P7SEL |= BIT3;
  }
  else
  {
    P7SEL &= ~BIT3;
  }

}

void SetupVibrationMotorTimerAndPwm(void)
{
  // Start with P7.3 as an output
  P7OUT &= ~BIT3;   // Low when a digital output
  P7SEL &= ~BIT3;   // P7 option select = false
  P7DIR |=  BIT3;   // P7 outputs

  STOP_VIBE_PWM_TIMER();
  
  // No expansion divide
  TA1EX0 = 0;       

  // do a PWM with 64 total steps.  This gives a count up of 32 and
  // a count down of 32
  TA1CCR0 =  31;

  // Compare channel 2 is used as output
  TA1CCTL2 = OUTMOD_6;         // PWM output mode: 6 - toggle/set
  TA1CCR2 = 10;                // 10 is a 2/3 duty cycle
  
  START_VIBE_PWM_TIMER();

}

void EnableVibratorPwm(void)
{
  START_VIBE_PWM_TIMER();  
}

void DisableVibratorPwm(void)
{
  STOP_VIBE_PWM_TIMER();
}

#endif

