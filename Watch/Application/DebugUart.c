//==============================================================================
//  Copyright 2011 - 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "hal_board_type.h"
#include "hal_clock_control.h"
#include "hal_rtc.h"
#include "hal_lpm.h"
#include "hal_battery.h"
#include "hal_calibration.h"
#include "hal_miscellaneous.h"
#include "hal_boot.h"
#include "DebugUart.h"
#include "Statistics.h"
#include "task.h"

#include "TermMode.h"
#include "Wrapper.h"

#define ASCII_BEGIN    32
#define ASCII_END     126

char const OK = '-';
char const NOK = '#';
char const PLUS = '+';
char const RCV = '>';
char const SND = '<';
char const CR = '\r';
char const LN = '\n';
char const SPACE = ' ';
char const ZERO = '0';
char const COLON = ':';
char const DOT = '.';
char const COMMA = ',';
char const STAR = '*';
char const HASH = '#';
char const DOLLAR = '$';
char const TILDE = '~';
char const PERCENT = '%';

static char Buffer[32];
static unsigned char DebugEnabled;
static unsigned char TimeStampEnabled = FALSE;
static xSemaphoreHandle UartMutex = 0;

static void WriteTxBuffer(const tString * pBuf);
static void WriteTimeStamp(void);
static void WriteTime(unsigned char Rtc, unsigned Separator);

/******************************************************************************/
/* if interrupts are disabled then return */
#define GIE_CHECK() {                         \
  if ((__get_interrupt_state() & GIE) == 0    \
      || DebugEnabled == FALSE) return;     \
}

#if PRETTY_PRINT

/* getting and releasing mutex require about 10 us each */
#define GET_MUTEX()  {xSemaphoreTake(UartMutex, portMAX_DELAY);}
#define GIVE_MUTEX() {xSemaphoreGive(UartMutex);}
  
#else

#define GET_MUTEX()  { }
#define GIVE_MUTEX() { }

#endif

#define WRITE_UART(_x) {UCA3TXBUF = _x; while ((UCA3IFG & UCTXIFG) == 0);}

void EnableDebugUart(unsigned char Enable)
{
  if (!UartMutex)
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
    
    DebugEnabled = TRUE;
    PrintR(); PrintC(STAR); PrintC(STAR); PrintC(STAR); PrintR();
  }
  
  DebugEnabled = Enable;
  EnableTermMode(Enable);
}

/******************************************************************************/
static void WriteTxBuffer(const tString *pBuf)
{
  EnableSmClkUser(BT_DEBUG_UART_USER);

  if (TimeStampEnabled) WriteTimeStamp();
  while (*pBuf) WRITE_UART(*pBuf++);
  
  /* wait until transmit is done */
  while (UCA3STAT & UCBUSY); 
  DisableSmClkUser(BT_DEBUG_UART_USER);  
}

static void WriteTimeStamp(void)
{
  WriteTime(RTCHOUR, COLON);
  WriteTime(RTCMIN, COLON);
  WriteTime(RTCSEC, SPACE);
}

static void WriteTime(unsigned char Rtc, unsigned Separator)
{
  WRITE_UART(BCD_H(Rtc) + ZERO);
  WRITE_UART(BCD_L(Rtc) + ZERO);
  WRITE_UART(Separator);
}

void EnableTimeStamp(void)
{
  WriteTxBuffer("--- ");
  WriteTime(RTCMON, DOT);
  WriteTime(RTCDAY, SPACE);
  WriteTxBuffer(" ---\r\n");
  
  TimeStampEnabled = TRUE;
}

/* 120 us - 90 us = 33 us */
void PrintC(char Char)
{
  GIE_CHECK();
  GET_MUTEX();
  if (Char >= ASCII_BEGIN && Char <= ASCII_END) WRITE_UART(Char);
  GIVE_MUTEX();
}

void PrintR(void)
{
  GIE_CHECK();
  GET_MUTEX();
  WRITE_UART(CR);
  WRITE_UART(LN);
  GIVE_MUTEX();
}

void PrintH(unsigned char Value)
{
  unsigned char MSB = Value >> 4;
  unsigned char LSB = Value & 0x0F;
  MSB += MSB > 9 ? 'A' - 10 : '0';
  LSB += LSB > 9 ? 'A' - 10 : '0';

  GIE_CHECK();
  GET_MUTEX();
  WRITE_UART(MSB);
  WRITE_UART(LSB);
  WRITE_UART(SPACE);
  GIVE_MUTEX();
}

void PrintS(const char *pString)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);
  WRITE_UART(CR);
  WRITE_UART(LN);
  GIVE_MUTEX();
}

void PrintW(const char *pString)
{
  GIE_CHECK();
  GET_MUTEX();
  WriteTxBuffer(pString);
  GIVE_MUTEX();
}

void PrintQ(unsigned char const *pHex, unsigned char const Len)
{
  unsigned char i;
  for (i = 0; i < Len; ++i) PrintH(*pHex++);
  PrintR();
}

void PrintF(const char *pFormat, ...)
{
  va_list args;

  va_start(args, pFormat);
  vSprintF(Buffer, pFormat, args);
  va_end(args);

  PrintS(Buffer);
}

void PrintE(const char *pFormat, ...)
{
  va_list args;

  va_start(args, pFormat);
  vSprintF(Buffer, pFormat, args);
  va_end(args);

  PrintW(Buffer);
}

/* callback from FreeRTOS
 *
 * if the bt stack is open and closed enough then memory becomes fragmented
 * enough so that a failure occurs
 *
 */
void vApplicationMallocFailedHook(void)
{
//  PrintE("@");
  __no_operation();
}

void vApplicationFreeFailedHook(unsigned char * pBuffer)
{
  PrintF("@F:%04X", pBuffer);
}

void vAssertCalled(void)
{
  PrintF("@ Assert");
  __no_operation();
}

extern xQueueHandle QueueHandles[];

void CheckStackAndQueueUsage(unsigned char Index)
{
  portBASE_TYPE Water = uxTaskGetStackHighWaterMark(NULL);
  portBASE_TYPE QueLen = QueueHandles[Index]->uxMessagesWaiting;

  if (Water < 10 || QueLen > 8)
    PrintF("~%c S:%d Q:%u", Index == DISPLAY_QINDEX ? 'D' : 'W', Water, QueLen);
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask, char *TaskName)
{
  /* try to print task name */
  PrintF("@ Stack Oflw:%s", TaskName);
  SoftwareReset();
}

void WhoAmI(void)
{
  PrintS(BT_LOCAL_NAME);
  PrintF("Msp430 Rev:%c HwVer:%d", GetMsp430HardwareRevision(), HardwareVersion());
  PrintF("BoardConfig: %d", GetBoardConfiguration());
  PrintF("Calibration: %d", ValidCalibration());
  PrintF("Errata: %d", Errata());
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
