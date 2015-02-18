//==============================================================================
//  Copyright 2011-2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hal_board_type.h"
#include "hal_clock_control.h"
#include "hal_miscellaneous.h"
#include "Messages.h"
#include "DebugUart.h"
#include "DrawHandler.h"
#include "ClockWidget.h"
#include "LcdDriver.h"
#include "SerialRam.h"
#include "LcdDisplay.h"
#include "LcdBuffer.h"
#include "BitmapData.h"
#include "CallNotifier.h"
#include "Property.h"
#include "Widget.h"

/* write and read status register */
#define SPI_RDSR                (0x05)
#define SPI_WRSR                (0x01)
#define SPI_OVERHEAD            (3)
#define SPI_INIT_DELAY_IN_MS    (10 * portTICK_RATE_MS)

/* the 256Kbit part does not have a 1 in bit position 1 */
#define DEFAULT_SR_VALUE        (0x02)
#define FINAL_SR_VALUE          (0x43)
#define DEFAULT_SR_VALUE_256    (0x00)
#define FINAL_SR_VALUE_256      (0x41)
#define SEQUENTIAL_MODE_COMMAND (0x41)

#define SRAM_TOTAL_SIZE         (8 * 1024)
#define MODE_BUF_START_ADDR     (0x0E00) // 1152x3 = 3456
#define MODE_BUFFER_SIZE        (0x1200) // 1152x4
#define WGT_BUF_START_ADDR      (0x80) //(1024x8-1152x7=128 (0x1B00) // 96x12x(4 + 2) = 6912

#define WRITE_BUFFER_TWO_LINES     (0x00)
#define MSG_OPT_WRTBUF_BPL_MASK    (0x38)

#define STATUS_BAR_IN_MODES ((1 << IDLE_MODE) | (1 << APP_MODE)) // | (1 << MUSIC_MODE))

#define IDLE_PAGE_NUM             4
#define NOTIF_TOTAL_PAGES         5
#define DRAW_PAGE                 0
#define SHOW_PAGE                 1

#define TEMPLATE_1                2  //starts at 2 because 0,1 are clear and fill
#define TEMPLATE_2                3
#define TEMPLATE_3                4
#define TEMPLATE_4                5

/* errata - DMA variables cannot be function scope */
static unsigned char const DummyData = 0x00;

static unsigned char ReadData = 0x00;
static unsigned char DmaBusy  = 0;

static unsigned char const ModeAddrIndex[] = {0, 1, 3, 2};
static unsigned char IdleShowPage = 0;
static unsigned char NotifShowPage = 0;
static unsigned char NotifDrawPage = 0;
static unsigned char NotifPageNum = 0;
static unsigned char StatusBarInModes = STATUS_BAR_IN_MODES;

/* avoid conflicting widget buffer read/write btw SetWidgetList and UpdateDisplay */
xSemaphoreHandle SramMutex;

static unsigned char SramBuf[SRAM_HEADER_LEN];
static unsigned char const ModePriority[] = {NOTIF_MODE, APP_MODE, IDLE_MODE, MUSIC_MODE};

typedef struct
{
  unsigned char StartRow;
  unsigned char RowNum;
} Rect_t;

#define MODE_START_ADDR(_x) (ModeAddrIndex[_x] * BYTES_PER_SCREEN + MODE_BUF_START_ADDR)

/******************************************************************************/
static void SetAddr(unsigned int Addr);
static signed char ComparePriority(unsigned char Mode);
static void TurnPage(unsigned char PageNo);

//#define MSG_OPT_NEWUI             (0x80)
//#define MSG_OPT_HOME_WGT          (0x40) // new ui only
//#define MSG_OPT_WRTBUF_1_LINE     (0x10)
//#define WRITE_BUFFER_TWO_LINES    (0x00)
//#define MODE_MASK                 (0x07)
//#define MSG_OPT_WRTBUF_MULTILINE  (0x40) // for music mode
//#define MSG_OPT_WRTBUF_BPL_MASK   (0x38) // for music mode

void WriteBufferHandler(tMessage *pMsg)
{
  if (pMsg->Options & MSG_OPT_NEWUI) WriteWidgetBuffer(pMsg);
  else
  {
    unsigned int Addr = MODE_START_ADDR(pMsg->Options & MODE_MASK) + *pMsg->pBuffer * BYTES_PER_LINE;
    unsigned char *pBuffer = pMsg->pBuffer + 1 - SRAM_HEADER_LEN;
    unsigned char BytesPerLine = BYTES_PER_LINE;
    unsigned char DataLength = pMsg->Length - 1;

    if ((pMsg->Options & MODE_MASK) == NOTIF_MODE)
    {
        Addr += NotifDrawPage * BYTES_PER_SCREEN;
    }

    if (pMsg->Options & MSG_OPT_WRTBUF_MULTILINE)
    {
        Addr += *(pMsg->pBuffer + 1);
        pBuffer ++; // first 2 bytes for y, x
        DataLength --; // exclude first 2 bytes
        BytesPerLine = (pMsg->Options & MSG_OPT_WRTBUF_BPL_MASK) >> 3;
    }

    unsigned char LineNum = DataLength / BytesPerLine;
    if (BytesPerLine != BYTES_PER_LINE) PrintF("- Wrtbuf BPL:%d %s %d", BytesPerLine, "Ln:", LineNum);
    
    while (LineNum--)
    {
      pBuffer[0] = SPI_WRITE;
      pBuffer[1] = Addr >> 8;
      pBuffer[2] = Addr;
      Write((unsigned long)pBuffer, BytesPerLine, DMA_COPY);

      Addr += BYTES_PER_LINE;
      pBuffer += BytesPerLine + (BytesPerLine == BYTES_PER_LINE);
    }
  }
}

unsigned char CurrentIdleScreen(void)
{
  return IdleShowPage;
}

//#define MSG_OPT_NEWUI             (0x80)
//#define MSG_OPT_SET_PAGE          (0x20)
//#define MSG_OPT_UPD_HWGT          (0x20)
//#define MSG_OPT_UPD_INTERNAL      (0x10)
//#define MSG_OPT_PAGE_NO           (0x0C)
//#define MSG_OPT_TURN_PAGE         (0x0C)
//#define MSG_OPT_PRV_PAGE          (0x08)
//#define MSG_OPT_NXT_PAGE          (0x04)
//#define MODE_MASK                 (0x03)

void UpdateDisplayHandler(tMessage *pMsg)
{
  unsigned char Mode = pMsg->Options & MODE_MASK;

  if ((pMsg->Options & MSG_OPT_NEWUI) && GetProperty(PROP_PHONE_DRAW_TOP)) //for idle update only
  {
    if (!(pMsg->Options & MSG_OPT_UPD_INTERNAL) && pMsg->Options & MSG_OPT_SET_PAGE)
    { // set current page only
      IdleShowPage = (pMsg->Options & MSG_OPT_PAGE_NO) >> SET_PAGE_SHFT;
    }
    else if (pMsg->Options & MSG_OPT_TURN_PAGE)
    { // turn page
      if ((pMsg->Options & MSG_OPT_TURN_PAGE) == MSG_OPT_NXT_PAGE) IdleShowPage ++;

#if WWZ
      if (IdleShowPage == IDLE_PAGE_NUM)
      {
        IdleShowPage = 0;
        SendMessage(CountDownMsg, MSG_OPT_NONE);
        return;
      }
#endif
      if (IdleShowPage == IDLE_PAGE_NUM) IdleShowPage = 0;
      SendMessage(ChangeModeMsg, MSG_OPT_CHGMOD_IND); // report curr page|mode
    }

    // not in idle mode idle page
    if (CurrentMode != IDLE_MODE || PageType != PAGE_TYPE_IDLE) return;

    xSemaphoreTake(SramMutex, portMAX_DELAY);

    DrawWidgetToLcd(IdleShowPage);

    xSemaphoreGive(SramMutex);
  }
  else
  {
    // In watch_draw_top case, if mode change back to idle from other modes,
    // redraw date&time. In any case of mode change, notify LcdDisplay to
    // switch button definition for the mode.
    // for external msg, NOTIF > APP > IDLE > MUSIC
    if (pMsg->Options & MSG_OPT_UPD_INTERNAL)
    {
      if ((pMsg->Options & MSG_OPT_TURN_PAGE) == MSG_OPT_NXT_PAGE)
      {
        if (NotifPageNum == 0) return;
        TurnPage(SHOW_PAGE);
      }
      else if (Mode == NOTIF_MODE && Ringing()) TurnPage(DRAW_PAGE);
    }
    else
    {
      signed char Result = ComparePriority(Mode);
//      PrintF("- UpdDsp Priority:%d", Result);

      if (Result >= 0)
      {
        if (Result > 0) ChangeMode(Mode);
        if (Mode == NOTIF_MODE) TurnPage(DRAW_PAGE);
        if (CurrentMode != IDLE_MODE) ResetModeTimer();
        else if (PageType != PAGE_TYPE_IDLE) return;
      }
      else return;
    }
//    PrintF("- UpdDsp PgTp:%d", PageType);

    // determine starting line
    unsigned char StartRow = (Mode == IDLE_MODE && !GetProperty(PROP_PHONE_DRAW_TOP)) ?
                            WATCH_DRAW_SCREEN_ROW_NUM : 0;
    unsigned char RowNum = LCD_ROW_NUM - StartRow;

    if (Mode == CurrentMode && pMsg->Length)
    {
      Rect_t *pRect = (Rect_t *)pMsg->pBuffer;
      if (pRect->StartRow < LCD_ROW_NUM) StartRow = pRect->StartRow;
      if (pRect->RowNum && pRect->RowNum + StartRow <= LCD_ROW_NUM) RowNum = pRect->RowNum;
    }

    /* now calculate the absolute address */
    unsigned int Addr = MODE_START_ADDR(Mode) + StartRow * BYTES_PER_LINE;
    if (Mode == NOTIF_MODE) Addr += NotifShowPage * BYTES_PER_SCREEN;
//    PrintF("UpdDsp NtfShwPg:%u Rows:%u", NotifShowPage, RowNum);
    tLcdLine *DrawBuf = NULL;
    LcdReadBuffer_t *LcdBuf = (LcdReadBuffer_t *)pvPortMalloc(LCD_READ_BUFFER_SIZE);

    while (RowNum --)
    {
      SramBuf[0] = SPI_READ;
      SramBuf[1] = Addr >> 8;
      SramBuf[2] = Addr;
      Read(SramBuf, (unsigned char *)LcdBuf, BYTES_PER_LINE);
      /* now add the row number */
      LcdBuf->Line.Row = StartRow ++;
      LcdBuf->Line.Trailer = 0;

      if (Mode == NOTIF_MODE && NotifPageNum > 0 &&
          LcdBuf->Line.Row >= NOTIF_PAGE_NO_START_ROW &&
          LcdBuf->Line.Row <= NOTIF_PAGE_NO_END_ROW)
      {
        if (DrawBuf == NULL) DrawBuf = (tLcdLine *)GetLcdBuffer();
        memcpy(&DrawBuf[LcdBuf->Line.Row], (unsigned char *)&LcdBuf->Line, sizeof(tLcdLine));
      }

      WriteToLcd(&LcdBuf->Line, 1);
      Addr += BYTES_PER_LINE;
    }
    vPortFree(LcdBuf);

    if (DrawBuf)
    {
      unsigned char PageMore;
      if (NotifShowPage < NotifDrawPage) PageMore = NotifShowPage + NotifPageNum - NotifDrawPage;
      else PageMore = NotifShowPage - NotifDrawPage;

      DrawNotifPageNo(PageMore); // include freebuf
    }
  }

  DrawStatusBar();
}

static signed char ComparePriority(unsigned char Mode)
{
  if (Mode == CurrentMode) return 0;
  
  unsigned char i = 0;
  for (; i < MODE_NUM; ++i) if (ModePriority[i] == Mode) break;
  for (++i; i < MODE_NUM; ++i) if (ModePriority[i] == CurrentMode) return 1;
  return -1;
}

static void TurnPage(unsigned char PageNo)
{
  if (PageNo == DRAW_PAGE)
  {
    NotifShowPage = NotifDrawPage;
    if (++NotifDrawPage == NOTIF_TOTAL_PAGES) NotifDrawPage = 0;
    if (NotifPageNum < NOTIF_TOTAL_PAGES) NotifPageNum ++;
  }
  else if (NotifShowPage-- == 0) NotifShowPage = NotifPageNum - 1;
}

void DrawStatusBar(void)
{
  if ((1 << CurrentMode) & StatusBarInModes)
  {
    if (CurrentMode == IDLE_MODE) DrawStatusBarToWidget();
    else DrawStatusBarToLcd();
  }
}

void Read(unsigned char *pWriteData, unsigned char *pReadData, unsigned int Length)
{
  DmaBusy = 1;
  SRAM_CSN_ASSERT();

  /*
   * SPI has to write bytes to receive bytes so
   *
   * two DMA channels are used
   *
   * read requires 4 leading bytes because the shift in of the read data
   * lags the transmit data by one byte
   */

  /* USCIA0 TXIFG is the DMA trigger for DMA0 and RXIFG is the DMA trigger
   * for dma1 (DMACTL0 controls both)
   */
  DMACTL0 = DMA1TSEL_16 | DMA0TSEL_17;
  __data16_write_addr((unsigned short)&DMA0SA, (unsigned long)pWriteData);
  __data16_write_addr((unsigned short)&DMA0DA, (unsigned long)&UCA0TXBUF);
  DMA0SZ = Length + SRAM_HEADER_LEN + 1;

  /* don't enable interrupt for transmit dma done(channel 0)
   * increment the source address because the message contains the address
   * the other bytes are don't care
   */
  DMA0CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMALEVEL;

  /* receive data is source for dma 1 */
  __data16_write_addr((unsigned short)&DMA1SA, (unsigned long)&UCA0RXBUF);
  __data16_write_addr((unsigned short)&DMA1DA, (unsigned long)pReadData);
  DMA1SZ = Length + SRAM_HEADER_LEN + 1;

  /* increment destination address */
  DMA1CTL = DMADT_0 + DMADSTINCR_3 + DMASBDB + DMALEVEL + DMAIE;

  /* start the transfer */
  DMA1CTL |= DMAEN;
  DMA0CTL |= DMAEN;
  while (DmaBusy);
  
  SRAM_CSN_DEASSERT();
}

/* use DMA to write a block of data to the serial ram */
void Write(unsigned long const pData, unsigned int Length, unsigned char Op)
{
  EnableSmClkUser(SERIAL_RAM_USER);
  DmaBusy = 1;
  SRAM_CSN_ASSERT();

  /* USCIA0 TXIFG is the DMA trigger */
  DMACTL0 = DMA0TSEL_17;

  __data16_write_addr((unsigned short)&DMA0SA, pData);
  __data16_write_addr((unsigned short)&DMA0DA, (unsigned long)&UCA0TXBUF);

  DMA0SZ = Length + SRAM_HEADER_LEN;

  /*
   * single transfer, source byte and dest byte,
   * level sensitive, enable interrupt, clear interrupt flag
   */
  DMA0CTL = DMADT_0 + DMASBDB + DMALEVEL + DMAIE;

  /*  increment source addres */
  if (Op == DMA_COPY) DMA0CTL += DMASRCINCR_3;

  /* start the transfer */
  DMA0CTL |= DMAEN;
  while (DmaBusy);

  SRAM_CSN_DEASSERT();
  DisableSmClkUser(SERIAL_RAM_USER);
}

static void SetAddr(unsigned int Addr)
{
  SRAM_CSN_ASSERT();

  SramBuf[0] = SPI_WRITE;
  SramBuf[1] = Addr >> 8;
  SramBuf[2] = Addr;

  unsigned char i;

  for (i = 0; i < 3; ++i)
  {
    UCA0TXBUF = SramBuf[i];
    while (!(UCA0IFG&UCTXIFG));
    while (!(UCA0IFG&UCRXIFG));
    SramBuf[i] = UCA0RXBUF;
  }
}

/* Load a template from flash into mode SRAM */
void LoadTemplateHandler(tMessage *pMsg)
{
  unsigned int Addr = MODE_START_ADDR(pMsg->Options & MODE_MASK);
  if ((pMsg->Options & MODE_MASK) == NOTIF_MODE) Addr += NotifDrawPage * BYTES_PER_SCREEN;
  SetAddr(Addr);

  if (pMsg->pBuffer == NULL)
  { // internal usage: high 4-bit is TmpID
    Write((unsigned long)&pTemplate[pMsg->Options >> 4], BYTES_PER_SCREEN - SRAM_HEADER_LEN, DMA_COPY);
  }
  else if (*pMsg->pBuffer <= 1)
  {
    /* clear or fill the screen */
    Write((unsigned long)(*pMsg->pBuffer ? &FILL_BLACK : &FILL_WHITE), BYTES_PER_SCREEN - SRAM_HEADER_LEN, DMA_FILL);
  }
  else
  {
    /* template zero is reserved for simple patterns */
    Write((unsigned long)&pWatchFace[*pMsg->pBuffer - TEMPLATE_1][0], BYTES_PER_SCREEN - SRAM_HEADER_LEN, DMA_COPY);
    PrintF("-Template:%d", *pMsg->pBuffer);
  }
}

void LoadBuffer(unsigned char QuadIndex, unsigned char const *pTemp)
{
  SetAddr(QuadIndex * BYTES_PER_QUAD + WGT_BUF_START_ADDR);
  Write((unsigned long)pTemp, BYTES_PER_QUAD - SRAM_HEADER_LEN, DMA_COPY);
}

void ClearSram(unsigned char Mode)
{
  unsigned int Addr = MODE_START_ADDR(Mode);
  if (Mode == NOTIF_MODE) Addr += NotifDrawPage * BYTES_PER_SCREEN;
  SetAddr(Addr);
  Write((unsigned long)&DummyData, BYTES_PER_SCREEN - SRAM_HEADER_LEN, DMA_FILL);
}

#define SRAM_READ_OVERHEAD  (SRAM_HEADER_LEN + 1)

void DrawBitmapToSram(Draw_t *Info, unsigned char WidthInBytes, unsigned char const *pBitmap, unsigned char Mode)
{
  unsigned int Addr = (Info->X >> 3) + Info->Y * BYTES_PER_LINE + MODE_START_ADDR(Mode);
  if (Mode == NOTIF_MODE) Addr += NotifDrawPage * BYTES_PER_SCREEN;
//  PrintF("DrwBmpSrm NtfDrwPg:%u", NotifDrawPage);

  unsigned char SramBytes = ((Info->Width + Info->X % 8) >> 3) + 1;
  int Overflow = SramBytes + (Info->X >> 3) - BYTES_PER_LINE;
  if (Overflow > 0) SramBytes -= Overflow;
//  PrintF("WB:%u", SramBytes);

  unsigned char x, y;
  unsigned char Set;
  unsigned char *pBuf = (unsigned char *)pvPortMalloc(SramBytes + SRAM_READ_OVERHEAD);
//  memset(pBuf, 0x55, SramBytes + SRAM_READ_OVERHEAD);

  for (y = 0; y < Info->Height && (y + Info->Y) < LCD_ROW_NUM; ++y)
  {
    unsigned char ColBit = BIT0 << (Info->X % 8); // dst
    unsigned char MaskBit = BIT0; // src
    unsigned char const *pBmp = pBitmap + y * WidthInBytes;
    unsigned char *pByte = pBuf + SRAM_READ_OVERHEAD;

    SramBuf[0] = SPI_READ;
    SramBuf[1] = Addr >> 8;
    SramBuf[2] = Addr;
    Read(SramBuf, pBuf, SramBytes);
//    PrintQ(pBuf, SramBytes + SRAM_READ_OVERHEAD);

    for (x = 0; x < Info->Width && (x + Info->X) < LCD_COL_NUM; ++x)
    {
      if (pByte >= pBuf + SramBytes + SRAM_READ_OVERHEAD) PrintF("#DrwSram x:%u y:%u", x+Info->X, y+Info->Y);

      Set = *pBmp & MaskBit;
      BitOp(pByte, ColBit, Set, Info->Opt & DRAW_OPT_MASK);

      MaskBit <<= 1;
      if (MaskBit == 0)
      {
        MaskBit = BIT0;
        pBmp ++;
      }
      
      ColBit <<= 1;
      if (ColBit == 0)
      {
        ColBit = BIT0;
        pByte ++;
      }
    }

    pBuf[1] = SPI_WRITE;
    pBuf[2] = Addr >> 8;
    pBuf[3] = Addr;
    Write((unsigned long)(pBuf + 1), SramBytes, DMA_COPY);
    Addr += BYTES_PER_LINE;
  }

  vPortFree(pBuf);
  if ((y + Info->Y) >= LCD_ROW_NUM || (x + Info->X) >= LCD_COL_NUM)
    PrintF("DrwBmp x:%d y:%d", x + Info->X, y + Info->Y);
}

void DrawTemplateToSram(Draw_t *Info, unsigned char Mode)
{
  unsigned int Addr = MODE_START_ADDR(Mode) + Info->Y * BYTES_PER_LINE;
  if (Mode == NOTIF_MODE) Addr += NotifDrawPage * BYTES_PER_SCREEN;
  
  SetAddr(Addr);
  Write((unsigned long)pTemplate[Info->Id & TMPL_ID_MASK], BYTES_PER_SCREEN - SRAM_HEADER_LEN, DMA_COPY);
}

/* configure the MSP430 SPI peripheral */
void InitSerialRam(void)
{
  /* assert reset when configuring */
  UCA0CTL1 = UCSWRST;
  EnableSmClkUser(SERIAL_RAM_USER);

  SRAM_SCLK_PSEL |= SRAM_SCLK_PIN;
  SRAM_SOMI_PSEL |= SRAM_SOMI_PIN;
  SRAM_SIMO_PSEL |= SRAM_SIMO_PIN;

  /* 3 pin SPI master, MSB first, clock inactive when low, phase is 1 */
  UCA0CTL0 |= UCMST + UCMSB + UCCKPH + UCSYNC;
  UCA0CTL1 |= UCSSEL__SMCLK;

  /* spi clock of 8.39 MHz (chip can run at 16 MHz max)*/
  UCA0BR0 = 0x02;
  UCA0BR1 = 0x00;

  /* release reset and wait for SPI to initialize */
  UCA0CTL1 &= ~UCSWRST;
  vTaskDelay(SPI_INIT_DELAY_IN_MS);
    /*
     * Read the status register
     */
    SRAM_CSN_ASSERT();
    while (!(UCA0IFG & UCTXIFG));

    /* writing automatically clears flag */
    UCA0TXBUF = SPI_RDSR;
    while (!(UCA0IFG&UCTXIFG));
    UCA0TXBUF = DummyData;
    while (!(UCA0IFG&UCTXIFG));
    WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE();
    ReadData = UCA0RXBUF;

    unsigned char FinalSrValue = DEFAULT_SR_VALUE;
    unsigned char DefaultSrValue = FINAL_SR_VALUE;

    if (GetBoardConfiguration() >= 2)
    {
      DefaultSrValue = DEFAULT_SR_VALUE_256;
      FinalSrValue = FINAL_SR_VALUE_256;
    }

    /* make sure correct value is read from the part */
    if (ReadData != DefaultSrValue && ReadData != FinalSrValue) PrintS("# SRAM Init1");
    SRAM_CSN_DEASSERT();

    /* put the part into sequential mode */
    SRAM_CSN_ASSERT();
    UCA0TXBUF = SPI_WRSR;
    while (!(UCA0IFG&UCTXIFG));
    UCA0TXBUF = SEQUENTIAL_MODE_COMMAND;
    while (!(UCA0IFG&UCTXIFG));
    WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE();
    SRAM_CSN_DEASSERT();

    SRAM_CSN_ASSERT();
    UCA0TXBUF = SPI_RDSR;
    while (!(UCA0IFG&UCTXIFG));

    UCA0TXBUF = DummyData;
    while (!(UCA0IFG&UCTXIFG));
    WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE();
    ReadData = UCA0RXBUF;

    /* make sure correct value is read from the part */
    if (ReadData != FinalSrValue) PrintS("# SRAM Init2");
    SRAM_CSN_DEASSERT();

  /* now use the DMA to clear the serial ram */
    SetAddr(MODE_BUF_START_ADDR);
    Write((unsigned long)&DummyData, MODE_BUFFER_SIZE - SRAM_HEADER_LEN, DMA_FILL);

  InitWidget();
  
  SramMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(SramMutex);

  DisableSmClkUser(SERIAL_RAM_USER);
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
  case 0: break;
  case 2: DmaBusy = 0; break;
  case 4: DmaBusy = 0; break;
  case 6: LcdDmaIsr(); break;
  default: break;
  }
}
