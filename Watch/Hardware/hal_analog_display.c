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
/*! \file hal_analog_display.c
*
* Does not follow the coding standard
*/
/******************************************************************************/

#include "hal_board_type.h"
#include "hal_analog_display.h"
#include "hal_lpm.h"

/*! Temporarily increase the pulse rate by a factor of 512
 *  We normally have a 0.100 Hz clock rate with the configured prescale 
 */
static const unsigned int FastDivideCount = 
  (ANALOG_DISP_TIMER_DIVIDER / ANALOG_DISP_DIVIDE_RATIO) - 1;

volatile unsigned int NumberOfFastPulses = 0;

/* documented in header */
void AdvanceAnalogDisplay(unsigned int NumberOfSeconds)
{
  // This is non-zero if we have an advance in progress
  if( 0 != NumberOfFastPulses )
  {
    return;
  }
  
  // The number of pulses is the number of seconds multipled by pulses per second
  NumberOfFastPulses = NumberOfSeconds / 10;
  
  // We speed up 512 pulses for each pulse. We need to add one for every 512
  // to make up for the elapsed time while advancing the display
  unsigned int temp = (NumberOfFastPulses / ANALOG_DISP_DIVIDE_RATIO);
  NumberOfFastPulses += temp;
  
  // A 512x speed increase
  TB0CCR0 = FastDivideCount;  
  
  // restart timer
  TB0CTL = TBCLR;         
  TB0CTL = ID__8 | TBSSEL__ACLK | MC__UP;
  
  // Enable timer B0 interupts
  TB0CCTL0 = CCIE;         
}


/* documented in header */
void SetupTimerForAnalogDisplay( void )
{
  // The analog clock is on P4.1 and P4.2
  P4DIR |= BIT1 | BIT2;   
  P4SEL |= BIT1 | BIT2;

  // Ensure the timer is stopped.
  TB0CTL = TBCLR;
 
  // count up using ACLK/8.
  TB0CTL = ID__8 | TBSSEL__ACLK | MC__UP;
 
  // additional prescale by 8
  TB0EX0 = TBIDEX__8;           

  // Set the compare match value according to get a pulse every 1 second
  // 5120 = (1 second)/((1/32.768e3)*64)  
  TB0CCR0 = ( ANALOG_DISP_TIMER_DIVIDER - 1);

  // It's not good to use zero as a toggle point because you can get an
  // extra toggle depending on how things line up.

  // PWM output mode 4 is toggle
  TB0CCTL1 = OUTMOD_4;         
  TB0CCR1 = 3;
  TB0CCTL2 = OUTMOD_4;         
  TB0CCR2 = 5;
     
}


/*! ISR for the analog movement PWM/timer
 *
 * Normally this interrupt is not enabled.  When we do a rapid advance of the
 * watch hands, we count the number of pulses sent to the display to know when
 * to stop the fast advance.
 *
 * \note This is the capture-compare zero interrupt which occurs when the PWM
 * timer hits its end count.  We get one interrupt per pulse to the display.
 *
 * \param none
 *
 * \return Restores the PWM timer to the normal rate when complete.
 */
 
#ifdef ANALOG

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(TIMER0_B0_ISR,".text:_isr");
#endif


#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void)
{
  NumberOfFastPulses--;

  if(0 == NumberOfFastPulses)
  {
    // Disable timer B0 interupts
    TB0CCTL0 &=~ CCIE;
    // Restore normal divider
    TB0CCR0 = ( ANALOG_DISP_TIMER_DIVIDER - 1);
  }

  // Exit LPM3 on interrupt exit (RETI).  
  EXIT_LPM_ISR();
  
}
#endif
