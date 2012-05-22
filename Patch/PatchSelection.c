//==============================================================================
//  Copyright 2012 Meta Watch Ltd. - http://www.MetaWatch.org/
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
/*! \file PatchSelection.c
 *
 * Control what patch is loaded onto the radio
 */
/******************************************************************************/

#include "Adc.h"
#include "PatchSelection.h"

/******************************************************************************/

static etPatchVersion PatchVersion = PatchVersion1315;

void SetPatchVersion(etPatchVersion Version)
{
  PatchVersion = Version;    
}

etPatchVersion GetPatchVersion(void)
{
  return PatchVersion;  
}

/******************************************************************************/

unsigned int GetPatchLength(void)
{
  unsigned int Length = 0;
  
#if defined(INCLUDE_BOTH_PATCHES)
  
  if ( GetPatchVersion() == PatchVersion1316 )
  {
    Length = sizeof(Patch1316);
  }
  else
  {
    Length = sizeof(Patch1315);  
  }

#elif defined(INCLUDE_1316_PATCH)

  Length = sizeof(Patch1316);
  
#elif defined(INCLUDE_1315_PATCH)
  
  Length = sizeof(Patch1315); 

#else

#error "At least one patch must be included"

#endif
  
  return Length;

}

/******************************************************************************/

#ifdef __IAR_SYSTEMS_ICC__

unsigned char const __data20 * GetPatchAddress(void)
{
  unsigned char const __data20 * pPatch;
  
#if defined(INCLUDE_BOTH_PATCHES)
  
  if ( GetPatchVersion() == PatchVersion1316 )
  {
    pPatch = Patch1316;
  }
  else
  {
    pPatch = Patch1315;  
  }

#elif defined(INCLUDE_1316_PATCH)

  pPatch = Patch1316;
  
#elif defined(INCLUDE_1315_PATCH)
  
  pPatch = Patch1315;  

#endif
  
  return pPatch;
  
}

#endif

/*! todo - this doesn't check that the radio supports low energy
 * just that the patch selection does
 */
unsigned char QuerySupportLowEnergy(void)
{
  unsigned char result = 0;
  
#ifdef SUPPORT_LOW_ENERGY
  #ifdef INCLUDE_BOTH_PATCHES
    result = 1;
  #endif
  #ifdef INCLUDE_1316_PATCH
    result = 1;
  #endif
#endif
  
  return result;
}


   /* The following function is used to copy                            */
   /* Patch data to a local buffer.  This is done because a SMALL data  */
   /* model configuration can't access data past 64k.  The function     */
   /* receives as its first parameter the offset from the beginning of  */
   /* the Patch data where the copy should begin.  The second parameter */
   /* is a pointer to a local buffer where the data is to be moved.  The*/
   /* last parameter is the number of bytes that are to be moved.       */
   /* * NOTE * The C compiler will not process the information within   */
   /*          the 'asm' statements so we must reference the parameters */
   /*          passed in to the optimizer will remove variables that are*/
   /*          not referenced.  If none of the variables are referenced */
   /*          in 'C' code then the entire function will be optimized   */
   /*          out.                                                     */
   /* * NOTE * This function will not allow an offset of 0xFFFF.        */
void MovePatchBytes(unsigned int offset, unsigned char *dest, unsigned int length)
{

  /* Check to make sure that the parameters passed in appear valid.    */
  if((offset+1) && (dest) && (length))
  {
    
#ifdef __IAR_SYSTEMS_ICC__

    while(length--)
    {
      *dest++ = *((unsigned char __data20 *)(GetPatchAddress() + offset));
      offset++;
    }
      
#else /* TI_COMPILER_VERSION */

    
    asm("LOOP:");
      
#if defined(INCLUDE_BOTH_PATCHES)
  
    if ( GetPatchVersion() == PatchVersion1316 )
    {
      asm("    MOVX.B   Patch1316+0(r12),0(r13)");  /* Move 1 bytes         */    
    }
    else
    {
      asm("    MOVX.B   Patch1315+0(r12),0(r13)");  /* Move 1 bytes         */         
    }

#elif defined(INCLUDE_1316_PATCH)
      
    asm("    MOVX.B   Patch1316+0(r12),0(r13)");  /* Move 1 bytes         */
  
#elif defined(INCLUDE_1315_PATCH)
    
    asm("    MOVX.B   Patch1315+0(r12),0(r13)");  /* Move 1 bytes         */         
           
#endif /* patch selection */
  
    asm("    ADD.W    #1,r12");                   /* INC offset           */
    asm("    ADD.W    #1,r13");                   /* INC dest             */
    asm("    SUB.W    #1,r14");                   /* DEC length           */
    asm("    CMP.W    #0,r14");                   /* Check (length == 0)  */
    asm("    JNE      LOOP");
       
#endif /* environment selection */

  }
  else
  {
    /* Clear the Packet Flag.                                         */
    *dest = 0;
  }
}
