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
static void ActivateBuffer(tMessage* pMsg);
static void WaitForDmaEnd(void);

/******************************************************************************/
unsigned char GetStartingRow(unsigned char MsgOptions);
unsigned int GetActiveBufferStartAddress(unsigned char MsgOptions);
unsigned int GetDrawBufferStartAddress(unsigned char MsgOptions);

static unsigned char IdleActiveBuffer;
static unsigned char IdleDrawBuffer;

static unsigned char ApplicationActiveBuffer;
static unsigned char ApplicationDrawBuffer;

static unsigned char NotificationActiveBuffer;
static unsigned char NotificationDrawBuffer;

static unsigned char ScrollActiveBuffer;
static unsigned char ScrollDrawBuffer;

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
  
  /***************************************************************************/
  
  /* assign the active and idle buffers */
  IdleActiveBuffer = 0;
  IdleDrawBuffer = 1;
  ApplicationActiveBuffer = 2;
  ApplicationDrawBuffer = 3;
  NotificationActiveBuffer = 4;
  NotificationDrawBuffer = 5;
  ScrollActiveBuffer = 6;
  ScrollDrawBuffer = 7;
  
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
  unsigned char MsgOptions = pMsg->Options;
  
  /* map the payload */
  tSerialRamPayload* pSerialRamPayload = (tSerialRamPayload*)pMsg->pBuffer;
  
  unsigned char RowA = pSerialRamPayload->RowSelectA;
  unsigned char RowB = pSerialRamPayload->RowSelectB;
  
  /* use the options to determine what buffer to write then
   * determine what buffer is active/draw 
   * get the buffer address
   * then add in the row number for the absolute address
   */
  unsigned int BufferAddress = GetDrawBufferStartAddress(MsgOptions);
  unsigned int AbsoluteAddress = BufferAddress + (RowA*BYTES_PER_LINE);
  
  pWorkingBuffer[0] = SPI_WRITE;
  pWorkingBuffer[1] = (unsigned char)(AbsoluteAddress >> 8);
  pWorkingBuffer[2] = (unsigned char) AbsoluteAddress; 
  
  /* copy the data */
  for(unsigned char i = 0; i < BYTES_PER_LINE; i++)
  {
    pWorkingBuffer[3+i] = pSerialRamPayload->pLineA[i];
  }
  
  WriteBlockToSram(pWorkingBuffer,15);
  
  /* if the bit is one then only draw one line */
  if ( (MsgOptions & WRITE_BUFFER_ONE_LINE_MASK) == 0 )
  {
    /* calculate address for second row */
    AbsoluteAddress = BufferAddress + (RowB*BYTES_PER_LINE);
    
    pWorkingBuffer[0] = SPI_WRITE;
    pWorkingBuffer[1] = (unsigned char)(AbsoluteAddress >> 8);
    pWorkingBuffer[2] = (unsigned char) AbsoluteAddress; 
  
    /* copy the data ... */
    for(unsigned char i = 0; i < BYTES_PER_LINE; i++)
    {
      pWorkingBuffer[3+i] = pSerialRamPayload->pLineB[i];
    }
  
    /* point to first character to dma */
    WriteBlockToSram(pWorkingBuffer,15);
  }
  
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
  //DEBUG4_HIGH();
  
  unsigned char Options = pMsg->Options;
  
  if ( (Options & DRAW_BUFFER_ACTIVATION_MASK) == ACTIVATE_DRAW_BUFFER )
  { 
    ActivateBuffer(pMsg);
  }
  
  /* premature exit if the idle page is not the normal page */
  if (   (Options & BUFFER_SELECT_MASK) == IDLE_BUFFER_SELECT 
      && QueryIdlePageNormal() == 0 )
  {
    return;
  }
  
  /* get the buffer address */
  unsigned int BufferAddress = GetActiveBufferStartAddress(Options);
  unsigned int DrawBufferAddress = GetDrawBufferStartAddress(Options);
  
  /* now calculate the absolute address */
  unsigned int AbsoluteAddress = BufferAddress;
  unsigned int AbsoluteDrawAddress = DrawBufferAddress;
  
  /* if it is the idle buffer then determine starting line */
  unsigned char LcdRow = GetStartingRow(Options);

  /* update address because of possible starting row change */
  AbsoluteAddress += BYTES_PER_LINE*LcdRow;
  AbsoluteDrawAddress += BYTES_PER_LINE*LcdRow;
       
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

    /* if there was more ram then it would be better to do a screen copy  */
    if ( (Options & UPDATE_COPY_MASK ) == COPY_ACTIVE_TO_DRAW_DURING_UPDATE)
    {
      /* now format the message for a serial ram write while taking into account
       * that the data is offset in the buffer
       */
      WriteLineBuffer.Reserved1 = SPI_WRITE;
      WriteLineBuffer.LcdCommand = (unsigned char)(AbsoluteDrawAddress >> 8);
      WriteLineBuffer.RowNumber = (unsigned char)AbsoluteDrawAddress;
      
      WriteBlockToSram((unsigned char*)&WriteLineBuffer,15);
      
      WaitForDmaEnd();
    }

    /* now add the row number */
    WriteLineBuffer.RowNumber = LcdRow;
    
    WriteLcdHandler(&WriteLineBuffer);
    
    AbsoluteAddress += BYTES_PER_LINE;
    AbsoluteDrawAddress += BYTES_PER_LINE;

  }
  
  /* now that the screen has been drawn put the LCD into a lower power mode */
  PutLcdIntoStaticMode();
  
  /*
   * now signal that the display task that the operation has completed 
   */
  SetupMessage(&OutgoingMsg,ChangeModeMsg,Options);
  RouteMsg(&OutgoingMsg);
  
  //DEBUG4_LOW();
  
}

/* Load a template from flash into a draw buffer (ram)
 *
 * This can be used by the phone or the watch application to save drawing time
 */
void LoadTemplateHandler(tMessage* pMsg)
{
  unsigned int BufferAddress = 
    GetDrawBufferStartAddress(pMsg->Options);
                                                    
  /* now calculate the absolute address */
  unsigned int AbsoluteAddress = BufferAddress;
  
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
  
}

/* Activate the current draw buffer (swap pointers) */
void ActivateBuffer(tMessage* pMsg)
{
  unsigned char BufferSelect = (pMsg->Options) & BUFFER_SELECT_MASK;
  unsigned char BufferSwap;
  
#if 0
  PrintStringAndDecimal("BufferSwap ", BufferSelect);
#endif
  
  switch (BufferSelect)
  {
  case 0:
    BufferSwap = IdleActiveBuffer;
    IdleActiveBuffer = IdleDrawBuffer;
    IdleDrawBuffer = BufferSwap;
    break;
  case 1:
    BufferSwap = ApplicationActiveBuffer;
    ApplicationActiveBuffer = ApplicationDrawBuffer;
    ApplicationDrawBuffer = BufferSwap;
    break;
  case 2:
    BufferSwap = NotificationActiveBuffer;
    NotificationActiveBuffer = NotificationDrawBuffer;
    NotificationDrawBuffer = BufferSwap;
    break;
  case 3:
    BufferSwap = ScrollActiveBuffer;
    ScrollActiveBuffer = ScrollDrawBuffer;
    ScrollDrawBuffer = BufferSwap;
    break;
  default:
    PrintStringAndHex("Invalid Buffer Selected: 0x",BufferSelect);
    break;
  }
  
}

/* determine if the phone is controlling all of the idle screen */
unsigned char GetStartingRow(unsigned char MsgOptions)
{
  unsigned char StartingRow = 0;

  if (   (MsgOptions & BUFFER_SELECT_MASK) == IDLE_BUFFER_SELECT 
      && GetIdleBufferConfiguration() == WATCH_CONTROLS_TOP )
  {
    StartingRow = 30;
  }
  else
  {
    StartingRow = 0;  
  }
  
  return StartingRow;
}

/* Get the start address of the active buffer in SRAM */
unsigned int GetActiveBufferStartAddress(unsigned char MsgOptions)
{
  unsigned char BufferIndex;
  unsigned int BufferStartAddress;
  
  unsigned char BufferSelect = MsgOptions & BUFFER_SELECT_MASK;
  
  switch (BufferSelect)
  {
  case IDLE_BUFFER_SELECT:
    BufferIndex = IdleActiveBuffer;
    break;
  case APPLICATION_BUFFER_SELECT:
    BufferIndex = ApplicationActiveBuffer;
    break;
  case NOTIFICATION_BUFFER_SELECT:
    BufferIndex = NotificationActiveBuffer;
    break;
  case SCROLL_BUFFER_SELECT:
    BufferIndex = ScrollActiveBuffer;
    break;
  default:
    break;
  }
  
  BufferStartAddress = BYTES_PER_SCREEN * BufferIndex;
  
  /* scroll buffers are half the size of normal buffers */
  if ( BufferIndex == 7 )
  {
    BufferStartAddress -= BYTES_PER_SCREEN/2;     
  }
  
  return BufferStartAddress;
  
}

/* Get the start address of the Draw buffer in SRAM */
unsigned int GetDrawBufferStartAddress(unsigned char MsgOptions)
{
  unsigned char BufferIndex;
  unsigned int BufferStartAddress;
  
  unsigned char BufferSelect = MsgOptions & BUFFER_SELECT_MASK;
  
  switch (BufferSelect)
  {
  case IDLE_BUFFER_SELECT:
    BufferIndex = IdleDrawBuffer;
    break;
  case APPLICATION_BUFFER_SELECT:
    BufferIndex = ApplicationDrawBuffer;
    break;
  case NOTIFICATION_BUFFER_SELECT:
    BufferIndex = NotificationDrawBuffer;
    break;
  case SCROLL_BUFFER_SELECT:
    BufferIndex = ScrollDrawBuffer;
    break;
  default:
    break;
  }
  
  BufferStartAddress = BYTES_PER_SCREEN * BufferIndex;
  
  /* scroll buffers are half the size of normal buffers */
  if ( BufferIndex == 7 )
  {
    BufferStartAddress -= BYTES_PER_SCREEN/2;     
  }
  
  return BufferStartAddress;
}


/* Serial RAM controller uses two dma channels
 * LCD driver task uses one dma channel
 */
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
