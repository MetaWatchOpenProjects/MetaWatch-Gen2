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

/******************************************************************************/

static unsigned char AccelerometerBusy;
static unsigned char LengthCount;
static unsigned char Index;
static unsigned char* pAccelerometerData;
  
static xSemaphoreHandle AccelerometerMutex;

  
/******************************************************************************/

/* the accelerometer can run at 400 kHz */
void InitAccelerometerPeripheral(void)
{
  /* enable reset before configuration */
  ACCELEROMETER_CTL1 |= UCSWRST;
  
  /* configure as master using smclk / 40 = 399.5 kHz */
  ACCELEROMETER_CTL0 = UCMST + UCMODE_3 + UCSYNC;     
  ACCELEROMETER_CTL1 = UCSSEL__SMCLK + UCSWRST;
  ACCELEROMETER_BR0 = 42;
    
  ACCELEROMETER_BR1 = 0;
  ACCELEROMETER_I2CSA = KIONIX_DEVICE_ADDRESS; 
  
  /* release reset */
  ACCELEROMETER_CTL1 &= ~UCSWRST;
  
  AccelerometerMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(AccelerometerMutex);
  
}

/* not very efficient, but simple */
void AccelerometerWrite(unsigned char RegisterAddress,
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
  LengthCount = Length;
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
   * send the register address
   */
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_IE |= UCTXIE;
  ACCELEROMETER_TXBUF = RegisterAddress;
  while(AccelerometerBusy);
  while(ACCELEROMETER_CTL1 & UCTXSTP);
  
  DisableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreGive(AccelerometerMutex);
  
}

void AccelerometerReadSingle(unsigned char RegisterAddress,
                             unsigned char* pData)
{
#if 0
  /* short circuit */
  if ( Length == 0 )
  {
    return;
  }
#endif
  
  EnableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreTake(AccelerometerMutex,portMAX_DELAY);
  
  /* wait for bus to be free */
  while(UCB1STAT & UCBBUSY);
  
  AccelerometerBusy = 1;
  LengthCount = 1;
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
  
  
  /* send a repeated start (same slave address now it is a read command) 
   * read possible extra character from rxbuffer
   */
  ACCELEROMETER_RXBUF;
  ACCELEROMETER_IFG = 0;
  ACCELEROMETER_IE |= UCRXIE;
  ACCELEROMETER_CTL1 &= ~UCTR;
    
  /* for a read of a single byte the stop must be sent while the byte is being 
   * received. If this is interrupted an extra byte may be read.
   * however, it will be discarded during the next read
   */
  if ( LengthCount == 1 )
  {
    /* errata usci30: prevent interruption of sending stop 
     * so that only one byte is read
     * this requires 62 us @ 320 kHz, 51 @ 400 kHz
     */
    portENTER_CRITICAL();
  
    ACCELEROMETER_CTL1 |= UCTXSTT;
  
    while(ACCELEROMETER_CTL1 & UCTXSTT);

    ACCELEROMETER_CTL1 |= UCTXSTP;
  
    portEXIT_CRITICAL();
  }
  else
  {
    ACCELEROMETER_CTL1 |= UCTXSTT;
  }
  
  /* wait until all data has been received and the stop bit has been sent */
  while(AccelerometerBusy);
  while(ACCELEROMETER_CTL1 & UCTXSTP);
  Index = 0;
  pAccelerometerData = NULL;
  
  DisableSmClkUser(ACCELEROMETER_USER);
  xSemaphoreGive(AccelerometerMutex);
}

/* errata usci30: only perform single reads 
 * second solution: use DMA
 */
void AccelerometerRead(unsigned char RegisterAddress,
                       unsigned char* pData,
                       unsigned char Length)
{
  unsigned char i;
  for ( i = 0; i < Length; i++ )
  {
    AccelerometerReadSingle(RegisterAddress+i,pData+i);
  }  
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
  
    if (LengthCount > 0)
    {
      
#if 0
      /* 
       * Errata USCI30
       * SCL must be low for 3 bit times
       * this solution only applies to 320 kHz I2C clock
       * this causes bluetooth characters to get lost
       */
      unsigned char workaround_count = 10;
      
      while ( (workaround_count--) && ( LengthCount > 1 ) )
      {
        // If SCL is not low for 3 bit times, restart counter
        if (!(UCB1STAT & UCSCLLOW))
        {
          workaround_count = 10;
        }
      }
#endif
      
      pAccelerometerData[Index++] = ACCELEROMETER_RXBUF;
      LengthCount--;
      
      if ( LengthCount == 1 )
      {
        /* All but one byte received. Send stop */
        ACCELEROMETER_CTL1 |= UCTXSTP;
      }
        else if ( LengthCount == 0 )
      {
        /* Last byte received; disable rx interrupt */
        ACCELEROMETER_IE &= ~UCRXIE;
        AccelerometerBusy = 0;
      }
    }
    break;
    
  case ACCELEROMETER_TXIFG:
    
    if ( LengthCount > 0 )
    {
      ACCELEROMETER_TXBUF = pAccelerometerData[Index++];
      LengthCount--;
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





