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
/*! \file CommandTask.h
 * 
 * The command task handles commands.  Commands were used instead of messages so
 * that a call to buffer allocate was not required in an ISR.  Many messages only
 * need type and options to do what they need to.
 *
 * It may be better to create a new type that is passed in the queues. So that 
 * the same queue could handle both a 'message' and a 'command'
 * struct 
 * { 
 *   Length;
 *   MessageType;
 *   Options;
 *   pBuffer;
 * }
 * This will increase the memory footprint of the queues, but speed up overall
 * operation.
 */
/******************************************************************************/

#ifndef COMMAND_TASK_H
#define COMMAND_TASK_H

/*! Create command task and initialize command queue 
 *
 * The command task handles buttons, vibration, one second timers, and the 
 * watchdog.
 * 
 */
void InitializeCommandTask(void);


/*! Send a command to the command task.
 *
 * \param CmdType Enumerated command type described in messages.h
 */
void RouteCommand(eCmdType CmdType);

/*! Send a command to the command task from an ISR 
 *
 * \param CmdType Enumerated command type described in messages.h
 */
void RouteCommandFromIsr(eCmdType CmdType);

/*! Call from the Idle task to determine if the 
 * command task is ready to sleep (the command queue is empty)
 *
 * \return 0 - not ready, 1 - ready to sleep
 */
unsigned char CommandTaskReadyToSleep(void);

/*! Force the processor to reset by writing an invalid key to the watchdog register */
void ForceWatchdogReset(void);

#endif /* COMMAND_TASK_H */