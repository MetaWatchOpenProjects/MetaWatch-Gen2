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

/*! The number of printable characters in the font tables
    [CharNum][WidthInBytes * Height] */

extern const unsigned char MetaWatch5table[][5];
extern const unsigned char MetaWatch7table[][7];
extern const unsigned int MetaWatch16table[][16];
extern const unsigned int TimeTable[][19];
extern const unsigned char TimeBlockTable[][60];
extern const unsigned char TimeGTable[][3*28];
extern const unsigned char TimeKTable[][3*56];

extern const unsigned char MetaWatch5width[];
extern const unsigned char MetaWatch7width[];
extern const unsigned char MetaWatch16width[];
extern const unsigned char TimeWidth[];
extern const unsigned char TimeBlockWidth[];
extern const unsigned char TimeGWidth[];
extern const unsigned char TimeKWidth[];


/*! Get the bitmap for the specified character
 *
 * \param Character is the desired character
 * \param pBitmap pointer to an array of integers that holds the bitmap
 *
 * \note pBitmap must point to an object large enough to hold the largest bitmap
 *
 * \note For the LCD bitmaps the font height is the same as number of rows.
 * If the width is less than 8 then each row is a byte.
 * If the width is less than 16 but > 8 then each row is a word.
 * The function works with ints so that it is generic for both types
 *
 */
void GetCharacterBitmap(unsigned char Character,unsigned int * pBitmap);

/*! Get the width for a specified character *
 *
 * \param Character is the desired character
 * \return Width of character in columns
 */

unsigned char *GetCharacterBitmapPointer(unsigned char Char);

unsigned char GetCharacterWidth(unsigned char Character);

/*! Set the font type used for future Get operations *
 *
 * \param Type of font
 */
void SetFont(etFontType Type);

const tFont *GetCurrentFont(void);

/*! Set the font spacing for the current font*
 *
 * \param Spacing is the number of columns between characters
 *
 * \note The characters do not have any 'natural' spacing between them
 */
//void SetFontSpacing(unsigned char Spacing);

/*!
 * \return The font spacing in columns
 */
unsigned char GetFontSpacing(void);

/*!
 * \return The font height in rows
 */
unsigned char GetFontHeight(etFontType Type);
unsigned char GetCurrentFontHeight(void);

#endif /*FONTS_H*/
