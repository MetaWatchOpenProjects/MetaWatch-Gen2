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
/*! \file hal_clock_control.c
*
*/
/******************************************************************************/

#include "portmacro.h"
#include "hal_board_type.h"

/* resetting the UARTs did not solve the power consumption problem */
static unsigned char SmClkRequests;

/* enable the SMCLK and keep track of a new user */
void EnableSmClkUser(unsigned char User)
{

  portENTER_CRITICAL();
  
#ifdef CLOCK_CONTROL_DEBUG
  DEBUG5_HIGH();
#endif
  
  SmClkRequests |= User;
  
  UCSCTL8 |= SMCLKREQEN;
  
  portEXIT_CRITICAL();
  
}  

/* remove a user and disable clock if there are 0 users */
void DisableSmClkUser(unsigned char User)
{
  portENTER_CRITICAL();
  
  SmClkRequests &= ~User;
    
  if ( SmClkRequests == 0 )
  {
    UCSCTL8 &= ~SMCLKREQEN;
    
#ifdef CLOCK_CONTROL_DEBUG
    DEBUG5_LOW();
#endif
  };
  
  portEXIT_CRITICAL();
  
}
