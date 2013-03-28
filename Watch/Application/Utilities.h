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

#ifndef UTILITIES_H
#define UTILITIES_H

#ifndef TASK_H
  #error "task.h must be included before Utilities.h"
#endif

#ifndef DEBUG_UART_H
  #error "DebugUart.h must be included before Utilities.h"
#endif
  
/*! This determines how many tasks can be kept track of in the utility functions */
#define MAX_NUM_TASK_LOG_ENTRIES  ( TOTAL_TASKS )

#if TASK_DEBUG
/*! Cycle through all of the tasks and display how much stack they are using 
 * (PrintS( information for one task each call)
 */
void UTL_FreeRtosTaskStackCheck(void);
#endif

/*! Check how much statck a task is currently using
 *
 * \param TaskHandle points to the task
 * \param TaskName is a string representation of the task
 */
void CheckStackUsage(xTaskHandle TaskHandle,tString * TaskName);

/*! check how full a queue is */
void CheckQueueUsage(xQueueHandle Qhandle);

#endif /* UTILITIES_H */
