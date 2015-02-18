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

#define WDT_SHORT       (WDTIS_3) // 16s
#define WDT_LONG        (WDTIS_2) // 4x60 = 240s

/*! kick the watchdog timer */
void ResetWatchdog(void);

/*! cause a reset to occur because of the watchdog expiring */
void WatchdogReset(void);

void SetWatchdogInterval(unsigned char Intvl);

void UpdateQueueInfo(void);

typedef enum
{
  eWrapperTaskCheckInId = 0,
  eDisplayTaskCheckInId = 1
  /* add user tasks here */
  
} etTaskCheckInId;

#define ALL_TASKS_HAVE_CHECKED_IN ( 0x03 )

void TaskCheckIn(etTaskCheckInId TaskId);
void CheckLpm(void);

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
