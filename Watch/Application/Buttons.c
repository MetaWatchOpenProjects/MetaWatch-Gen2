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
/*! \file Buttons.c
*
*/
/******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "Messages.h"

#include "hal_lpm.h"
#include "hal_board_type.h"
#include "hal_rtc.h"
#include "hal_vibe.h"

#include "Buttons.h"
#include "Background.h"
#include "Buttons.h"
#include "DebugUart.h"
#include "Wrapper.h"
#include "Utilities.h"
#include "MessageQueues.h"
#include "Display.h"
#include "OneSecondTimers.h"

/* Allocate an array of structures to keep track of button data.  Index 4 is not
 * used, but it complicates things too much to skip it.  Everything is sized and
 * indexed like we have a 8 bit port with 8 buttons.
*/
static tButtonData ButtonData[NUMBER_OF_BUTTONS];

// Local function prototypes
static void ChangeButtonState(unsigned char btnIndex, unsigned char btnState);
static void CheckForAllButtonsOff(void);

static void InitializeButtonDataStructures(void);


static void ButtonStateMachine(unsigned char ButtonOn,
                               unsigned char btnIndex);

tButtonConfiguration ButtonCfg[NUMBER_OF_BUTTON_MODES][NUMBER_OF_BUTTONS];

static const tButtonConfiguration cUnusedButtonConfiguration = 
{
  (BUTTON_ABSOLUTE_MASK | ALL_BUTTON_EVENTS_MASKED),
  {InvalidMessage,InvalidMessage,InvalidMessage,InvalidMessage},
  {NO_MSG_OPTIONS,NO_MSG_OPTIONS,NO_MSG_OPTIONS,NO_MSG_OPTIONS}
};


static void InitializeButtonConfigurationStructure(void);

unsigned char GetAbsoluteButtonMask(unsigned char ButtonIndex);
unsigned char GetButtonImmediateModeMask(unsigned char ButtonIndex);

static void HandleButtonEvent(unsigned char ButtonIndex,
                              unsigned char ButtonPressType);

static tMessage OutgoingEventMsg;
        
/******************************************************************************/

void InitializeButtons(void)
{
  CONFIGURE_BUTTON_PINS();
  
  InitializeButtonDataStructures();
  
  InitializeButtonConfigurationStructure();
}

static void InitializeButtonDataStructures(void)
{
  // Initalize the button state structures. In this case it ends up being all
  // zeros, but that may not always be the case.
  unsigned char ii;
  for(ii = 0; ii < NUMBER_OF_BUTTONS; ii++)
  {
      ButtonData[ii].BtnFilter = 0;
      ButtonData[ii].BtnState = BUTTON_STATE_OFF;
      ButtonData[ii].BtnHoldCounter = 0;
  }
  
}

/*! This is the event handler for the Button Event Message that is called
 * from the command task
 *
 *  DO NOT START THE BUTTON TIMER unless there is at least one bit set for
 *  one of the buttons.  Otherwise the state machine sees the buttons as all off
 *  already and never stops the timer.
 *
 *  When it is detected that no buttons are pressed and timeouts have completed
 *  the button timer ISR is disabled.
 *
 * A byte indicating the state of each button (one bit per button)
 *
 */
void ButtonStateHandler(void)
{
  // NOTE The BTN_PORT_IN has a tilde or not depending on the direction of the
  // bits on the buttons.  This converts the read to positive logic where a
  // pressed button is a "1"
  unsigned char portBtns = (BUTTON_PORT_IN);
  
  // This is the loop that handles managing the button state machine.
  // because this a state machine the mask must be applied again.
  unsigned char btnIndex;
  for(btnIndex = 0; btnIndex < NUMBER_OF_BUTTONS; btnIndex++)
  {
    if ( ButtonData[btnIndex].BtnState != BUTTON_STATE_OFF )
    {
      ButtonStateMachine(portBtns & (0x01<<btnIndex),btnIndex);
    }
  }
  
}

static void ButtonStateMachine(unsigned char ButtonOn,
                               unsigned char btnIndex)
{
  if ( ButtonOn )
  {
    if(   ButtonData[btnIndex].BtnState == BUTTON_STATE_OFF
       || ButtonData[btnIndex].BtnState == BUTTON_STATE_DEBOUNCE )
    {  
      // Make it more "On"
      ButtonData[btnIndex].BtnFilter++;  
      
      if(ButtonData[btnIndex].BtnFilter == BTN_ON_COUNT)
      {
        ChangeButtonState(btnIndex, BUTTON_STATE_PRESSED);
      }
    }
    else  // it's on one of the on (pressed) states.
    {
      if ( ButtonData[btnIndex].BtnHoldCounter < 65535 )
      {
        ButtonData[btnIndex].BtnHoldCounter++;
      }
              
      
      if(ButtonData[btnIndex].BtnHoldCounter == BTN_HOLD_COUNT)
      {
        ChangeButtonState(btnIndex, BUTTON_STATE_HOLD);
      }
      
      if(ButtonData[btnIndex].BtnHoldCounter == BTN_LONG_HOLD_COUNT)
      {
        ChangeButtonState(btnIndex, BUTTON_STATE_LONG_HOLD);
      }
    
    }
  
  }
  else  // The button is not pressed, but it may still be in the on state
  {

    // see if we still have a filter count (still in the on state)
    if( ButtonData[btnIndex].BtnFilter > 0 )
    {
      ButtonData[btnIndex].BtnFilter--;  //  make it closer to off

      // When we reach a filter count of zero, by definition we are in
      // the off state
      if( ButtonData[btnIndex].BtnFilter == 0 )
      {
        // Don't go from the off state to the off state.  If we ramp
        // up but don't change to the BtnPressed state then that is
        // due to switch bounce
        if(ButtonData[btnIndex].BtnState != BUTTON_STATE_OFF)
        {
          ChangeButtonState(btnIndex, BUTTON_STATE_OFF); 
          
          // Clear the hold counter so we start at the original off state
          ButtonData[btnIndex].BtnHoldCounter = 0;
        
          // If this one is off, maybe they all are off.  When all
          // buttons are off, we stop the button timer ISR
          CheckForAllButtonsOff( );
        }
      }
    }
  }  // button on or off
}

/*! Changes the state variable associated with the button specified
 *
 * The purpose of the state change being a function call is so that there is a
 * consistent place to determine a state change.  State changes normally cause
 * an event, so we want only one place that does that.
 *
 * \param btnIndex index of the button ( 0 to 7 )
 * \param btnState the button state to change to
 *
 */
static void ChangeButtonState(unsigned char btnIndex, unsigned char btnState)
{

  /* pressing and releasing the button will cause menu change 
   *
   * but holding the button will cause execution
   *
   */
  if (   btnState == BUTTON_STATE_PRESSED
      && ButtonData[btnIndex].BtnState == BUTTON_STATE_DEBOUNCE
      && GetButtonImmediateModeMask(btnIndex) == 0)
  {
    HandleButtonEvent(btnIndex,BUTTON_STATE_IMMEDIATE);
    
    /* need to know that we are in the immediate state, but go to the 
     * button pressed state 
     */
    btnState = BUTTON_STATE_PRESSED;
  }
  else if (   btnState == BUTTON_STATE_OFF 
           && ButtonData[btnIndex].BtnState == BUTTON_STATE_PRESSED )
  {
    HandleButtonEvent(btnIndex,BUTTON_STATE_PRESSED);
  }
  else if (   btnState == BUTTON_STATE_OFF 
           && ButtonData[btnIndex].BtnState == BUTTON_STATE_HOLD )
  {
    HandleButtonEvent(btnIndex,BUTTON_STATE_HOLD);
  }
  else if (   btnState == BUTTON_STATE_OFF 
           && ButtonData[btnIndex].BtnState == BUTTON_STATE_LONG_HOLD )
  {
    HandleButtonEvent(btnIndex,BUTTON_STATE_LONG_HOLD);  
  }
  
  /* Update the state of the specified button after we have detected
   * a possible edge
   */
  ButtonData[btnIndex].BtnState = btnState;

}

/*! Enable button callback */
void EnableButtonAction(unsigned char DisplayMode,
                        unsigned char ButtonIndex,
                        unsigned char ButtonPressType,
                        unsigned char CallbackMsgType,
                        unsigned char CallbackMsgOptions)
{
  tButtonConfiguration* pLocalCfg = &(ButtonCfg[DisplayMode][ButtonIndex]);
  
  /* disable mask */  
  switch (ButtonPressType)
  {
  case BUTTON_STATE_IMMEDIATE:
    pLocalCfg->MaskTable &= ~(BUTTON_ABSOLUTE_MASK  | BUTTON_IMMEDIATE_MASK);
    break;
  case BUTTON_STATE_PRESSED:
    pLocalCfg->MaskTable &= ~(BUTTON_ABSOLUTE_MASK | BUTTON_PRESS_MASK);
    break;
  case BUTTON_STATE_HOLD:
    pLocalCfg->MaskTable &= ~(BUTTON_ABSOLUTE_MASK | BUTTON_HOLD_MASK);
    break;
  case BUTTON_STATE_LONG_HOLD:
    pLocalCfg->MaskTable &= ~(BUTTON_ABSOLUTE_MASK | BUTTON_LONG_HOLD_MASK);
    break;
  default:
    break;
  }

  pLocalCfg->CallbackMsgType[ButtonPressType] = CallbackMsgType;
  pLocalCfg->CallbackMsgOptions[ButtonPressType] = CallbackMsgOptions;

}

/*! Disable button callback for the specified mode and button press type */
void DisableButtonAction(unsigned char DisplayMode,
                         unsigned char ButtonIndex,
                         unsigned char ButtonPressType)
{
  tButtonConfiguration* pLocalCfg = &(ButtonCfg[DisplayMode][ButtonIndex]);
  
  /* disable mask */  
  switch (ButtonPressType)
  {
  case BUTTON_STATE_IMMEDIATE:
    pLocalCfg->MaskTable |= BUTTON_IMMEDIATE_MASK;
    break;
  case BUTTON_STATE_PRESSED:
    pLocalCfg->MaskTable |= BUTTON_PRESS_MASK;
    break;
  case BUTTON_STATE_HOLD:
    pLocalCfg->MaskTable |= BUTTON_HOLD_MASK;
    break;
  case BUTTON_STATE_LONG_HOLD:
    pLocalCfg->MaskTable |= BUTTON_LONG_HOLD_MASK;
    break;
  default:
    break;
  }

  /* if all of the button actions are masked then
   * turn on the absolute mask
   */
  if ( pLocalCfg->MaskTable == ALL_BUTTON_EVENTS_MASKED )
  {
    pLocalCfg->MaskTable |= BUTTON_ABSOLUTE_MASK;  
  }
  
  pLocalCfg->CallbackMsgType[ButtonPressType] = InvalidMessage;
  pLocalCfg->CallbackMsgOptions[ButtonPressType] = 0;

}

/*! Read a button configuration 
 *
 * Another function could be added to read the entire structure
 */
void ReadButtonConfiguration(unsigned char DisplayMode,
                             unsigned char ButtonIndex,
                             unsigned char ButtonPressType,
                             unsigned char* pPayload)
{
  tButtonConfiguration* pLocalCfg = &(ButtonCfg[DisplayMode][ButtonIndex]);
  
  pPayload[0] = DisplayMode;
  pPayload[1] = ButtonIndex;
  pPayload[2] = pLocalCfg->MaskTable;
  pPayload[3] = pLocalCfg->CallbackMsgType[ButtonPressType];
  pPayload[4] = pLocalCfg->CallbackMsgOptions[ButtonPressType];

}

/*! Set all of the buttons to unused */
static void InitializeButtonConfigurationStructure(void)
{
  unsigned char DisplayMode = 0;  
  unsigned char ButtonIndex = 0;
  
  for ( DisplayMode = 0; DisplayMode < NUMBER_OF_BUTTON_MODES; DisplayMode++ )
  {
    for ( ButtonIndex = 0; ButtonIndex < NUMBER_OF_BUTTONS; ButtonIndex++ )
    {
      ButtonCfg[DisplayMode][ButtonIndex] = cUnusedButtonConfiguration;   
    }
  }
    
}

/*! A valid button event has occurred.  Now send a message
 *
 * \param unsigned char ButtonIndex
 * \param unsigned char ButtonPressType
 */
static void HandleButtonEvent(unsigned char ButtonIndex,
                              unsigned char ButtonPressType)
{
  tButtonConfiguration* pLocalCfg = &(ButtonCfg[QueryButtonMode()][ButtonIndex]);
  
  eMessageType Type = (eMessageType)pLocalCfg->CallbackMsgType[ButtonPressType];
  unsigned char Options = pLocalCfg->CallbackMsgOptions[ButtonPressType];
  
  if ( (pLocalCfg->MaskTable & (1 << ButtonPressType)) == 0 )
  {
    /* if the message type is non-zero then generate a message */
    if ( Type != InvalidMessage )
    {
        /* if this button press is going to the bluetooth then allocate
         * a buffer and add the button index
         */
        if ( Type == ButtonEventMsg )
        {
          SetupMessageAndAllocateBuffer(&OutgoingEventMsg,Type,Options);
          OutgoingEventMsg.pBuffer[0] = ButtonIndex;
          OutgoingEventMsg.Length = 1;
        }
        else
        {
          SetupMessage(&OutgoingEventMsg,Type,Options);   
        }

        RouteMsg(&OutgoingEventMsg);
        
    }
        
  }
  
}
 
/*! 
 * \note buttons masks are stored as '1' equals mask so it must be inverted 
 * before it is used 
 *
 * \return 0 when bit is enabled, 1 when masked (mask==ignore)
 */
unsigned char GetAbsoluteButtonMask(unsigned char ButtonIndex)
{
  unsigned char Rval = 0;
  
  Rval = ButtonCfg[QueryButtonMode()][ButtonIndex].MaskTable;
  Rval &= BUTTON_ABSOLUTE_MASK;
  Rval = Rval << ButtonIndex;  
  
  return Rval;
}

/*! Determines if the immediate press of a button is masked.
 * 
 * \return 0 when bit is enabled, 1 when masked (mask==ignore)
 */
unsigned char GetButtonImmediateModeMask(unsigned char ButtonIndex)
{
  unsigned char Rval = 0;
  
  Rval = ButtonCfg[QueryButtonMode()][ButtonIndex].MaskTable;
  Rval &= BUTTON_IMMEDIATE_MASK;
  Rval = Rval << ButtonIndex;  
  
  return Rval;  
}


/*! Disable the button debounce ISR when all buttons are in the off state. */
static void CheckForAllButtonsOff( )
{
  unsigned char btnIndex;
  unsigned char AllButtonsOff = 1;
  
  // Go through the array of button structs looking for any buttons that are
  // still on the on (not off) state.
  for(btnIndex = 0; btnIndex < NUMBER_OF_BUTTONS; btnIndex++)
  {
    if(ButtonData[btnIndex].BtnState != BUTTON_STATE_OFF)
    {
     AllButtonsOff = 0;
     break;        
    }
  }
  
  // If all the buttons are off then stop the ISR
  if(AllButtonsOff)
  {
    PrintString("AllButtonsOff\r\n");
  
    DisableRtcPrescaleInterruptUser(RTC_TIMER_BUTTON);
  }

}

/*******************************************************************************

Purpose: Interrupt handler for the port 2 ISR.  Port 2 is configured as interrupt
generating I/O.  All pins except P.4 have a normally open button to
ground.  The internal resistor pullups are used to keep the pin normally
high.  When the button is pressed, the pin is pulled low and an
interrupt is generated.

*******************************************************************************/
#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(ButtonPortIsr,".text:_isr");
#endif

#pragma vector=BUTTON_PORT_VECTOR
__interrupt void ButtonPortIsr(void)
{
  unsigned char ButtonInterruptFlags = BUTTON_PORT_IFG;
  unsigned char StartDebouncing = 0;
    
  unsigned char i;
  for (i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    /* if the button bit position is one then determine 
     * if the button should be masked 
     */
    unsigned char temp = (ButtonInterruptFlags & (1<<i)) & ~GetAbsoluteButtonMask(i);
    
    if ( temp )
    {
      if ( ButtonData[i].BtnState == BUTTON_STATE_OFF )
      {
        ButtonData[i].BtnState = BUTTON_STATE_DEBOUNCE; 
      }
      StartDebouncing = 1;
    }
  }
  
  BUTTON_PORT_IFG = 0;

  if(StartDebouncing)
  {
    EnableRtcPrescaleInterruptUser(RTC_TIMER_BUTTON); 
  }

}



