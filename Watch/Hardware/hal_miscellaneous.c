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
/*! \file hal_miscellaneous.c
*
* Set pins to outputs and setup clock
*/
/******************************************************************************/

#include "hal_board_type.h"
#include "hal_miscellaneous.h"

#include "HAL_PMM.h"
#include "HAL_UCS.h"


void SetAllPinsToOutputs(void)
{
  // jtag (don't set the pins high or spy-bi-wire won't work)
  PJOUT  = 0;
  PJDIR = 0xff;

  /* prevent floating inputs */
  P1DIR = 0xFF;
  P2DIR = 0xFF;
  P3DIR = 0xFF;
  P4DIR = 0xFF;
  P5DIR = 0xFF;
  P6DIR = 0xFF;
  P7DIR = 0xFF;
  P8DIR = 0xFF;
  P9DIR = 0xFF;
  P10DIR = 0xFF;
  P11DIR = 0xFF;
}

void SetupAclkToRadio(void)
{
  // Setting DIR = 1 and SEL = 1 enables ACLK out on P11.0
  // and disable MCLK and SMCLK
  P11DIR |= BIT0;
  P11SEL |= BIT0;
  
}


void SetupClockAndPowerManagementModule(void)
{
  unsigned int status;
  
  // see Frequency vs Supply Voltage in MSP4305438A data sheet
  SetVCore(PMMCOREV_2);                     
  
  // setup pins for XT1
  P7SEL |= BIT0 + BIT1;
  
  // Startup LFXT1 32 kHz crystal 
  do
  {
    status = LFXT_Start_Timeout(XT1DRIVE_0, 50000);
    
  } while(status == UCS_STATUS_ERROR);
 
  // select the sources for the FLL reference and ACLK
  SELECT_ACLK(SELA__XT1CLK);                
  SELECT_FLLREF(SELREF__XT1CLK);           
  
  // second parameter is 16000/32768 = 488
  Init_FLL_Settle(16000,488);                 

  // setup for quick wake up from interrupt and
  // minimal power consumption in sleep mode
  DISABLE_SVSL();                           // SVS Low side is turned off
  DISABLE_SVML();                           // Monitor low side is turned off
  DISABLE_SVMH();                           // Monitor high side is turned off
  ENABLE_SVSH();                            // SVS High side is turned on
  ENABLE_SVSH_RESET();                      // Enable POR on SVS Event
  
  SVSH_ENABLED_IN_LPM_FULL_PERF();          // SVS high side Full perf mode,
                                            // stays on in LPM3,enhanced protect
  
  // Wait until high side, low side settled
  while (((PMMIFG & SVSMLDLYIFG) == 0)&&((PMMIFG & SVSMHDLYIFG) == 0));
  CLEAR_PMM_IFGS();
    
}

