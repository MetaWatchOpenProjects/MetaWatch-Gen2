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
/*! \file Buttons.h
*
* Along with the RTC and Command Task the Button functions process the buttons 
* and generate a message or event when buttons are pressed.
*
*/
/******************************************************************************/

#ifndef BUTTONS_H
#define BUTTONS_H

/*! Number of buttons */
#define BTN_NUM               (TOTAL_BTN_NUM - 1)

#define BTN_INDEX_A           (0)
#define BTN_INDEX_B           (1)
#define BTN_INDEX_C           (2)
#define BTN_INDEX_D           (3)
#define BTN_INDEX_E           (4)
#define BTN_INDEX_F           (5)
#define BTN_INDEX_P           (6)

#define BTN_EVT_NUM           (3)
#define BTN_EVT_IMDT          (0)
#define BTN_EVT_RELS          (1)
#define BTN_EVT_HOLD          (2)

/*! Structure to hold the configuration of a button
 *
 * \param MaskTable holds the absolute mask and button press mask 
 * \param CallbackMsgType holds the callback message for each of the button press types
 * \param CallbackMsgOptions holds options 
 */
typedef struct
{
  unsigned char MaskTable;
  unsigned char CallbackMsgType[BTN_EVT_NUM];
  unsigned char CallbackMsgOptions[BTN_EVT_NUM];

} tButtonConfiguration;

/*! Don't generate an event for an immediate button press */
#define BUTTON_IMMEDIATE_MASK    ( BIT0 )

/*! Don't generate an event for a button press */
#define BUTTON_PRESS_MASK        ( BIT1 )

/*! Don't generate an event for a button hold */
#define BUTTON_HOLD_MASK         ( BIT2 )

/*! Don't generate an event for a long hold */
#define BUTTON_LONG_HOLD_MASK    ( BIT3 )

/*! Don't generate any events for button (short circuit) */
#define BUTTON_ABSOLUTE_MASK     ( BIT4 )

/*! Use to determine if the absolute mask should be set */
#define ALL_BUTTON_EVENTS_MASKED ( BUTTON_PRESS_MASK | BUTTON_HOLD_MASK | \
   BUTTON_LONG_HOLD_MASK | BUTTON_IMMEDIATE_MASK )
  

/*! Initialize the pins associated with the buttons.  Initialize the 
 * memory structures used in the button state machine.
 *
 * The display task will finish initializing the buttons
 */
void InitButton(void);
void ResetButtonAction(void);

/*! Button State Machine */
void ButtonStateHandler(void);

void EnableButtonMsgHandler(tMessage* pMsg);
void DisableButtonMsgHandler(tMessage* pMsg);

/*! Read a button press's association
 *
 * \param ButtonMode is idle, application or notification
 * \param ButtonIndex is A-F, or pull switch
 * \param ButtonEvent is immediate,press,hold, or long hold
 * \param pPayload must point to a 5 byte or greater structure.  It will
 * return [0] = display mode, [1] = ButtonIndex, [2] = MaskTable, [3] = CallbackMsgType,
 * [4] = callback msg options.
 *
 * \note This message is used to save the state of a button.  However, the 
 * phone should use application mode and not destroy the buttons used for idle mode.
 *
 */
void ReadButtonConfigHandler(tMessage* pMsg);

#endif /* BUTTONS_H */
