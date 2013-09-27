//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#ifndef DRAW_MSG_HANDLER_H
#define DRAW_MSG_HANDLER_H

typedef struct
{
  unsigned char Id; // e.g. FontId, IconId, etc.;
  unsigned char X; // starting point in pixels
  unsigned char Y;
  unsigned char Opt; // bitwise OR/SET/NOT; overlap
  unsigned char Width; //
  unsigned char Height; //
} Draw_t;

#define FUNC_GET_TIME        (1 << 4)
#define FUNC_GET_DATE        (2 << 4)
#define FUNC_GET_SEC         (3 << 4)
#define FUNC_GET_DOW         (4 << 4)
#define FUNC_GET_HOUR        (5 << 4)
#define FUNC_GET_MIN         (6 << 4)
#define FUNC_GET_AMPM        (7 << 4)

#define FUNC_GET_BT_STATE    (1 << 4)
#define FUNC_GET_BATTERY     (2 << 4)
#define FUNC_DRAW_TEMPLATE   (3 << 4)
#define FUNC_DRAW_ICON       (4 << 4)

#define FUNC_DRAW_BLOCK      (5 << 4)
#define FUNC_DRAW_HANZI      (6 << 4)

#define DRAW_ID_TYPE_FONT     (0)
#define DRAW_ID_TYPE_BMP      (0x80)
#define DRAW_ID_TYPE          (0x80)
#define DRAW_ID_BMP_MASK      (0x7F)

#define DRAW_ID_SUB_TYPE      (0x70)
#define DRAW_ID_SUB_ID        (0x0F)

#define DRAW_MODE             (0xC0)
#define DRAW_MSG_BEGIN        (0x20)
#define DRAW_MSG_END          (0x10)

#define WIDTH_IN_BYTES(_x) (_x % 8 ? (_x >> 3) + 1 : _x >> 3)

#define TMPL_ID_MASK                  (0x0F)
#define TMPL_TYPE_4Q                  (0)
#define TMPL_TYPE_2Q                  (1)

#define DRAW_OPT_OVERLAP_BT           (0x04)
//#define DRAW_OPT_OVERLAP_BATTERY      (0x08)
#define DRAW_OPT_OVERLAP_SEC          (0x08)
#define DRAW_OPT_OVERLAP_MASK         (0x0C)

#define DRAW_FONT_WIDTH_MASK          (0x3F)
#define DRAW_WIDTH_IN_BYTES           (0xC0)

void DrawMsgHandler(tMessage *pMsg);
void Draw(Draw_t *Info, unsigned char const *pData, unsigned char Mode);

#endif // DRAW_MSG_HANDLER_H

