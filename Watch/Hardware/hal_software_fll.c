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

/******************************************************************************/
/*! \file hal_software_fll.c
 *
 * This is a modified copy of SLAA489A from TI for UCS10
 *
 *  The FLL loop control should already be disabled.
 *
 * The EnableSoftwareFll function enables a capture compare interrupt on the 
 * falling edge of ACLK.  This interrupt occurs every ~30 us.  The number of
 * SMCLKs that should have occurred during this time is captured.  Based on this
 * the DCO is adjusted.
 *
 * The software FLL loop is currently enabled and disabled while waiting for the 
 * battery measurement to become valid.
 *
 */
/******************************************************************************/

#include "hal_board_type.h"
#include "Statistics.h"
#include "hal_clock_control.h"

/******************************************************************************/

/* MCLK/SMCLK = 512*32768 = 167777216 
 * so the number of counts is the same as the ACLK_MULTIPLIER 
 */
#define EXPECTED_DELTA ( 512 )

#define MOD_MASK        ( 0x1ff8 )
#define MOD_MASK_LOW    ( 0 )

/******************************************************************************/

static unsigned int ucs_dco_mod;
static unsigned int current;
static unsigned int previous;
static unsigned int difference;
static unsigned char FirstPassComplete;

/******************************************************************************/

void EnableSoftwareFll(void)
{
  /* FLL loop control should already be disabled */
  EnableSmClkUser(FLL_USER);
  
  /* don't divide the clock */
  TB0EX0 = 0;

  /* Use TB0.6 CCI6B which is internally connected to ACLK in Capture on
   * falling edge mode
   */
  TB0CCTL6 = CM_2 + CCIS_1 + CAP + CCIE;
  
  /* Start a timer to Re-adjust fll , using SMCLK in continuous mode
   * will interrupt on every input capture
   */
  TB0CTL = TBSSEL__SMCLK + MC__CONTINUOUS;
}

void DisableSoftwareFll(void)
{
  TB0CTL = 0;
  TB0CCTL6 = 0;  
  DisableSmClkUser(FLL_USER);
  FirstPassComplete = 0;
}

/******************************************************************************/

/* INLINE or macro these because we don't want extra overhead of a function call */
static inline void IncrementMod(void)
{
  if ( ucs_dco_mod != MOD_MASK )
  {
    /* Increment MOD+DCO bits if they haven't reached high limit */
    ucs_dco_mod += MOD0;

#if DEBUG_SOFTWARE_FLL
    DEBUG4_PULSE();
#endif

  }
  else
  { 
    /* current DCORSEL settings don't support this frequency
     * DCORSEL must be adjusted properly before using SW FLL 
     */
    gAppStats.FllFailure = 1;  
  }
  
}

static inline void DecrementMod(void)
{    
  if ( ucs_dco_mod != MOD_MASK_LOW )
  {
    /* Decrement MOD+DCO bits if they haven't reached low limit */
    ucs_dco_mod -= MOD0;
    
#if DEBUG_SOFTWARE_FLL
    DEBUG5_PULSE();
#endif
  
  }
  else
  { 
    /* current DCORSEL settings don't support this frequency
     * DCORSEL must be adjusted properly before using SW FLL 
     */
    gAppStats.FllFailure = 1;
  }  

}


/******************************************************************************/

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(TIMER0_B0_VECTOR_ISR,".text:_isr");
#endif

#ifndef BOOTLOADER
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_VECTOR_ISR(void)
#else
__interrupt void TIMER0_B0_VECTOR_ISR(void)
#endif
{
  __no_operation();  
}

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(TIMER0_B1_VECTOR_ISR,".text:_isr");
#endif

#ifndef BOOTLOADER
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_VECTOR_ISR(void)
#else
__interrupt void TIMER0_B1_VECTOR_ISR(void)
#endif
{
  switch(__even_in_range(TB0IV,14))
  {
  case 0: break;  // No interrupt
  case 2: break;  // CCR1 not used
  case 4: break;  // CCR2 not used
  case 6: break;  // CCR3 reserved
  case 8: break;  // CCR4 reserved
  case 10: break; // CCR5 reserved
  case 12:        // CCR6
    /* INLINED because we don't want extra overhead of a function call */
    /* This requires ~10 us */
    {
      current = TB0CCR6;
      
#if DEBUG_SOFTWARE_FLL
      /* set debug pulse after reading count value */
      DEBUG3_HIGH();
#endif
      
      difference = current - previous;
      
      /* Get current dco_mod value ignoring reserved bits */
      ucs_dco_mod = (UCSCTL0 & MOD_MASK);

      if ( FirstPassComplete == 'F')
      {
        if ( difference > EXPECTED_DELTA )
        {
          DecrementMod();
        }
        else if ( difference < EXPECTED_DELTA )
        {
          IncrementMod();
        }
        
        /* Write the value to the register */
        UCSCTL0 = (ucs_dco_mod & MOD_MASK);
      }
      else
      {
        /* set flag and wait for next time */
        FirstPassComplete = 'F';
      }
      
      previous = current;
      TB0CCTL6 &= ~CCIFG;
      
#if DEBUG_SOFTWARE_FLL
      DEBUG3_LOW();
#endif
      
    }
    break;  
  default:
    break;
  }
}
