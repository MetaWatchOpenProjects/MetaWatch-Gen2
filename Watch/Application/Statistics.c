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
/*! \file Statistics.c
*
*/
/******************************************************************************/
#include "FreeRTOS.h"

#include "portmacro.h"

#include "Messages.h"
#include "Statistics.h"
#include "DebugUart.h"
#include "Wrapper.h"

/* Global application statistics */
tApplicationStatistics gAppStats;

/* Global Bluetooth statistics */
tBluetoothStatistics gBtStats;


void IncrementUpTime(void)
{
  if ( QueryPhoneConnected() )
  {
    gBtStats.UpTime++;  
  }
  else
  {
    if ( gBtStats.UpTime > gBtStats.MaxUpTime )
    {
      gBtStats.MaxUpTime = gBtStats.UpTime;
    }
    
    gBtStats.UpTime = 0;
  }
}

void IncrementRxCrcFailureCount(void)
{
  gBtStats.RxCrcFailureCount++;
}
