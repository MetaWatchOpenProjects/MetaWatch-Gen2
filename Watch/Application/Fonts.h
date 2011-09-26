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

/*! There are three fonts defined for the MetaWatch LCD 
 * they are 5, 7 and 16 pixels tall with variable width
 */
typedef enum 
{
  MetaWatch5,
  MetaWatch7,
  MetaWatch16,
  MetaWatch5Oled,
  MetaWatch7Oled,
  MetaWatch16Oled,
  MetaWatchIconOled,
  
} etFontType;

/*! Use to size the bitmap used in the Display Task for printing characters FOR 
 * THE LCD VERSION
 */
#define MAX_FONT_ROWS ( 16 )

/*! The maximum number of columns any font (or icon) requires.  This is 
 * used to define the size of the bitmap passed into font functions FOR THE
 * OLED version
 */
#define MAX_FONT_COLUMNS ( 30 )

/*! Convert a character into an index into the font table
 *
 * \param CharIn is the input character
 * \return Index into the table
 */
unsigned char MapCharacterToIndex(unsigned char CharIn);

/*! Convert a digit (0-9) into an index into the font table
 *
 * \param Digit is a number
 * \return Index into the table
 */
unsigned char MapDigitToIndex(unsigned char Digit);

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
unsigned char GetCharacterWidth(unsigned char Character);

/*! Set the font type used for future Get operations *
 *
 * \param Type of font
 */
void SetFont(etFontType Type);

/*! Set the font spacing for the current font*
 *
 * \param Spacing is the number of columns between characters
 *
 * \note The characters do not have any 'natural' spacing between them
 */
void SetFontSpacing(unsigned char Spacing);

/*! 
 * \return The font spacing in columns 
 */
unsigned char GetFontSpacing(void);


/*! 
 * \return The font height in rows 
 */
unsigned char GetCharacterHeight(void);

#endif /*FONTS_H*/