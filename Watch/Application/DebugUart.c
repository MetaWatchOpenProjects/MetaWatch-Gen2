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
#include "hal_clock_control.h"
#include "hal_rtc.h"

#include "DebugUart.h"
#include "Statistics.h"

#define TX_BUFFER_SIZE ( 255 )
static unsigned char TxBuffer[TX_BUFFER_SIZE];
static unsigned char WriteIndex;
static unsigned char ReadIndex;
static unsigned char TxCount;
static unsigned char TxBusy;

static void WriteTxBuffer(tString * const pBuf);
static void IncrementWriteIndex(void);
static void IncrementReadIndex(void);


static unsigned char DisableSmClkCounter;

tString ConversionString[6];

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
  
}

static void WriteTxBuffer(tString * const pBuf)
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
    portENTER_CRITICAL();
    
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
    
    portEXIT_CRITICAL();
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

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(USCI_A3_ISR,".text:_isr");
#endif

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
void ToDecimalString(unsigned int Value, tString * pString)
{
  unsigned int bar = 10000;
  unsigned char index = 0;
  unsigned int temp = 0;
  unsigned char first = 0;
  
  unsigned char i;
  for ( i = 0; i < 5; i++ )
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
void IntToHexString(unsigned int Value, tString * pString)
{
  unsigned char parts[4];
  unsigned char index = 0;
  
  parts[3] = (0xF000 & Value) >> 12;
  parts[2] = (0x0F00 & Value) >> 8; 
  parts[1] = (0x00F0 & Value) >> 4;
  parts[0] = (0x000F & Value);
    
  signed char i = 0;
  for ( i = 3; i > -1; i-- )
  {
      if ( parts[i] > 9 )
      {
        pString[index++] = parts[i] + 'A' - 10;
      }
      else
      {
        pString[index++] = parts[i] + '0';
      }
    
    }
    
  /* null */
  pString[index] = 0;
  
  }
    
/*! convert a 8 bit value into a hexadecimal string */
void ByteToHexString(unsigned char Value, tString * pString)
{
  unsigned char parts[2];
  unsigned char index = 0;
  
  parts[1] = (0x00F0 & Value) >> 4;
  parts[0] = (0x000F & Value);
  
  signed char i = 0;
  for ( i = 1; i > -1; i-- )
  {
    if ( parts[i] > 9 )
    {
      pString[index++] = parts[i] + 'A' - 10;
    }
    else
    {
      pString[index++] = parts[i] + '0';
}

  }

  /* null */
  pString[index] = 0;

}
/******************************************************************************/
void PrintCharacter(char Character)
{
  WriteTxBuffer((tString *)&Character);  
}

void PrintString(tString * const pString)
{
  WriteTxBuffer(pString);
}

void PrintString2(tString * const pString1,tString * const pString2)
{
  WriteTxBuffer(pString1);
  WriteTxBuffer(pString2);
}

void PrintString3(tString * const pString1,
                  tString * const pString2,
                  tString * const pString3)
{
  WriteTxBuffer(pString1);
  WriteTxBuffer(pString2);
  WriteTxBuffer(pString3);
}

void PrintDecimal(unsigned int Value)
{
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);  
}

void PrintDecimalAndNewline(unsigned int Value)
{
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");  
}

void PrintSignedDecimalAndNewline(signed int Value)
{
  if ( Value < 0 )
  {
    Value = ~Value + 1;
    WriteTxBuffer("-");
  }
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");  
}

void PrintStringAndDecimal(tString * const pString,unsigned int Value)
{
  WriteTxBuffer(pString);  
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
}

void PrintStringAndSpaceAndDecimal(tString * const pString,unsigned int Value)
{
  WriteTxBuffer(pString);
  WriteTxBuffer(" ");  
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
}

void PrintStringAndThreeDecimals(tString * const pString1,
                                 unsigned int Value1,
                                 tString * const pString2,
                                 unsigned int Value2,
                                 tString * const pString3,
                                 unsigned int Value3)
{
  WriteTxBuffer(pString1);  
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  
  WriteTxBuffer(pString2);  
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  
  WriteTxBuffer(pString3);  
  ToDecimalString(Value3,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
}

void PrintStringSpaceAndTwoDecimals(tString * const pString1,
                                    unsigned int Value1,
                                    unsigned int Value2)
{
  WriteTxBuffer(pString1);  
  WriteTxBuffer(" ");
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer("\r\n");
}


void PrintStringSpaceAndThreeDecimals(tString * const pString1,
                                      unsigned int Value1,
                                      unsigned int Value2,
                                      unsigned int Value3)
{
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
}

void PrintStringAndHex(tString * const pString,unsigned int Value)
{
  WriteTxBuffer(pString);  
  IntToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);   
  WriteTxBuffer("\r\n");
}

void PrintStringAndHexByte(tString * const pString,unsigned char Value)
{
  WriteTxBuffer(pString);  
  ByteToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);   
  WriteTxBuffer("\r\n");
}


void PrintTimeStamp(void)
{
  IntToHexString(RTCPS,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(" ");
}

/* callback from FreeRTOS 
 *
 * if the bt stack is open and closed enough then memory becomes fragmented
 * enough so that a failure occurs
 *
 */
void vApplicationMallocFailedHook(size_t xWantedSize)
{
  PrintStringAndDecimal("Malloc failed: ",(unsigned int)xWantedSize);
  
  int FreeBytes = xPortGetFreeHeapSize();
  PrintStringAndDecimal("FreeBytes: ",FreeBytes);
  
  __no_operation();
}
