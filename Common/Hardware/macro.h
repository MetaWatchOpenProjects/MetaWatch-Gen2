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
/*! \file macro.h
*
* These macros store the interrupt state and restore the interrupt state
* The single brackets are on purpose (the two macros must be used together)
*
*/
/******************************************************************************/

#ifndef MACRO_H
#define MACRO_H


/*! Save the interrupt state and disable interrutps */
#define ENTER_CRITICAL_REGION_QUICK() \
  { int quick = __get_interrupt_state(); __disable_interrupt();

/*! Restore saved interrupt state */
#define LEAVE_CRITICAL_REGION_QUICK() __set_interrupt_state(quick); }


/*! DisableFlow comes from SerialPortProfile */
extern void DisableFlow(void);
/*! EnableFlow comes from serial port profile */
extern void EnableFlow(void);

/*! Save interrupt state, disable interrupts, and disable flow from bluetooth serial port */
#define ENTER_CRITICAL_REGION_SLOW() \
  { int slow = __get_interrupt_state(); DisableFlow(); __disable_interrupt();


/*! Restore interrupt state and enable flow from bluetooth serial port */
#define LEAVE_CRITICAL_REGION_SLOW() __set_interrupt_state(slow); EnableFlow();}


#endif /* MACRO_H */