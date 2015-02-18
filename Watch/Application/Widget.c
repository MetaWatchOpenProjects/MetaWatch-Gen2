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
#include "Messages.h"
#include "DebugUart.h"
#include "DrawHandler.h" //draw_t
#include "LcdDriver.h"
#include "LcdDisplay.h"
#include "LcdBuffer.h"
#include "BitmapData.h"
#include "Property.h"
#include "Widget.h"
#include "ClockWidget.h"
#include "SerialRam.h"
#include "hal_rtc.h"

#define MAX_WIDGET_NUM          (16)
#define QUAD_NO_MASK            (0x03)
#define IDLE_PAGE_MASK          (0x30)
#define IDLE_PAGE_SHFT          (4)

#define WGT_BUF_START_ADDR      (0x80) //(1024x8-1152x7=128 (0x1B00) // 96x12x(4 + 2) = 6912

#define BUFFER_TAG_BITS         (16)
#define WGTLST_PART_INDX_MASK   (0x3)
#define WGTLST_PARTS_MASK       (0xC)
#define WGTLST_PART_INDX_SHFT   (0)
#define WGTLST_PARTS_SHFT       (2)
#define CLOCK_WIDGET_ID_RANGE   (15) // home widget: 0 - 15
#define INVERT_BIT              (BIT6)
#define CLOCK_WIDGET_BIT_SHFT   (7)

#define WGT_CHG_REMOVE          (0)
#define WGT_CHG_ADD             (1)
#define WGT_CHG_CLK_ADD         (2)
#define WGT_CHG_CLK             (3)
#define WGT_EQU_CLK             (4)
#define WGT_CHG_SETTING         (5)
#define WGT_EQU_SETTING         (6)
#define WGT_CHG_REMOVE_ADD      (7)
#define WGT_CHG_CLEAR           (8)

#define BOARDER_PATTERN_ROW     (0x66)
#define BOARDER_PATTERN_COL     (4)

extern xSemaphoreHandle SramMutex;
static unsigned char *pWgtBuf = NULL;
static unsigned char *pClkBuf = NULL;

typedef struct
{
  unsigned char Id;
  unsigned char Layout;
} WidgetList_t;

#define WIDGET_HEADER_LEN  (sizeof(WidgetList_t))

typedef struct
{
  unsigned char Id;
  unsigned char Layout;
  unsigned char Buffers[QUAD_NUM];
} Widget_t;

static Widget_t Widget[MAX_WIDGET_NUM + MAX_WIDGET_NUM];
static Widget_t *pCurrWidgetList = &Widget[MAX_WIDGET_NUM]; // point to left/right half of double sized Widget[]

typedef struct
{
  unsigned char QuadNum;
  unsigned char Step;
} Layout_t;

Layout_t const Layout[] = {{1, 0}, {2, 1}, {2, 2}, {4, 1}};

static unsigned int BufTag = 0;
///* low 4 bits: Clock widget ID; high 4 bits: to be updated if set */
//static unsigned char ClkWgtUpd[CLOCK_WIDGET_ID_RANGE + 1];

typedef struct
{
  unsigned char Id;
  unsigned char Row;
} WidgetHeader_t;

typedef struct
{
  unsigned char Page;
  unsigned char Layout[QUAD_NUM];
  unsigned int Addr[QUAD_NUM];
} QuadAddr_t;
#define QUAD_ADDR_SIZE    (sizeof(QuadAddr_t))

#define IS_CLOCK_WIDGET(_x) ((_x & CLOCK_WIDGET_BIT) >> CLOCK_WIDGET_BIT_SHFT)
#define LAYOUT_TYPE(_x) ((_x & LAYOUT_MASK) >> LAYOUT_SHFT)
#define WGTLST_INDEX(_x) ((_x & WGTLST_PART_INDX_MASK) >> WGTLST_PART_INDX_SHFT)
#define WGTLST_TOTAL(_x) ((_x & WGTLST_PARTS_MASK) >> WGTLST_PARTS_SHFT)

static void AssignWidgetBuffer(Widget_t *pWidget);
static void FreeWidgetBuffer(Widget_t *pWidget);
static unsigned int GetAddr(WidgetHeader_t *pData);
static void GetQuadAddr(QuadAddr_t *pAddr);
static unsigned char GetWidgetChange(unsigned char CurId, unsigned char CurOpt, unsigned char MsgId, unsigned char MsgOpt);
static void AllocateBuffer(unsigned char *pBuf);

static void WriteWidget(unsigned char Index);
static unsigned char WidgetIndex(unsigned char Id);
static void DrawHanzi(Draw_t *Info, unsigned char Index);

static void ConvertFaceId(WidgetList_t *pWidget);
static unsigned char OnCurrentPage(unsigned char Layout);

void SetWidgetList(tMessage *pMsg)
{
  static Widget_t *pCurrWidget = NULL; // point to Widget in current Widget[]
  static Widget_t *pNextWidget = NULL; // point to Widget in new Widget[]
  static unsigned char ClockId = INVALID_ID;

  xSemaphoreTake(SramMutex, portMAX_DELAY);

  WidgetList_t *pMsgWgtLst = (WidgetList_t *)pMsg->pBuffer;
  unsigned char WidgetNum = pMsg->Length / WIDGET_HEADER_LEN;

  unsigned char i = 0;
  PrintF(">SetWLst I:%d %s %d %s %d", WGTLST_INDEX(pMsg->Options), "T:", WGTLST_TOTAL(pMsg->Options), "Num:", WidgetNum);
  for(; i<WidgetNum; ++i) {PrintH(pMsgWgtLst[i].Id); PrintH(pMsgWgtLst[i].Layout);} PrintR();

  if (pNextWidget == NULL) // first time call, only add widgets
  {
    pCurrWidget = pCurrWidgetList;
    pNextWidget = &Widget[0];
  }
  else
  {
    if (WGTLST_INDEX(pMsg->Options) == 0 &&
      (pCurrWidget != pCurrWidgetList || (pNextWidget != &Widget[0] && pNextWidget != &Widget[MAX_WIDGET_NUM])))
    { // last SetWLst failed in the middle.Clean up whole list
      PrintS("# Last SetWgtLst broken!");

      pCurrWidget = pCurrWidgetList;
      pNextWidget = &Widget[0] + (&Widget[MAX_WIDGET_NUM] - pCurrWidgetList);
      ClearWidgetList();
      BufTag = 0;
    }
  }

  while (WidgetNum) // number of list items
  {
      /* old clock widgets */
    if (!IS_CLOCK_WIDGET(pMsgWgtLst->Layout) && pMsgWgtLst->Id <= CLOCK_WIDGET_ID_RANGE) ConvertFaceId(pMsgWgtLst);
    unsigned char Change = GetWidgetChange(pCurrWidget->Id, pCurrWidget->Layout, pMsgWgtLst->Id, pMsgWgtLst->Layout);
    PrintF("WgtChg:%u", Change);

    switch (Change)
    {
    case WGT_EQU_CLK:
    case WGT_EQU_SETTING:
      PrintF("=%02X", pCurrWidget->Id);
      *pNextWidget++ = *pCurrWidget++;
      pMsgWgtLst ++;
      WidgetNum --;
      break;

    case WGT_CHG_CLK:
      PrintS("*Clk");
      if (OnCurrentPage(pMsgWgtLst->Layout)) ClockId = pMsgWgtLst->Id;

    case WGT_CHG_SETTING:
     //cpy layout to curr; cpy curr to next; msg, curr, next ++
      PrintF("*%02X", pCurrWidget->Id);
      pCurrWidget->Id = pMsgWgtLst->Id;
      pCurrWidget->Layout = pMsgWgtLst->Layout;
      *pNextWidget++ = *pCurrWidget++;
      pMsgWgtLst ++;
      WidgetNum --;
      break;

    case WGT_CHG_REMOVE:
    case WGT_CHG_REMOVE_ADD:
    // remove widget: curr ++
      PrintF("-%02X", pCurrWidget->Id);
      if (pCurrWidget->Id != INVALID_ID)
      {
        FreeWidgetBuffer(pCurrWidget);
        pCurrWidget ++;
      }
      if (Change == WGT_CHG_REMOVE) break;

    case WGT_CHG_ADD:

      if (IS_CLOCK_WIDGET(pMsgWgtLst->Layout))
      {
        PrintS("+Clk");
        if (OnCurrentPage(pMsgWgtLst->Layout)) ClockId = pMsgWgtLst->Id;
      }

     // add new widget: cpy msg to next; msg and next ++; curr stays
      PrintF("+%02X", pMsgWgtLst->Id);

      pNextWidget->Id = pMsgWgtLst->Id;
      pNextWidget->Layout = pMsgWgtLst->Layout;
      AssignWidgetBuffer(pNextWidget);

      pNextWidget ++;
      pMsgWgtLst ++;
      WidgetNum --;
      break;

    case WGT_CHG_CLEAR:
      PrintS("-Clear");
      WidgetNum = 0;
    default: break;
    }

  }
  PrintR();

  // if part index + 1 == parts, SetWidgetList complete
  if (WGTLST_TOTAL(pMsg->Options) == WGTLST_INDEX(pMsg->Options) + 1)
  {
    while (pCurrWidget->Id != INVALID_ID && pCurrWidget < &pCurrWidgetList[MAX_WIDGET_NUM])
    {
      FreeWidgetBuffer(pCurrWidget);
      pCurrWidget->Id = INVALID_ID;
      pCurrWidget->Layout = 0;
      pCurrWidget ++;
    }

    for (i = 0; i < MAX_WIDGET_NUM; ++i)
    {
      if (pCurrWidgetList[i].Id != INVALID_ID)
      { // clear the widget id in the curr list
        pCurrWidgetList[i].Id = INVALID_ID;
        pCurrWidgetList[i].Layout = 0;
      }
    }

    pNextWidget = pCurrWidgetList;
    pCurrWidgetList = &Widget[0] + (&Widget[MAX_WIDGET_NUM] - pCurrWidgetList);
    pCurrWidget = pCurrWidgetList;

    unsigned char k;
    for (i = 0; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
    {
      unsigned QuadNum = Layout[LAYOUT_TYPE(pCurrWidgetList[i].Layout)].QuadNum;
      for (k = 0; k < QuadNum; ++k)
      {
        if (pCurrWidgetList[i].Buffers[k] == BUFFER_TAG_BITS)
          AllocateBuffer(&pCurrWidgetList[i].Buffers[k]);
      }
    }

    PrintF("Tg:%04X", BufTag);

    if (ClockId != INVALID_ID)
    {
//      PrintS("------ SetWLst UdCk");
      SendMessage(DrawClockWidgetMsg, ClockId);
      ClockId = INVALID_ID;
    }
  }
  xSemaphoreGive(SramMutex);
}

static unsigned char GetWidgetChange(unsigned char CurId, unsigned char CurOpt,
                                     unsigned char MsgId, unsigned char MsgOpt)
{
  unsigned char CurFaceId = FACE_ID(CurId);
  unsigned char MsgFaceId = FACE_ID(MsgId);
  unsigned char Change;

  if (IS_CLOCK_WIDGET(CurOpt)) CurId = CurId & 0x0F;
  if (IS_CLOCK_WIDGET(MsgOpt)) MsgId = MsgId & 0x0F;

  PrintF("CurId:%02X O:%02X MsgId:%02X O:%02X", CurId, CurOpt, MsgId, MsgOpt);

  if (CurId == INVALID_ID && MsgId == INVALID_ID) Change = WGT_CHG_CLEAR;
  else if (CurId == MsgId)
  {
    if (LAYOUT_TYPE(CurOpt) != LAYOUT_TYPE(MsgOpt))
      Change = WGT_CHG_REMOVE_ADD;

    else if (IS_CLOCK_WIDGET(CurOpt))
      Change = (CurFaceId != MsgFaceId || CurOpt != MsgOpt) ?
                WGT_CHG_CLK : WGT_EQU_CLK;
    else
      Change = (CurOpt != MsgOpt) ? WGT_CHG_SETTING : WGT_EQU_SETTING;
  }
  else if (CurId < MsgId) Change = WGT_CHG_REMOVE;
  else Change = WGT_CHG_ADD;
  
  return Change;
}

static void AssignWidgetBuffer(Widget_t *pWidget)
{
  unsigned char QuadNum = Layout[LAYOUT_TYPE(pWidget->Layout)].QuadNum;
  unsigned char i;

  for (i = 0; i < QuadNum; ++i) AllocateBuffer(&pWidget->Buffers[i]);
}

static void AllocateBuffer(unsigned char *pBuf)
{
  unsigned char Tag;
  for (Tag = 0; Tag < BUFFER_TAG_BITS; ++Tag) // go through 16 bufs
  {
    if ((BufTag & (1 << Tag)) == 0) // found empty buffer
    {
      *pBuf = Tag;
      BufTag |= 1 << Tag;

      // add "Loading..." template
      LoadBuffer(Tag, pWidgetTemplate[TMPL_WGT_LOADING]);
      break;
    }
  }

  if (Tag == BUFFER_TAG_BITS) *pBuf = BUFFER_TAG_BITS;
}

static void FreeWidgetBuffer(Widget_t *pWidget)
{
  unsigned char QuadNum = Layout[LAYOUT_TYPE(pWidget->Layout)].QuadNum;
  unsigned char i;

  for (i = 0; i < QuadNum; ++i)
  {
    BufTag &= ~(1 << pWidget->Buffers[i]);
    // add "+" template to empty buffer
    LoadBuffer(pWidget->Buffers[i], pWidgetTemplate[TMPL_WGT_EMPTY]);
  }

  PrintF("-Tg:%04X", BufTag);
}

static void ClearWidgetList(void)
{
  unsigned char i;
  for (i = 0; i < MAX_WIDGET_NUM + MAX_WIDGET_NUM; ++i)
  {
    Widget[i].Id = INVALID_ID;
    Widget[i].Layout = 0;
  }
}

unsigned char CreateDrawBuffer(unsigned char Id)
{
  if (Id > CLOCK_WIDGET_ID_RANGE && pWgtBuf ||
    (Id <= CLOCK_WIDGET_ID_RANGE && pClkBuf)) return TRUE;

  unsigned char Type = LAYOUT_TYPE(pCurrWidgetList[WidgetIndex(Id)].Layout);
  unsigned int Size = Layout[Type].QuadNum * BYTES_PER_QUAD + SRAM_HEADER_LEN;
  if (Type == LAYOUT_VERT_SCREEN) Size += BYTES_PER_QUAD;

  unsigned char *pBuffer = (unsigned char *)pvPortMalloc(Size);
  memset(pBuffer, 0, Size);
  if (Id > CLOCK_WIDGET_ID_RANGE) pWgtBuf = pBuffer;
  else pClkBuf = pBuffer;

  if (!pBuffer) PrintF("@%sBuf:%u", Id > CLOCK_WIDGET_ID_RANGE ? "Wgt" : "Clk", Size);
//  PrintF("%s %c(%04X %u", Id > CLOCK_WIDGET_ID_RANGE ? "Wgt" : "Clk", pBuffer ? PLUS : AT, pBuffer, Size);
  return pBuffer > 0;
}

void DrawWidgetToSram(unsigned char Id)
{
  PrintF("-WgtSrm:%02X", Id);

  WriteWidget(WidgetIndex(Id));

  PrintF("-%04X wgt)", pWgtBuf);
  vPortFree(pWgtBuf);
  pWgtBuf = NULL;
}

static unsigned char WidgetIndex(unsigned char Id)
{
  unsigned char i = 0;

  for (; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
  {
    if (Id <= CLOCK_WIDGET_ID_RANGE && CLOCK_ID(pCurrWidgetList[i].Id) == Id ||
     (Id > CLOCK_WIDGET_ID_RANGE && pCurrWidgetList[i].Id == Id &&
     !IS_CLOCK_WIDGET(pCurrWidgetList[i].Layout))) break;
  }
  return i;
}

static void WriteWidget(unsigned char Index)
{
  if (Index >= MAX_WIDGET_NUM) return;

  unsigned char LayoutType = LAYOUT_TYPE(pCurrWidgetList[Index].Layout);
  unsigned char *pBuf = IS_CLOCK_WIDGET(pCurrWidgetList[Index].Layout) ? pClkBuf : pWgtBuf;
  unsigned char i;

  for (i = 0; i < Layout[LayoutType].QuadNum; ++i)
  {
    unsigned int Addr = pCurrWidgetList[Index].Buffers[i] * BYTES_PER_QUAD + WGT_BUF_START_ADDR;
    pBuf[0] = SPI_WRITE;
    pBuf[1] = Addr >> 8;
    pBuf[2] = Addr;
    
    Write((unsigned long)pBuf, BYTES_PER_QUAD, DMA_COPY);

    pBuf += BYTES_PER_QUAD;
    if (LayoutType == LAYOUT_VERT_SCREEN) pBuf += BYTES_PER_QUAD;
  }
}

void DrawBitmapToIdle(Draw_t *Info, unsigned char WidthInBytes, unsigned char const *pBitmap)
{
  unsigned char *pByte = SRAM_HEADER_LEN + (Info->WidgetId > CLOCK_WIDGET_ID_RANGE ? pWgtBuf : pClkBuf);
  if (pByte == NULL)
  {
    PrintS("@ DrwBmpIdle NulBuf");
    return;
  }

  pByte += Info->Y / HALF_SCREEN_ROWS * BYTES_PER_HALF_SCREEN  +
           Info->Y % HALF_SCREEN_ROWS * BYTES_PER_QUAD_LINE +
           Info->X / HALF_SCREEN_COLS * BYTES_PER_QUAD +
           ((Info->X % HALF_SCREEN_COLS) >> 3);

  unsigned char Type = LAYOUT_TYPE(pCurrWidgetList[WidgetIndex(Info->WidgetId)].Layout);
  unsigned char DrawOp = Info->Opt & DRAW_OPT_MASK;
  unsigned char ColBit = BIT0 << (Info->X & 0x07); // dst
  unsigned char MaskBit = BIT0; // src
  unsigned int Delta;
  unsigned char Set; // src bit is set or clear
  unsigned char x, y, BorderX, BorderY;

  if (Info->X < HALF_SCREEN_COLS)
  {
    BorderX = (Type == LAYOUT_QUAD_SCREEN || Type == LAYOUT_VERT_SCREEN) ?
      HALF_SCREEN_COLS : LCD_COL_NUM;
  }
  else BorderX = LCD_COL_NUM;

  if (Info->Y < HALF_SCREEN_ROWS)
  {
    BorderY = (Type == LAYOUT_QUAD_SCREEN || Type == LAYOUT_HORI_SCREEN) ?
      HALF_SCREEN_ROWS : LCD_ROW_NUM;
  }
  else BorderY = LCD_ROW_NUM;

  for (x = 0; x < Info->Width && (Info->X + x) < BorderX; ++x)
  {
    for (y = 0; y < Info->Height && (Info->Y + y) < BorderY; ++y)
    {
      Set = *(pBitmap + (DrawOp != DRAW_OPT_FILL ? y * WidthInBytes : 0)) & MaskBit;
      Delta = (Type == LAYOUT_FULL_SCREEN || Type == LAYOUT_VERT_SCREEN) &&
              (Info->Y < HALF_SCREEN_ROWS && (Info->Y + y) >= HALF_SCREEN_ROWS) ?
              BYTES_PER_QUAD : 0;

      BitOp(pByte + y * BYTES_PER_QUAD_LINE + Delta, ColBit, Set, DrawOp);
    }

    MaskBit <<= 1;
    if (MaskBit == 0)
    {
      MaskBit = BIT0;
      if (DrawOp != DRAW_OPT_FILL) pBitmap ++;
    }
    
    ColBit <<= 1;
    if (ColBit == 0)
    {
      ColBit = BIT0;
      pByte ++;
      // check next pixel x
      if ((Info->X + x + 1) == HALF_SCREEN_COLS) pByte += BYTES_PER_QUAD - BYTES_PER_QUAD_LINE;
    }
  }

//  if (x < Info->Width || y < Info->Height)
//    PrintF("# DrwOvrBdr x:%u y:%u", Info->X + x, Info->Y + y);
}

void DrawTemplateToIdle(Draw_t *Info)
{
#if __IAR_SYSTEMS_ICC__
  const unsigned char __data20 *pTemp;
#else
  const unsigned char *pTemp;
#endif

  unsigned char *pByte = Info->WidgetId > CLOCK_WIDGET_ID_RANGE ? pWgtBuf : pClkBuf + SRAM_HEADER_LEN;
  unsigned char TempId = Info->Id & TMPL_ID_MASK;

  pTemp =  ((Info->Opt & 0x0F) == TMPL_TYPE_4Q) ? pTemplate[TempId] : pTemplate2Q[TempId];
  unsigned char RowNum = ((Info->Opt & 0x0F) == TMPL_TYPE_4Q) ? LCD_ROW_NUM : HALF_SCREEN_ROWS;

  unsigned char i, k;

  for (i = 0; i < RowNum; ++i)
  {
    if (i == HALF_SCREEN_ROWS) pByte += BYTES_PER_QUAD;

    for (k = 0; k < BYTES_PER_QUAD_LINE; ++k) pByte[k] = pTemp[k];
    for (k = 0; k < BYTES_PER_QUAD_LINE; ++k) pByte[k + BYTES_PER_QUAD] = pTemp[k + BYTES_PER_QUAD_LINE];

    pByte += BYTES_PER_QUAD_LINE;
    pTemp += BYTES_PER_LINE;
  }
}

#define CN_CLK_DIAN     12
#define CN_CLK_ZHENG    13
#define CN_CLK_FEN      29

#define CN_CLK_HOURH    0
#define CN_CLK_HOUR_SHI 1
#define CN_CLK_HOURL    2

#define CN_CLK_MINH     12
#define CN_CLK_MIN_SHI  18
#define CN_CLK_MINL     19

#define CN_CLK_ZI_WIDTH     15
#define CN_CLK_ZI_WIDTH_IN_BYTES  ((CN_CLK_ZI_WIDTH >> 3) + 1)
#define CN_CLK_ZI_HEIGHT    18
#define CN_CLK_ZI_PER_LINE  6

void DrawHanziClock(Draw_t *Info)
{
  Info->Id = DRAW_ID_TYPE_BMP;
  Info->Width = CN_CLK_ZI_WIDTH;
  Info->Height = CN_CLK_ZI_HEIGHT;
  Info->Opt |= DRAW_OPT_DST_NOT;

  unsigned char Time = RTCHOUR;
  if (!GetProperty(PROP_24H_TIME_FORMAT)) Time = To12H(Time);

  if (Time >= 0x20) DrawHanzi(Info, CN_CLK_HOURH);
  if (Time >= 0x10) DrawHanzi(Info, CN_CLK_HOUR_SHI);
  if (Time != 0x20 && Time != 0x10) DrawHanzi(Info, CN_CLK_HOURL + BCD_L(Time));

  Time = RTCMIN;
  if (Time)
  {
    if (Time >= 0x20) DrawHanzi(Info, CN_CLK_MINH + BCD_H(Time));
    if (Time >= 0x10) DrawHanzi(Info, CN_CLK_MIN_SHI);
    else DrawHanzi(Info, CN_CLK_MINL); // 0
    
    if (BCD_L(Time)) DrawHanzi(Info, CN_CLK_MINL + BCD_L(Time));
    DrawHanzi(Info, CN_CLK_FEN);
  }
  else DrawHanzi(Info, CN_CLK_ZHENG);
}

static void DrawHanzi(Draw_t *Info, unsigned char Index)
{
  Info->X = Index % CN_CLK_ZI_PER_LINE * (CN_CLK_ZI_WIDTH + 1);
  Info->Y = Index / CN_CLK_ZI_PER_LINE * (CN_CLK_ZI_HEIGHT + 1) + 1;
  DrawBitmapToIdle(Info, WIDTH_IN_BYTES(Info->Width), pClkBuf); //dst_not
}

/******************************************************************************/

void WriteWidgetBuffer(tMessage *pMsg)
{
  unsigned int Addr = GetAddr((WidgetHeader_t *)pMsg->pBuffer);
  unsigned char *pBuf = pMsg->pBuffer + WIDGET_HEADER_LEN - SRAM_HEADER_LEN;
//  PrintF("Id:%u R:%u %04X", pBuf[1], pBuf[2], Addr);
  pBuf[0] = SPI_WRITE;
  pBuf[1] = Addr >> 8;
  pBuf[2] = Addr;

  Write((unsigned long)pBuf, pMsg->Length - WIDGET_HEADER_LEN, DMA_COPY);
//  PrintQ(pBuf + 3, pMsg->Length - WIDGET_HEADER_LEN);
}

static unsigned int GetAddr(WidgetHeader_t *pData)
{
  static unsigned char i = 0;
  unsigned int Addr = 0;

  if (pData->Id != pCurrWidgetList[i].Id)
  {
    for (i = 0; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
    {
      if (pData->Id == pCurrWidgetList[i].Id &&
          !IS_CLOCK_WIDGET(pCurrWidgetList[i].Layout)) break;
    }

    if (pCurrWidgetList[i].Id == INVALID_ID || i == MAX_WIDGET_NUM)
    {
      PrintS("# wgt addr not found");
      i = 0;
      return Addr;
    }
  }
  
  //find right buffer according to row
  Addr = pCurrWidgetList[i].Buffers[pData->Row / QUAD_ROW_NUM] * BYTES_PER_QUAD +
         (pData->Row % QUAD_ROW_NUM) * BYTES_PER_QUAD_LINE + WGT_BUF_START_ADDR;

  return Addr;
}

static void GetQuadAddr(QuadAddr_t *pAddr)
{
  unsigned char i = 0;
  for (; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
  {
    if ((pCurrWidgetList[i].Layout & IDLE_PAGE_MASK) >> IDLE_PAGE_SHFT == pAddr->Page)
    {
      unsigned char Type = LAYOUT_TYPE(pCurrWidgetList[i].Layout);
      unsigned char Quad = pCurrWidgetList[i].Layout & QUAD_NO_MASK;
      unsigned char k = 0;
      //PrintS("W:"); PrintH(pCurrWidgetList[i].Id);

      while (k < Layout[Type].QuadNum)
      {
        pAddr->Addr[Quad] = pCurrWidgetList[i].Buffers[k] * BYTES_PER_QUAD + WGT_BUF_START_ADDR;
        pAddr->Layout[Quad] = pCurrWidgetList[i].Layout; //(pCurrWidgetList[i].Layout & LAYOUT_MASK) >> LAYOUT_SHFT;
        Quad += Layout[Type].Step;
        k ++;
      }
    }
  }
}

void DrawWidgetToLcd(unsigned char Page)
{
  UpdateClockWidget();

  QuadAddr_t QuadAddr;
  memset((unsigned char *)&QuadAddr, 0, QUAD_ADDR_SIZE);
  QuadAddr.Page = Page;

  GetQuadAddr(&QuadAddr);
  
  unsigned char i = 0;
  while (BufTag & (1 << i)) i ++; // find out first empty buffer
  unsigned int Addr = i * BYTES_PER_QUAD + WGT_BUF_START_ADDR;
  
  for (i = 0; i < QUAD_NUM; ++i)
  {
   if (QuadAddr.Addr[i] == 0) QuadAddr.Addr[i] = Addr;
  }

  unsigned char SramBuf[SRAM_HEADER_LEN];
  LcdReadBuffer_t *LcdBuf = (LcdReadBuffer_t *)pvPortMalloc(LCD_READ_BUFFER_SIZE);
  unsigned char Row = 0;
  i = 0; // 0 for upper Quads, 1 for lower Quads

  while (Row < LCD_ROW_NUM)
  {
    unsigned char k = 1; // 0 for left Quads, 1 for right Quads
    do
    {
      Addr = QuadAddr.Addr[i+k] + (Row % HALF_SCREEN_ROWS) * BYTES_PER_QUAD_LINE;
      
      SramBuf[0] = SPI_READ;
      SramBuf[1] = Addr >> 8;
      SramBuf[2] = Addr;

      Read(SramBuf, (unsigned char *)LcdBuf + BYTES_PER_QUAD_LINE * k, BYTES_PER_QUAD_LINE);

      unsigned char c; // Column byte number
      
      if (QuadAddr.Layout[i+k] & INVERT_BIT)
      { // Invert pixel
        for (c = 0; c < BYTES_PER_QUAD_LINE; ++c)
          LcdBuf->Line.Data[c + BYTES_PER_QUAD_LINE * k] = ~LcdBuf->Line.Data[c + BYTES_PER_QUAD_LINE * k];
      }

      if (GetProperty(PROP_WIDGET_GRID))
      {
        unsigned char LayoutType = LAYOUT_TYPE(QuadAddr.Layout[i+k]);

        if (Row % BOARDER_PATTERN_COL == 1 || Row % BOARDER_PATTERN_COL == 2)
        {// black dots
          // Rule 2: left and right side vertical boarders
//              if (k) LcdBuf.Line.Data[BYTES_PER_LINE - 1] |= 0x80; // right side boarder
//              else  LcdBuf.Line.Data[0] |= 0x01; // left side boarder

         // Rule 3 inner vertical boarders
         if (LayoutType == 0 || LayoutType == 2)
         {
          if (k) LcdBuf->Line.Data[BYTES_PER_QUAD_LINE] |= 0x01; // right inner boarder
          else  LcdBuf->Line.Data[BYTES_PER_QUAD_LINE - 1] |= 0x80; // left inner boarder
         }
        }
        else
        {// white dots
          // Rule 2: left and right side vertical boarders
//              if (k) LcdBuf.Line.Data[BYTES_PER_LINE - 1] &= 0x7F; // right side boarder
//              else  LcdBuf.Line.Data[0] &= 0xFE; // left side boarder

         // Rule 3 inner vertical boarders
         if (LayoutType == 0 || LayoutType == 2)
         {
          if (k) LcdBuf->Line.Data[BYTES_PER_QUAD_LINE] &= 0xFE; // right inner boarder
          else  LcdBuf->Line.Data[BYTES_PER_QUAD_LINE - 1] &= 0x7F; // left inner boarder
         }
        }

        // Rule 4: inner horizontal boarders
        if (LayoutType == 0 || LayoutType == 1)
        {
          if (Row == HALF_SCREEN_ROWS - 1 || Row == HALF_SCREEN_ROWS)
          {
            for (c = 0; c < BYTES_PER_QUAD_LINE; ++c)
              LcdBuf->Line.Data[c + BYTES_PER_QUAD_LINE * k] = BOARDER_PATTERN_ROW;
          }
        }
      }
    }  while (k--);

    LcdBuf->Line.Row = Row ++; // Lcd row number starts from 1

    WriteToLcd(&LcdBuf->Line, 1);
    if (Row == HALF_SCREEN_ROWS) i += 2;
  }

  vPortFree(LcdBuf);
}

void DrawStatusBarToWidget(void)
{
  unsigned char i;
  for (i = 0; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
  {
    if (OnCurrentPage(pCurrWidgetList[i].Layout) &&
        LAYOUT_TYPE(pCurrWidgetList[i].Layout) == LAYOUT_FULL_SCREEN &&
        !IS_CLOCK_WIDGET(pCurrWidgetList[i].Layout))
    {
      DrawStatusBarToLcd();
      break;
    }
  }
}

void InitWidget(void)
{
  ClearWidgetList();

  // load 16 buffers with empty widget template
  unsigned char i = 0;
  for (; i < MAX_WIDGET_NUM; ++i) LoadBuffer(i, pWidgetTemplate[TMPL_WGT_EMPTY]);
}

/******************************************************************************/

unsigned char UpdateClockWidget(void)
{
  unsigned char Updated = FALSE;
  unsigned char i = 0;

  for (; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
  {
    if (!IS_CLOCK_WIDGET(pCurrWidgetList[i].Layout)) continue;

    if (OnCurrentPage(pCurrWidgetList[i].Layout))
    {
      DrawClockWidget(pCurrWidgetList[i].Id);
      Updated = TRUE;
    }
  }
  return Updated;
}

// FaceId???
void DrawClockToSram(unsigned char Id)
{
  unsigned char i;

//  PrintF("-ClkSrm:%02X", Id);
  for (i = 0; pCurrWidgetList[i].Id != INVALID_ID && i < MAX_WIDGET_NUM; ++i)
  {
    if (!IS_CLOCK_WIDGET(pCurrWidgetList[i].Layout)) continue;
    
    if (OnCurrentPage(pCurrWidgetList[i].Layout))
    {
      if (CLOCK_ID(pCurrWidgetList[i].Id) == Id)
      {
        WriteWidget(i);
        break;
      }
    }
  }

//  PrintF("-%04X clk)", pClkBuf);
  vPortFree(pClkBuf);
  pClkBuf = NULL;
}

static unsigned char OnCurrentPage(unsigned char Layout)
{
  return ((Layout & IDLE_PAGE_MASK) >> IDLE_PAGE_SHFT) == CurrentIdleScreen();
}

static void ConvertFaceId(WidgetList_t *pWidget)
{
  PrintF("Clk BF: Id:0x%02X Layout:0x%02X", pWidget->Id, pWidget->Layout);
  
  // copy layout type to faceId,
  pWidget->Id |= LAYOUT_TYPE(pWidget->Layout) << 4;

  // set clock widget bit
  pWidget->Layout |= CLOCK_WIDGET_BIT;
  PrintF("Clk AF: Id:0x%02X Layout:0x%02X", pWidget->Id, pWidget->Layout);
}
