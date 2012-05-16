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
/*! \file OledDriver.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"

#include "hal_board_type.h"
#include "hal_clock_control.h"

#include "Messages.h"
#include "DebugUart.h"
#include "hal_oled.h"
#include "OledDriver.h"

#define COMMAND_CONTROL_BYTE           ( 0x80 )
#define DATA_CONTROL_BYTE              ( 0xC0 )
#define DATA_CONTINUATION_CONTROL_BYTE ( 0x40 )

void OledPowerUpSequence(void)
{
  /* 
   * special power on sequence 
   */
  
  /* power on io */
  OLED_IO_POWER_ENABLE();
  __delay_cycles(3000);
  
  /* assert reset */
  OLED_RSTN_ASSERT();
  __delay_cycles(3000);
  
  /* enable power */
  OLED_POWER_ENABLE();  
  __delay_cycles(3000);
  
  /* disable reset */
  OLED_RSTN_DEASSERT();
  __delay_cycles(3000);
    
}

void OledPowerDown(void)
{
  OLED_IO_POWER_DISABLE();    
  OLED_IO_POWER_DISABLE();
}

void SetOledDeviceAddress(etOledPosition OledPosition)
{
  if ( OledPosition == TopOled )
  {
    OLED_I2C_I2CSA = DISPLAY_ADDRESS_TOP;
  }
  else
  {
    OLED_I2C_I2CSA = DISPLAY_ADDRESS_BOTTOM;  
  }
}


/* for the OLED many things are two byte sequences
 * the first indicates the type (cmd/data) and the next byte is the data
 */
void WriteOledDataPacket(unsigned char data)
{
  OledWrite(DATA_CONTROL_BYTE,&data,1);
}

void WriteOneByteOledCommand(unsigned char cmd)
{
  OledWrite(COMMAND_CONTROL_BYTE,&cmd,1);  
}

void InitializeOledDisplayController(etOledPosition OledPosition,
                                     unsigned char Contrast)
{
  SetOledDeviceAddress(OledPosition);
  
  //internal dc/dc on/off setting
  WriteOneByteOledCommand(0xad); 
  
  //internal dc/dc off
  WriteOneByteOledCommand(0x8e); 
  
  //display off
  WriteOneByteOledCommand(0xae); 
  
  // NOTE:  a 64 multiplex ratio seems to be the only one that gives a visible
  // output on the display...
  
  //multiplex ratio, /16 duty
  WriteOneByteOledCommand(0xA8);
  WriteOneByteOledCommand(0x3f); 
  
  //display offset, second byte
  WriteOneByteOledCommand(0xD3);
  WriteOneByteOledCommand(0x00);
  
  //start line
  WriteOneByteOledCommand(0x40); 
  
  //com out scan direction
  WriteOneByteOledCommand(0xc0); 
  
  //set normal/inverse display (0xa6: normal display)
  WriteOneByteOledCommand(0xa6); 
  
  //set entire display on/off (0xa4: normal display, 0xa5 all on)
  WriteOneByteOledCommand(0xa5); 
  
  //contrast setting Bank 0, second byte
  WriteOneByteOledCommand(OLED_CMD_CONTRAST);
  WriteOneByteOledCommand(Contrast);
 
  //Pre-charge/Discharge, second byte
  WriteOneByteOledCommand(0xD9); 
  WriteOneByteOledCommand(0x11);
  
  //set COM pins hardware configuration, second byte
  WriteOneByteOledCommand(0xDA); 
  WriteOneByteOledCommand(0x12);
  
  //set area color mode/low power display mode, second byte
  WriteOneByteOledCommand(0xD8); 
  WriteOneByteOledCommand(0x05);
  
  // segmet remap - flip the display horizontally 
  if( OledPosition == TopOled ) 
  {
    WriteOneByteOledCommand(OLED_CMD_SEGMENT_ORDER_REVERSE); 
  }
  else
  {
    WriteOneByteOledCommand(OLED_CMD_SEGMENT_ORDER_NORMAL);  
  }
  
  //display on normal mode
  WriteOneByteOledCommand(OLED_CMD_DISPLAY_ON_NORMAL_MODE); 
 
  /* setup for horizontal addressing mode */
  WriteOneByteOledCommand(0x20);
  WriteOneByteOledCommand(0x00);
 
}

/* use the data continuation command to write data to the display memory */
void WriteOledData(unsigned char* pData,unsigned char Length)
{
  OledWrite(DATA_CONTINUATION_CONTROL_BYTE,pData,Length); 
}


/* this only supports a row of 0 or 1 */
void SetRowInOled(unsigned char RowNumber,etOledPosition OledPosition)
{
  /* flip the rows for the bottom oled */
  if ( OledPosition == BottomOled )
  {
    if ( RowNumber == 0 )
    {
      RowNumber = 1;  
    }
    else
    {
      RowNumber = 0;
    }
  }
  
  // the set page command is 0xB0 the page number is the 3 lsbs
  WriteOneByteOledCommand( (0xb0  + OLED_FIRST_PAGE_INDEX +  RowNumber));
  
  // intialize the column address, this is a two byte value, each byte
  // contains a nibble of the column address

  // set lower column start address for page addressing mode
  WriteOneByteOledCommand((0x00 | ( OLED_COLUMN_OFFSET & 0x0f    )));
  // higher column address  
  WriteOneByteOledCommand((0x10 | ((OLED_COLUMN_OFFSET & 0xf0)>>4)));  
  
}
