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

#ifndef PROPERTY_H
#define PROPERTY_H

/* Option byte of Nval operation (0x30) message */
#define PROP_24H_TIME_FORMAT    (0x01)
#define PROP_DDMM_DATE_FORMAT   (0x02)
#define PROP_TIME_SECOND        (0x04)
#define PROP_WIDGET_GRID        (0x08)
#define PROP_AUTO_BACKLIGHT     (0x10)

/* internal usage */
#define PROP_PHONE_DRAW_TOP     (0x20)
#define PROP_INVERT_DISPLAY     (0x40)
//#define PROP_DISABLE_LINK_ALARM (0x80)

#define PROP_VALID              (0x01F)
#define PROP_DEFAULT            (PROP_WIDGET_GRID | PROP_AUTO_BACKLIGHT | PROP_PHONE_DRAW_TOP)

#define PROPERTY_READ           (0x80)

unsigned char GetProperty(unsigned char Bits);
void SetProperty(unsigned char Bits, unsigned char Val);
void ToggleProperty(unsigned char Bit);
unsigned char PropertyBit(unsigned int NvIdentifier);

#endif /* PROPERTY_H */
