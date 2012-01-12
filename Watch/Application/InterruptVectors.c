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
/*! \file InterruptVectors.c
 *
 * This file contains all of the interrupt vectors
 * if they are defined somewhere else then they should not be included here
 *
 * This is for debugging only
 * 
 * Don't include this for text output version because of assert
 */
/******************************************************************************/

#include <assert.h>
#include "hal_board_type.h"

#if 0
  /* hal_rtc */
  #pragma vector=RTC_VECTOR          /* 0xFFD2 */
  __interrupt void RTC_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#if 0
  /* buttons */
  #pragma vector=PORT2_VECTOR        /* 0xFFD4 */
  __interrupt void PORT2_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#pragma vector=USCI_B3_VECTOR      /* 0xFFD6 */
__interrupt void USCI_B3_VECTOR_ISR(void)
{
  assert(0);
}

#if 0
  #pragma vector=USCI_A3_VECTOR      /* 0xFFD8 */
  __interrupt void USCI_A3_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#if 0
  /* accelerometer */ 
  #pragma vector=USCI_B1_VECTOR      /* 0xFFDA */
  __interrupt void USCI_B1_VECTOR_ISR(void)
  {
    assert(0);
  }
  #endif

#if 0
  /* debug or bluetooth uart */
  #pragma vector=USCI_A1_VECTOR      /* 0xFFDC */
  __interrupt void USCI_A1_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#if 0
  /* Bluetooth interrupts */
  #pragma vector=PORT1_VECTOR        /* 0xFFDE */
  __interrupt void PORT1_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#if 1
  /* timer is used by vibe motors, but interrupts aren't */
  #pragma vector=TIMER1_A1_VECTOR    /* 0xFFE0 */
  __interrupt void TIMER1_A1_VECTOR_ISR(void)
  {
    assert(0);
  }


  #pragma vector=TIMER1_A0_VECTOR    /* 0xFFE2 */
  __interrupt void TIMER1_A0_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#if 0
#pragma vector=DMA_VECTOR          /* 0xFFE4 */
__interrupt void DMA_VECTOR_ISR(void)
{
  assert(0);
}
#endif

#pragma vector=USCI_B2_VECTOR      /* 0xFFE6 */
__interrupt void USCI_B2_VECTOR_ISR(void)
{
  assert(0);
}

#pragma vector=USCI_A2_VECTOR      /* 0xFFE8 */
__interrupt void USCI_A2_VECTOR_ISR(void)
{
  assert(0);
}


#if 0 /* used by millesecond timers */
#pragma vector=TIMER0_A1_VECTOR    /* 0xFFEA */
__interrupt void TIMER0_A1_VECTOR_ISR(void)
{
  assert(0);
}
#endif

#if 0
  /* used by FreeRtos */
  #pragma vector=TIMER0_A0_VECTOR    /* 0xFFEC */
  __interrupt void TIMER0_A0_VECTOR_ISR(void)
  {
    __no_operation();
  }
#endif

#if 0
#pragma vector=ADC12_VECTOR        /* 0xFFEE */
__interrupt void ADC12_VECTOR_ISR(void)
{
  assert(0);
}
#endif

#ifdef DIGITAL
  #pragma vector=USCI_B0_VECTOR      /* 0xFFF0 */
  __interrupt void USCI_B0_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#pragma vector=USCI_A0_VECTOR      /* 0xFFF2 */
__interrupt void USCI_A0_VECTOR_ISR(void)
{
  assert(0);
}

#pragma vector=WDT_VECTOR          /* 0xFFF4 */
__interrupt void WDT_VECTOR_ISR(void)
{
  assert(0);
}

#if 0
#pragma vector=TIMER0_B1_VECTOR    /* 0xFFF6 */
__interrupt void TIMER0_B1_VECTOR_ISR(void)
{
  assert(0);
}
#endif

#if 0
  #pragma vector=TIMER0_B0_VECTOR    /* 0xFFF8 */
  __interrupt void TIMER0_B0_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif

#pragma vector=UNMI_VECTOR         /* 0xFFFA */
__interrupt void UNMI_VECTOR_ISR(void)
{
  assert(0);
}

#pragma vector=SYSNMI_VECTOR       /* 0xFFFC */
__interrupt void SYSNMI_VECTOR_ISR(void)
{
  assert(0);
}


#if 0
  #pragma vector=RESET_VECTOR        /* 0xFFFE */
  __interrupt void RESET_VECTOR_ISR(void)
  {
    assert(0);
  }
#endif
