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

#include "FreeRTOS.h"
#include "hal_board_type.h"
#include "hal_clock_control.h"
#include "Messages.h"
#include "DebugUart.h"
#include "LcdDriver.h"
#include "LcdDisplay.h"
#include "Property.h"

/******************************************************************************/

// SMCLK = 16MHz -> divide by 16 to get 1 MHz SPI clock
#define SPI_PRESCALE_L      0x10
#define SPI_PRESCALE_H      0x00

#define LCD_STATIC_CMD      0x00
#define LCD_WRITE_CMD       0x01     
#define LCD_CLEAR_CMD       0x04     

/* the first LCD line is one, NOT zero */
#define FIRST_LCD_LINE_OFFSET  1

/* errata - DMA variables cannot be function scope */
static unsigned char LcdDmaBusy = 0;

static void Write(unsigned char Cmd, unsigned char *pBuffer, unsigned int Size);

void LcdPeripheralInit(void)
{
  ENABLE_LCD_POWER();

  CONFIG_LCD_PINS();
  
  /*
   * configure the MSP430 SPI peripheral for use with Lcd
   */

  /* Put state machine in reset while it is configured */
  LCD_SPI_UCBxCTL1 |= UCSWRST;

  /* 
   * 3-pin, 8-bit SPI master, Clock polarity low
   * Clock phase set, least sig bit first
   * SMCLK is the clock source
   * set the clock prescaler
  */
  LCD_SPI_UCBxCTL0 |= UCMST+ UCCKPH + UCSYNC;      
                                                   
  LCD_SPI_UCBxCTL1 |= UCSSEL_2;                    
  LCD_SPI_UCBxBR0 = SPI_PRESCALE_L;               
  LCD_SPI_UCBxBR1 = SPI_PRESCALE_H;               

  /* remove reset */
  LCD_SPI_UCBxCTL1 &= ~UCSWRST;
}

/*! Writes a single line to the LCD */
void WriteToLcd(tLcdLine *pData, unsigned char LineNum)
{
  /* flip bits */
  if (!GetProperty(PROP_INVERT_DISPLAY))
  {
  	unsigned char i, k;
    for (i = 0; i < LineNum; ++i)
    {
      pData[i].Row += FIRST_LCD_LINE_OFFSET;
      for (k = 0; k < BYTES_PER_LINE; ++k)
        pData[i].Data[k] = ~(pData[i].Data[k]);
    }
  }

  Write(LCD_WRITE_CMD, (unsigned char *)pData, sizeof(tLcdLine) * LineNum);
}

static void Write(unsigned char Cmd, unsigned char *pBuffer, unsigned int Size)
{  
  EnableSmClkUser(LCD_USER);
  LCD_CS_ASSERT();
  
#if LCD_DMA
  
  LcdDmaBusy = 1;
  
  /* send the lcd write command before starting the dma */
  LCD_SPI_UCBxTXBUF = Cmd;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  
  /* USCIB0 TXIFG is the DMA trigger
   * DMACTL1 controls dma2 and [dma3]
   */
  DMACTL1 = DMA2TSEL_19;    
    
  __data16_write_addr((unsigned short) &DMA2SA, (unsigned long) pBuffer);
  __data16_write_addr((unsigned short) &DMA2DA, (unsigned long) &LCD_SPI_UCBxTXBUF);
            
  DMA2SZ = Size;
  
  /* 
   * single transfer, increment source address, source byte and dest byte,
   * level sensitive, enable interrupt, clear interrupt flag
   */
  DMA2CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMALEVEL + DMAIE;  
  
  /* start the transfer */
  DMA2CTL |= DMAEN;
  
  while(LcdDmaBusy);

#else

  /* send the lcd write command */
  LCD_SPI_UCBxTXBUF = Cmd;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  
  for (unsigned int i = 0; i < Size; ++i)
  {
    LCD_SPI_UCBxTXBUF = pBuffer[i];
    while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  }
    
#endif
  
  /* add one more dummy byte at the end */
  LCD_SPI_UCBxTXBUF = 0x00;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  
  /* wait for shift to complete ( ~3 us ) */
  while((LCD_SPI_UCBxSTAT & 0x01) != 0);
        
  LCD_SPI_UCBxTXBUF = LCD_STATIC_CMD;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));

  LCD_SPI_UCBxTXBUF = 0x00;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));

  /* now the chip select can be deasserted */
  LCD_CS_DEASSERT();
  DisableSmClkUser(LCD_USER);
}

void ClearLcd(void)
{
  EnableSmClkUser(LCD_USER);
  LCD_CS_ASSERT();

  LCD_SPI_UCBxTXBUF = LCD_CLEAR_CMD;
  while (!(LCD_SPI_UCBxIFG & UCTXIFG));

  LCD_SPI_UCBxTXBUF = 0x00;
  while (!(LCD_SPI_UCBxIFG & UCTXIFG));

  /* wait for shift to complete ( ~3 us ) */
  while((LCD_SPI_UCBxSTAT & 0x01) != 0);

  LCD_CS_DEASSERT();
  DisableSmClkUser(LCD_USER);
}

void LcdDmaIsr(void)
{
  LcdDmaBusy = 0;
}
