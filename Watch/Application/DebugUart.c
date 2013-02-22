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
#include "hal_lpm.h"
#include "hal_battery.h"

#include "DebugUart.h"
#include "Statistics.h"
#include "task.h"
#include "Utilities.h"
#include "TermMode.h"

/******************************************************************************/

const char OK[] = "- ";
const char NOK[] = "# ";
const char CR[] = "\r\n";
const char SPACE[] = " ";

tString ConversionString[6];
static unsigned char DebugEnabled;
static xSemaphoreHandle UartMutex = 0;

static void InitDebugUart(void);
static void WriteTxBuffer(const tString * pBuf);

/******************************************************************************/
/* if interrupts are disabled then return */
#define GIE_CHECK() {                         \
  if ((__get_interrupt_state() & GIE) == 0    \
      || DebugEnabled == pdFALSE)             \
  {                                           \
    return;                                   \
  }                                           \
}  

#if PRETTY_PRINT

/* getting and releasing mutex require about 10 us each */
#define GET_MUTEX()  { xSemaphoreTake(UartMutex,portMAX_DELAY); }
#define GIVE_MUTEX() { xSemaphoreGive(UartMutex); }
  
#else

#define GET_MUTEX()  { }
#define GIVE_MUTEX() { }

#endif

/******************************************************************************/

static void InitDebugUart(void)
{
  UCA3CTL1 = UCSWRST;
  
  /* set the baud rate to 115200 (from table 32-5 in slau208j) */
  UCA3CTL1 |= UCSSEL__SMCLK;
  UCA3BR0 = 145;
  UCA3MCTL = UCBRS_5 + UCBRF_0;
  
  /* configure tx and rx pins */
  P10SEL |= BIT4;
  P10SEL |= BIT5;
  
  /* clear status */
  UCA3STAT = 0;
  
  /* take uart out of reset */
  UCA3CTL1 &= ~UCSWRST; 
  
  /* prime transmitter */
  UCA3IE = 0;
  UCA3IFG = 0; //UCTXIFG;
  
  UartMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(UartMutex);
  
  WriteTxBuffer("\r\n\r\n**** DebugUart ****\r\n\r\n");
}

void EnableDebugUart(unsigned char Enable)
{
  if (!UartMutex) InitDebugUart();

  DebugEnabled = Enable;
  EnableTermMode(Enable);
}

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(DebugUartIsr,".text:_isr");
#endif

#ifndef BOOTLOADER
#pragma vector=USCI_A3_VECTOR
__interrupt void DebugUartIsr(void)
#else
__interrupt void DebugUartIsr(void)
#endif
{
  unsigned char ExitLpm = 0;
  
  // Vector 2 - RXIFG; Vector 4 - TXIFG
  switch (__even_in_range(UCA3IV, 4))
  {
  case 2: ExitLpm = TermModeIsr();
  default: break;
  }
  
  if (ExitLpm) EXIT_LPM_ISR();
}

/******************************************************************************/
static void WriteTxBuffer(const tString * pBuf)
{
  EnableSmClkUser(BT_DEBUG_UART_USER);
  
  unsigned char i = 0;

  while (pBuf[i] != 0)
  {
    /* writing buffer clears TXIFG */
    UCA3TXBUF = pBuf[i++];
    
    /* wait for transmit complete flag */
    while( (UCA3IFG & UCTXIFG) == 0 );
  }
  
  /* wait until transmit is done */
  while(UCA3STAT & UCBUSY); 
  DisableSmClkUser(BT_DEBUG_UART_USER);
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

/* 120 us - 90 us = 33 us */
void PrintCharacter(char Character)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer((tString *)&Character);
  GIVE_MUTEX();
}

void PrintString(const tString * pString)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);
  GIVE_MUTEX();
  
}

void PrintString2(const tString * pString1, const tString * pString2)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString1);
  WriteTxBuffer(pString2);
  GIVE_MUTEX();
  
}

void PrintString3(const tString *pString1, const tString *pString2, const tString *pString3)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString1);
  WriteTxBuffer(pString2);
  WriteTxBuffer(pString3);
  GIVE_MUTEX();
  
}

void PrintDecimal(unsigned int Value)
{
  GIE_CHECK();
  GET_MUTEX();
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);  
  GIVE_MUTEX();
  
}

void PrintDecimalAndNewline(unsigned int Value)
{
  GIE_CHECK();
  GET_MUTEX();
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);  
  GIVE_MUTEX();
  
}

void PrintSignedDecimalAndNewline(signed int Value)
{
  GIE_CHECK();
  GET_MUTEX();
  if ( Value < 0 )
  {
    Value = ~Value + 1;
    WriteTxBuffer("-");
  }
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);  
  GIVE_MUTEX();
  
}

void PrintStringAndDecimal(const tString * pString, unsigned int Value)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);  
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintStringAndSpaceAndDecimal(tString * const pString,unsigned int Value)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);
  WriteTxBuffer(SPACE);  
  ToDecimalString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintStringAndTwoDecimals(tString * const pString1,
                               unsigned int Value1,
                               tString * const pString2,
                               unsigned int Value2)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString1);  
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  
  WriteTxBuffer(pString2);  
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintStringAndThreeDecimals(tString * const pString1,
                                 unsigned int Value1,
                                 tString * const pString2,
                                 unsigned int Value2,
                                 tString * const pString3,
                                 unsigned int Value3)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString1);  
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  
  WriteTxBuffer(pString2);  
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  
  WriteTxBuffer(pString3);  
  ToDecimalString(Value3,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintStringSpaceAndTwoDecimals(tString * const pString1,
                                    unsigned int Value1,
                                    unsigned int Value2)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString1);  
  WriteTxBuffer(SPACE);
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}


void PrintStringSpaceAndThreeDecimals(tString * const pString1,
                                      unsigned int Value1,
                                      unsigned int Value2,
                                      unsigned int Value3)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString1);  
  WriteTxBuffer(SPACE);
  ToDecimalString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  ToDecimalString(Value2,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  ToDecimalString(Value3,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
}

void PrintStringAndHex(tString * const pString,unsigned int Value)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);  
  IntToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);   
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintHex(unsigned char Value)
{
  GIE_CHECK();
  GET_MUTEX();
  ByteToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);   
  WriteTxBuffer(SPACE);
  GIVE_MUTEX();
  
}

void PrintStringAndHexByte(tString * const pString,unsigned char Value)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);  
  ByteToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);   
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintStringAndTwoHexBytes(tString * const pString,unsigned char Value, unsigned char Value1)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);
  ByteToHexString(Value,ConversionString);
  WriteTxBuffer(ConversionString);
  ByteToHexString(Value1,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(CR);
  GIVE_MUTEX();
  
}

void PrintTimeStamp(void)
{
  GIE_CHECK();
  GET_MUTEX();
  IntToHexString(RTCPS,ConversionString);
  WriteTxBuffer(ConversionString);
  WriteTxBuffer(SPACE);
  GIVE_MUTEX();
}

/* callback from FreeRTOS 
 *
 * if the bt stack is open and closed enough then memory becomes fragmented
 * enough so that a failure occurs
 *
 */
void vApplicationMallocFailedHook(size_t xWantedSize)
{
  PrintStringAndTwoDecimals("@ Ask: ",(unsigned int)xWantedSize, " Free: ", xPortGetFreeHeapSize());
  
  __no_operation();
}
