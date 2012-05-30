/*
   FreeRTOS.org V5.4.0 - Copyright (C) 2003-2009 Richard Barry.

   This file is part of the FreeRTOS.org distribution.

   FreeRTOS.org is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License (version 2) as published
   by the Free Software Foundation and modified by the FreeRTOS exception.
   **NOTE** The exception to the GPL is included to allow you to distribute a
   combined work that includes FreeRTOS.org without being obliged to provide
   the source code for any proprietary components.  Alternative commercial
   license and support terms are also available upon request.  See the
   licensing section of http://www.FreeRTOS.org for full details.

   FreeRTOS.org is distributed in the hope that it will be useful,   but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with FreeRTOS.org; if not, write to the Free Software Foundation, Inc., 59
   Temple Place, Suite 330, Boston, MA  02111-1307  USA.


   ***************************************************************************
   *                                                                         *
   * Get the FreeRTOS eBook!  See http://www.FreeRTOS.org/Documentation      *
   *                                                                         *
   * This is a concise, step by step, 'hands on' guide that describes both   *
   * general multitasking concepts and FreeRTOS specifics. It presents and   *
   * explains numerous examples that are written using the FreeRTOS API.     *
   * Full source code for all the examples is provided in an accompanying    *
   * .zip file.                                                              *
   *                                                                         *
   ***************************************************************************

   1 tab == 4 spaces!

   Please ensure to read the configuration and relevant port sections of the
   online documentation.

   http://www.FreeRTOS.org - Documentation, latest information, license and
   contact details.

   http://www.SafeRTOS.com - A version that is certified for use in safety
   critical systems.

   http://www.OpenRTOS.com - Commercial support, development, porting,
   licensing and training services.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#ifndef ASM_DEFINED
#include <intrinsics.h>
#endif
#include <msp430.h>

/*
Two interrupt examples are provided -

 + Method 1 does everything in C code.
 + Method 2 uses an assembly file wrapper.

Code size:
Method 1 uses assembly macros to save and restore the task context, whereas
method 2 uses functions. This means method 1 will be faster, but method 2 will
use less code space.

Simplicity:
Method 1 is very simplistic, whereas method 2 is more elaborate. This
elaboration results in the code space saving, but also requires a slightly more
complex procedure to define interrupt service routines.

Interrupt efficiency:
Method 1 uses the compiler generated function prologue and epilogue code to save
and restore the necessary registers within an interrupt service routine (other
than the RTOS tick ISR). Should a context switch be required from within the ISR
the entire processor context is saved. This can result in some registers being saved
twice - once by the compiler generated code, and then again by the FreeRTOS code.
Method 2 saves and restores all the processor registers within each interrupt service
routine, whether or not a context switch actually occurs. This means no registers
ever get saved twice, but imposes an overhead on the occasions that no context switch
occurs.
*/

#define configINTERRUPT_EXAMPLE_METHOD 1


/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/
#define configUSE_TICK_HOOK                 0
#define configUSE_IDLE_HOOK                 1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES               0
#define configMAX_CO_ROUTINE_PRIORITIES     (2)
#define configUSE_PREEMPTION                1

#define TOTAL_TASKS ( 7 )

/* the mulitplier is used by the FLL to generate the CLK */
#define ACLK_FREQUENCY_HZ                   ((unsigned int)32768)
#define ACLK_MULTIPLIER                     ((unsigned int)512)

#define configCPU_CLOCK_HZ                  ((unsigned portLONG) 16777216) /* 512*32768 */
#define configTICK_RATE_HZ                  ((portTickType)1024)

/* SPP threads use priority of 3 */
#define configMAX_PRIORITIES                ((unsigned portBASE_TYPE)4)
#define configMINIMAL_STACK_SIZE           ((unsigned portSHORT)90)

#ifdef SUPPORT_LOW_ENERGY
#define configTOTAL_HEAP_SIZE               ((size_t)6500)
#else
#define configTOTAL_HEAP_SIZE               ((size_t)5500)
#endif

#define configMAX_TASK_NAME_LEN             (16)
#define configUSE_TRACE_FACILITY            0
#define configUSE_16_BIT_TICKS              1
#define configIDLE_SHOULD_YIELD             1
#define configUSE_MUTEXES                   1
#define configCHECK_FOR_STACK_OVERFLOW      1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       0
#define configUSE_MALLOC_FAILED_HOOK        1
#define configUSE_APPLICATION_TASK_TAG      0

/*! the rtos tick count is approximatly 1 ms
 * 32768/32 = 1024 kHz = 0.9765625 ms 
 */
#define RTOS_TICK_COUNT  ( 1 )

#define DONT_WAIT ( 0 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet            0
#define INCLUDE_uxTaskPriorityGet           0
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskCleanUpResources       0
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             0
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/* include xTaskGetCurrentHandle for Threaded BTPSKRNL  */
#define INCLUDE_xTaskGetCurrentTaskHandle   1

#define INCLUDE_pcTaskGetTaskName 1

#ifdef SUPPORT_LOW_ENERGY
/* when clock is 16 MHz then each cycle is 62.5 ns */
#define __delay_us(x) __delay_cycles(x*16)
#else
/* when clock is 16777261 MHz then each cycle is 59.6 ns
 * -> 1013.28 us
 */
#define __delay_us(x) __delay_cycles(x*17)
#endif

#endif /* FREERTOS_CONFIG_H */
