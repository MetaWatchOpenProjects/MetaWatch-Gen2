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
/*! \file OneSecondTimers.h
 *
 * Software based timers with 1 second resolution.  These use the 1 second tick
 * from the Real Time Clock.
 * 
 * The number of timers is directly proportional to execution time.  Simple loops
 * are used (not linked lists).
 *
 */
/******************************************************************************/


#ifndef ONE_SECOND_TIMERS_H
#define ONE_SECOND_TIMERS_H

/*! timers are not handled using lists
 * so don't allocate more timers than are required
*/
#define TOTAL_TIMERS    (6)
#define ONE_SECOND      (1)
#define UNASSIGNED      (-1)

/*! setting the repeat count to 0xFF causes a timer to repeat forever */
#define NO_REPEAT      (0)
#define REPEAT_FOREVER (0xFF)

/*! Timer id is a signed character but is typedefed so compiler helps keep
 * track of things */
typedef signed char tTimerId;

/*! One Second Timer handler that occurs in interrupt context */
unsigned char OneSecondTimerHandlerIsr(void);

/*! Stop timer associated with TimerId */
void StopTimer(tTimerId Id);

void ResetTimer(tTimerId Id);

/*! Setup and start a timer  
 *
 * \param Timeout in seconds
 * \param RepeatCount Number of times to count
 * \param Qindex is the index of the queue to put the message into
 * \param CallbackMsgType The type of message to send when the timer expires
 * \param MsgOptions Options to send with the message
*/
tTimerId StartTimer(unsigned int Timeout,
                    unsigned char RepeatCount,
                    unsigned char Qindex,
                    eMessageType CallbackMsgType,
                    unsigned char MsgOptions);

#endif /* ONE_SECOND_TIMERS_H */
