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
/*! \file hal_rtos_timer.c
*
* This also includes the crystal timers.
*/
/******************************************************************************/

#include "FreeRTOS.h"

#include "hal_board_type.h"
#include "hal_rtos_timer.h"
#include "hal_crystal_timers.h"
#include "hal_lpm.h"

#include "DebugUart.h"

/* these are shared with the assembly code */
unsigned char RtosTickEnabled = 0;
unsigned int RtosTickCount = RTOS_TICK_COUNT;

static unsigned char (*pCrystalCallback1)(void);
static unsigned char (*pCrystalCallback2)(void);
static unsigned char (*pCrystalCallback3)(void);
static unsigned char (*pCrystalCallback4)(void);

static unsigned char Timer0Users;

#define TIMER0_RTOS_USER ( 0 )
#define TIMER0_USER1     ( 1 )
#define TIMER0_USER2     ( 2 )
#define TIMER0_USER3     ( 3 )
#define TIMER0_USER4     ( 4 )

static void AddUser(unsigned char User, unsigned int Ticks);
static void RemoveUser(unsigned char User);

/*
 * Setup timer to generate the RTOS tick
 */
void SetupRtosTimer(void)
{
  /* Ensure the timer is stopped */
  TA0CTL = 0;
  
  /* Clear everything to start with */
  TA0CTL |= TACLR;
  
  /* divide clock by 8
   * the total divide is by 32
   * 32768 kHz -> 1024 kHz -> 0.9765625 ms
   */
  TA0EX0 = 0x7;

  Timer0Users = 0;
  
  EnableRtosTick();

}

/* the timer is not stopped unless the rtos is off and all of the other timers
 * are inactive 
 */
void EnableRtosTick(void)
{
  RtosTickEnabled = 1;
  AddUser(TIMER0_RTOS_USER,RTOS_TICK_COUNT);
}

void DisableRtosTick(void)
{
  RtosTickEnabled = 0;
  RemoveUser(TIMER0_RTOS_USER);
}

static void AddUser(unsigned char User,unsigned int CrystalTicks)
{
  portENTER_CRITICAL();
  
  /* minimum value of 1 tick */
  if ( CrystalTicks < 1 )
  {
    CrystalTicks = 1;
  }  
  
  unsigned int CaptureTime = TA0R + CrystalTicks;

  /* clear ifg, add to ccr register, enable interrupt */
  switch (User)
  {
  case 0: TA0CCTL0 = 0; TA0CCR0 = CaptureTime; TA0CCTL0 = CCIE; break;
  case 1: TA0CCTL1 = 0; TA0CCR1 = CaptureTime; TA0CCTL1 = CCIE; break;
  case 2: TA0CCTL2 = 0; TA0CCR2 = CaptureTime; TA0CCTL2 = CCIE; break;
  case 3: TA0CCTL3 = 0; TA0CCR3 = CaptureTime; TA0CCTL3 = CCIE; break;
  case 4: TA0CCTL4 = 0; TA0CCR4 = CaptureTime; TA0CCTL4 = CCIE; break;
  default: break;
    
  }
  
  /* start counting up in continuous mode if not already doing so */
  if ( Timer0Users == 0 )
  {
    TA0CTL |= TASSEL_1 | MC_2 | ID_2; 
  }
  
  /* keep track of users */
  Timer0Users |= (1 << User);
  
  portEXIT_CRITICAL();
  
}

static void RemoveUser(unsigned char User)
{
  portENTER_CRITICAL();

  switch (User)
  {
  case 0: TA0CCTL0 = 0; break;
  case 1: TA0CCTL1 = 0; break;
  case 2: TA0CCTL2 = 0; break;
  case 3: TA0CCTL3 = 0; break;
  case 4: TA0CCTL4 = 0; break;
  default: break;
    
  }
  
  /* remove a user */
  Timer0Users &= ~(1 << User);
    
  /* disable timer if no one is using it */
  if ( Timer0Users == 0 )
  {
    TA0CTL = 0;  
  }
  
  portEXIT_CRITICAL();
  
}


/* 0 means off */
unsigned char QuerySchedulerState(void)
{
  return RtosTickEnabled;  
}

void StartCrystalTimer(unsigned char TimerId,
                       unsigned char (*pCallback) (void),
                       unsigned int Ticks)
{   
  if ( pCallback == 0 )
  {
    PrintString("Invalid function pointer given to StartTimer0\r\n"); 
  }
  
  /* assign callback */
  switch (TimerId)
  {
  case 1: pCrystalCallback1 = pCallback; break;
  case 2: pCrystalCallback2 = pCallback; break;
  case 3: pCrystalCallback3 = pCallback; break;
  case 4: pCrystalCallback4 = pCallback; break;
  default: break;
    
  }
  
  AddUser(TimerId,Ticks);
  
}

void StopCrystalTimer(unsigned char TimerId)
{
  RemoveUser(TimerId);  
}

/* 
 * timer0 ccr0 has its own interrupt (TIMER0_A0) 
 */
#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(TIMER0_A1_VECTOR_ISR,".text:_isr");
#endif
 
 
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_VECTOR_ISR(void)
{
  unsigned char ExitLpm = 0;
  
  /* callback when timer expires */
  switch(__even_in_range(TA0IV,8))
  {
  /* remove the user first in case the callback is re-enabling this user */
  case 0: break;                  
  case 2: RemoveUser(1); ExitLpm = pCrystalCallback1(); break;
  case 4: RemoveUser(2); ExitLpm = pCrystalCallback2(); break;
  case 6: RemoveUser(3); ExitLpm = pCrystalCallback3(); break;
  case 8: RemoveUser(4); ExitLpm = pCrystalCallback4(); break;
  default: break;
  }
  
  if ( ExitLpm )
  {
    EXIT_LPM_ISR();  
  }
  
}
