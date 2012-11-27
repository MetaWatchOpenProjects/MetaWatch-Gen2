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

#include <stdio.h>
#include <string.h>

#include "hal_board_type.h"
#include "hal_lpm.h"
#include "hal_miscellaneous.h"
#include "hal_calibration.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Wrapper.h"

#include "TestMode.h"

/******************************************************************************/

/* don't forget null character */
#define MIN_COMMAND_LENGTH    ( 4 )
#define MAX_COMMAND_LENGTH    ( 8 )
#define DELIMITER_CHARACTER   ( 0x0D )
#define END_OF_XMIT_CHARACTER ( 0x04 )

/******************************************************************************/

static unsigned char RxIndex;

/******************************************************************************/

tString CmdString[MAX_COMMAND_LENGTH];

/******************************************************************************/

static unsigned char WaitForCommandToBeProcessed;

/******************************************************************************/

typedef struct
{
  const tString CommandNameString[MAX_COMMAND_LENGTH];
  void (*fpHandler)(void);  
  
} tCommand;

/******************************************************************************/

static void HelpCmdHandler(void);
static void BootloaderHandler(void);

/******************************************************************************/

const tCommand COMMAND_TABLE[] =
{
/*->0123456789ABCDEF<- max command length */
  {"help", HelpCmdHandler},
  {"whoami", WhoAmI},
  {"boot", BootloaderHandler},
  {"ship", SetShippingModeFlag},
  {"reset", SoftwareReset},
  
  /* this is the Get Watch Info command that comes from the PC tool */
  {{0x01,0x10,0x03,0x3f,0x3f,0x3f,0x30}, BootloaderHandler}
}; 

#define NUMBER_OF_COMMANDS ( sizeof(COMMAND_TABLE)/sizeof(tCommand) )

/******************************************************************************/

void InitTestMode(void)
{ 
  unsigned char i;
  for ( i = 0; i < MAX_COMMAND_LENGTH; i++ )
  {
    CmdString[i] = 0;
  }
  
  RxIndex = 0;
}


/******************************************************************************/

unsigned char RxTestModeCharacterIsr(unsigned char Character)
{
  unsigned char ExitLpm = 0;
  
  if ( WaitForCommandToBeProcessed == 0 )
  {
    CmdString[RxIndex++] = Character;  
  
    /* parse things based on carriage return */
    if (  ( Character == DELIMITER_CHARACTER || Character == END_OF_XMIT_CHARACTER )
        && RxIndex > MIN_COMMAND_LENGTH )
    {
      /* change delimiter to null */
      CmdString[RxIndex-1] = 0;
      WaitForCommandToBeProcessed = 1;

      /* send message so that message can be processed */
      tMessage Msg;
      SetupMessage(&Msg, TestModeMsg, MSG_OPT_NONE);
      SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);

      ExitLpm = 1;
    }
    else if ( RxIndex > MAX_COMMAND_LENGTH-1 ) /* always let last char be 0 */
    {
      RxIndex = 0;  
    }
  }
  
  return ExitLpm;
}

/******************************************************************************/

void TestModeCommandHandler(void)
{
  /* see if the command is in the table */
  unsigned char i = 0;
  unsigned char match = 0;
  
  /* the more commands there are - the longer this will take */
  while ( i < NUMBER_OF_COMMANDS && !match )
  {
    /* match command to one in the table */
    if ( strcmp(CmdString,COMMAND_TABLE[i++].CommandNameString) == 0 )
    {
      match = 1;
    }
  }
  
  /* if the command exists then call the handler */
  if ( match )
  {
    COMMAND_TABLE[i-1].fpHandler();
  }
  else
  {
    PrintString3("Unknown Command ",CmdString,CR);
  }
   
  /* clear flags used by rx interrupt */
  portENTER_CRITICAL();
  RxIndex = 0;
  WaitForCommandToBeProcessed = 0;
  portEXIT_CRITICAL();
}


/******************************************************************************/

static void HelpCmdHandler(void)
{
  PrintString("\r\n************ List of Commands ****************\r\n");
  
  unsigned char i = 0;
  /* don't print the last command because it is for PC tool */
  while ( i < NUMBER_OF_COMMANDS-1 )  
  {
    PrintString2((tString*)COMMAND_TABLE[i].CommandNameString,CR);
    i++;
  }
}

/* this function probably belongs somewhere else */
void WhoAmI(void)
{
  extern const char BUILD[];
  extern const char VERSION[];
  PrintString3("Version: ", VERSION,CR);
  PrintString3("Build: ", BUILD,CR);
  
  tVersion Version = GetWrapperVersion();
  PrintString3("Wrapper: ", Version.pSwVer,CR);
  
  PrintString2(SPP_DEVICE_NAME,CR);
  PrintString("Msp430 Version ");
  PrintCharacter(GetMsp430HardwareRevision());
  PrintString(CR);
  
  PrintStringAndDecimal("HwVersion: ", HardwareVersion());
}

/******************************************************************************/

static void BootloaderHandler(void)
{
  PrintString("bootloader handler\r\n");  
  CreateAndSendMessage(MenuButtonMsg, MENU_BUTTON_OPTION_ENTER_BOOTLOADER_MODE);
}
