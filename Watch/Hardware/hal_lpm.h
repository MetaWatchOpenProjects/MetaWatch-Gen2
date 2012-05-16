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
/*! \file hal_lpm.h
 *
 * Most of the time the watch should be in low power mode (lpm). For this project,
 * the RTOS tick must be disabled before entering sleep mode.
 *
 */
/******************************************************************************/

#ifndef HAL_LPM_H
#define HAL_LPM_H

/* 
 * MSP430 Exit LPM3 macro
*/
#define EXIT_LPM_ISR() (LPM3_EXIT)

/*!
 * Put the processor into LPM3.  If the shipping mode flag is set then
 * put the processor into LPM4.  Once in LPM4 the part must be reset (attached
 * to the clip).
 */
void MSP430_LPM_ENTER(void);

/*! set the shipping mode flag that allows the part to be placed into LPM4 */
void SetShippingModeFlag(void);

/*! clear the shipping mode flag that allows the part to be placed into LPM4 */
void ClearShippingModeFlag(void);

/*! reset the micro by writing to PMMCTL0 */
void SoftwareReset(void);


#define RST_PIN_ENABLED  ( 0x01 )
#define RST_PIN_DISABLED ( 0x02 )

/*! Query whether or not the reset pin is enabled
 *
 * \return 0 = not enabled, 1 = reset pin enabled
 */
unsigned char QueryRstPinEnabled(void);

/*! Configure the reset pin functionality */
void ConfigureResetPinFunction(unsigned char Control);

/*! Enable the Rst pin (let it reset part) */
void EnableRstPin(void);

/*! Disable the Rst pin (don't let it reset the MSP430) */
void DisableRstPin(void);

#endif /* HAL_LPM_H */
