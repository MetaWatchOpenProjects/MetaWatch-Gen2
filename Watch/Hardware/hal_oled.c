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
/*! \file hal_oled.c
*
* The OLEDs are I2C devices
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"

#include "hal_board_type.h"
#include "hal_oled.h"
#include "hal_clock_control.h"
#include "DebugUart.h"

/******************************************************************************/

static unsigned char OledBusy;
static unsigned char Count;
static unsigned char Index;
static unsigned char* pOledData;

/******************************************************************************/

void InitOledI2cPeripheral(void)
{
  /* setup the pins */
  OLED_I2C_CONFIG_FOR_PERIPHERAL_USE();
  
  /* enable reset before configuration */
  OLED_I2C_CTL1 |= UCSWRST;
  
  /* configure as master using smclk / 50 = 320 kHz */
  OLED_I2C_CTL0 = UCMST + UCMODE_3 + UCSYNC;     
  OLED_I2C_CTL1 = UCSSEL__SMCLK + UCSWRST;            
  OLED_I2C_BR0 = 160;                
  OLED_I2C_BR1 = 0;
  OLED_I2C_I2CSA = DISPLAY_ADDRESS_TOP; 
  
  /* release reset */
  OLED_I2C_CTL1 &= ~UCSWRST;
  
  /* enable interrupts after serial controller is released from reset */
  /* OLED_I2C_IE |= UCNACKIE + UCSTPIE + UCSTTIE + UCTXIE + UCRXIE  */
  OLED_I2C_IE |= UCNACKIE;
  
}

/* not very efficient, but simple */
void OledWrite(unsigned char Command,unsigned char* pData,unsigned char Length)
{
  
  if ( Length < 1 )
  {
    PrintString("Invalid OLED write Length\r\n");
    return;
  }
  
  //OLED_I2C_CONFIG_FOR_PERIPHERAL_USE();
  EnableSmClkUser(OLED_I2C_USER);
  
  OledBusy = 1;
  Count = Length;
  Index = 0;
  pOledData = pData;
  
  /* 
   * enable transmit interrupt and 
   * setup for write and send the start condition
   */
  OLED_I2C_IFG = 0;
  OLED_I2C_CTL1 |= UCTR + UCTXSTT;
  while(!(OLED_I2C_IFG & UCTXIFG));
  
  /* 
   * clear transmit interrupt flag, enable interrupt, 
   * send the command first (data,command,or continuation)
   */
  OLED_I2C_IFG = 0;
  OLED_I2C_IE |= UCTXIE;
  OLED_I2C_TXBUF = Command;
  while(OledBusy);
  while(OLED_I2C_CTL1 & UCTXSTP);
  
  DisableSmClkUser(OLED_I2C_USER);
  //OLED_I2C_CONFIG_FOR_SLEEP();
  
}


#define OLED_I2C_NO_INTERRUPTS ( 0 )
#define OLED_I2C_ALIFG         ( 2 )
#define OLED_I2C_NACKIFG       ( 4 )
#define OLED_I2C_STTIFG        ( 6 )
#define OLED_I2C_STPIFG        ( 8 )
#define OLED_I2C_RXIFG         ( 10 )
#define OLED_I2C_TXIFG         ( 12 )

#pragma vector = USCI_OLED_I2C_VECTOR
__interrupt void OLED_I2C_ISR(void)
{
  switch(__even_in_range(USCI_OLED_I2C_IV,12))
  {
  case OLED_I2C_NO_INTERRUPTS: 
    break;
  case OLED_I2C_ALIFG: 
    break;
  case OLED_I2C_NACKIFG:
    __no_operation();
    break; 
  case OLED_I2C_STTIFG:
    __no_operation();
    break; 
  case OLED_I2C_STPIFG: 
    break; 
  case OLED_I2C_RXIFG: 
    __no_operation();
    break;
    
  case OLED_I2C_TXIFG:
    
    if ( Count > 0 )
    {
      OLED_I2C_TXBUF = pOledData[Index++];
      Count--;
    }
    else
    {
      /* disable transmit interrupt and send stop */
      OLED_I2C_IE &= ~UCTXIE;
      OLED_I2C_CTL1 |= UCTXSTP;
      OledBusy = 0;
    }
    break; 
  default: 
    break;
  }  
  
}





