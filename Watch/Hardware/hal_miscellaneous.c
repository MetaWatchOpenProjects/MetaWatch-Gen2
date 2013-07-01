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
/*! \file hal_miscellaneous.c
*
* Set pins to outputs and setup clock
*/
/******************************************************************************/

#include "FreeRTOSConfig.h"

#include "portmacro.h"

#include "hal_board_type.h"
#include "hal_miscellaneous.h"

#include "HAL_PMM.h"
#include "HAL_UCS.h"

unsigned char BoardType;

static unsigned char PMM15Check(void);

/******************************************************************************/
#define HARDWARE_REVISION_ADDRESS (0x1a07)

unsigned char GetMsp430HardwareRevision(void)
{
  return *(unsigned char *)(unsigned char *)HARDWARE_REVISION_ADDRESS + '1';
}

unsigned char Errata(void)
{
  unsigned char Revision = GetMsp430HardwareRevision();
  return (Revision < 'F' || Revision == 'G');
}

/******************************************************************************/

#if ISOLATE_RADIO
/* Set all the pins to the radio to be inputs so that they can be
 * driven by an HCI tester
 */
void SetRadioControlPinsToInputs(void);
#endif

static void BluetoothSidebandConfig(void);

/******************************************************************************/

void SetupClockAndPowerManagementModule(void)
{
  // see Frequency vs Supply Voltage in MSP4305438A data sheet
  SetVCore(PMMCOREV_2);

  // setup pins for XT1
  P7SEL |= BIT0 + BIT1;

  // Startup LFXT1 32 kHz crystal
  while (LFXT_Start_Timeout(XT1DRIVE_0, 50000) == UCS_STATUS_ERROR);

  // select the sources for the FLL reference and ACLK
  SELECT_ACLK(SELA__XT1CLK);
  SELECT_FLLREF(SELREF__XT1CLK);

  // 512 * 32768 = 16777216 / 1024
  Init_FLL_Settle(configCPU_CLOCK_HZ/configTICK_RATE_HZ, ACLK_MULTIPLIER);
  
  // Disable FLL loop control
  __bis_SR_register(SCG0);
  
  // setup for quick wake up from interrupt and
  // minimal power consumption in sleep mode
  DISABLE_SVSL();                           // SVS Low side is turned off
  DISABLE_SVSL_RESET();
  
  DISABLE_SVML();                           // Monitor low side is turned off
  DISABLE_SVML_INTERRUPT();
  
  DISABLE_SVMH();                           // Monitor high side is turned off
  DISABLE_SVMH_INTERRUPT();
  
  ENABLE_SVSH();                            // SVS High side is turned on
  ENABLE_SVSH_RESET();                      // Enable POR on SVS Event

  SVSH_ENABLED_IN_LPM_FULL_PERF();          // SVS high side Full perf mode,
                                            // stays on in LPM3,enhanced protect

  // Wait until high side, low side settled
  while ((PMMIFG & SVSMLDLYIFG) == 0 && (PMMIFG & SVSMHDLYIFG) == 0);
  CLEAR_PMM_IFGS();

#if CHECK_FOR_PMM15
  /* make sure error pmm15 does not exist */
  while (PMM15Check());
#endif

  if (Errata())
  {
    /* Errata PMM17 - automatic prolongation mechanism
    * SVSLOW is disabled
    */
    *(unsigned int*)(0x0110) = 0x9602;
    *(unsigned int*)(0x0112) |= 0x0800;
  }
}

/*! Check the configuration of the power management module to make sure that
 * it is not setup in a mode that will cause problems
 */
static unsigned char PMM15Check(void)
{
  // First check if SVSL/SVML is configured for fast wake-up
  if ((!(SVSMLCTL & SVSLE)) || ((SVSMLCTL & SVSLE) && (SVSMLCTL & SVSLFP)) ||
      (!(SVSMLCTL & SVMLE)) || ((SVSMLCTL & SVMLE) && (SVSMLCTL & SVMLFP)))
  { 
    // Next Check SVSH/SVMH settings to see if settings are affected by PMM15
    if ((SVSMHCTL & SVSHE) && (!(SVSMHCTL & SVSHFP)))
    {
      if ( (!(SVSMHCTL & SVSHMD)) ||
           ((SVSMHCTL & SVSHMD) && (SVSMHCTL & SVSMHACE)) )
        return 1; // SVSH affected configurations
    }

    if ((SVSMHCTL & SVMHE) && (!(SVSMHCTL & SVMHFP)) && (SVSMHCTL & SVSMHACE))
      return 1; // SVMH affected configurations
  }

  return 0; // SVS/M settings not affected by PMM15
}

/* The following is responsible for initializing the target hardware.*/
void ConfigureDefaultIO(void)
{
  // Setting DIR = 1 and SEL = 1 enables ACLK out on P11.0
  // and disable MCLK and SMCLK
  P11DIR |= BIT0;
  P11SEL |= BIT0;

  BluetoothSidebandConfig();

#if ANALOG
  CONFIG_OLED_PINS();
#endif

#ifdef HW_DEVBOARD_V2
  CONFIG_DEBUG_PINS();
  CONFIG_LED_PINS();
#endif

  CONFIG_SRAM_PINS();
  APPLE_CONFIG();

  CONFIG_ACCELEROMETER_PINS();

  if (BoardType >= 2) ENABLE_ACCELEROMETER_POWER();

#if ISOLATE_RADIO
  SetRadioControlPinsToInputs();
#endif
}

/* these pins were originally used for debug
 * on later boards they are not connected to the radio
 *
 */
static void BluetoothSidebandConfig(void)
{
  if (BoardType < 2)
  {
#ifdef HW_DEVBOARD_V2
    /* BT_CLK_REQ on devboard == BT_IO2 on watch
     * (BT_CLK_REQ is not connected on watch's MSP430)
     */
    BT_CLK_REQ_CONIFIG_AS_INPUT();
    BT_IO1_CONFIG_AS_OUTPUT_LOW();
    BT_IO2_CONFIG_AS_OUTPUT_LOW();
#else
    BT_CLK_REQ_CONIFIG_AS_OUTPUT_LOW();
    BT_IO1_CONFIG_AS_OUTPUT_LOW();
    BT_IO2_CONFIG_AS_INPUT();
#endif
  }
  else
  {
    /* todo: check power consumption on actual hardware */
    BT_CLK_REQ_CONIFIG_AS_OUTPUT_LOW();
    BT_IO1_CONFIG_AS_OUTPUT_LOW();
    BT_IO2_CONFIG_AS_OUTPUT_LOW();
  }
}

unsigned char GetBoardConfiguration(void)
{
  return BoardType;
}

#if ISOLATE_RADIO

static void SetRadioControlPinsToInputs(void)
{
#ifdef HW_DEVBOARD_V2
  BT_CLK_REQ_PDIR &= ~BT_CLK_REQ_PIN;
#endif

  BT_IO1_PDIR &= ~BT_IO1_PIN;
  BT_IO2_PDIR &= ~BT_IO2_PIN;

  /* cts */
  P1OUT &= ~BIT3;

  /* rts */
  P1OUT &= ~BIT0;

  /* BT_RX */
  P5OUT &= ~BIT7;

  /* BT_TX */
  P5OUT &= ~BIT6;

  /* BT_RST */
  P10OUT &= ~BIT3;
}
#endif

