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
/*! \file PatchSelection.h
 *
 * This controls which patches are stored in flash.  If both patches are
 * included then the firmware reads the board configuration register to determine
 * the correct patch to load into the radio.
 *
 * PAN1315 - CC2560
 * PAN1316 - CC2560A, CC2564, CC2567
 * 
 * todo update for new patch
 *
 * CC2560       - 39150 bytes
 * CC2564       - 8948 bytes 
 * CC2564 + LE  - 9975 bytes (+1027) 
 * BOTH PATCHES - 48098 bytes
 * BOTH + LE    - 49125 bytes
 *
 * By defining INCLUDE_1315_PATCH or INCLUDE_1316_PATCH only one patch is 
 * included. There isn't any protection for selecting the wrong patch.
 *
 * define SUPPORT_LOW_ENERGY to include LE portion of the patch 
 * 
 */
/******************************************************************************/

#ifndef PATCH_SELECTION_H
#define PATCH_SELECTION_H

/******************************************************************************/
typedef enum
{
  PatchVersion1315, /* cc2560 */
  PatchVersion1316  /* cc2560A, cc2564, cc2567 */

} etPatchVersion;

void SetPatchVersion(etPatchVersion Version);

etPatchVersion GetPatchVersion(void);

/******************************************************************************/

/* select one or both patches */
#if defined(INCLUDE_BOTH_PATCHES)
  
  #include "Patch_PAN1315.h"
  #include "Patch_PAN1316.h"

#elif defined(INCLUDE_1316_PATCH)

  #include "Patch_PAN1316.h"
  
#elif defined(INCLUDE_1315_PATCH)
  
  #include "Patch_PAN1315.h"

#endif

/******************************************************************************/
 
/*! \return The length of the patch in bytes */
unsigned int GetPatchLength(void);

#ifdef __IAR_SYSTEMS_ICC__

/*! \return The starting address of the radio patch */
unsigned char const __data20 * GetPatchAddress(void);

#else

	/* code composer does not have __data20
	 * need to use assembly to load patch 
	 */
	#if defined(INCLUDE_BOTH_PATCHES)
	  
	  extern const unsigned char Patch1315[];
	  extern const unsigned char Patch1316[];
	
	#elif defined(INCLUDE_1316_PATCH)
	
	  extern const unsigned char Patch1316[];
	  
	#elif defined(INCLUDE_1315_PATCH)
	  
	  extern const unsigned char Patch1315[];
	
	#endif

#endif

/******************************************************************************/

void MovePatchBytes(unsigned int offset, unsigned char *dest, unsigned int length);

/******************************************************************************/

/*! \return 1 if low energy is supported */
unsigned char QuerySupportLowEnergy(void);     

/******************************************************************************/
     

#endif /* PATCH_SELECTION_H */
