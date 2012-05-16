;
;       FreeRTOS.org V5.4.0 - Copyright (C) 2003-2009 Richard Barry.
;
;       This file is part of the FreeRTOS.org distribution.
;
;       FreeRTOS.org is free software; you can redistribute it and/or modify it
;       under the terms of the GNU General Public License (version 2) as published
;       by the Free Software Foundation and modified by the FreeRTOS exception.
;       **NOTE** The exception to the GPL is included to allow you to distribute a
;       combined work that includes FreeRTOS.org without being obliged to provide
;       the source code for any proprietary components.  Alternative commercial
;       license and support terms are also available upon request.  See the
;       licensing section of http://www.FreeRTOS.org for full details.
;
;       FreeRTOS.org is distributed in the hope that it will be useful, but WITHOUT
;       ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;       FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
;       more details.
;
;       You should have received a copy of the GNU General Public License along
;       with FreeRTOS.org; if not, write to the Free Software Foundation, Inc., 59
;       Temple Place, Suite 330, Boston, MA  02111-1307  USA.
;
;
;       ***************************************************************************
;       *                                                                         *
;       * Get the FreeRTOS eBook!  See http://www.FreeRTOS.org/Documentation      *
;       *                                                                         *
;       * This is a concise, step by step, 'hands on' guide that describes both   *
;       * general multitasking concepts and FreeRTOS specifics. It presents and   *
;       * explains numerous examples that are written using the FreeRTOS API.     *
;       * Full source code for all the examples is provided in an accompanying    *
;       * .zip file.                                                              *
;       *                                                                         *
;       ***************************************************************************
;
;       1 tab == 4 spaces!
;
;       Please ensure to read the configuration and relevant port sections of the
;       online documentation.
;
;       http://www.FreeRTOS.org - Documentation, latest information, license and
;       contact details.
;
;       http://www.SafeRTOS.com - A version that is certified for use in safety
;       critical systems.
;
;       http://www.OpenRTOS.com - Commercial support, development, porting,
;       licensing and training services.

;
   .cdecls C, List,"FreeRTOSConfig.h"
 ;  .sect        ".text:_isr"

             .global usCriticalNesting
             .global pxCurrentTCB
             .global vTaskIncrementTick
             ;.ref    vTaskIncrementTick
             .global vTaskSwitchContext
             .global SetupRtosTimer
             .global vPortYield
             .global xPortStartScheduler
             .global RtosTickEnabled
             .global RtosTickCount

portSAVE_CONTEXT .macro
                bic        #CPUOFF+SCG1+SCG0,0(SP)
                pushm.a    #0x0c, r15
                movx.w   &usCriticalNesting, r15
                pushx.w    r15
                movx.w   &pxCurrentTCB, r12
                movx.w   r1, 0(r12)
               .endm


; /*-----------------------------------------------------------*/

; // ;               NAME portRESTORE_CONTEXT

portRESTORE_CONTEXT .macro

                movx.w   &pxCurrentTCB, r12
                movx.w   @r12, r1

                popx.w     r15

                movx.w   r15, &usCriticalNesting

                                popm.a  #0x0c, r15
                reti

               .endm

; /*-----------------------------------------------------------*/

; /*
; * The RTOS tick ISR.
; *
; * If the cooperative scheduler is in use this simply increments the tick
; * count.
; *
; * If the preemptive scheduler is in use a context switch can also occur.
; */
				.sect 		".text"
				

xPortStartScheduler:

;               /* Setup the hardware to generate the tick.  Interrupts*/
;               /* are disabled when this function is called.          */

                ;calla    #prvSetupTimerInterrupt
				        calla    #SetupRtosTimer
;               /* Restore the context of the first task that is going */
;               /* to run.                                             */

                portRESTORE_CONTEXT

;/*
; * Manual context switch called by the portYIELD() macro.
; */
				.sect ".text"
vPortYield:

;               /* Mimic an interrupt by pushing the SR.                */

;               /* SR is 16-bits in 430X architecture                   */
				bic.w     #CPUOFF+SCG1+SCG0,SR 
                pushx.w    SR

;                /* Now the SR is stacked we can disable interrupts.    */

                dint
				nop
                bicx.w #0xF000,0(r1)
                swpbx.w +4(r1)
                rlax.w +4(r1)
                rlax.w +4(r1)
                rlax.w +4(r1)
                rlax.w +4(r1)
                addx.w +4(r1),0(r1)
                movx.w +2(r1),+4(r1)
                movx.w 0(r1),+2(r1)
                incdx.a r1

;               /* Save the context of the current task.                */
                portSAVE_CONTEXT

;               /* Switch to the highest priority task that is ready to */
;               /* run.                                                 */
                calla    #vTaskSwitchContext

       			 portRESTORE_CONTEXT

			.sect        ".text:_isr"



vTickISRCheck:  tst.b &RtosTickEnabled
                jne vTickISR  
                reti



			.sect        ".text:_isr"


vTickISR:
                portSAVE_CONTEXT
  				      movx.w &TA0CCR0,&TA0R
                add.w &RtosTickCount,&TA0CCR0
                calla    #vTaskIncrementTick
                calla    #vTaskSwitchContext 
                portRESTORE_CONTEXT

; /*-----------------------------------------------------------*/





;              /* Place the tick ISR in the correct vector.             */
               .sect ".int54"                  ; TIMER0_A0_VECTOR
               .short   vTickISRCheck
               .end


