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

// This is the number of consecutive samples by the RTC ISR that need to be
// asserted for a button to be moved to the state
#define BTN_ON_COUNT          2    
#define BTN_ONE_SEC_COUNT     32
#define BTN_HOLD_COUNT        (2 * BTN_ONE_SEC_COUNT)
#define BTN_LONG_HOLD_COUNT   (5 * BTN_ONE_SEC_COUNT)
#define BTN_LONGER_HOLD_COUNT (10 * BTN_ONE_SEC_COUNT)

/* Immediate state is when a button is pressed but is not released */
#define BUTTON_STATE_IMMEDIATE ( 0 )
#define BUTTON_STATE_PRESSED   ( 1 )
#define BUTTON_STATE_HOLD      ( 2 )
#define BUTTON_STATE_LONG_HOLD ( 3 )
#define BUTTON_STATE_OFF       ( 4 )
#define BUTTON_STATE_DEBOUNCE  ( 5 )

/*! Number of states that can generate a button event */
#define NUMBER_OF_BUTTON_EVENT_TYPES ( 4 )

/*! Structure to consolidate the data used to manage the button state
 *
 * \param BtnFilter is for the leaky integrator filter
 * \param BtnState is the current button state 
 * \param BtnHoldCounter is the amount of time the button has been pressed
 */
typedef  struct
{
  unsigned char BtnFilter;          
  unsigned char BtnState;           
  unsigned int BtnHoldCounter;     

} tButtonData;

/*! Structure to hold the configuration of a button
 *
 * \param MaskTable holds the absolute mask and button press mask 
 * \param CallbackMsgType holds the callback message for each of the button press types
 * \param CallbackMsgOptions holds options 
 */
typedef struct
{
  unsigned char MaskTable;
  unsigned char CallbackMsgType[NUMBER_OF_BUTTON_EVENT_TYPES];
  unsigned char CallbackMsgOptions[NUMBER_OF_BUTTON_EVENT_TYPES];

} tButtonConfiguration;

/*! Don't generate any events for button (short circuit) */
#define BUTTON_ABSOLUTE_MASK     ( BIT0 )

/*! Don't generate an event for a button press */
#define BUTTON_PRESS_MASK        ( BIT1 )

/*! Don't generate an event for a button hold */
#define BUTTON_HOLD_MASK         ( BIT2 )

/*! Don't generate an event for a long hold */
#define BUTTON_LONG_HOLD_MASK    ( BIT3 )

/*! Don't generate an event for an immediate button press */
#define BUTTON_IMMEDIATE_MASK    ( BIT4 )

/*! Use to determine if the absolute mask should be set */
#define ALL_BUTTON_EVENTS_MASKED ( BUTTON_PRESS_MASK | BUTTON_HOLD_MASK | \
   BUTTON_LONG_HOLD_MASK | BUTTON_IMMEDIATE_MASK )
  

/*! Initialize the pins associated with the buttons.  Initialize the 
 * memory structures used in the button state machine.
 *
 * The display task will finish initializing the buttons
 */
void InitializeButtons(void);

/*! Button State Machine */
void ButtonStateHandler(void);

/*! Associate and action with a button
 *
 * \param DisplayMode is idle, application or notification
 * \param ButtonIndex is A-F, or pull switch
 * \param ButtonPressType is immediate,press,hold, or long hold
 * \param CallbackMsgType is the message type for the callback
 * \param CallbackMsgOptions allows options to be sent with the message
 * the payload is not configurable.
 */
void EnableButtonAction(unsigned char DisplayMode,
                        unsigned char ButtonIndex,
                        unsigned char ButtonPressType,
                        unsigned char CallbackMsgType,
                        unsigned char CallbackMsgOptions);

/*! Delete a button press type and its association
 *
 * \param DisplayMode is idle, application or notification
 * \param ButtonIndex is A-F, or pull switch
 * \param ButtonPressType is immediate,press,hold, or long hold
 */
void DisableButtonAction(unsigned char DisplayMode,
                         unsigned char ButtonIndex,
                         unsigned char ButtonPressType);

/*! Read a button press's association
 *
 * \param DisplayMode is idle, application or notification
 * \param ButtonIndex is A-F, or pull switch
 * \param ButtonPressType is immediate,press,hold, or long hold
 * \param pPayload must point to a 5 byte or greater structure.  It will
 * return [0] = display mode, [1] = ButtonIndex, [2] = MaskTable, [3] = CallbackMsgType,
 * [4] = callback msg options.
 *
 * \note This message is used to save the state of a button.  However, the 
 * phone should use application mode and not destroy the buttons used for idle mode.
 *
 */
void ReadButtonConfiguration(unsigned char DisplayMode,
                             unsigned char ButtonPressType,
                             unsigned char ButtonIndex,
                             unsigned char* pPayload);

#endif /* BUTTONS_H */
