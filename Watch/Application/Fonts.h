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
/*! \file Fonts.h
*
* Fonts functions common to OLED and LCD version
*
* LCD is ROW based, OLED is column based
*/
/******************************************************************************/

#ifndef FONTS_H
#define FONTS_H

#define FONT_TYPE_TIME  (1)

/*! There are three fonts defined for the MetaWatch LCD
 * they are 5, 7 and 16 pixels tall with variable width
 */
typedef enum
{
  MetaWatch5,
  MetaWatch7,
  MetaWatch16,
  Time,
  TimeBlock,
  TimeG,
  TimeK
} etFontType;

/*! Font Structure
 *
 * \param Type is the enumerated type of font
 * \param Height
 * \param Spacing is the horizontal spacing that should be inserted when
 * drawing characters
 */
typedef struct
{
  const unsigned char Height;
  const unsigned char Spacing;
  const unsigned char MaxWidth;
  const unsigned char WidthInBytes;
  const unsigned char Type; //  FONT_TYPE_TIME for time font (0-9:)
  const unsigned char *pWidth;
} tFont;

/*! Use to size the bitmap used in the Display Task for printing characters FOR
 * THE LCD VERSION
 */
#define MAX_FONT_ROWS ( 56 ) //TimeK

/*! The maximum number of columns any font (or icon) requires.  This is
 * used to define the size of the bitmap passed into font functions FOR THE
 * OLED version
 */
#define MAX_FONT_COLUMNS ( 30 )

#define SPACE_CHAR_TO_ALPHANUM_OFFSET (0x15)
#define NUM_TO_ALPHANUM_OFFSET (0x30)
#define PRINTABLE_CHARACTERS (94)

/*! Get the bitmap for the specified character */
unsigned char const *GetFontBitmap(char const Char, etFontType Type);

unsigned char GetCharWidth(char const Char, etFontType Type);

/*! Set the font type used for future Get operations */
tFont const *GetFont(etFontType Type);

#endif /*FONTS_H*/
