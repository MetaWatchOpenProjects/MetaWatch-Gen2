
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
/*! \file hal_accelerometer.c
 *
 * The accelerometer hardware holds the SCL line low when data has not been
 * read from the receive register.
 */
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "hal_board_type.h"
#include "hal_accelerometer.h"
#include "hal_clock_control.h"
#include "macro.h"

/******************************************************************************/

static unsigned char AccelerometerBusy;
static unsigned char Count;
static unsigned char Index;
static unsigned char* pAccelerometerData;

static xSemaphoreHandle AccelerometerMutex;

  
/******************************************************************************/

/* the accelerometer can run at 400 kHz */
void InitAccelerometerPeripheral(void)
{
  /* enable reset before configuration */
  ACCELEROMETER_CTL1 |= UCSWRST;
  
  /* configure as master using smclk / 50 = 320 kHz */
  ACCELEROMETER_CTL0 = UCMST + UCMODE_3 + UCSYNC;     
  ACCELEROMETER_CTL1 = UCSSEL__SMCLK + UCSWRST;            
  ACCELEROMETER_BR0 = 50;                
  ACCELEROMETER_BR1 = 0;
  ACCELEROMETER_I2CSA = KIONIX_DEVICE_ADDRESS; 
  
  /* release reset */
  ACCELEROMETER_CTL1 &= ~UCSWRST;
  
  /* enable interrupts after serial controller is released from reset */
  /* ACCELEROMETER_IE |= UCNACKIE + UCSTPIE + UCSTTIE + UCTXIE + UCRXIE  */
  ACCELEROMETER_IE |= UCNACKIE;

  AccelerometerMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(AccelerometerMutex);
  
}

/* not very efficient, but simple */
void AccelerometerWrite(unsigned char RegisterAddress,
                        unsigned char* pData,
                        unsigned char Length)
{
  
  EnableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreTake(AccelerometerMutex,portMAX_DELAY);
  
  while(UCB1STAT & UCBBUSY);
  
  AccelerometerBusy = 1;
  Count = Length;
  Index = 0;
  pAccelerometerData = pData;
  
  /* 
   * enable transmit interrupt and 
   * setup for write and send the start condition
   */
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_CTL1 |= UCTR + UCTXSTT;
  while(!(ACCELEROMETER_IFG & UCTXIFG));
  
  /* 
   * clear transmit interrupt flag, enable interrupt, 
   * send the register address (this could be part of the data) 
   */
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_IE |= UCTXIE;
  ACCELEROMETER_TXBUF = RegisterAddress;
  while(AccelerometerBusy);
  while(ACCELEROMETER_CTL1 & UCTXSTP);
  
  DisableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreGive(AccelerometerMutex);
  
}

void AccelerometerRead(unsigned char RegisterAddress,
                       unsigned char* pData,
                       unsigned char Length)
{
  /* short circuit */
  if ( Length == 0 )
  {
    return;
  }
  
  EnableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreTake(AccelerometerMutex,portMAX_DELAY);
  
  while(UCB1STAT & UCBBUSY);
  
  AccelerometerBusy = 1;
  Count = Length;
  Index = 0;
  pAccelerometerData = pData;
  
  /* transmit address */
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_CTL1 |= UCTR + UCTXSTT;
  while(!(ACCELEROMETER_IFG & UCTXIFG));
  
  /* write register address */
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_TXBUF = RegisterAddress;
  while(!(ACCELEROMETER_IFG & UCTXIFG));
  
  /* send a repeated start (same slave address now it is a read command) */
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_CTL1 &= ~UCTR;
  
  /* for a read of a single byte stop must be sent while the byte is being 
   * received.  in this case don't enable the interrupt
   */
  if ( Length == 1 )
  {
    ACCELEROMETER_CTL1 |= UCTXSTT;
  
    /* GZ: Wait for end of Sr condition and SAD+R acknowledged by slave. */
    while (ACCELEROMETER_CTL1 & UCTXSTT);
    
    /* GZ: When the first data byte is received: 
     * Send NACK, generate P condition.
     */
    ACCELEROMETER_CTL1 |= UCTXSTP;
    
    /* wait for the end of the cycle and read the data */
    while(ACCELEROMETER_CTL1 & UCTXSTP);
    pAccelerometerData[0] = ACCELEROMETER_RXBUF;
    AccelerometerBusy = 0;
    Count = 0;
  }
  else
  {
    ACCELEROMETER_IE |= UCRXIE;
    ACCELEROMETER_CTL1 |= UCTXSTT;
    while(AccelerometerBusy);
      
  }

  DisableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreGive(AccelerometerMutex);

}

// length = data length
// type = read or write
// options = register address
// payload is data

#define ACCELEROMETER_NO_INTERRUPTS ( 0 )
#define ACCELEROMETER_ALIFG         ( 2 )
#define ACCELEROMETER_NACKIFG       ( 4 )
#define ACCELEROMETER_STTIFG        ( 6 )
#define ACCELEROMETER_STPIFG        ( 8 )
#define ACCELEROMETER_RXIFG         ( 10 )
#define ACCELEROMETER_TXIFG         ( 12 )

#pragma vector = USCI_ACCELEROMETER_VECTOR
__interrupt void ACCERLEROMETER_ISR(void)
{
  switch(__even_in_range(USCI_ACCELEROMETER_IV,12))
  {
  case ACCELEROMETER_NO_INTERRUPTS: 
    break;
  case ACCELEROMETER_ALIFG: 
    break;
  case ACCELEROMETER_NACKIFG:
    __no_operation();
    break; 
  case ACCELEROMETER_STTIFG:
    __no_operation();
    break; 
  case ACCELEROMETER_STPIFG: 
    break; 
  
  case ACCELEROMETER_RXIFG:
 
    if (Count > 0)
    { 
      pAccelerometerData[Index++] = ACCELEROMETER_RXBUF;
      Count--;
      
      if ( Count == 1 )
      { 
        /* All but one byte received. Send NACK, generate P condition. */
        ACCELEROMETER_CTL1 |= UCTXSTP;
      }
      else if ( Count == 0 )
      { 
        /* Last byte received */
        ACCELEROMETER_IE &= ~UCRXIE;
        AccelerometerBusy = 0;
      }
    }
    break;
    
  case ACCELEROMETER_TXIFG:
    
    if ( Count > 0 )
    {
      ACCELEROMETER_TXBUF = pAccelerometerData[Index++];
      Count--;
    }
    else
    {
      /* disable transmit interrupt and send stop */
      ACCELEROMETER_IE &= ~UCTXIE;
      ACCELEROMETER_CTL1 |= UCTXSTP;
      AccelerometerBusy = 0;
    }
    break; 
  default: 
    break;
  }  
}





