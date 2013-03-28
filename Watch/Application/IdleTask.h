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

#ifndef IDLE_TASK_H
#define IDLE_TASK_H

/*! Parameters that are used to determine if micro can go into LPM3 */
typedef struct
{
  unsigned char SppReadyToSleep;
  unsigned char TaskDelayLockCount;
  unsigned int DisplayMessagesWaiting;
  unsigned int SppMessagesWaiting;
  
} tWatchdogInfo;

/*! This function prints about the cause of a watchdog reset
 * It also saves the values to non-volatile memory for the last occurence
 * of a watchdog reset.
 *
 * \param ResetSource ( SYSRSTIV )
 */
void ShowWatchdogInfo(void);

/*! This function keeps track of the number of messages in each queue. */
void UpdateWatchdogInfo(void);

/*! kick the watchdog timer */
void ResetWatchdog(void);

/*! cause a reset to occur because of the watchdog expiring */
void WatchdogReset(void);

/******************************************************************************/

typedef enum
{
  eWrapperTaskCheckInId = 0,
  eDisplayTaskCheckInId = 1
  /* add user tasks here */
  
} etTaskCheckInId;

#define ALL_TASKS_HAVE_CHECKED_IN ( 0x03 )

void TaskCheckIn(etTaskCheckInId TaskId);

#if CHECK_CSTACK

#define STACK_END_ADDR    (0x5B7F)
#define STACK_SIZE          (140)
#define FILL                '.'
#define MARK                'c'

/* Calling a function uses the stack - so use a macro */
#define PopulateCStack() {                \
  char *pStack = (char *)STACK_END_ADDR;  \
  char i = STACK_SIZE;                    \
  while(i--) *pStack-- = FILL;            \
}                                                           

#define CheckCStack() {                   \
  char *pStack = (char *)STACK_END_ADDR;  \
  char i = STACK_SIZE, k = 0;             \
  while (i--) {                           \
    if (*pStack != FILL) k++;             \
    PrintC(*pStack-- == FILL ? FILL : MARK);\
  }                                       \
  PrintF(" CSTK used:%d", k);     \
}

#endif //CHECK_CSTACK

#endif /* IDLE_TASK_H */
