//==============================================================================
//  Copyright 2011 Meta Watch Ltd. - http://www.MetaWatch.org/
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
/*! \file hal_miscellaneous.h
*
* Set pins to outputs; setup clock and power management module
*/
/******************************************************************************/

#ifndef HAL_MISCELLANEOUS_H
#define HAL_MISCELLANEOUS_H


/*! 
 * This sets all of the pins to outputs.
 * It is up to each peripheral's driver to setup their pins 
 */
void SetAllPinsToOutputs(void);


/*! Send the 32 kHz clock to the Bluetooth Radio */
void SetupAclkToRadio(void);

/*! Setup the CPU clock and the power managment module.
 * This function uses TI provided routines to start the crystal
 * and setup the FLL.
 *
 * the core voltage must be set appropriately for the MHz that is desired
 *
 */
void SetupClockAndPowerManagementModule(void);


/*! setup pins that aren't setup by specific functions 
 * catch-all
 * \note Board Configuration must be valid before this is called
 */
void ConfigureDefaultIO(unsigned char BoardConfiguration);


#endif /* HAL_MISCELLANEOUS_H */
