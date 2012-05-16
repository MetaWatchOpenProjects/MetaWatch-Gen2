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
/*! \file OledFonts.c
*
*/
/******************************************************************************/
#include "FreeRTOS.h"
#include "Fonts.h"
#include "OledFonts.h"
#include "DebugUart.h"

#define PRINTABLE_CHARACTERS ( 95 )

const unsigned char MetaWatch5tableOled[];
const unsigned char MetaWatch7tableOled[];
const unsigned int MetaWatch16tableOled[];
const unsigned int MetaWatchIconTableOled[];

const unsigned char MetaWatch5widthOled[PRINTABLE_CHARACTERS];
const unsigned char MetaWatch7widthOled[PRINTABLE_CHARACTERS];
const unsigned char MetaWatch16widthOled[PRINTABLE_CHARACTERS];
const unsigned char MetaWatchIconWidthOled[TOTAL_OLED_ICONS];

const unsigned int MetaWatch5offsetOled[PRINTABLE_CHARACTERS];
const unsigned int MetaWatch7offsetOled[PRINTABLE_CHARACTERS];
const unsigned int MetaWatch16offsetOled[PRINTABLE_CHARACTERS];
const unsigned int MetaWatchIconOffsetOled[TOTAL_OLED_ICONS];

/*! Font Type structure 
 *
 * \param Type is the type of font
 * \param Height is the height in pixels of the font
 * \param Spacing is the default spacing between characters in pixels
 *
 * \note fonts are stored without horizontal spacing 
 */
typedef struct
{
  etFontType Type;
  unsigned char Height;
  unsigned char Spacing;
  
} tOledFont;

static tOledFont CurrentFont;

void SetFont(etFontType Type)
{
  switch (Type)
  {
  case MetaWatch5Oled:
    CurrentFont.Type = Type;
    CurrentFont.Height = 5;
    CurrentFont.Spacing = 1;
    break;
  
  case MetaWatch7Oled:
    CurrentFont.Type = Type;
    CurrentFont.Height = 7;
    CurrentFont.Spacing = 1;
    break;
  
  case MetaWatch16Oled:
    CurrentFont.Type = Type;
    CurrentFont.Height = 16;
    CurrentFont.Spacing = 1;
    break;
    
  case MetaWatchIconOled:
    CurrentFont.Type = Type;
    CurrentFont.Height = 16;
    CurrentFont.Spacing = 2;
    break;
    
  default:
    PrintString("Undefined Font Selected\r\n");
    break;
  }  
}

unsigned char MapDigitToIndex(unsigned char Digit)
{
  /* default is a space (the first printable character) */
  unsigned char Result = 0;

  if ( Digit < 10 )
  {
    Result = Digit + 0x10;  
  }
  
  return Result;
  
}


unsigned char GetCharacterWidth(unsigned char Character)
{ 
  unsigned char index = MapCharacterToIndex(Character);
  unsigned char Width = 0;
  
  switch (CurrentFont.Type)
  {
  case MetaWatch5Oled:    Width = MetaWatch5widthOled[index];    break;
  case MetaWatch7Oled:    Width = MetaWatch7widthOled[index];    break;
  case MetaWatch16Oled:   Width = MetaWatch16widthOled[index];   break;
  case MetaWatchIconOled: Width = MetaWatchIconWidthOled[index]; break;
  default : 
    break;
  }
  
  return Width;
  
}

static unsigned int GetCharacterOffset(unsigned char Character)
{ 
  unsigned char index = MapCharacterToIndex(Character);
  unsigned int Offset = 0;
  
  switch (CurrentFont.Type)
  {
  case MetaWatch5Oled:    Offset = MetaWatch5offsetOled[index];      break;
  case MetaWatch7Oled:    Offset = MetaWatch7offsetOled[index];      break;
  /* the offsets are in bytes */
  case MetaWatch16Oled:   Offset = MetaWatch16offsetOled[index]/2;   break;
  case MetaWatchIconOled: Offset = MetaWatchIconOffsetOled[index]/2; break;
  
  default : 
    break;
  }
  
  return Offset;
  
}

unsigned char GetCharacterHeight(void)
{
  return CurrentFont.Height;  
}

void SetFontSpacing(unsigned char Spacing)
{
  CurrentFont.Spacing = Spacing;  
}

unsigned char GetFontSpacing(void)
{
  return CurrentFont.Spacing;  
}

unsigned char MapCharacterToIndex(unsigned char CharIn)
{
  unsigned char Result = 0;

  switch (CurrentFont.Type)
  {

  case MetaWatchIconOled:
    Result = CharIn;
    break;
    
  default : 
    // space = 0x20 and 0x7f = delete character
    if ( (CharIn >= 0x20) && (CharIn < 0x7f) )
    {
      Result = CharIn - 0x20;
    }
    break;
  }
  
  
  return Result;
  
}

void GetCharacterBitmap(unsigned char Character,unsigned int * pBitmap)
{
  unsigned char width = GetCharacterWidth(Character);
  unsigned int offset = GetCharacterOffset(Character);
  
  unsigned char col;
  for (col = 0; col < width; col++ )
  {
    switch (CurrentFont.Type)
    {
    case MetaWatch5Oled:
      /* right shift 5 pixel font because they are top justified in
       * bit font creator */
      pBitmap[col] = (unsigned int)(MetaWatch5tableOled[offset+col] >> 2);
      break;
  
    case MetaWatch7Oled:
      pBitmap[col] = (unsigned int)MetaWatch7tableOled[offset+col];  
      break;
  
    case MetaWatch16Oled:
      pBitmap[col] = MetaWatch16tableOled[offset+col];  
      break;
  
    case MetaWatchIconOled:
      pBitmap[col] = MetaWatchIconTableOled[offset+col];
      
    default:
      break;
    }
  
  }

}

const unsigned char MetaWatch5tableOled[] = 
{

/* character 0x20 (' '): (width=2, offset=0) */
0x00, 0x00, 

/* character 0x21 ('!'): (width=1, offset=2) */
0xE8, 

/* character 0x22 ('"'): (width=3, offset=3) */
0xC0, 0x00, 0xC0, 

/* character 0x23 ('#'): (width=5, offset=6) */
0x50, 0xF8, 0x50, 0xF8, 0x50, 

/* character 0x24 ('$'): (width=2, offset=11) */
0x00, 0x00, 

/* character 0x25 ('%'): (width=2, offset=13) */
0x00, 0x00, 

/* character 0x26 ('&'): (width=5, offset=15) */
0x50, 0xA8, 0x68, 0x18, 0x28, 

/* character 0x27 ('''): (width=1, offset=20) */
0xC0, 

/* character 0x28 ('('): (width=2, offset=21) */
0x70, 0x88, 

/* character 0x29 (')'): (width=2, offset=23) */
0x88, 0x70, 

/* character 0x2A ('*'): (width=5, offset=25) */
0x20, 0xA8, 0x70, 0xA8, 0x20, 

/* character 0x2B ('+'): (width=5, offset=30) */
0x20, 0x20, 0xF8, 0x20, 0x20, 

/* character 0x2C (','): (width=1, offset=35) */
0x18, 

/* character 0x2D ('-'): (width=3, offset=36) */
0x20, 0x20, 0x20, 

/* character 0x2E ('.'): (width=1, offset=39) */
0x08, 

/* character 0x2F ('/'): (width=5, offset=40) */
0x08, 0x10, 0x20, 0x40, 0x80, 

/* character 0x30 ('0'): (width=4, offset=45) */
0x70, 0x88, 0x88, 0x70, 

/* character 0x31 ('1'): (width=3, offset=49) */
0x88, 0xF8, 0x08, 

/* character 0x32 ('2'): (width=4, offset=52) */
0x48, 0x98, 0xA8, 0x48, 

/* character 0x33 ('3'): (width=4, offset=56) */
0x88, 0xA8, 0xA8, 0xD0, 

/* character 0x34 ('4'): (width=4, offset=60) */
0x30, 0x50, 0xF8, 0x10, 

/* character 0x35 ('5'): (width=4, offset=64) */
0xE8, 0xA8, 0xA8, 0xB0, 

/* character 0x36 ('6'): (width=4, offset=68) */
0x70, 0xA8, 0xA8, 0x10, 

/* character 0x37 ('7'): (width=4, offset=72) */
0x80, 0x98, 0xA0, 0xC0, 

/* character 0x38 ('8'): (width=4, offset=76) */
0x50, 0xA8, 0xA8, 0x50, 

/* character 0x39 ('9'): (width=4, offset=80) */
0x40, 0xA8, 0xA8, 0x70, 

/* character 0x3A (':'): (width=1, offset=84) */
0x50, 

/* character 0x3B (';'): (width=2, offset=85) */
0x08, 0x50, 

/* character 0x3C ('<'): (width=3, offset=87) */
0x20, 0x50, 0x88, 

/* character 0x3D ('='): (width=4, offset=90) */
0x50, 0x50, 0x50, 0x50, 

/* character 0x3E ('>'): (width=3, offset=94) */
0x88, 0x50, 0x20, 

/* character 0x3F ('?'): (width=3, offset=97) */
0x80, 0xA8, 0x40, 

/* character 0x40 ('@'): (width=2, offset=100) */
0x00, 0x00, 

/* character 0x41 ('A'): (width=5, offset=102) */
0x08, 0x30, 0xD0, 0x30, 0x08, 

/* character 0x42 ('B'): (width=4, offset=107) */
0xF8, 0xA8, 0xA8, 0x50, 

/* character 0x43 ('C'): (width=4, offset=111) */
0x70, 0x88, 0x88, 0x50, 

/* character 0x44 ('D'): (width=4, offset=115) */
0xF8, 0x88, 0x88, 0x70, 

/* character 0x45 ('E'): (width=4, offset=119) */
0xF8, 0xA8, 0xA8, 0x88, 

/* character 0x46 ('F'): (width=4, offset=123) */
0xF8, 0xA0, 0xA0, 0x80, 

/* character 0x47 ('G'): (width=4, offset=127) */
0x70, 0x88, 0xA8, 0x30, 

/* character 0x48 ('H'): (width=4, offset=131) */
0xF8, 0x20, 0x20, 0xF8, 

/* character 0x49 ('I'): (width=3, offset=135) */
0x88, 0xF8, 0x88, 

/* character 0x4A ('J'): (width=4, offset=138) */
0x10, 0x08, 0x08, 0xF0, 

/* character 0x4B ('K'): (width=4, offset=142) */
0xF8, 0x20, 0x50, 0x88, 

/* character 0x4C ('L'): (width=4, offset=146) */
0xF8, 0x08, 0x08, 0x08, 

/* character 0x4D ('M'): (width=5, offset=150) */
0xF8, 0x40, 0x20, 0x40, 0xF8, 

/* character 0x4E ('N'): (width=5, offset=155) */
0xF8, 0x40, 0x20, 0x10, 0xF8, 

/* character 0x4F ('O'): (width=4, offset=160) */
0x70, 0x88, 0x88, 0x70, 

/* character 0x50 ('P'): (width=4, offset=164) */
0xF8, 0xA0, 0xA0, 0x40, 

/* character 0x51 ('Q'): (width=5, offset=168) */
0x70, 0x88, 0x88, 0x78, 0x08, 

/* character 0x52 ('R'): (width=4, offset=173) */
0xF8, 0xA0, 0xA0, 0x58, 

/* character 0x53 ('S'): (width=4, offset=177) */
0x48, 0xA8, 0xA8, 0x90, 

/* character 0x54 ('T'): (width=3, offset=181) */
0x80, 0xF8, 0x80, 

/* character 0x55 ('U'): (width=4, offset=184) */
0xF0, 0x08, 0x08, 0xF0, 

/* character 0x56 ('V'): (width=5, offset=188) */
0x80, 0x60, 0x18, 0x60, 0x80, 

/* character 0x57 ('W'): (width=5, offset=193) */
0xC0, 0x38, 0xC0, 0x38, 0xC0, 

/* character 0x58 ('X'): (width=4, offset=198) */
0xD8, 0x20, 0x20, 0xD8, 

/* character 0x59 ('Y'): (width=5, offset=202) */
0x80, 0x40, 0x38, 0x40, 0x80, 

/* character 0x5A ('Z'): (width=4, offset=207) */
0x98, 0xA8, 0xC8, 0x88, 

/* character 0x5B ('['): (width=2, offset=211) */
0xF8, 0x88, 

/* character 0x5C ('\'): (width=5, offset=213) */
0x80, 0x40, 0x20, 0x10, 0x08, 

/* character 0x5D (']'): (width=2, offset=218) */
0x88, 0xF8, 

/* character 0x5E ('^'): (width=5, offset=220) */
0x20, 0x40, 0x80, 0x40, 0x20, 

/* character 0x5F ('_'): (width=4, offset=225) */
0x08, 0x08, 0x08, 0x08, 

/* character 0x60 ('`'): (width=1, offset=229) */
0xC0, 

/* character 0x61 ('a'): (width=5, offset=230) */
0x08, 0x30, 0xD0, 0x30, 0x08, 

/* character 0x62 ('b'): (width=4, offset=235) */
0xF8, 0xA8, 0xA8, 0x50, 

/* character 0x63 ('c'): (width=4, offset=239) */
0x70, 0x88, 0x88, 0x50, 

/* character 0x64 ('d'): (width=4, offset=243) */
0xF8, 0x88, 0x88, 0x70, 

/* character 0x65 ('e'): (width=4, offset=247) */
0xF8, 0xA8, 0xA8, 0x88, 

/* character 0x66 ('f'): (width=4, offset=251) */
0xF8, 0xA0, 0xA0, 0x80, 

/* character 0x67 ('g'): (width=4, offset=255) */
0x70, 0x88, 0xA8, 0x30, 

/* character 0x68 ('h'): (width=4, offset=259) */
0xF8, 0x20, 0x20, 0xF8, 

/* character 0x69 ('i'): (width=3, offset=263) */
0x88, 0xF8, 0x88, 

/* character 0x6A ('j'): (width=4, offset=266) */
0x10, 0x08, 0x08, 0xF0, 

/* character 0x6B ('k'): (width=4, offset=270) */
0xF8, 0x20, 0x50, 0x88, 

/* character 0x6C ('l'): (width=4, offset=274) */
0xF8, 0x08, 0x08, 0x08, 

/* character 0x6D ('m'): (width=5, offset=278) */
0xF8, 0x40, 0x20, 0x40, 0xF8, 

/* character 0x6E ('n'): (width=5, offset=283) */
0xF8, 0x40, 0x20, 0x10, 0xF8, 

/* character 0x6F ('o'): (width=4, offset=288) */
0x70, 0x88, 0x88, 0x70, 

/* character 0x70 ('p'): (width=4, offset=292) */
0xF8, 0xA0, 0xA0, 0x40, 

/* character 0x71 ('q'): (width=5, offset=296) */
0x70, 0x88, 0x88, 0x78, 0x08, 

/* character 0x72 ('r'): (width=4, offset=301) */
0xF8, 0xA0, 0xA0, 0x58, 

/* character 0x73 ('s'): (width=4, offset=305) */
0x48, 0xA8, 0xA8, 0x90, 

/* character 0x74 ('t'): (width=3, offset=309) */
0x80, 0xF8, 0x80, 

/* character 0x75 ('u'): (width=4, offset=312) */
0xF0, 0x08, 0x08, 0xF0, 

/* character 0x76 ('v'): (width=5, offset=316) */
0x80, 0x60, 0x18, 0x60, 0x80, 

/* character 0x77 ('w'): (width=5, offset=321) */
0xC0, 0x38, 0xC0, 0x38, 0xC0, 

/* character 0x78 ('x'): (width=4, offset=326) */
0xD8, 0x20, 0x20, 0xD8, 

/* character 0x79 ('y'): (width=5, offset=330) */
0x80, 0x40, 0x38, 0x40, 0x80, 

/* character 0x7A ('z'): (width=4, offset=335) */
0x98, 0xA8, 0xC8, 0x88, 

/* character 0x7B ('{'): (width=2, offset=339) */
0x70, 0x88, 

/* character 0x7C ('|'): (width=1, offset=341) */
0xF8, 

/* character 0x7D ('}'): (width=2, offset=342) */
0x88, 0x70, 

/* character 0x7E ('~'): (width=5, offset=344) */
0x00, 0x00, 0x00, 0x00, 0x00, 
};

const unsigned char MetaWatch5widthOled[PRINTABLE_CHARACTERS] = 
{
/*		width    char    hexcode */
/*		=====    ====    ======= */
  		  2, /*          20      */
  		  1, /*   !      21      */
  		  3, /*   "      22      */
  		  5, /*   #      23      */
  		  2, /*   $      24      */
  		  2, /*   %      25      */
  		  5, /*   &      26      */
  		  1, /*   '      27      */
  		  2, /*   (      28      */
  		  2, /*   )      29      */
  		  5, /*   *      2A      */
  		  5, /*   +      2B      */
  		  1, /*   ,      2C      */
  		  3, /*   -      2D      */
  		  1, /*   .      2E      */
  		  5, /*   /      2F      */
  		  4, /*   0      30      */
  		  3, /*   1      31      */
  		  4, /*   2      32      */
  		  4, /*   3      33      */
  		  4, /*   4      34      */
  		  4, /*   5      35      */
  		  4, /*   6      36      */
  		  4, /*   7      37      */
  		  4, /*   8      38      */
  		  4, /*   9      39      */
  		  1, /*   :      3A      */
  		  2, /*   ;      3B      */
  		  3, /*   <      3C      */
  		  4, /*   =      3D      */
  		  3, /*   >      3E      */
  		  3, /*   ?      3F      */
  		  2, /*   @      40      */
  		  5, /*   A      41      */
  		  4, /*   B      42      */
  		  4, /*   C      43      */
  		  4, /*   D      44      */
  		  4, /*   E      45      */
  		  4, /*   F      46      */
  		  4, /*   G      47      */
  		  4, /*   H      48      */
  		  3, /*   I      49      */
  		  4, /*   J      4A      */
  		  4, /*   K      4B      */
  		  4, /*   L      4C      */
  		  5, /*   M      4D      */
  		  5, /*   N      4E      */
  		  4, /*   O      4F      */
  		  4, /*   P      50      */
  		  5, /*   Q      51      */
  		  4, /*   R      52      */
  		  4, /*   S      53      */
  		  3, /*   T      54      */
  		  4, /*   U      55      */
  		  5, /*   V      56      */
  		  5, /*   W      57      */
  		  4, /*   X      58      */
  		  5, /*   Y      59      */
  		  4, /*   Z      5A      */
  		  2, /*   [      5B      */
  		  5, /*   \      5C      */
  		  2, /*   ]      5D      */
  		  5, /*   ^      5E      */
  		  4, /*   _      5F      */
  		  1, /*   `      60      */
  		  5, /*   a      61      */
  		  4, /*   b      62      */
  		  4, /*   c      63      */
  		  4, /*   d      64      */
  		  4, /*   e      65      */
  		  4, /*   f      66      */
  		  4, /*   g      67      */
  		  4, /*   h      68      */
  		  3, /*   i      69      */
  		  4, /*   j      6A      */
  		  4, /*   k      6B      */
  		  4, /*   l      6C      */
  		  5, /*   m      6D      */
  		  5, /*   n      6E      */
  		  4, /*   o      6F      */
  		  4, /*   p      70      */
  		  5, /*   q      71      */
  		  4, /*   r      72      */
  		  4, /*   s      73      */
  		  3, /*   t      74      */
  		  4, /*   u      75      */
  		  5, /*   v      76      */
  		  5, /*   w      77      */
  		  4, /*   x      78      */
  		  5, /*   y      79      */
  		  4, /*   z      7A      */
  		  2, /*   {      7B      */
  		  1, /*   |      7C      */
  		  2, /*   }      7D      */
  		  5, /*   ~      7E      */
};

const unsigned int MetaWatch5offsetOled[PRINTABLE_CHARACTERS] =
{
/*		offset    char    hexcode */
/*		======    ====    ======= */
  		     0, /*          20      */
  		     2, /*   !      21      */
  		     3, /*   "      22      */
  		     6, /*   #      23      */
  		    11, /*   $      24      */
  		    13, /*   %      25      */
  		    15, /*   &      26      */
  		    20, /*   '      27      */
  		    21, /*   (      28      */
  		    23, /*   )      29      */
  		    25, /*   *      2A      */
  		    30, /*   +      2B      */
  		    35, /*   ,      2C      */
  		    36, /*   -      2D      */
  		    39, /*   .      2E      */
  		    40, /*   /      2F      */
  		    45, /*   0      30      */
  		    49, /*   1      31      */
  		    52, /*   2      32      */
  		    56, /*   3      33      */
  		    60, /*   4      34      */
  		    64, /*   5      35      */
  		    68, /*   6      36      */
  		    72, /*   7      37      */
  		    76, /*   8      38      */
  		    80, /*   9      39      */
  		    84, /*   :      3A      */
  		    85, /*   ;      3B      */
  		    87, /*   <      3C      */
  		    90, /*   =      3D      */
  		    94, /*   >      3E      */
  		    97, /*   ?      3F      */
  		   100, /*   @      40      */
  		   102, /*   A      41      */
  		   107, /*   B      42      */
  		   111, /*   C      43      */
  		   115, /*   D      44      */
  		   119, /*   E      45      */
  		   123, /*   F      46      */
  		   127, /*   G      47      */
  		   131, /*   H      48      */
  		   135, /*   I      49      */
  		   138, /*   J      4A      */
  		   142, /*   K      4B      */
  		   146, /*   L      4C      */
  		   150, /*   M      4D      */
  		   155, /*   N      4E      */
  		   160, /*   O      4F      */
  		   164, /*   P      50      */
  		   168, /*   Q      51      */
  		   173, /*   R      52      */
  		   177, /*   S      53      */
  		   181, /*   T      54      */
  		   184, /*   U      55      */
  		   188, /*   V      56      */
  		   193, /*   W      57      */
  		   198, /*   X      58      */
  		   202, /*   Y      59      */
  		   207, /*   Z      5A      */
  		   211, /*   [      5B      */
  		   213, /*   \      5C      */
  		   218, /*   ]      5D      */
  		   220, /*   ^      5E      */
  		   225, /*   _      5F      */
  		   229, /*   `      60      */
  		   230, /*   a      61      */
  		   235, /*   b      62      */
  		   239, /*   c      63      */
  		   243, /*   d      64      */
  		   247, /*   e      65      */
  		   251, /*   f      66      */
  		   255, /*   g      67      */
  		   259, /*   h      68      */
  		   263, /*   i      69      */
  		   266, /*   j      6A      */
  		   270, /*   k      6B      */
  		   274, /*   l      6C      */
  		   278, /*   m      6D      */
  		   283, /*   n      6E      */
  		   288, /*   o      6F      */
  		   292, /*   p      70      */
  		   296, /*   q      71      */
  		   301, /*   r      72      */
  		   305, /*   s      73      */
  		   309, /*   t      74      */
  		   312, /*   u      75      */
  		   316, /*   v      76      */
  		   321, /*   w      77      */
  		   326, /*   x      78      */
  		   330, /*   y      79      */
  		   335, /*   z      7A      */
  		   339, /*   {      7B      */
  		   341, /*   |      7C      */
  		   342, /*   }      7D      */
  		   344, /*   ~      7E      */  
};

const unsigned char MetaWatch7tableOled[] = 
{

/* character 0x20 (' '): (width=2, offset=0) */
0x00, 0x00, 

/* character 0x21 ('!'): (width=1, offset=2) */
0xFA, 

/* character 0x22 ('"'): (width=3, offset=3) */
0xC0, 0x00, 0xC0, 

/* character 0x23 ('#'): (width=7, offset=6) */
0x08, 0x2C, 0x38, 0x6C, 0x38, 0x68, 0x20, 

/* character 0x24 ('$'): (width=5, offset=13) */
0x24, 0x54, 0xFE, 0x54, 0x48, 

/* character 0x25 ('%'): (width=7, offset=18) */
0x62, 0x94, 0x68, 0x10, 0x2C, 0x52, 0x8C, 

/* character 0x26 ('&'): (width=5, offset=25) */
0x6C, 0x92, 0x6A, 0x04, 0x0A, 

/* character 0x27 ('''): (width=3, offset=30) */
0xC0, 0x00, 0xC0, 

/* character 0x28 ('('): (width=3, offset=33) */
0x38, 0x44, 0x82, 

/* character 0x29 (')'): (width=3, offset=36) */
0x82, 0x44, 0x38, 

/* character 0x2A ('*'): (width=7, offset=39) */
0x10, 0x54, 0x38, 0xFE, 0x38, 0x54, 0x10, 

/* character 0x2B ('+'): (width=5, offset=46) */
0x10, 0x10, 0x7C, 0x10, 0x10, 

/* character 0x2C (','): (width=1, offset=51) */
0x06, 

/* character 0x2D ('-'): (width=4, offset=52) */
0x10, 0x10, 0x10, 0x10, 

/* character 0x2E ('.'): (width=1, offset=56) */
0x02, 

/* character 0x2F ('/'): (width=4, offset=57) */
0x06, 0x18, 0x30, 0xC0, 

/* character 0x30 ('0'): (width=4, offset=61) */
0x7C, 0x82, 0x82, 0x7C, 

/* character 0x31 ('1'): (width=2, offset=65) */
0x40, 0xFE, 

/* character 0x32 ('2'): (width=4, offset=67) */
0x46, 0x8A, 0x92, 0x62, 

/* character 0x33 ('3'): (width=4, offset=71) */
0x44, 0x92, 0x92, 0x6C, 

/* character 0x34 ('4'): (width=5, offset=75) */
0x18, 0x28, 0xC8, 0x3E, 0x08, 

/* character 0x35 ('5'): (width=4, offset=80) */
0xE4, 0xA2, 0xA2, 0x9C, 

/* character 0x36 ('6'): (width=4, offset=84) */
0x7C, 0xA2, 0xA2, 0x1C, 

/* character 0x37 ('7'): (width=4, offset=88) */
0x80, 0x8E, 0xB0, 0xC0, 

/* character 0x38 ('8'): (width=4, offset=92) */
0x6C, 0x92, 0x92, 0x6C, 

/* character 0x39 ('9'): (width=4, offset=96) */
0x70, 0x8A, 0x8A, 0x7C, 

/* character 0x3A (':'): (width=1, offset=100) */
0x28, 

/* character 0x3B (';'): (width=1, offset=101) */
0x16, 

/* character 0x3C ('<'): (width=3, offset=102) */
0x10, 0x28, 0x44, 

/* character 0x3D ('='): (width=4, offset=105) */
0x28, 0x28, 0x28, 0x28, 

/* character 0x3E ('>'): (width=3, offset=109) */
0x44, 0x28, 0x10, 

/* character 0x3F ('?'): (width=4, offset=112) */
0x80, 0x9A, 0xA0, 0x40, 

/* character 0x40 ('@'): (width=7, offset=116) */
0x38, 0x44, 0x92, 0xAA, 0xBA, 0x8A, 0x70, 

/* character 0x41 ('A'): (width=7, offset=123) */
0x02, 0x0C, 0x38, 0xC8, 0x38, 0x0C, 0x02, 

/* character 0x42 ('B'): (width=5, offset=130) */
0xFE, 0x92, 0x92, 0x92, 0x6C, 

/* character 0x43 ('C'): (width=5, offset=135) */
0x38, 0x44, 0x82, 0x82, 0x44, 

/* character 0x44 ('D'): (width=5, offset=140) */
0xFE, 0x82, 0x82, 0x44, 0x38, 

/* character 0x45 ('E'): (width=4, offset=145) */
0xFE, 0x92, 0x92, 0x82, 

/* character 0x46 ('F'): (width=4, offset=149) */
0xFE, 0x90, 0x90, 0x80, 

/* character 0x47 ('G'): (width=6, offset=153) */
0x38, 0x44, 0x82, 0x92, 0x54, 0x18, 

/* character 0x48 ('H'): (width=5, offset=159) */
0xFE, 0x10, 0x10, 0x10, 0xFE, 

/* character 0x49 ('I'): (width=3, offset=164) */
0x82, 0xFE, 0x82, 

/* character 0x4A ('J'): (width=5, offset=167) */
0x04, 0x02, 0x02, 0x02, 0xFC, 

/* character 0x4B ('K'): (width=5, offset=172) */
0xFE, 0x10, 0x28, 0x44, 0x82, 

/* character 0x4C ('L'): (width=4, offset=177) */
0xFE, 0x02, 0x02, 0x02, 

/* character 0x4D ('M'): (width=7, offset=181) */
0xFE, 0x60, 0x18, 0x06, 0x18, 0x60, 0xFE, 

/* character 0x4E ('N'): (width=6, offset=188) */
0xFE, 0x40, 0x30, 0x18, 0x04, 0xFE, 

/* character 0x4F ('O'): (width=6, offset=194) */
0x38, 0x44, 0x82, 0x82, 0x44, 0x38, 

/* character 0x50 ('P'): (width=4, offset=200) */
0xFE, 0x90, 0x90, 0x60, 

/* character 0x51 ('Q'): (width=7, offset=204) */
0x38, 0x44, 0x82, 0x82, 0x44, 0x3A, 0x02, 

/* character 0x52 ('R'): (width=5, offset=211) */
0xFE, 0x90, 0x90, 0x98, 0x66, 

/* character 0x53 ('S'): (width=4, offset=216) */
0x64, 0x92, 0x92, 0x4C, 

/* character 0x54 ('T'): (width=5, offset=220) */
0x80, 0x80, 0xFE, 0x80, 0x80, 

/* character 0x55 ('U'): (width=5, offset=225) */
0xFC, 0x02, 0x02, 0x02, 0xFC, 

/* character 0x56 ('V'): (width=7, offset=230) */
0x80, 0x60, 0x18, 0x06, 0x18, 0x60, 0x80, 

/* character 0x57 ('W'): (width=7, offset=237) */
0xF8, 0x06, 0x18, 0xE0, 0x18, 0x06, 0xF8, 

/* character 0x58 ('X'): (width=5, offset=244) */
0xC6, 0x6C, 0x10, 0x6C, 0xC6, 

/* character 0x59 ('Y'): (width=7, offset=249) */
0x80, 0x40, 0x20, 0x1E, 0x20, 0x40, 0x80, 

/* character 0x5A ('Z'): (width=5, offset=256) */
0x86, 0x8A, 0x92, 0xA2, 0xC2, 

/* character 0x5B ('['): (width=3, offset=261) */
0xFE, 0x82, 0x82, 

/* character 0x5C ('\'): (width=4, offset=264) */
0xC0, 0x30, 0x18, 0x06, 

/* character 0x5D (']'): (width=3, offset=268) */
0x82, 0x82, 0xFE, 

/* character 0x5E ('^'): (width=5, offset=271) */
0x20, 0x40, 0x80, 0x40, 0x20, 

/* character 0x5F ('_'): (width=5, offset=276) */
0x02, 0x02, 0x02, 0x02, 0x02, 

/* character 0x60 ('`'): (width=1, offset=281) */
0xC0, 

/* character 0x61 ('a'): (width=7, offset=282) */
0x02, 0x0C, 0x38, 0xC8, 0x38, 0x0C, 0x02, 

/* character 0x62 ('b'): (width=5, offset=289) */
0xFE, 0x92, 0x92, 0x92, 0x6C, 

/* character 0x63 ('c'): (width=5, offset=294) */
0x38, 0x44, 0x82, 0x82, 0x44, 

/* character 0x64 ('d'): (width=5, offset=299) */
0xFE, 0x82, 0x82, 0x44, 0x38, 

/* character 0x65 ('e'): (width=4, offset=304) */
0xFE, 0x92, 0x92, 0x82, 

/* character 0x66 ('f'): (width=4, offset=308) */
0xFE, 0x90, 0x90, 0x80, 

/* character 0x67 ('g'): (width=6, offset=312) */
0x38, 0x44, 0x82, 0x92, 0x54, 0x18, 

/* character 0x68 ('h'): (width=5, offset=318) */
0xFE, 0x10, 0x10, 0x10, 0xFE, 

/* character 0x69 ('i'): (width=3, offset=323) */
0x82, 0xFE, 0x82, 

/* character 0x6A ('j'): (width=5, offset=326) */
0x04, 0x02, 0x02, 0x02, 0xFC, 

/* character 0x6B ('k'): (width=5, offset=331) */
0xFE, 0x10, 0x28, 0x44, 0x82, 

/* character 0x6C ('l'): (width=4, offset=336) */
0xFE, 0x02, 0x02, 0x02, 

/* character 0x6D ('m'): (width=7, offset=340) */
0xFE, 0x60, 0x18, 0x06, 0x18, 0x60, 0xFE, 

/* character 0x6E ('n'): (width=6, offset=347) */
0xFE, 0x40, 0x30, 0x18, 0x04, 0xFE, 

/* character 0x6F ('o'): (width=6, offset=353) */
0x38, 0x44, 0x82, 0x82, 0x44, 0x38, 

/* character 0x70 ('p'): (width=4, offset=359) */
0xFE, 0x90, 0x90, 0x60, 

/* character 0x71 ('q'): (width=7, offset=363) */
0x38, 0x44, 0x82, 0x82, 0x44, 0x3A, 0x02, 

/* character 0x72 ('r'): (width=5, offset=370) */
0xFE, 0x90, 0x90, 0x98, 0x66, 

/* character 0x73 ('s'): (width=4, offset=375) */
0x64, 0x92, 0x92, 0x4C, 

/* character 0x74 ('t'): (width=5, offset=379) */
0x80, 0x80, 0xFE, 0x80, 0x80, 

/* character 0x75 ('u'): (width=5, offset=384) */
0xFC, 0x02, 0x02, 0x02, 0xFC, 

/* character 0x76 ('v'): (width=7, offset=389) */
0x80, 0x60, 0x18, 0x06, 0x18, 0x60, 0x80, 

/* character 0x77 ('w'): (width=7, offset=396) */
0xF8, 0x06, 0x18, 0xE0, 0x18, 0x06, 0xF8, 

/* character 0x78 ('x'): (width=5, offset=403) */
0xC6, 0x6C, 0x10, 0x6C, 0xC6, 

/* character 0x79 ('y'): (width=7, offset=408) */
0x80, 0x40, 0x20, 0x1E, 0x20, 0x40, 0x80, 

/* character 0x7A ('z'): (width=5, offset=415) */
0x86, 0x8A, 0x92, 0xA2, 0xC2, 

/* character 0x7B ('{'): (width=3, offset=420) */
0x38, 0x44, 0x82, 

/* character 0x7C ('|'): (width=1, offset=423) */
0xFE, 

/* character 0x7D ('}'): (width=3, offset=424) */
0x82, 0x44, 0x38, 

/* character 0x7E ('~'): (width=5, offset=427) */
0x04, 0x04, 0x02, 0x02, 0x00, 
};

const unsigned char MetaWatch7widthOled[PRINTABLE_CHARACTERS] = 
{
/*		width    char    hexcode */
/*		=====    ====    ======= */
  		  2, /*          20      */
  		  1, /*   !      21      */
  		  3, /*   "      22      */
  		  7, /*   #      23      */
  		  5, /*   $      24      */
  		  7, /*   %      25      */
  		  5, /*   &      26      */
  		  3, /*   '      27      */
  		  3, /*   (      28      */
  		  3, /*   )      29      */
  		  7, /*   *      2A      */
  		  5, /*   +      2B      */
  		  1, /*   ,      2C      */
  		  4, /*   -      2D      */
  		  1, /*   .      2E      */
  		  4, /*   /      2F      */
  		  4, /*   0      30      */
  		  2, /*   1      31      */
  		  4, /*   2      32      */
  		  4, /*   3      33      */
  		  5, /*   4      34      */
  		  4, /*   5      35      */
  		  4, /*   6      36      */
  		  4, /*   7      37      */
  		  4, /*   8      38      */
  		  4, /*   9      39      */
  		  1, /*   :      3A      */
  		  1, /*   ;      3B      */
  		  3, /*   <      3C      */
  		  4, /*   =      3D      */
  		  3, /*   >      3E      */
  		  4, /*   ?      3F      */
  		  7, /*   @      40      */
  		  7, /*   A      41      */
  		  5, /*   B      42      */
  		  5, /*   C      43      */
  		  5, /*   D      44      */
  		  4, /*   E      45      */
  		  4, /*   F      46      */
  		  6, /*   G      47      */
  		  5, /*   H      48      */
  		  3, /*   I      49      */
  		  5, /*   J      4A      */
  		  5, /*   K      4B      */
  		  4, /*   L      4C      */
  		  7, /*   M      4D      */
  		  6, /*   N      4E      */
  		  6, /*   O      4F      */
  		  4, /*   P      50      */
  		  7, /*   Q      51      */
  		  5, /*   R      52      */
  		  4, /*   S      53      */
  		  5, /*   T      54      */
  		  5, /*   U      55      */
  		  7, /*   V      56      */
  		  7, /*   W      57      */
  		  5, /*   X      58      */
  		  7, /*   Y      59      */
  		  5, /*   Z      5A      */
  		  3, /*   [      5B      */
  		  4, /*   \      5C      */
  		  3, /*   ]      5D      */
  		  5, /*   ^      5E      */
  		  5, /*   _      5F      */
  		  1, /*   `      60      */
  		  7, /*   a      61      */
  		  5, /*   b      62      */
  		  5, /*   c      63      */
  		  5, /*   d      64      */
  		  4, /*   e      65      */
  		  4, /*   f      66      */
  		  6, /*   g      67      */
  		  5, /*   h      68      */
  		  3, /*   i      69      */
  		  5, /*   j      6A      */
  		  5, /*   k      6B      */
  		  4, /*   l      6C      */
  		  7, /*   m      6D      */
  		  6, /*   n      6E      */
  		  6, /*   o      6F      */
  		  4, /*   p      70      */
  		  7, /*   q      71      */
  		  5, /*   r      72      */
  		  4, /*   s      73      */
  		  5, /*   t      74      */
  		  5, /*   u      75      */
  		  7, /*   v      76      */
  		  7, /*   w      77      */
  		  5, /*   x      78      */
  		  7, /*   y      79      */
  		  5, /*   z      7A      */
  		  3, /*   {      7B      */
  		  1, /*   |      7C      */
  		  3, /*   }      7D      */
  		  5, /*   ~      7E      */
};

const unsigned int MetaWatch7offsetOled[PRINTABLE_CHARACTERS] = 
{
/*		offset    char    hexcode */
/*		======    ====    ======= */
  		     0, /*          20      */
  		     2, /*   !      21      */
  		     3, /*   "      22      */
  		     6, /*   #      23      */
  		    13, /*   $      24      */
  		    18, /*   %      25      */
  		    25, /*   &      26      */
  		    30, /*   '      27      */
  		    33, /*   (      28      */
  		    36, /*   )      29      */
  		    39, /*   *      2A      */
  		    46, /*   +      2B      */
  		    51, /*   ,      2C      */
  		    52, /*   -      2D      */
  		    56, /*   .      2E      */
  		    57, /*   /      2F      */
  		    61, /*   0      30      */
  		    65, /*   1      31      */
  		    67, /*   2      32      */
  		    71, /*   3      33      */
  		    75, /*   4      34      */
  		    80, /*   5      35      */
  		    84, /*   6      36      */
  		    88, /*   7      37      */
  		    92, /*   8      38      */
  		    96, /*   9      39      */
  		   100, /*   :      3A      */
  		   101, /*   ;      3B      */
  		   102, /*   <      3C      */
  		   105, /*   =      3D      */
  		   109, /*   >      3E      */
  		   112, /*   ?      3F      */
  		   116, /*   @      40      */
  		   123, /*   A      41      */
  		   130, /*   B      42      */
  		   135, /*   C      43      */
  		   140, /*   D      44      */
  		   145, /*   E      45      */
  		   149, /*   F      46      */
  		   153, /*   G      47      */
  		   159, /*   H      48      */
  		   164, /*   I      49      */
  		   167, /*   J      4A      */
  		   172, /*   K      4B      */
  		   177, /*   L      4C      */
  		   181, /*   M      4D      */
  		   188, /*   N      4E      */
  		   194, /*   O      4F      */
  		   200, /*   P      50      */
  		   204, /*   Q      51      */
  		   211, /*   R      52      */
  		   216, /*   S      53      */
  		   220, /*   T      54      */
  		   225, /*   U      55      */
  		   230, /*   V      56      */
  		   237, /*   W      57      */
  		   244, /*   X      58      */
  		   249, /*   Y      59      */
  		   256, /*   Z      5A      */
  		   261, /*   [      5B      */
  		   264, /*   \      5C      */
  		   268, /*   ]      5D      */
  		   271, /*   ^      5E      */
  		   276, /*   _      5F      */
  		   281, /*   `      60      */
  		   282, /*   a      61      */
  		   289, /*   b      62      */
  		   294, /*   c      63      */
  		   299, /*   d      64      */
  		   304, /*   e      65      */
  		   308, /*   f      66      */
  		   312, /*   g      67      */
  		   318, /*   h      68      */
  		   323, /*   i      69      */
  		   326, /*   j      6A      */
  		   331, /*   k      6B      */
  		   336, /*   l      6C      */
  		   340, /*   m      6D      */
  		   347, /*   n      6E      */
  		   353, /*   o      6F      */
  		   359, /*   p      70      */
  		   363, /*   q      71      */
  		   370, /*   r      72      */
  		   375, /*   s      73      */
  		   379, /*   t      74      */
  		   384, /*   u      75      */
  		   389, /*   v      76      */
  		   396, /*   w      77      */
  		   403, /*   x      78      */
  		   408, /*   y      79      */
  		   415, /*   z      7A      */
  		   420, /*   {      7B      */
  		   423, /*   |      7C      */
  		   424, /*   }      7D      */
  		   427, /*   ~      7E      */    
};

const unsigned int MetaWatch16tableOled[] = 
{

/* character 0x20 (' '): (width=4, offset=0) */
0x0000, 0x0000, 0x0000, 0x0000, 

/* character 0x21 ('!'): (width=2, offset=8) */
0x3FD8, 0x3FD8, 

/* character 0x22 ('"'): (width=5, offset=12) */
0x3800, 0x7000, 0x0000, 0x3800, 
0x7000, 

/* character 0x23 ('#'): (width=12, offset=22) */
0x0220, 0x0660, 0x0660, 0x0FF8, 
0x1FF0, 0x0660, 0x0660, 0x0FF8, 
0x1FF0, 0x0660, 0x0660, 0x0440, 

/* character 0x24 ('$'): (width=6, offset=46) */
0x1E30, 0x3F38, 0xF31E, 0xF19E, 
0x39F8, 0x18F0, 

/* character 0x25 ('%'): (width=10, offset=58) */
0x3838, 0x7C70, 0x6CE0, 0x7DC0, 
0x3B80, 0x0770, 0x0EF8, 0x1CD8, 
0x38F8, 0x7070, 

/* character 0x26 ('&'): (width=10, offset=78) */
0x00E0, 0x1DF0, 0x3FB8, 0x3718, 
0x3FB8, 0x1DF0, 0x00E0, 0x01F0, 
0x01B8, 0x0018, 

/* character 0x27 ('''): (width=2, offset=98) */
0x3800, 0x7000, 

/* character 0x28 ('('): (width=4, offset=102) */
0x0FE0, 0x3FF8, 0x701C, 0x8002, 

/* character 0x29 (')'): (width=4, offset=110) */
0x8002, 0x701C, 0x3FF8, 0x0FE0, 

/* character 0x2A ('*'): (width=8, offset=118) */
0x0D80, 0x0D80, 0x0700, 0x3FE0, 
0x3FE0, 0x0700, 0x0D80, 0x0D80, 

/* character 0x2B ('+'): (width=8, offset=134) */
0x0180, 0x0180, 0x0180, 0x0FF0, 
0x0FF0, 0x0180, 0x0180, 0x0180, 

/* character 0x2C (','): (width=2, offset=150) */
0x000E, 0x001C, 

/* character 0x2D ('-'): (width=8, offset=154) */
0x0180, 0x0180, 0x0180, 0x0180, 
0x0180, 0x0180, 0x0180, 0x0180, 

/* character 0x2E ('.'): (width=2, offset=170) */
0x0018, 0x0018, 

/* character 0x2F ('/'): (width=6, offset=174) */
0x0018, 0x0078, 0x01E0, 0x0780, 
0x1E00, 0x1800, 

/* character 0x30 ('0'): (width=7, offset=186) */
0x07C0, 0x1FF0, 0x3838, 0x3018, 
0x3838, 0x1FF0, 0x07C0, 

/* character 0x31 ('1'): (width=3, offset=200) */
0x1800, 0x3FF8, 0x3FF8, 

/* character 0x32 ('2'): (width=6, offset=206) */
0x1878, 0x38F8, 0x31D8, 0x3398, 
0x3F18, 0x1E18, 

/* character 0x33 ('3'): (width=6, offset=218) */
0x1830, 0x3838, 0x3318, 0x3318, 
0x3FF8, 0x1DF0, 

/* character 0x34 ('4'): (width=7, offset=230) */
0x01C0, 0x07C0, 0x3EC0, 0x38C0, 
0x07F8, 0x07F8, 0x00C0, 

/* character 0x35 ('5'): (width=6, offset=244) */
0x3F30, 0x3F38, 0x3318, 0x3318, 
0x33F8, 0x31F0, 

/* character 0x36 ('6'): (width=6, offset=256) */
0x07F0, 0x1FF8, 0x3B18, 0x3318, 
0x03F8, 0x01E0, 

/* character 0x37 ('7'): (width=6, offset=268) */
0x3000, 0x3000, 0x30F8, 0x33F8, 
0x3F00, 0x3C00, 

/* character 0x38 ('8'): (width=6, offset=280) */
0x1EF0, 0x3FF8, 0x3318, 0x3318, 
0x3FF8, 0x1EF0, 

/* character 0x39 ('9'): (width=6, offset=292) */
0x1F00, 0x3F80, 0x3198, 0x31B8, 
0x3FF0, 0x1FC0, 

/* character 0x3A (':'): (width=2, offset=304) */
0x0660, 0x0660, 

/* character 0x3B (';'): (width=2, offset=308) */
0x0670, 0x06E0, 

/* character 0x3C ('<'): (width=8, offset=312) */
0x0100, 0x0100, 0x0380, 0x0380, 
0x06C0, 0x06C0, 0x0C60, 0x0C60, 

/* character 0x3D ('='): (width=7, offset=328) */
0x0360, 0x0360, 0x0360, 0x0360, 
0x0360, 0x0360, 0x0360, 

/* character 0x3E ('>'): (width=8, offset=342) */
0x0C60, 0x0C60, 0x06C0, 0x06C0, 
0x0380, 0x0380, 0x0100, 0x0100, 

/* character 0x3F ('?'): (width=6, offset=358) */
0x1800, 0x3800, 0x31D8, 0x33D8, 
0x3F00, 0x1C00, 

/* character 0x40 ('@'): (width=11, offset=370) */
0x07C0, 0x1FF0, 0x1830, 0x3398, 
0x37D8, 0x36D8, 0x37D8, 0x37D8, 
0x38D0, 0x1FC0, 0x0F80, 

/* character 0x41 ('A'): (width=9, offset=392) */
0x0038, 0x00F8, 0x03E0, 0x0F20, 
0x3C20, 0x0F20, 0x03E0, 0x00F8, 
0x0038, 

/* character 0x42 ('B'): (width=7, offset=410) */
0x3FF8, 0x3FF8, 0x3318, 0x3318, 
0x3318, 0x3FF8, 0x1DF0, 

/* character 0x43 ('C'): (width=7, offset=424) */
0x1FF0, 0x3FF8, 0x3018, 0x3018, 
0x3018, 0x3838, 0x1830, 

/* character 0x44 ('D'): (width=7, offset=438) */
0x3FF8, 0x3FF8, 0x3018, 0x3018, 
0x3018, 0x3FF8, 0x1FF0, 

/* character 0x45 ('E'): (width=7, offset=452) */
0x3FF8, 0x3FF8, 0x3318, 0x3318, 
0x3318, 0x3018, 0x3018, 

/* character 0x46 ('F'): (width=6, offset=466) */
0x3FF8, 0x3FF8, 0x3300, 0x3300, 
0x3300, 0x3300, 

/* character 0x47 ('G'): (width=7, offset=478) */
0x1FF0, 0x3FF8, 0x3018, 0x3198, 
0x3198, 0x39F8, 0x19F0, 

/* character 0x48 ('H'): (width=7, offset=492) */
0x3FF8, 0x3FF8, 0x0300, 0x0300, 
0x0300, 0x3FF8, 0x3FF8, 

/* character 0x49 ('I'): (width=4, offset=506) */
0x3018, 0x3FF8, 0x3FF8, 0x3018, 

/* character 0x4A ('J'): (width=6, offset=514) */
0x0030, 0x0038, 0x0018, 0x0018, 
0x3FF8, 0x3FF0, 

/* character 0x4B ('K'): (width=7, offset=526) */
0x3FF8, 0x3FF8, 0x07C0, 0x0EE0, 
0x1C70, 0x3838, 0x3018, 

/* character 0x4C ('L'): (width=6, offset=540) */
0x3FF8, 0x3FF8, 0x0018, 0x0018, 
0x0018, 0x0018, 

/* character 0x4D ('M'): (width=11, offset=552) */
0x3FF8, 0x1FF8, 0x0E00, 0x0700, 
0x0380, 0x01C0, 0x0380, 0x0700, 
0x0E00, 0x1FF8, 0x3FF8, 

/* character 0x4E ('N'): (width=9, offset=574) */
0x3FF8, 0x1FF8, 0x0E00, 0x0700, 
0x0380, 0x01C0, 0x00E0, 0x3FF0, 
0x3FF8, 

/* character 0x4F ('O'): (width=7, offset=592) */
0x1FF0, 0x3FF8, 0x3018, 0x3018, 
0x3018, 0x3FF8, 0x1FF0, 

/* character 0x50 ('P'): (width=7, offset=606) */
0x3FF8, 0x3FF8, 0x3180, 0x3180, 
0x3180, 0x3F80, 0x1F00, 

/* character 0x51 ('Q'): (width=8, offset=620) */
0x1FF0, 0x3FF8, 0x3018, 0x3018, 
0x301C, 0x3FFE, 0x1FF6, 0x0004, 

/* character 0x52 ('R'): (width=7, offset=636) */
0x3FF8, 0x3FF8, 0x3180, 0x3180, 
0x3180, 0x3FF8, 0x1EF8, 

/* character 0x53 ('S'): (width=6, offset=650) */
0x1E30, 0x3F38, 0x3318, 0x3198, 
0x39F8, 0x18F0, 

/* character 0x54 ('T'): (width=6, offset=662) */
0x3000, 0x3000, 0x3FF8, 0x3FF8, 
0x3000, 0x3000, 

/* character 0x55 ('U'): (width=7, offset=674) */
0x3FF0, 0x3FF8, 0x0018, 0x0018, 
0x0018, 0x3FF8, 0x3FF0, 

/* character 0x56 ('V'): (width=7, offset=688) */
0x3800, 0x3F00, 0x07E0, 0x00F8, 
0x07E0, 0x3F00, 0x3800, 

/* character 0x57 ('W'): (width=11, offset=702) */
0x3800, 0x3F00, 0x07E0, 0x00F8, 
0x07E0, 0x1F00, 0x07E0, 0x00F8, 
0x07E0, 0x3F00, 0x3800, 

/* character 0x58 ('X'): (width=7, offset=724) */
0x3018, 0x3C78, 0x0FE0, 0x0380, 
0x0FE0, 0x3C78, 0x3018, 

/* character 0x59 ('Y'): (width=8, offset=738) */
0x3000, 0x3C00, 0x0F00, 0x03F8, 
0x03F8, 0x0F00, 0x3C00, 0x3000, 

/* character 0x5A ('Z'): (width=7, offset=754) */
0x3018, 0x3078, 0x30F8, 0x33D8, 
0x3F18, 0x3C18, 0x3018, 

/* character 0x5B ('['): (width=4, offset=768) */
0xFFFE, 0xFFFE, 0xC006, 0xC006, 

/* character 0x5C ('\'): (width=6, offset=776) */
0x1800, 0x1E00, 0x0780, 0x01E0, 
0x0078, 0x0018, 

/* character 0x5D (']'): (width=4, offset=788) */
0xC006, 0xC006, 0xFFFE, 0xFFFE, 

/* character 0x5E ('^'): (width=7, offset=796) */
0x0060, 0x01E0, 0x0780, 0x1E00, 
0x0780, 0x01E0, 0x0060, 

/* character 0x5F ('_'): (width=9, offset=810) */
0x0008, 0x0008, 0x0008, 0x0008, 
0x0008, 0x0008, 0x0008, 0x0008, 
0x0008, 

/* character 0x60 ('`'): (width=3, offset=828) */
0x1800, 0x0C00, 0x0600, 

/* character 0x61 ('a'): (width=6, offset=834) */
0x0270, 0x06F8, 0x06D8, 0x06D8, 
0x07F8, 0x03F8, 

/* character 0x62 ('b'): (width=6, offset=846) */
0x3FF8, 0x3FF8, 0x0618, 0x0618, 
0x07F8, 0x03F0, 

/* character 0x63 ('c'): (width=6, offset=858) */
0x03F0, 0x07F8, 0x0618, 0x0618, 
0x0738, 0x0330, 

/* character 0x64 ('d'): (width=6, offset=870) */
0x03F0, 0x07F8, 0x0618, 0x0618, 
0x3FF8, 0x3FF8, 

/* character 0x65 ('e'): (width=6, offset=882) */
0x03F0, 0x07F8, 0x06D8, 0x06D8, 
0x07D8, 0x03D0, 

/* character 0x66 ('f'): (width=4, offset=894) */
0x0600, 0x1FF8, 0x3FF8, 0x3600, 

/* character 0x67 ('g'): (width=6, offset=902) */
0x03F0, 0x07FA, 0x061B, 0x061B, 
0x07FF, 0x07FE, 

/* character 0x68 ('h'): (width=6, offset=914) */
0x3FF8, 0x3FF8, 0x0600, 0x0600, 
0x07F8, 0x03F8, 

/* character 0x69 ('i'): (width=2, offset=926) */
0x37F8, 0x37F8, 

/* character 0x6A ('j'): (width=5, offset=930) */
0x0002, 0x0003, 0x0003, 0x37FF, 
0x37FE, 

/* character 0x6B ('k'): (width=6, offset=940) */
0x3FF8, 0x3FF8, 0x01E0, 0x03F0, 
0x0738, 0x0618, 

/* character 0x6C ('l'): (width=2, offset=952) */
0x3FF8, 0x3FF8, 

/* character 0x6D ('m'): (width=10, offset=956) */
0x07F8, 0x07F8, 0x0300, 0x0600, 
0x07F8, 0x03F8, 0x0700, 0x0600, 
0x07F8, 0x03F8, 

/* character 0x6E ('n'): (width=6, offset=976) */
0x07F8, 0x07F8, 0x0300, 0x0600, 
0x07F8, 0x03F8, 

/* character 0x6F ('o'): (width=6, offset=988) */
0x03F0, 0x07F8, 0x0618, 0x0618, 
0x07F8, 0x03F0, 

/* character 0x70 ('p'): (width=6, offset=1000) */
0x07FF, 0x07FF, 0x0618, 0x0618, 
0x07F8, 0x03E0, 

/* character 0x71 ('q'): (width=6, offset=1012) */
0x03E0, 0x07F8, 0x0618, 0x0618, 
0x07FF, 0x07FF, 

/* character 0x72 ('r'): (width=5, offset=1024) */
0x07F8, 0x07F8, 0x0300, 0x0600, 
0x0600, 

/* character 0x73 ('s'): (width=5, offset=1034) */
0x0390, 0x07D8, 0x06D8, 0x06F8, 
0x0270, 

/* character 0x74 ('t'): (width=4, offset=1044) */
0x0600, 0x1FF0, 0x3FF8, 0x0618, 

/* character 0x75 ('u'): (width=6, offset=1052) */
0x07F0, 0x07F8, 0x0018, 0x0018, 
0x07F8, 0x07F8, 

/* character 0x76 ('v'): (width=7, offset=1064) */
0x0600, 0x0780, 0x01E0, 0x0078, 
0x01E0, 0x0780, 0x0600, 

/* character 0x77 ('w'): (width=11, offset=1078) */
0x0600, 0x0780, 0x01E0, 0x0078, 
0x01E0, 0x0780, 0x01E0, 0x0078, 
0x01E0, 0x0780, 0x0600, 

/* character 0x78 ('x'): (width=7, offset=1100) */
0x0618, 0x0738, 0x03F0, 0x01E0, 
0x03F0, 0x0738, 0x0618, 

/* character 0x79 ('y'): (width=6, offset=1114) */
0x07F0, 0x07FA, 0x001B, 0x001B, 
0x07FF, 0x07FE, 

/* character 0x7A ('z'): (width=6, offset=1126) */
0x0618, 0x0638, 0x0678, 0x06D8, 
0x0798, 0x0718, 

/* character 0x7B ('{'): (width=4, offset=1138) */
0x0FE0, 0x3FF8, 0x701C, 0x8002, 

/* character 0x7C ('|'): (width=2, offset=1146) */
0x7CF8, 0x7CF8, 

/* character 0x7D ('}'): (width=4, offset=1150) */
0x8002, 0x701C, 0x3FF8, 0x0FE0, 

/* character 0x7E ('~'): (width=9, offset=1158) */
0x0040, 0x0080, 0x0080, 0x0080, 
0x0040, 0x0040, 0x0040, 0x0080, 
0x0000, 

};

const unsigned char MetaWatch16widthOled[PRINTABLE_CHARACTERS] = 
{
/*		width    char    hexcode */
/*		=====    ====    ======= */
  		  4, /*          20      */
  		  2, /*   !      21      */
  		  5, /*   "      22      */
  		 12, /*   #      23      */
  		  6, /*   $      24      */
  		 10, /*   %      25      */
  		 10, /*   &      26      */
  		  2, /*   '      27      */
  		  4, /*   (      28      */
  		  4, /*   )      29      */
  		  8, /*   *      2A      */
  		  8, /*   +      2B      */
  		  2, /*   ,      2C      */
  		  8, /*   -      2D      */
  		  2, /*   .      2E      */
  		  6, /*   /      2F      */
  		  7, /*   0      30      */
  		  3, /*   1      31      */
  		  6, /*   2      32      */
  		  6, /*   3      33      */
  		  7, /*   4      34      */
  		  6, /*   5      35      */
  		  6, /*   6      36      */
  		  6, /*   7      37      */
  		  6, /*   8      38      */
  		  6, /*   9      39      */
  		  2, /*   :      3A      */
  		  2, /*   ;      3B      */
  		  8, /*   <      3C      */
  		  7, /*   =      3D      */
  		  8, /*   >      3E      */
  		  6, /*   ?      3F      */
  		 11, /*   @      40      */
  		  9, /*   A      41      */
  		  7, /*   B      42      */
  		  7, /*   C      43      */
  		  7, /*   D      44      */
  		  7, /*   E      45      */
  		  6, /*   F      46      */
  		  7, /*   G      47      */
  		  7, /*   H      48      */
  		  4, /*   I      49      */
  		  6, /*   J      4A      */
  		  7, /*   K      4B      */
  		  6, /*   L      4C      */
  		 11, /*   M      4D      */
  		  9, /*   N      4E      */
  		  7, /*   O      4F      */
  		  7, /*   P      50      */
  		  8, /*   Q      51      */
  		  7, /*   R      52      */
  		  6, /*   S      53      */
  		  6, /*   T      54      */
  		  7, /*   U      55      */
  		  7, /*   V      56      */
  		 11, /*   W      57      */
  		  7, /*   X      58      */
  		  8, /*   Y      59      */
  		  7, /*   Z      5A      */
  		  4, /*   [      5B      */
  		  6, /*   \      5C      */
  		  4, /*   ]      5D      */
  		  7, /*   ^      5E      */
  		  9, /*   _      5F      */
  		  3, /*   `      60      */
  		  6, /*   a      61      */
  		  6, /*   b      62      */
  		  6, /*   c      63      */
  		  6, /*   d      64      */
  		  6, /*   e      65      */
  		  4, /*   f      66      */
  		  6, /*   g      67      */
  		  6, /*   h      68      */
  		  2, /*   i      69      */
  		  5, /*   j      6A      */
  		  6, /*   k      6B      */
  		  2, /*   l      6C      */
  		 10, /*   m      6D      */
  		  6, /*   n      6E      */
  		  6, /*   o      6F      */
  		  6, /*   p      70      */
  		  6, /*   q      71      */
  		  5, /*   r      72      */
  		  5, /*   s      73      */
  		  4, /*   t      74      */
  		  6, /*   u      75      */
  		  7, /*   v      76      */
  		 11, /*   w      77      */
  		  7, /*   x      78      */
  		  6, /*   y      79      */
  		  6, /*   z      7A      */
  		  4, /*   {      7B      */
  		  2, /*   |      7C      */
  		  4, /*   }      7D      */
  		  9, /*   ~      7E      */
};

const unsigned int MetaWatch16offsetOled[PRINTABLE_CHARACTERS] = 
{
/*		offset    char    hexcode */
/*		======    ====    ======= */
  		     0, /*          20      */
  		     8, /*   !      21      */
  		    12, /*   "      22      */
  		    22, /*   #      23      */
  		    46, /*   $      24      */
  		    58, /*   %      25      */
  		    78, /*   &      26      */
  		    98, /*   '      27      */
  		   102, /*   (      28      */
  		   110, /*   )      29      */
  		   118, /*   *      2A      */
  		   134, /*   +      2B      */
  		   150, /*   ,      2C      */
  		   154, /*   -      2D      */
  		   170, /*   .      2E      */
  		   174, /*   /      2F      */
  		   186, /*   0      30      */
  		   200, /*   1      31      */
  		   206, /*   2      32      */
  		   218, /*   3      33      */
  		   230, /*   4      34      */
  		   244, /*   5      35      */
  		   256, /*   6      36      */
  		   268, /*   7      37      */
  		   280, /*   8      38      */
  		   292, /*   9      39      */
  		   304, /*   :      3A      */
  		   308, /*   ;      3B      */
  		   312, /*   <      3C      */
  		   328, /*   =      3D      */
  		   342, /*   >      3E      */
  		   358, /*   ?      3F      */
  		   370, /*   @      40      */
  		   392, /*   A      41      */
  		   410, /*   B      42      */
  		   424, /*   C      43      */
  		   438, /*   D      44      */
  		   452, /*   E      45      */
  		   466, /*   F      46      */
  		   478, /*   G      47      */
  		   492, /*   H      48      */
  		   506, /*   I      49      */
  		   514, /*   J      4A      */
  		   526, /*   K      4B      */
  		   540, /*   L      4C      */
  		   552, /*   M      4D      */
  		   574, /*   N      4E      */
  		   592, /*   O      4F      */
  		   606, /*   P      50      */
  		   620, /*   Q      51      */
  		   636, /*   R      52      */
  		   650, /*   S      53      */
  		   662, /*   T      54      */
  		   674, /*   U      55      */
  		   688, /*   V      56      */
  		   702, /*   W      57      */
  		   724, /*   X      58      */
  		   738, /*   Y      59      */
  		   754, /*   Z      5A      */
  		   768, /*   [      5B      */
  		   776, /*   \      5C      */
  		   788, /*   ]      5D      */
  		   796, /*   ^      5E      */
  		   810, /*   _      5F      */
  		   828, /*   `      60      */
  		   834, /*   a      61      */
  		   846, /*   b      62      */
  		   858, /*   c      63      */
  		   870, /*   d      64      */
  		   882, /*   e      65      */
  		   894, /*   f      66      */
  		   902, /*   g      67      */
  		   914, /*   h      68      */
  		   926, /*   i      69      */
  		   930, /*   j      6A      */
  		   940, /*   k      6B      */
  		   952, /*   l      6C      */
  		   956, /*   m      6D      */
  		   976, /*   n      6E      */
  		   988, /*   o      6F      */
  		  1000, /*   p      70      */
  		  1012, /*   q      71      */
  		  1024, /*   r      72      */
  		  1034, /*   s      73      */
  		  1044, /*   t      74      */
  		  1052, /*   u      75      */
  		  1064, /*   v      76      */
  		  1078, /*   w      77      */
  		  1100, /*   x      78      */
  		  1114, /*   y      79      */
  		  1126, /*   z      7A      */
  		  1138, /*   {      7B      */
  		  1146, /*   |      7C      */
  		  1150, /*   }      7D      */
  		  1158, /*   ~      7E      */
};


const unsigned int MetaWatchIconTableOled[] = 
{

0x6C00, 0x4400, 0x0000, 0x4400, 
0x6C00, 

0x1B00, 0x7100, 0xE000, 0xF100, 
0xDB00, 

0x1830, 0x3C78, 0x3EF8, 0x1FF0, 
0x0FE0, 0x07C0, 0x0FE0, 0x1FF0, 
0x3EF8, 0x3C78, 0x1830, 

0x0800, 0x1C00, 0x0E00, 0x0700, 
0x0E00, 0x1C00, 0x3800, 0x7000, 
0x2000, 

0x0200, 0x0700, 0x0380, 0x01C0, 
0x0380, 0x0700, 0x0E00, 0x1C00, 
0x0800, 

0x2400, 0x1800, 0x1800, 0x2400, 

0x0410, 0x0630, 0x0360, 0x01C0, 
0x7FFF, 0x3366, 0x1E3C, 0x0C18, 

0x27E0, 0x3FF0, 0x3E78, 0x3C1C, 
0x3E1C, 0x0000, 0x0000, 0x387C, 
0x383C, 0x1E7C, 0x0FFC, 0x07E4, 

0x7FFE, 0x41AA, 0x41FE, 0x41AA, 
0x41FE, 0x41AA, 0x7FFE, 

0x7FFE, 0x41AA, 0x41FE, 0x41AA, 
0x41FE, 0x41AA, 0x7FFE, 0x0000, 
0x1830, 0x3C78, 0x3EF8, 0x1FF0, 
0x0FE0, 0x07C0, 0x0FE0, 0x1FF0, 
0x3EF8, 0x3C78, 0x1830, 

0x7FEE, 0x7FEE, 0x7FEE, 0x0000, 
0x0000, 0x3FF8, 0x3FF8, 0x3FF8, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x3C78, 
0x0440, 0x0380, 

0x3FF8, 0x3FF8, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x07C0, 
0x0380, 

0x3FF8, 0x2008, 0x2008, 0x2408, 
0x2208, 0x2108, 0x2188, 0x2FC8, 
0x27E8, 0x2308, 0x2108, 0x2088, 
0x2048, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x2408, 
0x2208, 0x2108, 0x2188, 0x2FC8, 
0x27E8, 0x2308, 0x2108, 0x2088, 
0x2048, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3BF8, 
0x3DF8, 0x3EF8, 0x2188, 0x2FC8, 
0x27E8, 0x2308, 0x2108, 0x2088, 
0x2048, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3BF8, 
0x3DF8, 0x3EF8, 0x3E78, 0x3038, 
0x3818, 0x3CF8, 0x2108, 0x2088, 
0x2048, 0x2008, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3BF8, 
0x3DF8, 0x3EF8, 0x3E78, 0x3038, 
0x3818, 0x3CF8, 0x3EF8, 0x3F78, 
0x3FB8, 0x3FF8, 0x2008, 0x2008, 
0x2008, 0x2008, 0x3C78, 0x0440, 
0x0380, 

0x3FF8, 0x3FF8, 0x3FF8, 0x3BF8, 
0x3DF8, 0x3EF8, 0x3E78, 0x3038, 
0x3818, 0x3CF8, 0x3EF8, 0x3F78, 
0x3FB8, 0x3FF8, 0x3FF8, 0x3FF8, 
0x3FF8, 0x3FF8, 0x3FF8, 0x07C0, 
0x0380, 

};

const unsigned int MetaWatchIconOffsetOled[TOTAL_OLED_ICONS] = 
{
/*		offset  */
/*		======  */
  		     0, 
  		    10, 
  		    20, 
  		    42, 
  		    60, 
  		    78, 
  		    86, 
  		   102, 
  		   126, 
  		   140, 
  		   178, 
  		   230, 
  		   272, 
  		   314, 
  		   356, 
  		   398, 
  		   440, 
  		   482, 
  		   524, 
  		   566, 
  		   608, 
  		   650, 
  		   692, 
};

const unsigned char MetaWatchIconWidthOled[TOTAL_OLED_ICONS] = 
{
/*		width  */
/*		=====  */
  		  5, 
  		  5, 
  		 11, 
  		  9, 
  		  9, 
  		  4, 
  		  8, 
  		 12, 
  		  7, 
  		 19, 
  		 26, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21, 
  		 21,
};
