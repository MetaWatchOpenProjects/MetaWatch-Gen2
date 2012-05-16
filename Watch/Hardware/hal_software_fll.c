/*
 * This is a modified copy of SLAA489A from TI for UCS10
 */

#include "FreeRTOS.h"
#include "hal_board_type.h"
#include "Statistics.h"
#include "hal_clock_control.h"

#include "DebugUart.h"


#if 0
static void SoftwareFllCycleIsr(void);
static void IncrementMod(void);
static void DecrementMod(void);

static unsigned int ucs_dco_mod;
static unsigned int ucs_dco_mod_saved;

static unsigned int count_d0;
static unsigned int count_d1;
static unsigned int count_diff;
#endif

#define EXPECTED_COUNTS ( 268 )
#define MOD_MASK        ( 0x1ff8 )

void SoftwareFllInit(void)
{
  // Disable FLL loop control
  __bis_SR_register(SCG0); 

#if 0

  TB0CTL = 0;
  
  EnableSmClkUser(RESERVED_USER3);
  
  // additional prescale by 8
  TB0EX0 = TBIDEX__8;           

  // Start a timer to Re-adjust fll , using SMCLK in continous mode
  // will interrupt on every input capture
  TB0CTL = TBSSEL__SMCLK + MC__CONTINUOUS + TBIE + ID__8;

  // 262.144 ms
  // above / 0.9765625 = 268.43
  
  ucs_dco_mod_saved = UCSCTL0 & MOD_MASK;
#endif
  
}

#if 0
/* 400 us when doing work else 40 us */
static void SoftwareFllCycleIsr(void)
{
  // Get current dco_mod value ignoring reserved bits
  // this requires majority vote?
  ucs_dco_mod = (UCSCTL0 & MOD_MASK);
  
  count_d1 = count_d0;
  count_d0 = TA0R;
    
  /* check to make sure the value has been updated */
  if ( ucs_dco_mod == ucs_dco_mod_saved )
  {
    if ( count_d0 > count_d1 )
    {
      count_diff = count_d0 - count_d1;
    }
    else
    {
      count_diff = 0xffff - count_d1;
      count_diff = count_d1 + count_d0;
    }
    
    PrintDecimal(count_diff);
    
    /* if there are more counts than expected then the SMCLK is running to slow */
    if ( count_diff > EXPECTED_COUNTS + 3 )
    {    
      IncrementMod();
    }
    else if ( count_diff < EXPECTED_COUNTS - 3 )
    {
      DecrementMod();
    }
    else
    {
      PrintString(".");  
    }
    
    // Write the value to the register
    UCSCTL0 = (ucs_dco_mod & MOD_MASK);
    ucs_dco_mod_saved = ucs_dco_mod & MOD_MASK;
  }
  else
  {
    PrintString("$");  
  }
  
}

static void IncrementMod(void)
{
  if ( ucs_dco_mod < (MOD_MASK-MOD0) )
  {
    // Increment MOD+DCO bits if they haven't reached high limit
    ucs_dco_mod += MOD0;
  }
  else
  { 
    // current DCORSEL settings don't support this frequency
    // DCORSEL must be adjusted properly before using SW FLL
    gAppStats.FllFailure = 1;  
  }

  PrintString("+");  
}

static void DecrementMod(void)
{    
  if (ucs_dco_mod > MOD0)
  {
    // Decrement MOD+DCO bits if they haven't reached low limit
    ucs_dco_mod -= MOD0;
  }
  else
  { 
    // current DCORSEL settings don't support this frequency
    // DCORSEL must be adjusted properly before using SW FLL
    gAppStats.FllFailure = 1;
  }  

  PrintString("-");
}

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(TIMER0_B0_VECTOR_ISR,".text:_isr");
#endif

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_VECTOR_ISR(void)
{
  __no_operation();  
}

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(TIMER0_B1_VECTOR_ISR,".text:_isr");
#endif

/* case 12: example seemed stupid .... interrupt every 30 us .... */
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_VECTOR_ISR(void)
{
  switch(__even_in_range(TB0IV,14))
  {
  case 0: break;  // No interrupt
  case 2: break;  // CCR1 not used
  case 4: break;  // CCR2 not used
  case 6: break;  // CCR3 reserved
  case 8: break;  // CCR4 reserved
  case 10: break; // CCR5 reserved
  case 12: break; // CCR6 
  case 14:
    SoftwareFllCycleIsr();
    break;
  default:
    break;
  }
}
#endif

/* proposed algorithm
 *
 * use battery monitor interval ?
 * enable timer
 * don't check mod vs saved
 * first pass get one value
 * second pass get second value and run update cycle
 * during isr count is saved remainder of work is done in response to message
 *
 */
