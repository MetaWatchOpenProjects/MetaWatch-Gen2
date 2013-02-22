//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#include "FreeRTOS.h"
#include "task.h"

#include "hal_board_type.h"
#include "hal_battery.h"
#include "hal_software_fll.h"

#include "DebugUart.h"
#include "Adc.h"
#include "MuxMode.h"
#include "TermMode.h"
#include "Wrapper.h"

/*! The state of the battery charging circuit is dedcoded from the bits on 
 *  port 6 
 *
 *   Stat2    Stat1      Status                  Hex value
 *    0          0       Precharge                   0x00
 *    1          0       Fast charge                 0x10
 *    0          1       Charge Done                 0x08
 *    1          1       Timer Fault or Sleep        0x18
 */

#define CHARGE_STATUS_PRECHARGE               (0x00)
#define CHARGE_STATUS_FAST_CHARGE             (0x10)
#define CHARGE_STATUS_DONE                    (0x08)
#define CHARGE_STATUS_OFF                     (0x18)

// used to monitor battery status
static unsigned char ChargeStatus;
static unsigned char ChargeEnable;

#define BAT_CHARGE_OPEN_DRAIN_BITS \
  ( BAT_CHARGE_STAT1 + BAT_CHARGE_STAT2 + BAT_CHARGE_PWR_BIT )

#define BAT_CHARGE_STATUS_OPEN_DRAIN_BITS \
  ( BAT_CHARGE_STAT1 + BAT_CHARGE_STAT2 )

static void ChargingControl(void);

/*! Configure the pins used for reading the battery state and 
 * charging the battery.
 */
void InitBattery(void)
{
  // enable the resistor on the interface pins to the battery charger
  BAT_CHARGE_REN |= BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // Start with these as pull downs so we use less current
  BAT_CHARGE_OUT &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // Set these bits as inputs
  BAT_CHARGE_DIR &= ~BAT_CHARGE_OPEN_DRAIN_BITS;
  
  // charge enable output
  BAT_CHARGE_DIR |= BAT_CHARGE_ENABLE_PIN;
  
  /* turn on the pullup in the MSP430 for the open drain bit of the charger
   * If the watch has 5V then we don't care about the current draw of the 
   * of the pull down resistor. Otherwise, the charge chip will let
   * the pin go high
   */
  BAT_CHARGE_OUT |= BAT_CHARGE_PWR_BIT;
  BATTERY_CHARGE_DISABLE();
  
  InitAdc();

  ChargeEnable = pdTRUE;
  ChargeStatus = CHARGE_STATUS_OFF;
}

unsigned char CheckClip(void)
{
  static unsigned char Last = CLIP_INIT;
  unsigned char Changed = pdFALSE;

  unsigned char Clip = ClipOn();

  if (Clip != Last)
  {
    /* change the mux settings accordingly */
    ChangeMuxMode(Clip);
    EnableDebugUart(Clip);

    if (Clip == CLIP_ON) PrintString2("- Atch", CR);
    
    Changed = pdTRUE;
    Last = Clip;
  }
  return Changed;
}

void CheckBattery(void)
{
  BatterySenseCycle();

  if (!ClipOn())
  {
    ChargeStatus = CHARGE_STATUS_OFF;
    CheckBatteryLow();
  }
  else if (ChargeEnable) ChargingControl();
  
  PrintDecimal(BatteryLevel()); PrintString(CR);
}

unsigned char ClipOn(void)
{
  /* Read the bit, 0 is external power on. */
  return !(BAT_CHARGE_IN & BAT_CHARGE_PWR_BIT);
}

/*! Read battery state and update charge output 
 * \note Status can be used to update the LCD screen of a possible change
 * in battery charging state
 */
static void ChargingControl(void)
{
  /* prepare to read status bits */
  BAT_CHARGE_OUT |= BAT_CHARGE_STATUS_OPEN_DRAIN_BITS;

  /* always enable the charger (it may already be enabled)
   * status bits are not valid unless charger is enabled 
   */
  BATTERY_CHARGE_ENABLE();

  /* wait until signals are valid - measured 400 us 
   * during this delay we will also run the software FLL
   */
  TaskDelayLpmDisable();
  
  /* disable BT flow during FLL loop */
  portENTER_CRITICAL();
  unsigned char FlowDisabled = QueryFlowDisabled();
  if (!FlowDisabled) DisableFlow();
  portEXIT_CRITICAL();

  EnableSoftwareFll();
  vTaskDelay(1);
  DisableSoftwareFll();
  
  portENTER_CRITICAL();
  if (!FlowDisabled) EnableFlow();
  portEXIT_CRITICAL();

  TaskDelayLpmEnable();
  
  /* Decode the battery state */
  /* mask and shift to get the current battery charge status */
  ChargeStatus = BAT_CHARGE_IN & (BAT_CHARGE_STAT1 | BAT_CHARGE_STAT2);

  switch (ChargeStatus)
  {
  case CHARGE_STATUS_PRECHARGE:   PrintString(".");  break;
  case CHARGE_STATUS_FAST_CHARGE: PrintString(":"); break;
  case CHARGE_STATUS_DONE:        PrintString("|"); break;
  case CHARGE_STATUS_OFF:         PrintString("ChargeOff");  break;
  default:                        PrintString("Charge???");  break;
  }
}

unsigned char Charging(void)
{
  return (ChargeStatus == CHARGE_STATUS_PRECHARGE ||
          ChargeStatus == CHARGE_STATUS_FAST_CHARGE);
}

/* the charge enable bit is used to get status information so
 * knowing its value cannot be used to determine if the battery 
 * is being charged
 */
unsigned char ChargeEnabled(void)
{
  return ChargeEnable;
}

void ToggleCharging(void)
{
  ChargeEnable = !ChargeEnable;
  PrintString2("- Charge ", ChargeEnable ? "Enabled" : "Disabled");
  
  if (!ChargeEnable)
  {
    BATTERY_CHARGE_DISABLE();
    BAT_CHARGE_OUT &= ~BAT_CHARGE_STATUS_OPEN_DRAIN_BITS;
    ChargeStatus = CHARGE_STATUS_OFF;
  }
}
