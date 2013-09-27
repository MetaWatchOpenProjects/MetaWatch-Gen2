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

#include "FreeRTOS.h"
#include "portmacro.h"
#include "hal_boot.h"
#include "DebugUart.h"
#include "Property.h"
#include "hal_boot.h"

#define NVID_TIME_FORMAT                  ( 0x2009 )
#define NVID_DATE_FORMAT                  ( 0x200a )
#define NVID_DISPLAY_SECONDS              ( 0x200b )
#define NVID_IDLE_BUFFER_INVERT           ( 0x0003 )
#define NVID_LINK_ALARM_ENABLE            ( 0x2006 )

#if __IAR_SYSTEMS_ICC__
__no_init __root unsigned int niProperty @ PROPERTY_ADDR;
#else
extern unsigned int niProperty;
#endif

void InitProperty(void)
{
  niProperty = PROP_DEFAULT;
}

unsigned char GetProperty(unsigned char Bits)
{
  return niProperty & Bits;
}

unsigned char PropertyBit(unsigned int NvIdentifier)
{
  unsigned char PropertyBit = 0;

  switch (NvIdentifier)
  {
  case NVID_TIME_FORMAT:
    PropertyBit = PROP_24H_TIME_FORMAT;
    break;
    
  case NVID_DATE_FORMAT:
    PropertyBit = PROP_DDMM_DATE_FORMAT;
    break;

  case NVID_DISPLAY_SECONDS:
    PropertyBit = PROP_TIME_SECOND;
    break;
    
  case NVID_IDLE_BUFFER_INVERT:
    PropertyBit = PROP_INVERT_DISPLAY;
    break;

  case NVID_LINK_ALARM_ENABLE:

  default:
    break;
  }
  return PropertyBit;
}

void SetProperty(unsigned char Bits, unsigned char Val)
{
  niProperty |= Bits & Val; // set all 1 bits of Val
  niProperty &= ~Bits | Val; // set all 0 bits of Val
  PrintF("Prop:x%02X", niProperty);
}

void ToggleProperty(unsigned char Bit)
{
  if (niProperty & Bit) niProperty &= ~Bit;
  else niProperty |= Bit;
}
