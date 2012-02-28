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

	FreeRTOS.org is distributed in the hope that it will be useful,	but WITHOUT
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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "hal_rtos_timer.h"

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the MSP430 port.
 *----------------------------------------------------------*/

/*
 * Constants required for hardware setup.
 * The tick ISR runs off the ACLK, not the MCLK.
 */
#define portACLK_FREQUENCY_HZ			   ((portTickType)32768)
#define portINITIAL_CRITICAL_NESTING	((unsigned portSHORT)10)

#define portFLAGS_INT_ENABLED			   ((portSTACK_TYPE)0x08)

/* We require the address of the pxCurrentTCB variable, but don't want to know
any details of its type. */
typedef void tskTCB;
extern volatile tskTCB * volatile pxCurrentTCB;

/* Each task maintains a count of the critical section nesting depth.  Each
time a critical section is entered the count is incremented.  Each time a
critical section is exited the count is decremented - with interrupts only
being re-enabled if the count is zero.

usCriticalNesting will get set to zero when the scheduler starts, but must
not be initialised to zero as this will cause problems during the startup
sequence. */
volatile unsigned portSHORT usCriticalNesting = portINITIAL_CRITICAL_NESTING;
/*-----------------------------------------------------------*/

/*
 * Sets up the periodic ISR used for the RTOS tick.  This uses timer 0, but
 * could have alternatively used the watchdog timer or timer 1.
 */
void prvSetupTimerInterrupt( void );
/*-----------------------------------------------------------*/

/*
 * Initialise the stack of a task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 *
 * See the header file portable.h.
 */

portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{

	/*
		Place a few bytes of known values on the bottom of the stack.
		This is just useful for debugging and can be included if required.

		*pxTopOfStack = ( portSTACK_TYPE ) 0x1111;
		pxTopOfStack--;
		*pxTopOfStack = ( portSTACK_TYPE ) 0x2222;
		pxTopOfStack--;
		*pxTopOfStack = ( portSTACK_TYPE ) 0x3333;
		pxTopOfStack--;
	*/

	/* The msp430 automatically pushes the PC then SR onto the stack before
	executing an ISR.  We want the stack to look just as if this has happened
	so place a pointer to the start of the task on the stack first - followed
	by the flags we want the task to use when it starts up. */
	
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode;
	pxTopOfStack--;
	*pxTopOfStack = ((( portSTACK_TYPE )((((unsigned long)(pxCode)) >>4) & (0x0000F000))) | portFLAGS_INT_ENABLED);
	pxTopOfStack--;

	/* Next the general purpose registers. */
//  Make this so popm and pushm will work to optimize context switches
//  XXX Note - this is incompatible with previous ports unless the pop
//  and push orders are reversed in the context switches.

	*pxTopOfStack = ( portSTACK_TYPE ) 0x000f;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xffff;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x000e;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xeeee;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x000d;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xdddd;
	pxTopOfStack--;

	
// Args passed in R12...
	
	*pxTopOfStack = (portSTACK_TYPE) ((((unsigned long)pvParameters)>>16) & (0x0000000F));
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x000b;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xbbbb;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x000a;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xaaaa;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x0009;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x9999;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x0008;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x8888;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x0007;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x7777;
	pxTopOfStack--;


// pvParameters has been moved from R15 to R12.  The IAR compiler will
// move R12 into R10 upon task (function) entry.  Note this may not
// work on all data types...Data types may need to be known apriori
// to make this work in all cases.	

// Pass in R12...
	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x0006;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x6666;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x0005;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x5555;
	pxTopOfStack--;


	/* When the task starts is will expect to find the function parameter in
	R15. */
	*pxTopOfStack = ( portSTACK_TYPE ) 0x0004;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0x4444;
	pxTopOfStack--;


	/* A variable is used to keep track of the critical section nesting.
	This variable has to be stored as part of the task context and is
	initially set to zero. */

	/* this may need to be shifted as well...						  */
	*pxTopOfStack = ( portSTACK_TYPE ) portNO_CRITICAL_SECTION_NESTING;	


	/* Return a pointer to the top of the stack we have generated so this can
	be stored in the task control block for the task. */
	return pxTopOfStack;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the MSP430 port will get stopped.  If required simply
	disable the tick interrupt here. */
}
/*-----------------------------------------------------------*/



/*-----------------------------------------------------------*/

	
