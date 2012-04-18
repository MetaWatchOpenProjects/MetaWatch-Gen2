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
/*! \file SerialRam.c
*
*/
/******************************************************************************/


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "hal_board_type.h"
#include "hal_clock_control.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "SerialRam.h"
#include "LcdDriver.h"
#include "LcdDisplay.h"
#include "Utilities.h"

/******************************************************************************/

#define SPI_READ  ( 0x03 )
#define SPI_WRITE ( 0x02 )
/* write and read status register */
#define SPI_RDSR  ( 0x05 )
#define SPI_WRSR  ( 0x01 )

/* the 256Kbit part does not have a 1 in bit position 1 */
#define DEFAULT_SR_VALUE ( 0x02 )
#define FINAL_SR_VALUE   ( 0x43 )

#define SEQUENTIAL_MODE_COMMAND ( 0x41 )

/* errata - DMA variables cannot be function scope */
static unsigned char DummyData = 0x00;
static unsigned char ReadData = 0x00;
static unsigned char DmaBusy  = 0;

static tMessage OutgoingMsg;

static unsigned char pWorkingBuffer[16];

static tLcdMessagePayload WriteLineBuffer;

/******************************************************************************/

/* 96 * 12 */
#define BYTES_PER_SCREEN ( (unsigned int)1152 )
#define BYTES_PER_LINE   ( 12 )

/* command and two addres bytes */
#define SPI_OVERHEAD ( 3 )

/******************************************************************************/

#define FREE_BUFFER        ( 1 )
#define DO_NOT_FREE_BUFFER ( 0 )

static void ClearMemory(void);
static void SetupCycle(unsigned int Address,unsigned char CycleType);
static void WriteBlockToSram(unsigned char* pData,unsigned int Size);
static void ReadBlock(unsigned char* pWriteData,unsigned char* pReadData);
static void WaitForDmaEnd(void);

/******************************************************************************/
unsigned char GetStartingRow(unsigned char MsgOptions);
unsigned char GetBufferIndex(unsigned char Mode, unsigned char Type);
unsigned int GetBufferAddress(unsigned Index);
void SetBufferStatus(unsigned char Index, unsigned char Status);

// Status of buffers (2 for each of 4 display modes): 
// 0: clean; 1: buffer is read; 2: buffer is writing; 3: buffer is written
unsigned char BufferStatus[NUMBER_OF_MODES][NUMBER_OF_BUFFERS];

// Rules for deciding which of the two buffers to read and write
unsigned char BufSelRule[2][3] =
  {{BUFFER_WRITTEN, BUFFER_WRITING},
   {BUFFER_WRITING, BUFFER_CLEAN, BUFFER_WRITTEN}
  };

/******************************************************************************/

void SerialRamInit(void)
{
  /*
   * configure the MSP430 SPI peripheral
   */
  
  /* assert reset when configuring */
  UCA0CTL1 = UCSWRST;  
 
  EnableSmClkUser(SERIAL_RAM_USER);
  
  SRAM_SCLK_PSEL |= SRAM_SCLK_PIN;
  SRAM_SOMI_PSEL |= SRAM_SOMI_PIN;
  SRAM_SIMO_PSEL |= SRAM_SIMO_PIN;
  
  /* 3 pin SPI master, MSB first, clock inactive when low, phase is 1 */
  UCA0CTL0 |= UCMST+UCMSB+UCCKPH+UCSYNC;    
                      
  UCA0CTL1 |= UCSSEL__SMCLK;

  /* spi clock of 8 MHz */
  UCA0BR0 = 0x02;               
  UCA0BR1 = 0x00;               

  /* release reset and wait for SPI to initialize */
  UCA0CTL1 &= ~UCSWRST;
  TaskDelayLpmDisable();
  vTaskDelay(10);
  TaskDelayLpmEnable();
  
  /* 
   * Read the status register
   */
  SRAM_CSN_ASSERT();
  
  while (!(UCA0IFG&UCTXIFG));               
  
  /* writing automatically clears flag */
  UCA0TXBUF = SPI_RDSR;
  while (!(UCA0IFG&UCTXIFG));
  UCA0TXBUF = DummyData;
  while (!(UCA0IFG&UCTXIFG));
  WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE();
  ReadData = UCA0RXBUF;
  
  /* make sure correct value is read from the part */
  if (   ( ReadData != DEFAULT_SR_VALUE )
      && ( ReadData != FINAL_SR_VALUE ) )
  {
    PrintString("Serial RAM initialization failure (a) \r\n");
  }
  
  SRAM_CSN_DEASSERT();
  
  
  /*
   * put the part into sequential mode
   */
  SRAM_CSN_ASSERT();
  UCA0TXBUF = SPI_WRSR;
  while (!(UCA0IFG&UCTXIFG));
  UCA0TXBUF = SEQUENTIAL_MODE_COMMAND;
  while (!(UCA0IFG&UCTXIFG));
  WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE();
  
  SRAM_CSN_DEASSERT();
  
  /**/
  SRAM_CSN_ASSERT();
  UCA0TXBUF = SPI_RDSR;
  while (!(UCA0IFG&UCTXIFG));
  
  UCA0TXBUF = DummyData;
  while (!(UCA0IFG&UCTXIFG));
  WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE();
  ReadData = UCA0RXBUF;
  
  /* make sure correct value is read from the part */
  if ( ReadData != FINAL_SR_VALUE )
  {
    PrintString("Serial RAM initialization failure (b) \r\n"); 
  }
  SRAM_CSN_DEASSERT();
  
  /* now use the DMA to clear the entire serial ram */
  ClearMemory();
}

/* see tSerialRamMsgPayload for the payload formatting 
 * messages were designed so that the system could operate without
 * copying bytes
 */
void WriteBufferHandler(tMessage* pMsg)
{
  /* 
   * save the parameters that are going to get written over 
   */
  unsigned char Mode = pMsg->Options & BUFFER_SELECT_MASK;
  
  /* map the payload */
  tSerialRamPayload* pSerialRamPayload = (tSerialRamPayload*)pMsg->pBuffer;
  
  unsigned char RowA = pSerialRamPayload->RowSelectA;
  unsigned char RowB = pSerialRamPayload->RowSelectB;
  
  /* use the options to determine what buffer to write then
   * determine what buffer is active/draw 
   * get the buffer address
   * then add in the row number for the absolute address
   */
  unsigned char Index = GetBufferIndex(Mode, BUFFER_TYPE_WRITE);
  PrintStringAndHexByte("----->> MY: WriteBufferHandler: index: ", Index);
  PrintStringAndTwoHexBytes("   Buffers []: ",
		  BufferStatus[Mode][0], BufferStatus[Mode][1]);
  
  unsigned int BufferAddress = GetBufferAddress(Index);
  unsigned int AbsoluteAddress = BufferAddress + (RowA*BYTES_PER_LINE);
  
  pWorkingBuffer[0] = SPI_WRITE;
  pWorkingBuffer[1] = (unsigned char)(AbsoluteAddress >> 8);
  pWorkingBuffer[2] = (unsigned char) AbsoluteAddress; 
  
  /* copy the data */
  unsigned char i;
  for( i = 0; i < BYTES_PER_LINE; i++)
  {
    pWorkingBuffer[3+i] = pSerialRamPayload->pLineA[i];
  }
  
  WriteBlockToSram(pWorkingBuffer,15);
  
  /* if the bit is one then only draw one line */
  if ( (pMsg->Options & WRITE_BUFFER_ONE_LINE_MASK) == 0 )
  {
    /* calculate address for second row */
    AbsoluteAddress = BufferAddress + (RowB*BYTES_PER_LINE);
    
    pWorkingBuffer[0] = SPI_WRITE;
    pWorkingBuffer[1] = (unsigned char)(AbsoluteAddress >> 8);
    pWorkingBuffer[2] = (unsigned char) AbsoluteAddress; 
  
    /* copy the data ... */
    unsigned char i;
    for(i = 0; i < BYTES_PER_LINE; i++)
    {
      pWorkingBuffer[3+i] = pSerialRamPayload->pLineB[i];
    }
  
    /* point to first character to dma */
    WriteBlockToSram(pWorkingBuffer,15);
  }
  
  SetBufferStatus(Index, (pMsg->Options & BUFFER_WRITTEN_MASK) ?
                  BUFFER_WRITTEN : BUFFER_WRITING);
  PrintString("   MY: WriteBuffer done.\r\n");
}

/* use DMA to write a block of data to the serial ram */
static void WriteBlockToSram(unsigned char* pData,unsigned int Size)
{  
  DmaBusy = 1;
  SRAM_CSN_ASSERT();
  
  /* USCIA0 TXIFG is the DMA trigger */
  DMACTL0 = DMA0TSEL_17;                   
  
  __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) pData);
                                            
  __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCA0TXBUF);
             
  DMA0SZ = Size;
  
  /* 
   * single transfer, increment source address, source byte and dest byte,
   * level sensitive, enable interrupt, clear interrupt flag
   */
  DMA0CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMALEVEL + DMAIE;  
  
  /* start the transfer */
  DMA0CTL |= DMAEN;
  
  WaitForDmaEnd();

}

static void ClearBufferInSram(unsigned int Address,
                              unsigned char Data,
                              unsigned int Size)
{  
  DmaBusy = 1;
  SetupCycle(Address,SPI_WRITE);
  
  /* USCIA0 TXIFG is the DMA trigger */
  DMACTL0 = DMA0TSEL_17;                   
  
  __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) &Data);
                                            
  __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCA0TXBUF);
             
  DMA0SZ = Size;
  
  /* 
   * single transfer, DON'T increment source address, source byte and dest byte,
   * level sensitive, enable interrupt, clear interrupt flag
   */
  DMA0CTL = DMADT_0 + DMASBDB + DMALEVEL + DMAIE;  
  
  /* start the transfer */
  DMA0CTL |= DMAEN;
  
  WaitForDmaEnd();
  
}

static void SetupCycle(unsigned int Address,unsigned char CycleType)
{
  EnableSmClkUser(SERIAL_RAM_USER);
  SRAM_CSN_ASSERT();  
  
  UCA0TXBUF = CycleType;
  while (!(UCA0IFG&UCTXIFG));  
  while (!(UCA0IFG&UCRXIFG));
  ReadData = UCA0RXBUF;
  
  /* write 16 bit address (only 13 bits are used) */
  UCA0TXBUF = (unsigned char) ( Address >> 8 );
  while (!(UCA0IFG&UCTXIFG));  
  while (!(UCA0IFG&UCRXIFG));
  ReadData = UCA0RXBUF;
  
  /* 
   * wait until read is done (transmit must be done) 
   * then clear read flag so it is ready for the DMA
  */
  UCA0TXBUF = (unsigned char) Address;
  while (!(UCA0IFG&UCTXIFG));  
  while (!(UCA0IFG&UCRXIFG));
  ReadData = UCA0RXBUF;
  
}

/* 
 * Wait until the DMA interrupt clears the busy flag
 *
 */
static void WaitForDmaEnd(void)
{
  while(DmaBusy);
       
  SRAM_CSN_DEASSERT();
  DisableSmClkUser(SERIAL_RAM_USER);
  
}

static void ReadBlock(unsigned char* pWriteData,unsigned char* pReadData)
{  
  DmaBusy = 1;
  SRAM_CSN_ASSERT();
  
  /*
   * SPI has to write bytes to receive bytes so 
   *
   * two DMA channels are used
   * 
   * read requires 16 total bytes because the shift in of the read data
   * lags the transmit data by one byte
   */
  
  /* USCIA0 TXIFG is the DMA trigger for DMA0 and RXIFG is the DMA trigger
   * for dma1 (DMACTL0 controls both)
   */
  DMACTL0 = DMA1TSEL_16 | DMA0TSEL_17;                   
  __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) pWriteData);                                  
  __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCA0TXBUF);
  DMA0SZ = 15+1;
  /* don't enable interrupt for transmit dma done(channel 0) 
   * increment the source address because the message contains the address
   * the other bytes are don't care
   */
  DMA0CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMALEVEL;  
  
  /* receive data is source for dma 1 */
  __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) &UCA0RXBUF);                                  
  __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) pReadData);
  DMA1SZ = 15+1;
  /* increment destination address */
  DMA1CTL = DMADT_0 + DMADSTINCR_3 + DMASBDB + DMALEVEL + DMAIE;  

  /* start the transfer */
  DMA1CTL |= DMAEN;
  DMA0CTL |= DMAEN;
  
}


static void ClearMemory(void)
{  
  DmaBusy = 1;
  SetupCycle(0x0000,SPI_WRITE);
  
  /* USCIA0 TXIFG is the DMA trigger */
  DMACTL0 = DMA0TSEL_17;                   
  
  DummyData = 0;
  
  __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) &DummyData);
                                            
  __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCA0TXBUF);
             
  /* write the entire serial ram with zero */
  DMA0SZ = 8192;
  
  /* 
   * single transfer, source byte and dest byte,
   * level sensitive, enable interrupt, clear interrupt flag
   */
  DMA0CTL = DMADT_0 + DMASBDB + DMALEVEL + DMAIE;  
  
  /* start the transfer */
  DMA0CTL |= DMAEN;
  
  WaitForDmaEnd();
  
}

void UpdateDisplayHandler(tMessage* pMsg)
{ 
  unsigned char Mode = pMsg->Options & BUFFER_SELECT_MASK;
  
  PrintString("-----<< MY: Update Display\r\n");
  /* premature exit if the idle page is not the normal page */
  if ( Mode  == IDLE_MODE 
      && QueryIdlePageNormal() == 0 )
  {
    __no_operation();
    return;
  }
  /* get the buffer address */
  unsigned int Index;
//  if (pMsg->Options & BUFFER_FORCE_UPDATE)
//  {
//    Index = GetBufferIndex(Mode, BUFFER_TYPE_RECENT);
//    PrintString("    Force update\r\n");
//  }
//  else
    Index = GetBufferIndex(Mode, BUFFER_TYPE_READ);
    
  PrintStringAndHexByte("    Buffer Index: ", Index);
  PrintStringAndTwoHexBytes("   Buffers []: ", 
    BufferStatus[Mode][0], BufferStatus[Mode][1]);
  
  if (Index == BUFFER_UNAVAILABLE) return;
  SetBufferStatus(Index, BUFFER_READING);
  
  /* now calculate the absolute address */
  unsigned int AbsoluteAddress = GetBufferAddress(Index);
  
  /* if it is the idle buffer then determine starting line */
  unsigned char LcdRow = GetStartingRow(Mode);

  /* update address because of possible starting row change */
  AbsoluteAddress += BYTES_PER_LINE*LcdRow;
       
  /* A possible change would be store dirty bits at the beginning of the buffer
   * after reading this only the rows that changed would be read out and 
   * written to the lcd.
   * However, it is much easier to just draw the entire screen 
   */
  for ( ; LcdRow < 96; LcdRow++ )
  {
    /* one buffer is used for writing and another is used for reading 
     * the incoming message can't be used because it doesn't have a buffer
     */
    pWorkingBuffer[0] = SPI_READ;
    pWorkingBuffer[1] = (unsigned char)(AbsoluteAddress >> 8); 
    pWorkingBuffer[2] = (unsigned char) AbsoluteAddress;
    
    /* 
     * The tLcdMessagePayload accounts for the
     * 3+1 spots to starting location of data from dma read
     * (room for bytes read in when cmd and address are sent)
     */
    ReadBlock(pWorkingBuffer,(unsigned char *)&WriteLineBuffer);
    
    WaitForDmaEnd();

    /* now add the row number */
    WriteLineBuffer.RowNumber = LcdRow;
    WriteLcdHandler(&WriteLineBuffer);
    AbsoluteAddress += BYTES_PER_LINE;
  }
  
  SetBufferStatus(Index, BUFFER_CLEAN);
  PrintString("----- MY: Update Display Done.\r\n");
  
  /* now that the screen has been drawn put the LCD into a lower power mode */
  PutLcdIntoStaticMode();
  
  /*
   * now signal that the display task that the operation has completed 
   * 
   * When in idle mode this will cause the top portion of the screen to be
   * drawn
   */
  SetupMessage(&OutgoingMsg, ChangeModeMsg, pMsg->Options);
  RouteMsg(&OutgoingMsg);
  
}

/* Load a template from flash into a draw buffer (ram)
 *
 * This can be used by the phone or the watch application to save drawing time
 */
void LoadTemplateHandler(tMessage* pMsg)
{
  unsigned char Index = 
    GetBufferIndex(pMsg->Options & BUFFER_SELECT_MASK, BUFFER_TYPE_WRITE);
                                                    
  /* now calculate the absolute address */
  unsigned int AbsoluteAddress = GetBufferAddress(Index);
  
  /* 
   * templates don't have extra space in them for additional 3 bytes of 
   * cmd and address
   */
  SetupCycle(AbsoluteAddress,SPI_WRITE);

  tLoadTemplatePayload* pLoadTemplateMsg = (tLoadTemplatePayload*)pMsg->pBuffer;
   
  /* template zero is reserved for simple patterns */
  if ( pLoadTemplateMsg->TemplateSelect > 1 )
  {
    unsigned char const * pTemplate = 
      GetTemplatePointer(pLoadTemplateMsg->TemplateSelect);
  
    WriteBlockToSram((unsigned char*)pTemplate,BYTES_PER_SCREEN);
  }
  else
  {
    /* clear or fill the screen */
    if ( pLoadTemplateMsg->TemplateSelect == 0 )
    {
      ClearBufferInSram(AbsoluteAddress,0x00,BYTES_PER_SCREEN);
    }
    else
    {
      ClearBufferInSram(AbsoluteAddress,0xff,BYTES_PER_SCREEN);  
    }
  }
  
  SetBufferStatus(Index, BUFFER_WRITTEN);
}

/* determine if the phone is controlling all of the idle screen */
unsigned char GetStartingRow(unsigned char Mode)
{
  return (Mode == IDLE_BUFFER_SELECT 
    && GetIdleBufferConfiguration() == WATCH_CONTROLS_TOP) ? 30 : 0;
}

unsigned char GetBufferIndex(unsigned char Mode, unsigned char Type)
{
//  if (Type == BUFFER_TYPE_RECENT) 
//  {
//    return (BufferStatus[Mode][0] & BUFFER_STATUS_RECENT) ?
//      Mode << 1 : (Mode << 1) + 1;
//  }
  
  unsigned char j, i;
  unsigned char NumberOfRules = (Type == BUFFER_TYPE_READ) ?
    NUMBER_OF_READ_BUFFER_SEL_RULES : NUMBER_OF_WRITE_BUFFER_SEL_RULES;
  
  PrintStringSpaceAndTwoDecimals("----- MY: GetBufferIndex: Mode, Type: ", Mode, Type);
  for (j = 0; j < NumberOfRules; j++)
  {
    for (i = 0; i < NUMBER_OF_BUFFERS; i++)
    {
      if ((BufferStatus[Mode][i] & BUFFER_STATUS_MASK) == BufSelRule[Type][j]) {
        PrintStringAndHexByte("    MY: Rule j: ", j);
        return (Mode << 1) + i;
      }
    }
  }

  return (BufferStatus[Mode][0] & BUFFER_STATUS_RECENT) ?
    Mode << 1 : (Mode << 1) + 1;
//  return BUFFER_UNAVAILABLE;
  
  
//  unsigned char Index = Mode << 1;
//  unsigned char Status = (BufferStatus & (1 << Index)) >> Index;
//  // if the buffer status is NOT expected as Type, check the other buffer
//  if (Status != Type)
//  {
//    Status = (BufferStatus & (1 << Index + 1)) >> Index + 1;
//    // if both dirty, return the first; if both clean, dirty one not available 
//    if (Status != Type && Type == BUFFER_CLEAN)
//    {
//      Index = BUFFER_UNAVAILABLE;
//    }
//  }
//  PrintStringAndDecimal("MY: GetBufferIndex: ", Index);
//  return Index;
}

void SetBufferStatus(unsigned char Index, unsigned char Status)
{
  //[Mode][0|1]
  // Mark the buffer "current" and remove the "current" flag of the other buffer
  BufferStatus[Index >> 1][Index & 1] = Status | BUFFER_STATUS_RECENT;
  BufferStatus[Index >> 1][1 - (Index & 1)] &= BUFFER_STATUS_MASK;
//  unsigned char Mask = (1 << Index); 
//  unsigned char Current = (BufferStatus & Mask) >> Index;
//  if (Status != Current)
//  {
//    BufferStatus &= ~Mask;
//    BufferStatus |= Status << Index;
//  }
  PrintStringAndTwoHexBytes("   Set Buffers[]: ", 
    BufferStatus[Index >> 1][0], BufferStatus[Index >> 1][1]);
}

unsigned int GetBufferAddress(unsigned Index)
{
  unsigned int Address = BYTES_PER_SCREEN * Index;
  
  /* if mode is scroll, scroll buffers are half the size of normal buffers */
  if ( (Index >> 1) >= SCROLL_BUFFER_SELECT )
  {
    Address -= BYTES_PER_SCREEN >> 1;     
  }
  
  return Address;
}
      
/* Serial RAM controller uses two dma channels
 * LCD driver task uses one dma channel
 */
#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(DMA_ISR,".text:_isr");
#endif
 
#pragma vector=DMA_VECTOR
__interrupt void DMA_ISR(void)
{
  /* 0 is no interrupt and remainder are channels 0-7 */
  switch(__even_in_range(DMAIV,16))
  {
  case 0: 
    break;
  case 2: 
    DmaBusy = 0;
    break;
  case 4:
    DmaBusy = 0;
    break;
  case 6:
    LcdDmaIsr();
    break;
  default: 
    break;
  }
}


void RamTestHandler(tMessage* pMsg)
{
  unsigned int i;
  
  PrintString("***************************************************************"); 
  PrintString("Starting Ram Test\r\n");
  PrintString("***************************************************************"); 
  
  for ( i = 0; i < 0xffff; i++ )
  {
    ClearBufferInSram(0x0,0x0,1000); 
  }
  
  PrintString("Ram Test Complete");
  
}
