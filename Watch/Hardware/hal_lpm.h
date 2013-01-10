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
 *
 * This is different from the default in that it doesn't
 * modify the FLL control bit (SCG0) when exiting LPM
 * 
 * This intrinsic function clears the bits on return from the interrupt
 */
#define EXIT_LPM_ISR() { _BIC_SR_IRQ(SCG1+OSCOFF+CPUOFF); __no_operation(); }

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

/*! get the state of the shipping mode flag that allows the part to be placed into LPM4 */
unsigned char GetShippingModeFlag(void);

/*! reset the micro by writing to PMMCTL0 */
void SoftwareReset(void);


#define RST_PIN_DISABLED ( 0x00 )
#define RST_PIN_ENABLED  ( 0x01 )
#define RST_PIN_TOGGLED  ( 0x02 )

/*! Query reset pin configuration
 *
 * \return 0 = disabled, 1 = enabled
 */
unsigned char ResetPin(void);

/*! Configure the reset pin functionality.  This does not save the state. 
 * 
 * \param Control 0 = disabled, 1 = enable, 2 = toggle
 */
void ConfigResetPin(unsigned char Control);


#endif /* HAL_LPM_H */
