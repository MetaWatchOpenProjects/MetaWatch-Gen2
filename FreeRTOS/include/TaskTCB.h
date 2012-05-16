#ifndef TASKTCB_H_
#define TASKTCB_H_

#include "FreeRTOS.h"
#include "task.h"
#include "StackMacros.h"

/*
 * Task control block.  A task control block (TCB) is allocated to each task,
 * and stores the context of the task.
 */
typedef struct tskTaskControlBlock
{
   volatile portSTACK_TYPE *pxTopOfStack;    /*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE STRUCT. */

   #if ( portUSING_MPU_WRAPPERS == 1 )
      xMPU_SETTINGS xMPUSettings;            /*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE STRUCT. */
   #endif   
   
   xListItem            xGenericListItem; /*< List item used to place the TCB in ready and blocked queues. */
   xListItem            xEventListItem;      /*< List item used to place the TCB in event lists. */
   unsigned portBASE_TYPE  uxPriority;       /*< The priority of the task where 0 is the lowest priority. */
   portSTACK_TYPE       *pxStack;         /*< Points to the start of the stack. */
   signed char          pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */

   #if ( portSTACK_GROWTH > 0 )
      portSTACK_TYPE *pxEndOfStack;       /*< Used for stack overflow checking on architectures where the stack grows up from low memory. */
   #endif

   #if ( portCRITICAL_NESTING_IN_TCB == 1 )
      unsigned portBASE_TYPE uxCriticalNesting;
   #endif

   #if ( configUSE_TRACE_FACILITY == 1 )
      unsigned portBASE_TYPE  uxTCBNumber;   /*< This is used for tracing the scheduler and making debugging easier only. */
   #endif

   #if ( configUSE_MUTEXES == 1 )
      unsigned portBASE_TYPE uxBasePriority; /*< The priority last assigned to the task - used by the priority inheritance mechanism. */
   #endif

   #if ( configUSE_APPLICATION_TASK_TAG == 1 )
      pdTASK_HOOK_CODE pxTaskTag;
   #endif

   #if ( configGENERATE_RUN_TIME_STATS == 1 )
      unsigned long ulRunTimeCounter;     /*< Used for calculating how much CPU time each task is utilising. */
   #endif

} tskTCB;

#endif /*TASKTCB_H_*/
