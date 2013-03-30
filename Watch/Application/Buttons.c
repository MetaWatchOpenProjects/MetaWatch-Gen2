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
#include "DebugUart.h"
#include "Wrapper.h"
#include "Utilities.h"
#include "MessageQueues.h"
#include "OneSecondTimers.h"
#include "LcdDisplay.h"

// This is the number of consecutive samples by the RTC ISR that need to be
// asserted for a button to be moved to the state
#define BTN_ON_COUNT          (2)
#define BTN_ONE_SEC_COUNT     (32)
#define BTN_HOLD_COUNT        (3 * BTN_ONE_SEC_COUNT)
#define BTN_LONG_HOLD_COUNT   (5 * BTN_ONE_SEC_COUNT)
#define BTN_LONGER_HOLD_COUNT (10 * BTN_ONE_SEC_COUNT)

/* Immediate state is when a button is pressed but is not released */
#define BUTTON_STATE_IMMEDIATE ( 0 )
#define BUTTON_STATE_PRESSED   ( 1 )
#define BUTTON_STATE_HOLD      ( 2 )
#define BUTTON_STATE_LONG_HOLD ( 3 )
#define BUTTON_STATE_OFF       ( 4 )
#define BUTTON_STATE_DEBOUNCE  ( 5 )

#define BTN_A                 (0 << 4)
#define BTN_B                 (1 << 4)
#define BTN_C                 (2 << 4)
#define BTN_D                 (3 << 4)
#define BTN_E                 (4 << 4)
#define BTN_F                 (5 << 4)
#define BTN_P                 (6 << 4)

#define BTN_ACT_DISABLED      (0x80)
#define BTN_NO_MASK           (0x70)
#define BTN_NO_SHFT           (4)
#define BTN_MODE_PAGE_MASK    (0x0C)
#define BTN_MODE_PAGE_SHFT    (2)
#define BTN_EVT_MASK          (0x03)

#define IDLE_PAGE             (IDLE_MODE << 2)
#define NOTIF_PAGE            (NOTIF_MODE << 2)
#define MUSIC_PAGE            (MUSIC_MODE << 2)
#define APP_PAGE              (APP_MODE << 2)

#define INIT_PAGE             (0 << 2)
#define INFO_PAGE             (1 << 2)
#define CALL_PAGE             (2 << 2)

#define MENU_PAGE_0           (0 << 2)
#define MENU_PAGE_1           (1 << 2)
#define MENU_PAGE_2           (2 << 2)

/*! Structure to consolidate the data used to manage the button state
 *
 * \param BtnFilter is for the leaky integrator filter
 * \param State is the current button state 
 * \param BtnHoldCounter is the amount of time the button has been pressed
 */
typedef  struct
{
  unsigned char BtnFilter;          
  unsigned char State;           
  unsigned int BtnHoldCounter;     
} tButtonData;

typedef  struct
{
  unsigned char Info;
  unsigned char MsgType;
  unsigned char MsgOpt;
} tButtonAction;

#ifdef DIGITAL

#define CONN_PAGE_ACT_NUM      (20) // 3rd party needs 16

static const tButtonAction DisconnAction[] =
{
  {BTN_A | IDLE_PAGE | BTN_EVT_IMDT, ChangeModeMsg, NOTIF_MODE | MSG_OPT_UPD_INTERNAL},
  {BTN_B | IDLE_PAGE | BTN_EVT_IMDT, UpdateDisplayMsg, IDLE_MODE | MSG_OPT_NEWUI | MSG_OPT_NXT_PAGE | MSG_OPT_UPD_INTERNAL},
  {BTN_C | IDLE_PAGE | BTN_EVT_RELS, MenuModeMsg, Menu1Page},
  {BTN_D | IDLE_PAGE | BTN_EVT_IMDT, WatchStatusMsg, 0},
  {BTN_E | IDLE_PAGE | BTN_EVT_IMDT, ChangeModeMsg, MUSIC_MODE | MSG_OPT_UPD_INTERNAL},
  
  {BTN_A | NOTIF_PAGE | BTN_EVT_IMDT, ChangeModeMsg, IDLE_MODE | MSG_OPT_UPD_INTERNAL},
  {BTN_A | MUSIC_PAGE | BTN_EVT_IMDT, ChangeModeMsg, IDLE_MODE | MSG_OPT_UPD_INTERNAL},
  {BTN_A |   APP_PAGE | BTN_EVT_IMDT, ChangeModeMsg, IDLE_MODE | MSG_OPT_UPD_INTERNAL},
};
#define DISCONN_PAGE_ACT_NUM (sizeof(DisconnAction) / sizeof(tButtonAction))

static const tButtonAction InitAction[] =
{
  {BTN_A | INIT_PAGE | BTN_EVT_IMDT, ModifyTimeMsg, MODIFY_TIME_INCREMENT_MINUTE},
  {BTN_B | INIT_PAGE | BTN_EVT_IMDT, ModifyTimeMsg, MODIFY_TIME_INCREMENT_DOW},
  {BTN_C | INIT_PAGE | BTN_EVT_RELS, MenuModeMsg, Menu1Page},
  {BTN_D | INIT_PAGE | BTN_EVT_IMDT, WatchStatusMsg, 0},
  {BTN_E | INIT_PAGE | BTN_EVT_IMDT, ModifyTimeMsg, MODIFY_TIME_INCREMENT_HOUR},

  {BTN_A | INFO_PAGE | BTN_EVT_IMDT, IdleUpdateMsg, 0},
//  {BTN_C | INFO_PAGE | BTN_EVT_RELS, MenuModeMsg, Menu1Page},
  {BTN_D | INFO_PAGE | BTN_EVT_IMDT, IdleUpdateMsg, 0},

  {BTN_A | CALL_PAGE | BTN_EVT_IMDT, CallerNameMsg, SHOW_NOTIF_REJECT_CALL},
  {BTN_C | CALL_PAGE | BTN_EVT_RELS, MenuModeMsg, Menu1Page},
};
#define INIT_PAGE_ACT_NUM (sizeof(InitAction) / sizeof(tButtonAction))

static const tButtonAction MenuAction[] =
{
  {BTN_A | MENU_PAGE_0 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_TOGGLE_BLUETOOTH},
  {BTN_B | MENU_PAGE_0 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_DISPLAY_SECONDS},
  {BTN_C | MENU_PAGE_0 | BTN_EVT_RELS, MenuButtonMsg, MENU_BUTTON_OPTION_EXIT},
  {BTN_D | MENU_PAGE_0 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_TOGGLE_LINK_ALARM},
  {BTN_E | MENU_PAGE_0 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_INVERT_DISPLAY},
  
  {BTN_A | MENU_PAGE_1 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_TOGGLE_RST_NMI_PIN},
  {BTN_B | MENU_PAGE_1 | BTN_EVT_IMDT, MenuModeMsg, Menu3Page},
  {BTN_C | MENU_PAGE_1 | BTN_EVT_RELS, MenuButtonMsg, MENU_BUTTON_OPTION_EXIT},
  {BTN_D | MENU_PAGE_1 | BTN_EVT_IMDT, ResetMsg, MASTER_RESET_OPTION},
  {BTN_E | MENU_PAGE_1 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_TOGGLE_SERIAL_SBW_GND},

  {BTN_A | MENU_PAGE_2 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE},
  {BTN_B | MENU_PAGE_2 | BTN_EVT_IMDT, MenuModeMsg, Menu2Page},
  {BTN_C | MENU_PAGE_2 | BTN_EVT_RELS, MenuButtonMsg, MENU_BUTTON_OPTION_EXIT},
  {BTN_E | MENU_PAGE_2 | BTN_EVT_IMDT, MenuButtonMsg, MENU_BUTTON_OPTION_TOGGLE_ENABLE_CHARGING},
};
#define MENU_PAGE_ACT_NUM (sizeof(MenuAction) / sizeof(tButtonAction))

#else

#define CONN_PAGE_ACT_NUM      (20)

#endif

/* Allocate an array of structures to keep track of button data.  Index 4 is not
 * used, but it complicates things too much to skip it.  Everything is sized and
 * indexed like we have a 8 bit port with 8 buttons.
*/
static tButtonData ButtonData[BTN_NUM];
static tButtonAction ButtonAction[CONN_PAGE_ACT_NUM];

static unsigned char ButtonMode;
static unsigned char LastButton;

// Local function prototypes
static void ChangeButtonState(unsigned char Index, unsigned char State);
static void ButtonStateMachine(unsigned char ButtonOn, unsigned char Index);
static void HandleButtonEvent(unsigned char Index, unsigned char Event);

/******************************************************************************/

void InitButton(void)
{
  CONFIGURE_BUTTON_PINS();
  
  // Initalize the button state structures. In this case it ends up being all
  // zeros, but that may not always be the case.
  unsigned char i;
  for (i = 0; i < BTN_NUM; i++)
  {
    ButtonData[i].BtnFilter = 0;
    ButtonData[i].State = BUTTON_STATE_OFF;
    ButtonData[i].BtnHoldCounter = 0;
  }

#ifdef DIGITAL
  // init ButtonAction[] with DisconnButtonAction[]
  for (i = 0; i < DISCONN_PAGE_ACT_NUM; ++i) ButtonAction[i] = DisconnAction[i];
#endif
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
  unsigned char i;
  for(i = 0; i < BTN_NUM; ++i)
  {
    if (ButtonData[i].State != BUTTON_STATE_OFF)
    {
      if (ButtonData[i].State == BUTTON_STATE_DEBOUNCE)
        ButtonMode = CurrentMode;
      
      ButtonStateMachine(portBtns & (0x01 << (i >= SW_UNUSED_INDEX ? i + 1 : i)), i);
    }
  }
}

static void ButtonStateMachine(unsigned char ButtonOn, unsigned char Index)
{
  if (ButtonOn)
  {
    if (ButtonData[Index].State == BUTTON_STATE_OFF ||
        ButtonData[Index].State == BUTTON_STATE_DEBOUNCE)
    {  
      // Make it more "On"
      ButtonData[Index].BtnFilter++;  
      
      if (ButtonData[Index].BtnFilter == BTN_ON_COUNT)
      {
        ChangeButtonState(Index, BUTTON_STATE_PRESSED);
      }
    }
    else  // it's on one of the on (pressed) states.
    {
      if (ButtonData[Index].BtnHoldCounter < 65535)
      {
        ButtonData[Index].BtnHoldCounter++;
      }
              
      if (ButtonData[Index].BtnHoldCounter == BTN_HOLD_COUNT)
      {
        PrintS("btn HOLD");
        ChangeButtonState(Index, BUTTON_STATE_HOLD);
      }
    }
  }
  else  // The button is not pressed, but it may still be in the on state
  {
    // see if we still have a filter count (still in the on state)
    if (ButtonData[Index].BtnFilter > 0)
    {
      ButtonData[Index].BtnFilter --;  //  make it closer to off

      // When we reach a filter count of zero, by definition we are in
      // the off state
      if (ButtonData[Index].BtnFilter == 0)
      {
        // Don't go from the off state to the off state.  If we ramp
        // up but don't change to the BtnPressed state then that is
        // due to switch bounce
        if (ButtonData[Index].State != BUTTON_STATE_OFF)
        {
          ChangeButtonState(Index, BUTTON_STATE_OFF); 
          
          // Clear the hold counter so we start at the original off state
          ButtonData[Index].BtnHoldCounter = 0;
        
          // If this one is off, maybe they all are off.  When all
          // buttons are off, we stop the button timer ISR
          /*! Disable the button debounce ISR when all buttons are in the off state. */
          // Go through the array of button structs looking for any buttons that are
          // still on the on (not off) state.
          unsigned char i;
          for (i = 0; i < BTN_NUM; ++i)
            if (ButtonData[i].State != BUTTON_STATE_OFF) break;
          
          // If all the buttons are off then stop the ISR
          if (i == BTN_NUM) DisableRtcPrescaleInterruptUser(RTC_TIMER_BUTTON);
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
 * \param Index index of the button ( 0 to 7 )
 * \param State the button state to change to
 *
 */
static void ChangeButtonState(unsigned char Index, unsigned char State)
{
  /* pressing and releasing the button will cause menu change 
   *
   * but holding the button will cause execution
   *
   */
  if (State == BUTTON_STATE_PRESSED &&
      ButtonData[Index].State == BUTTON_STATE_DEBOUNCE)
  {
    HandleButtonEvent(Index, BTN_EVT_IMDT);
    
    /* need to know that we are in the immediate state, but go to the 
     * button pressed state 
     */
    State = BUTTON_STATE_PRESSED;
  }
  else if (State == BUTTON_STATE_HOLD)
  {
    HandleButtonEvent(Index, BTN_EVT_HOLD);
  }  
  else if (State == BUTTON_STATE_OFF &&
           ButtonData[Index].State == BUTTON_STATE_PRESSED)
  {
    HandleButtonEvent(Index, BTN_EVT_RELS);
  }
  
  /* Update the state of the specified button after we have detected
   * a possible edge
   */
  ButtonData[Index].State = State;
}

/*! A valid button event has occurred.  Now send a message
 *
 * \param unsigned char Index
 * \param unsigned char Event
 */
static void HandleButtonEvent(unsigned char Index, unsigned char Event)
{
  tMessage Msg;

//  PrintS(tringAndTwoDecimals("- BtnEvt i:", Index, "e:", Event);
//  PrintS(tringAndHexByte("LstBF:0x", LastButton);

#ifdef DIGITAL
  unsigned char Done = pdFALSE;

  if (Event == BTN_EVT_HOLD)
  {
    if (Index == BTN_INDEX_C && LastButton == (BTN_INDEX_F << BTN_NO_SHFT | BTN_EVT_RELS) &&
        CurrentIdlePage() == Menu1Page && BackLightOn())
    {
      SendMessage(&Msg, ServiceMenuMsg, MSG_OPT_NONE);
      Done = pdTRUE;
    }
    else if (Index == BTN_INDEX_B && LastButton == (BTN_INDEX_E << BTN_NO_SHFT | BTN_EVT_HOLD) ||
             Index == BTN_INDEX_E && LastButton == (BTN_INDEX_B << BTN_NO_SHFT | BTN_EVT_HOLD) ||
             Index == BTN_INDEX_F)
    {
      SendMessage(&Msg, ResetMsg, Index == BTN_INDEX_F ? MSG_OPT_NONE : MASTER_RESET_OPTION);
      Done = pdTRUE;
    }
  }
  else if (Event == BTN_EVT_RELS)
  {
//    if (ButtonMode != IDLE_MODE) ResetModeTimer();

    if (BackLightOn() || Index == BTN_INDEX_F)
    {
      SendMessage(&Msg, SetBacklightMsg, LED_ON_OPTION);
      if (Index == BTN_INDEX_F) Done = pdTRUE;
    }
  }
    
  if (Index != LastButton >> BTN_NO_SHFT && (Event == BTN_EVT_HOLD || Event == BTN_EVT_RELS))
    LastButton = (Index << BTN_NO_SHFT) | Event;
  
//  PrintS(tringAndHexByte("-LstAF:0x", LastButton);

  if (Done) return;
  
  const tButtonAction *pAction;
  unsigned char ActNum, ModePage;
  
  if (PageType == PAGE_TYPE_MENU)
  {
    pAction = MenuAction;
    ActNum = MENU_PAGE_ACT_NUM;
    ModePage = CurrentIdlePage() - Menu1Page;
  }
  else if (CurrentIdlePage() == DisconnectedPage)
  {// disconnected
    pAction = DisconnAction;
    ActNum = DISCONN_PAGE_ACT_NUM;
    ModePage = ButtonMode;
  }
  else if (CurrentIdlePage() == ConnectedPage)
  {
    pAction = ButtonAction;
    ActNum = CONN_PAGE_ACT_NUM;
    ModePage = ButtonMode;
  }
  else
  {// InitPage, StatusPage or CallPage
    pAction = InitAction;
    ActNum = INIT_PAGE_ACT_NUM;
    ModePage = CurrentIdlePage() - InitPage;
  }
  
//  PrintS(tringAndThreeDecimals("- CurrP:", CurrentIdlePage(), " ActNum:", ActNum, " ModePage:", ModePage);

#else

  const tButtonAction *pAction;
  unsigned char ActNum, ModePage;

  pAction = ButtonAction;
  ActNum = CONN_PAGE_ACT_NUM;
  ModePage = CurrentIdlePage();

#endif

  unsigned char i;
  for (i = 0; i < ActNum; ++i)
  {
    if ((pAction[i].Info & BTN_NO_MASK) >> BTN_NO_SHFT == Index &&
        (pAction[i].Info & BTN_MODE_PAGE_MASK) >> BTN_MODE_PAGE_SHFT == ModePage &&
        (pAction[i].Info & BTN_EVT_MASK) == Event) break;
  }
  
  if (i == ActNum) return;
  
  //PrintS(tringAndDecimal("-No mask:", Event);
  /* if this button press is going to the bluetooth then allocate
   * a buffer and add the button index
   */
  if (pAction[i].MsgType == ButtonEventMsg)
  {
    PrintF("- BtnEvtMsg:Evt:%d", Event);
    SetupMessageWithBuffer(&Msg, pAction[i].MsgType, pAction[i].MsgOpt);
    if (Msg.pBuffer != NULL)
    {
      Msg.pBuffer[0] = (Index >= SW_UNUSED_INDEX) ? Index + 1 : Index;
      Msg.pBuffer[1] = ButtonMode;
      Msg.pBuffer[2] = Event;
      Msg.pBuffer[3] = pAction[i].MsgType;
      Msg.pBuffer[4] = pAction[i].MsgOpt;
      Msg.Length = 5;
      RouteMsg(&Msg);
    }
  }
  else if (pAction[i].MsgType != InvalidMessage)
  {
    SendMessage(&Msg, pAction[i].MsgType, pAction[i].MsgOpt);
  }
}

/*! Attach callback to button press type. Each button press type is associated
 * with a display mode.
 *
 * No error checking
 *
 * \param tHostMsg* pMsg - A message with a tButtonActionPayload payload
 */
void EnableButtonMsgHandler(tMessage* pMsg)
{
  tButtonActionPayload *pAction = (tButtonActionPayload*)pMsg->pBuffer;
  if (pAction->ButtonIndex > SW_UNUSED_INDEX) pAction->ButtonIndex --;

//  PrintS(tringAndThreeDecimals("-M:", pAction->DisplayMode, " i:", pAction->ButtonIndex,
//                              "E:", pAction->ButtonEvent);
//  PrintS(tringAndTwoHexBytes(" M O:0x", pAction->CallbackMsgType, pAction->CallbackMsgOptions);
//
  /* redefine backlight key is not allowed */
  if (pAction->ButtonIndex == BTN_INDEX_F) return;

  unsigned char BtnInfo = pAction->DisplayMode << BTN_MODE_PAGE_SHFT |
                          pAction->ButtonIndex << BTN_NO_SHFT |
                          pAction->ButtonEvent;
  
  unsigned char i, k = CONN_PAGE_ACT_NUM;
  
  for (i = 0; i < CONN_PAGE_ACT_NUM; ++i)
  {
    if (ButtonAction[i].Info == BtnInfo) break;
    else if (ButtonAction[i].MsgType == InvalidMessage && k == CONN_PAGE_ACT_NUM) k = i;
  }
  
  if (i == CONN_PAGE_ACT_NUM && k < CONN_PAGE_ACT_NUM) i = k;
  
  if (i < CONN_PAGE_ACT_NUM)
  {
    ButtonAction[i].Info = BtnInfo;
    ButtonAction[i].MsgType = pAction->CallbackMsgType;
    ButtonAction[i].MsgOpt = pAction->CallbackMsgOptions;
  }
  else PrintS("# DefBtn");
}

/*! Remove callback for the specified button press type.
 * Each button press type is associated with a display mode.
 *
 * \param tHostMsg* pMsg - A message with a tButtonActionPayload payload
 */
void DisableButtonMsgHandler(tMessage* pMsg)
{
  tButtonActionPayload *pAction = (tButtonActionPayload*)pMsg->pBuffer;
  if (pAction->ButtonIndex > SW_UNUSED_INDEX) pAction->ButtonIndex --;

  unsigned char BtnInfo = pAction->DisplayMode << BTN_MODE_PAGE_SHFT |
                          pAction->ButtonIndex << BTN_NO_SHFT |
                          pAction->ButtonEvent;
  
  unsigned char i;
  for (i = 0; i < CONN_PAGE_ACT_NUM; ++i)
  {
    if (ButtonAction[i].Info == BtnInfo) ButtonAction[i].MsgType = InvalidMessage;
  }
}

/*! Read configuration of a specified button.  This is used to read the
 * configuration of a button that needs to be restored at a later time
 * by the application.
 *
 * \param tHostMsg* pMsg - A message with a tButtonActionPayload payload
 */
void ReadButtonConfigHandler(tMessage* pMsg)
{
  tMessage Msg;
  SetupMessageWithBuffer(&Msg, ReadButtonConfigResponse, MSG_OPT_NONE);
  if (Msg.pBuffer == NULL) return;

  Msg.Length = pMsg->Length;

  unsigned char i = 0;
  for (; i< Msg.Length; ++i) Msg.pBuffer[i] = pMsg->pBuffer[i];

  tButtonActionPayload *pAction = (tButtonActionPayload*)pMsg->pBuffer;
  unsigned char BtnInfo = pAction->DisplayMode << BTN_MODE_PAGE_SHFT |
                          pAction->ButtonIndex << BTN_NO_SHFT |
                          pAction->ButtonEvent;
  
  
  for (i = 0; i < CONN_PAGE_ACT_NUM; ++i)
    if (ButtonAction[i].Info == BtnInfo) break;

  if (i < CONN_PAGE_ACT_NUM)
  {
    if (Msg.pBuffer[1] >= SW_UNUSED_INDEX) Msg.pBuffer[1] ++;
    Msg.pBuffer[3] = ButtonAction[i].MsgType;
    Msg.pBuffer[4] = ButtonAction[i].MsgOpt;
  }
  
  RouteMsg(&Msg);
}

/*******************************************************************************
Purpose: Interrupt handler for the port 2 ISR.  Port 2 is configured as interrupt
generating I/O.  All pins except P.4 have a normally open button to
ground.  The internal resistor pullups are used to keep the pin normally
high.  When the button is pressed, the pin is pulled low and an
interrupt is generated.
*******************************************************************************/

#ifndef __IAR_SYSTEMS_ICC__
#pragma CODE_SECTION(ButtonPortIsr, ".text:_isr");
#endif

#pragma vector=BUTTON_PORT_VECTOR

__interrupt void ButtonPortIsr(void)
{
  unsigned char ButtonInterruptFlags = BUTTON_PORT_IFG;
  unsigned char StartDebouncing = 0;
    
  unsigned char i;
  for (i = 0; i < BTN_NUM; ++i) // include button F
  {
    if (ButtonInterruptFlags & (1 << (i >= SW_UNUSED_INDEX ? i + 1 : i)))
    {
      if (ButtonData[i].State == BUTTON_STATE_OFF)
      {
        ButtonData[i].State = BUTTON_STATE_DEBOUNCE; 
      }
      StartDebouncing = 1;
    }
  }
  
  BUTTON_PORT_IFG = 0;

  if (StartDebouncing)
  {
    EnableRtcPrescaleInterruptUser(RTC_TIMER_BUTTON); 
  }
}

