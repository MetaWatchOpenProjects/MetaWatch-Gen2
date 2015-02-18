//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#ifndef MUX_MODE_H
#define MUX_MODE_H

#define MUX_MODE_GND            0
#define MUX_MODE_SERIAL         1
#define MUX_MODE_SPY_BI_WIRE    2
#define MUX_MODE_OFF            3

/*! return the mode the mux control should be in
 * (output of micro can be overriden by pressing A (top right) during power-up
 *
 * \return 0 = gnd, 1 = serial, 2 = spy-bi-wire
 */
unsigned char GetMuxMode(void);

void SetMuxMode(unsigned char MuxMode);

/*! Initialize the state of the mux that allows the case back pins 
 * to be serial port, GND, or spy-bi-wire
 */
void ChangeMuxMode(unsigned char ClipOn);

#endif /* MUX_MODE_H */
