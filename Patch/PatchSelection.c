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
