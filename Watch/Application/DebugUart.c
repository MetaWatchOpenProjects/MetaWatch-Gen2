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
/*! \file DebugUart.c
 *
 *
 * The print functions cannot be used in interrupt context 
 */
/******************************************************************************/

#include "FreeRTOS.h"
#include "semphr.h"

#include "Messages.h"

#include "hal_board_type.h"
#include "macro.h"
#include "hal_clock_control.h"
#include "hal_rtc.h"

#include "DebugUart.h"
#include "Statistics.h"

/* printing was never intended to be used in interrupt context
 * the semaphore calls need to be different in ISR context
 * don't want to wait on uart in ISR 
 *
 * output may be garbled when this is defined, but there won't be problems
 * if print is used within ISR
 */
#define DEBUG_UART_INTERRUPT_SAFE

#define TX_BUFFER_SIZE ( 255 )
static unsigned char TxBuffer[TX_BUFFER_SIZE];
static unsigned char WriteIndex;
static unsigned char ReadIndex;
static unsigned char TxCount;
static unsigned char TxBusy;

static void WriteTxBuffer(signed char * const pBuf);
static void IncrementWriteIndex(void);
static void IncrementReadIndex(void);


static unsigned char DisableSmClkCounter;

signed char ConversionString[6];

#ifndef DEBUG_UART_INTERRUPT_SAFE
static xSemaphoreHandle UartMutex;
#endif

void InitDebugUart(void)
{
  UCA3CTL1 = UCSWRST;
  
  UCA3CTL1 |= UCSSEL__SMCLK;
  
  /* from table 26-5 */
  UCA3BR0 = 8;
  UCA3MCTL = UCOS16 + UCBRS_0 + UCBRF_11;
  
  /* configure tx and rx pins (rx is not used) */
  P10SEL |= BIT4;
  P10SEL |= BIT5;
  
  UCA3CTL1 &= ~UCSWRST; 
  
  WriteIndex = 0;
  ReadIndex = 0;
  TxCount = 0;
  TxBusy = 0;
  UCA3IFG = 0;
  UCA3IE = UCTXIE;
  
  WriteTxBuffer("\r\n*******************InitDebugUart********************\r\n");
  
#ifndef DEBUG_UART_INTERRUPT_SAFE
  UartMutex = xSemaphoreCreateMutex();
  UART_MUTEX_GIVE();
#endif

}

static void WriteTxBuffer(signed char * const pBuf)
{
  unsigned char i = 0;
  unsigned char LocalCount = TxCount;
 
  /* if there isn't enough room in the buffer then characters are lost */
  while ( pBuf[i] != 0 && LocalCount < TX_BUFFER_SIZE )
  { 
    TxBuffer[WriteIndex] = pBuf[i++];
    IncrementWriteIndex();
    LocalCount++;
  }    

  /* keep a sticky bit for lost characters */
  if ( pBuf[i] != 0 )
  {
    gAppStats.DebugUartOverflow = 1;
  }
  
  /* 
   * update the count (which can be decremented in the ISR 
   * and start sending characters if the UART is currently idle 
  */
  if ( i > 0 )
  {
    ENTER_CRITICAL_REGION_QUICK();
    
    TxCount += i;
    
#if 0
    if ( TxCount > TX_BUFFER_SIZE )
    {
      while(1);  
    }
#endif
    
    if ( TxBusy == 0 )
    {
      EnableSmClkUser(BT_DEBUG_UART_USER);
      UCA3TXBUF = TxBuffer[ReadIndex];
      IncrementReadIndex();
      TxBusy = 1;
      TxCount--;  
    }
    
    LEAVE_CRITICAL_REGION_QUICK();
  }
}


/* 
 * This part has a problem turning off the SMCLK when the uart is IDLE.
 * 
 * manually turn off the clock after the last character is sent 
 *
 * count more than one interrupt because we don't know when the first one
 * will happen 
*/
void DisableUartSmClkIsr(void)
{ 
  DisableSmClkCounter++;
  if ( DisableSmClkCounter > 2 ) 
  {
    if ( TxBusy == 0 )
    { 
      DisableSmClkUser(BT_DEBUG_UART_USER);
    }
  
    /* if we are transmitting again then disable this timer */
    DisableRtcPrescaleInterruptUser(RTC_TIMER_USER_DEBUG_UART);
  
  }
  
}


#pragma vector=USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
  switch(__even_in_range(UCA3IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:break;                             // Vector 2 - RXIFG
  case 4:                                   // Vector 4 - TXIFG
    
    if ( TxCount == 0 )
    {
      UCA3IFG = 0;
      TxBusy = 0;
      
      /* start the countdown to disable SMCLK */
      DisableSmClkCounter = 0;  
      EnableRtcPrescaleInterruptUser(RTC_TIMER_USER_DEBUG_UART);
     
    }
    else
    {
      /* send a character */
      UCA3TXBUF = TxBuffer[ReadIndex];
      IncrementReadIndex();
      TxCount--;  
    }
    break;
    
  default: break;
  }
}

static void IncrementWriteIndex(void)
{
  WriteIndex++;
  if ( WriteIndex == TX_BUFFER_SIZE )
  {
    WriteIndex = 0;
  }

}

static void IncrementReadIndex(void)
{
  ReadIndex++;
  if ( ReadIndex == TX_BUFFER_SIZE )
  {
    ReadIndex = 0;
  }

}

/*! convert a 16 bit value into a string */
void ToDecimalString(unsigned int Value, signed char * pString)
{
  unsigned int bar = 10000;
  unsigned char index = 0;
  unsigned int temp = 0;
  unsigned char first = 0;
  
  for ( unsigned char i = 0; i < 5; i++ )
  {
    temp = Value / bar;
    
    if ( temp > 0 || first || i == 4)
    {
      pString[index++] = temp + '0';
      first = 1;
    }
    
    Value = Value % bar;
    
    bar = bar / 10;
    
  }
    
  pString[index] = 0;
  
}


/*! convert a 16 bit value into a hexadecimal string */
void ToHexString(unsigned int Value, signed char * pString)
{
  unsigned char parts[4];
  unsigned char first = 0;
  unsigned char index = 0;
  
  parts[3] = (0xF000 & Value) >> 12;
  parts[2] = (0x0F00 & Value) >> 8; 
  parts[1] = (0x00F0 & Value) >> 4;
  parts[0] = (0x000F & Value);
    
  for ( signed char i = 3; i > -1; i-- )
  {
    if ( parts[i] > 0 || first || i == 0 )
    {
      if ( parts[i] > 9 )
      {
        pString[index++] = parts[i] + 'A' - 10;
      }
      else
      {
        pString[index++] = parts[i] + '0';
      }
      first = 1;    
    }
    
  }
    
  pString[index] = 0;
  
}

/******************************************************************************/

#ifdef DEBUG_UART_INTERRUPT_SAFE
  #define UART_MUTEX_TAKE() { }
  #define UART_MUTEX_GIVE() { }
#else
  #define UART_MUTEX_TAKE() { xSemaphoreTake(UartMutex,portMAX_DELAY); } 
  #define UART_MUTEX_GIVE() { xSemaphoreGive(UartMutex); }
#endif


void PrintString(signed char * const pString)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString);
  UART_MUTEX_GIVE();
}

void PrintString2(signed char * const pString1,signed char * const pString2)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString1);
  WriteTxBuffer(pString2);
  UART_MUTEX_GIVE();
}

void PrintString3(signed char * const pString1,
                  signed char * const pString2,
                  signed char * const pString3)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString1);
  WriteTxBuffer(pString2);
  WriteTxBuffer(pString3);
  UART_MUTEX_GIVE();
}

void PrintDecimal(unsigned int Value)
{
  UART_MUTEX_TAKE()
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);  
  UART_MUTEX_GIVE();
}

void PrintDecimalAndNewline(unsigned int Value)
{
  UART_MUTEX_TAKE()
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");  
  UART_MUTEX_GIVE();
}

void PrintSignedDecimalAndNewline(signed int Value)
{
  UART_MUTEX_TAKE()
  if ( Value < 0 )
  {
    Value = ~Value + 1;
  }
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer("-");
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");  
  UART_MUTEX_GIVE();
}

void PrintStringAndDecimal(signed char * const pString,unsigned int Value)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString);  
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
  UART_MUTEX_GIVE();
}

void PrintStringAndSpaceAndDecimal(signed char * const pString,unsigned int Value)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString);
  WriteTxBuffer(" ");  
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
  UART_MUTEX_GIVE();
}

void PrintStringAndTwoDecimals(signed char * const pString1,
                               unsigned int Value1,
                               signed char * const pString2,
                               unsigned int Value2)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString1);  
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  
  WriteTxBuffer(pString2);  
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
  UART_MUTEX_GIVE();
}

void PrintStringSpaceAndTwoDecimals(signed char * const pString1,
                                    unsigned int Value1,
                                    unsigned int Value2)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString1);  
  WriteTxBuffer(" ");
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
  UART_MUTEX_GIVE();
}

void PrintStringSpaceAndThreeDecimals(signed char * const pString1,
                                      unsigned int Value1,
                                      unsigned int Value2,
                                      unsigned int Value3)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString1);  
  WriteTxBuffer(" ");
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  ToDecimalString(Value3,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
  UART_MUTEX_GIVE();
}

void PrintStringAndHex(signed char * const pString,unsigned int Value)
{
  UART_MUTEX_TAKE()
  WriteTxBuffer(pString);  
  ToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);   
  WriteTxBuffer("\r\n");
  UART_MUTEX_GIVE();
}

void PrintTimeStamp(void)
{
  UART_MUTEX_TAKE()
  ToHexString(RTCPS,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  UART_MUTEX_GIVE();  
}

/* callback from FreeRTOS */
void vApplicationMallocFailedHook(size_t xWantedSize)
{
  PrintStringAndDecimal("Malloc failed: ",(unsigned int)xWantedSize);
}