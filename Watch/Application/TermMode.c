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

#include "FreeRTOS.h"
#include "queue.h"
#include "Messages.h"
#include "MessageQueues.h"

#include "DebugUart.h"
#include "hal_lpm.h"
#include "hal_clock_control.h"
#include "TermMode.h"
#include "LcdDisplay.h"
#include "hal_board_type.h"
#include "hal_boot.h"

/* don't forget null character */
#define MAX_COMMAND_LENGTH    ( 8 )
#define DELIMITER             ( 0x0D )
#define END_OF_XMIT           ( 0x04 )

static char CmdBuf[MAX_COMMAND_LENGTH];
static unsigned char ReceiveCompleted;

static void HelpCmdHandler(void);

typedef struct
{
  const char Cmd[MAX_COMMAND_LENGTH];
  void (*fpHandler)(void);  
} tCommand;

static const tCommand COMMAND_TABLE[] =
{
  {"boot", EnterBootloader},
  {"help", HelpCmdHandler},
  {"whoami", WhoAmI},
  {"reset", SoftwareReset},
  {"ship", EnableShippingMode},
  {{0x01,0x10,0x03,'?','?','?', 0x30}, EnterBootloader} // Metaboot
};
#define NUMBER_OF_COMMANDS (sizeof(COMMAND_TABLE)/sizeof(tCommand))

/******************************************************************************/

void EnableTermMode(unsigned char Enable)
{
  if (Enable)
  {
    ReceiveCompleted = pdFALSE;
    EnableSmClkUser(TERM_MODE_USER);
    /* enable Rx interrupt */
    UCA3IE |= UCRXIE;
  }
  else
  {
    UCA3IE &= ~UCRXIE;
    DisableSmClkUser(TERM_MODE_USER);
  }

  PrintF("%sEnTermMode", Enable ? OK : NOK);
}

/******************************************************************************/

unsigned char TermModeIsr(void)
{
  static unsigned char i = 0;

  if (!ReceiveCompleted)
  {
    CmdBuf[i] = UCA3RXBUF;
    
    /* parse things based on carriage return */
    if (CmdBuf[i] == DELIMITER || CmdBuf[i] == END_OF_XMIT)
    {
      /* change delimiter to null */
      CmdBuf[i] = '\0';
      i = 0;
      ReceiveCompleted = pdTRUE;

      /* send message so that message can be processed */
      tMessage Msg;
      SetupMessage(&Msg, TermModeMsg, MSG_OPT_NONE);
      SendMessageToQueueFromIsr(DISPLAY_QINDEX, &Msg);

      return pdTRUE;
    }
    else i = (i < MAX_COMMAND_LENGTH - 1) ? i + 1 : 0;
  }
  
  return pdFALSE;
}

/******************************************************************************/

void TermModeHandler(void)
{
  /* see if the command is in the table */
  unsigned char i = 0;
  
  /* the more commands there are - the longer this will take */
  while (i < NUMBER_OF_COMMANDS && strcmp(CmdBuf, COMMAND_TABLE[i].Cmd)) i ++;
  
  /* if the command exists then call the handler */
  if (i < NUMBER_OF_COMMANDS) COMMAND_TABLE[i].fpHandler();
  else PrintS(CmdBuf);

  ReceiveCompleted = pdFALSE;
}

/******************************************************************************/

static void HelpCmdHandler(void)
{
  PrintS("***** Commands *****");
  
  unsigned char i = 0;
  /* don't print the last command because it is for PC tool */
  while (i < NUMBER_OF_COMMANDS - 1) PrintS(COMMAND_TABLE[i++].Cmd);
}
