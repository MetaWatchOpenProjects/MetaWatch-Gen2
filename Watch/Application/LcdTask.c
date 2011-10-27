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
/*! \file LcdTask.c
 *
 */
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "hal_board_type.h"
#include "hal_clock_control.h"
#include "hal_lcd.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "BufferPool.h"     
#include "Utilities.h"      
#include "DebugUart.h"
#include "LcdTask.h"
#include "LcdDisplay.h"

#define LCD_TASK_MSG_QUEUE_LEN   12    
#define LCD_TASK_STACK_DEPTH     (configMINIMAL_STACK_DEPTH + 20)
#define LCD_TASK_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)

/******************************************************************************/

// SMCLK = 16MHz -> divide by 16 to get 1 MHz SPI clock
#define SPI_PRESCALE_L   0x10
#define SPI_PRESCALE_H   0x00

static unsigned char LCD_CLEAR_COMMAND[] = {LCD_CLEAR_CMD, 0x00, 0x00};

/* errata - DMA variables cannot be function scope */
static unsigned char LcdDmaBusy = 0;

static void LcdPeripheralInit(void);
static void LcdTask(void *pvParameters);
static void LcdTaskMessageHandler(tHostMsg* pMsg);

static tHostMsg* pLcdTaskMsg;
  
static void WriteLcdHandler(tHostMsg* pMsg);
static void ClearLcdHandler(void);
static void UpdateMyDisplayHandler(tHostMsg* pMsg);

/******************************************************************************/

static void LcdTask(void *pvParameters);
static void LcdTaskMessageHandler(tHostMsg* pMsg);

xTaskHandle LcdTaskHandle;

/******************************************************************************/
static void WriteLineToLcd(unsigned char* pData,unsigned char Size);


/******************************************************************************/

static void LcdPeripheralInit(void)
{
  /* LCD 5.0 V SUPPLY */
  LCD_5V_PDIR |= LCD_5V_BIT;
  LCD_5V_POUT |= LCD_5V_BIT;

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


void InitializeLcdTask(void)
{
  LcdPeripheralInit();
 
  // This is a Rx message queue
  QueueHandles[LCD_TASK_QINDEX] = 
    xQueueCreate( LCD_TASK_MSG_QUEUE_LEN, MESSAGE_QUEUE_ITEM_SIZE );
  
  // prams are: task function, task name, stack len , task params, priority, task handle
  xTaskCreate(LcdTask, 
              "LCD_DRIVER", 
              LCD_TASK_STACK_DEPTH, 
              NULL, 
              LCD_TASK_TASK_PRIORITY, 
              &LcdTaskHandle);
}

/*! The LCD task runs forever.  It writes data to the LCD based on the 
 * commands it receives in its queue.
 */
static void LcdTask(void *pvParameters)
{
  if ( QueueHandles[LCD_TASK_QINDEX] == 0 )
  {
    PrintString("Lcd Task Queue not created!\r\n");
  }

  /* the first task is to clear the LCD */
  BPL_AllocMessageBuffer(&pLcdTaskMsg);
  pLcdTaskMsg->Type = ClearLcd;
  RouteMsg(&pLcdTaskMsg);
  
  for(;;)
  {
    if( pdTRUE == xQueueReceive(QueueHandles[LCD_TASK_QINDEX], 
                                &pLcdTaskMsg, portMAX_DELAY) )
    {
      LcdTaskMessageHandler(pLcdTaskMsg);
        
      BPL_FreeMessageBuffer(&pLcdTaskMsg);
      
      CheckStackUsage(LcdTaskHandle,"Lcd Task");
      
    }
  }


}

/*! Process the messages routed to the LCD driver Task */
static void LcdTaskMessageHandler(tHostMsg* pMsg)
{
  unsigned char Type = pMsg->Type;
    
  switch(Type)
  {
  
  case WriteLcd:
    WriteLcdHandler(pMsg);
    break;
  
  case ClearLcd:
    ClearLcdHandler();
    break;
    
  case UpdateMyDisplayLcd:
    UpdateMyDisplayHandler(pMsg);
    break;
    
  default:
    PrintStringAndHex("<<Unhandled Message>> in Lcd Task: Type 0x", Type);
    break;
  }

}

/*! Writes a single line to the LCD */
static void WriteLcdHandler(tHostMsg* pMsg)
{
  tLcdMessage* pLcdMessage = (tLcdMessage*)(pMsg);
  
  pLcdMessage->LcdCommand = LCD_WRITE_CMD;
  pLcdMessage->RowNumber += FIRST_LCD_LINE_OFFSET;
  
  /* point to the first byte to be transmitted to LCD */
  unsigned char* pData = &pLcdMessage->LcdCommand;

  /* flip bits */
  if ( QueryInvertDisplay() == NORMAL_DISPLAY )
  {
    for ( unsigned char i = 0; i < 12; i++ )
    {
      pLcdMessage->pLine[i] = ~(pLcdMessage->pLine[i]); 
    }
  }

  pLcdMessage->Dummy1 = 0x00; 
  pLcdMessage->Dummy2 = 0x00;     
  
  /* 
   * 1 for command, 1 for row, 12 for line, 2 for dummy
   */
  unsigned char Size = 16;
  
  WriteLineToLcd(pData,Size);
  
}


static void ClearLcdHandler(void)
{
  WriteLineToLcd(LCD_CLEAR_COMMAND,LCD_CLEAR_CMD_SIZE);   
}
    
    
static void WriteLineToLcd(unsigned char* pData,unsigned char Size)
{  
  EnableSmClkUser(LCD_USER);
  LCD_CS_ASSERT();
  
#ifdef DMA
  
  DEBUG4_PULSE();
  
  LcdDmaBusy = 1;
  
  /* USCIB0 TXIFG is the DMA trigger
   * DMACTL1 controls dma2 and [dma3]
   */
  DMACTL1 = DMA2TSEL_19;    
    
  __data16_write_addr((unsigned short) &DMA2SA,(unsigned long) pData);
                                            
  __data16_write_addr((unsigned short) &DMA2DA,(unsigned long) &LCD_SPI_UCBxTXBUF);
            
  DMA2SZ = (unsigned int)Size;
  
  /* 
   * single transfer, increment source address, source byte and dest byte,
   * level sensitive, enable interrupt, clear interrupt flag
   */
  DMA2CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMALEVEL + DMAIE;  
  
  /* start the transfer */
  DMA2CTL |= DMAEN;
  
  while(LcdDmaBusy);
    
#else

  for ( unsigned char Index = 0; Index < Size; Index++ )
  {
    LCD_SPI_UCBxTXBUF = pData[Index];
    while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  }

#endif
    
  /* wait for shift to complete ( ~3 us ) */
  while( (LCD_SPI_UCBxSTAT & 0x01) != 0 );
  
  /* now the chip select can be deasserted */
  LCD_CS_DEASSERT();
  DisableSmClkUser(LCD_USER);
        
}

static void UpdateMyDisplayHandler(tHostMsg* pMsg)
{  
  
  tUpdateMyDisplayMsg* pUpdateMyDisplayMessage = (tUpdateMyDisplayMsg*)pMsg;
  
  EnableSmClkUser(LCD_USER);
  LCD_CS_ASSERT();
  
#ifdef DMA
  
  LcdDmaBusy = 1;
  
  /* send the lcd write command before starting the dma */
  LCD_SPI_UCBxTXBUF = LCD_WRITE_CMD;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  
  /* USCIB0 TXIFG is the DMA trigger
   * DMACTL1 controls dma2 and [dma3]
   */
  DMACTL1 = DMA2TSEL_19;    
    
  __data16_write_addr((unsigned short) &DMA2SA,
                      (unsigned long) pUpdateMyDisplayMessage->pMyDisplay);
                                            
  __data16_write_addr((unsigned short) &DMA2DA,(unsigned long) &LCD_SPI_UCBxTXBUF);
            
  DMA2SZ = (pUpdateMyDisplayMessage->TotalLines*sizeof(tLcdLine));
  
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
  LCD_SPI_UCBxTXBUF = LCD_WRITE_CMD;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  
  unsigned int Size = (pUpdateMyDisplayMessage->TotalLines*sizeof(tLcdLine));
  for ( unsigned int Index = 0; Index < Size; Index++ )
  {
    LCD_SPI_UCBxTXBUF = pUpdateMyDisplayMessage->pMyDisplay[Index];
    while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  }

#endif
  
  /* add one more dummy byte at the end */
  LCD_SPI_UCBxTXBUF = 0x00;
  while (!(LCD_SPI_UCBxIFG&UCTXIFG));
  
  /* wait for shift to complete ( ~3 us ) */
  while( (LCD_SPI_UCBxSTAT & 0x01) != 0 );
        
  /* now the chip select can be deasserted */
  LCD_CS_DEASSERT();
  DisableSmClkUser(LCD_USER);
        
}

void LcdDmaIsr(void)
{
  LcdDmaBusy = 0;
}
