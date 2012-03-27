/*****************************************************************************/
/* _SYSTEM_PRE_INIT.C   v3.3.3 - Perform any application-specific inits      */
/* Copyright (c) 2003-2010 Texas Instruments Incorporated                    */
/*****************************************************************************/

/*****************************************************************************/
/* _SYSTEM_PRE_INIT() - _system_pre_init() is called in the C/C++ startup    */
/* routine (_c_int100() in boot.c) and provides a mechanism for the user to  */
/* insert application specific low level initialization instructions prior   */
/* to calling main().  The return value of _system_pre_init() is used to     */
/* determine whether or not C/C++ global data initialization will be         */
/* performed (return value of 0 to bypass C/C++ auto-initialization).        */
/*                                                                           */
/* PLEASE NOTE THAT BYPASSING THE C/C++ AUTO-INITIALIZATION ROUTINE MAY      */
/* RESULT IN PROGRAM FAILURE.                                                */
/*                                                                           */
/* The version of _system_pre_init() below is skeletal and is provided to    */
/* illustrate the interface and provide default behavior.  To replace this   */
/* version rewrite the routine and include it as part of the current project.*/
/* The linker will include the updated version if it is linked in prior to   */
/* linking with the C/C++ runtime library (rts430.lib).                      */
/*****************************************************************************/
#include <msp430.h>
#include <string.h>
#include <intrinsics.h>

extern void * __bss__;
extern long   _stack;
extern long   end;
extern long   __STACK_END;

int _system_pre_init(void)
{
   unsigned char *dest;
   unsigned int   length;

   WDTCTL = WDTPW+WDTHOLD;

   dest   = (unsigned char *)&__bss__;
   length = (int)((long)&end - (long)dest);
   memset(dest, 0, length);

   length = (int)((long)&__STACK_END - (long)&_stack);
   memset(&_stack, 0x5A, (length-32));

   return 1;
}

