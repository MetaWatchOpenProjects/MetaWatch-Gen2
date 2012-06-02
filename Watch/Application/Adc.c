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
/*! \file Adc.c
*
*/
/******************************************************************************/

#include "hal_board_type.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "hal_clock_control.h"
#include "hal_battery.h"
#include "hal_calibration.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Utilities.h"
#include "Adc.h"
#include "Display.h"

#include "OSAL_Nv.h"
#include "NvIds.h"

#define HARDWARE_CFG_INPUT_CHANNEL  ( ADC12INCH_13 )
#define BATTERY_SENSE_INPUT_CHANNEL ( ADC12INCH_15 )
#define LIGHT_SENSE_INPUT_CHANNEL   ( ADC12INCH_1 )

#define ENABLE_REFERENCE()  {  }
#define DISABLE_REFERENCE() {  }

/* start conversion */
#define ENABLE_ADC()       { ADC12CTL0 |= ADC12ON; ADC12CTL0 |= ADC12ENC + ADC12SC; }
#define DISABLE_ADC()      { ADC12CTL0 &= ~ADC12ENC; ADC12CTL0 &= ~ADC12ON; }
#define CLEAR_START_ADDR() { ADC12CTL1 &= 0x0FFF; }

static xSemaphoreHandle AdcHardwareMutex;

#define MAX_SAMPLES ( 10 )
static unsigned int HardwareConfigurationVolts = 0;
static unsigned int BatterySense = 0;
static unsigned int LightSense = 0;
static unsigned int BatterySenseSamples[MAX_SAMPLES];
static unsigned int LightSenseSamples[MAX_SAMPLES];
static unsigned char BatterySenseSampleIndex;
static unsigned char LightSenseSampleIndex;
static unsigned char BatterySenseAverageReady = 0;
static unsigned char LightSenseAverageReady = 0;

static unsigned char LowBatteryWarningMessageSent;
static unsigned char LowBatteryBtOffMessageSent;
static tMessage Msg;
    

#define DEFAULT_LOW_BATTERY_WARNING_LEVEL    ( 3500 )
#define DEFAULT_LOW_BATTERY_BTOFF_LEVEL      ( 3300 )

static unsigned int LowBatteryWarningLevel;
static unsigned int LowBatteryBtOffLevel;

static void AdcCheck(void);
static void VoltageReferenceInit(void);

static void StartHardwareCfgConversion(void);
static void FinishHardwareCfgCycle(void);
static void SetBoardConfiguration(void);

static void StartBatterySenseConversion(void);
static void FinishBatterySenseCycle(void);

static void StartLightSenseConversion(void);
static void FinishLightSenseCycle(void);

static void WaitForAdcBusy(void);
static void EndAdcCycle(void);

/*! the voltage from the battery is divided
 * before it goes to ADC (so that it is less than
 * 2.5 volt reference)
 *
 * 
 * The output of this function is Voltage * 1000
 */
const double CONVERSION_FACTOR_BATTERY = 
  ((24300.0+38300.0)*2.5*1000.0)/(4095.0*24300.0);

/*! Convert the ADC count for the battery input into a voltage 
 *
 * \param Counts Battery Voltage in ADC counts
 * \return Battery voltage in millivolts
 */
unsigned int AdcCountsToBatteryVoltage(unsigned int Counts)
{
  return ((unsigned int)(CONVERSION_FACTOR_BATTERY*(double)Counts));
}

/*! conversion factor */
const double CONVERSION_FACTOR =  2.5*10000.0/4096.0;

/*! Convert ADC counts to a voltage (truncates)
 *
 * \param Counts Voltage in ADC counts
 * \return Voltage in millivolts*10
 */
unsigned int AdcCountsToVoltage(unsigned int Counts)
{
  return ((unsigned int)(CONVERSION_FACTOR*(double)Counts));
}


/* check the the adc is ready */
static void AdcCheck(void)
{
#if 0
  _assert_((ADC12CTL1 & ADC12BUSY) == 0);
  _assert_((ADC12CTL0 & ADC12ON) == 0);
  _assert_((ADC12CTL0 & ADC12ENC) == 0);
#endif
}

static void VoltageReferenceInit(void)
{
  /* 
   * the internal voltage reference is not used ( AVcc is used )
   * reference is controlled by ADC12MCTLx register
   *
   * 2.5 volt reference cannot be used because it requires external AVcc of 2.8
   * 
   * disable temperature sensor 
   */
  REFCTL0 = REFMSTR | REFTCOFF; 
}

/* NV items are required for this function */
void InitializeAdc(void)
{
  VoltageReferenceInit();

  LIGHT_SENSE_INIT();
  BATTERY_SENSE_INIT();
  HARDWARE_CFG_SENSE_INIT();
 
  /* allow conditional request for modosc */
  UCSCTL8 |= MODOSCREQEN;
  
  /* select ADC12SC bit as sample and hold source (00) 
   * and use pulse mode
   * use modosc / 8 because frequency must be 0.45 MHz to 2.7 MHz (0.625 MHz)
   */
  ADC12CTL1 = ADC12CSTARTADD_0 + ADC12SHP + ADC12SSEL_0 + ADC12DIV_7; 

  /* 12 bit resolution, only use reference when doing a conversion */
  ADC12CTL2 = ADC12TCOFF + ADC12RES_2 + ADC12REFBURST;

  /* setup input channels */
  ADC12MCTL0 = HARDWARE_CFG_INPUT_CHANNEL + ADC12EOS;
  ADC12MCTL1 = BATTERY_SENSE_INPUT_CHANNEL + ADC12EOS;
  ADC12MCTL2 = LIGHT_SENSE_INPUT_CHANNEL + ADC12EOS;

  BatterySenseSampleIndex = 0;
  LightSenseSampleIndex = 0;
  HardwareConfigurationVolts = 0;
  BatterySense = 0;
  LightSense = 0;
  BatterySenseAverageReady = 0;
  LightSenseAverageReady = 0;

  /* control access to adc peripheral */
  AdcHardwareMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(AdcHardwareMutex);
  
  InitializeLowBatteryLevels();
  LowBatteryWarningMessageSent = 0;
  LowBatteryBtOffMessageSent = 0;
  
  /*
   * A voltage divider on the board is populated differently
   * for each revision of the board.
   *
   * determine configuration at start-up
  */
  HARDWARE_CFG_SENSE_ENABLE();
  ENABLE_REFERENCE();
  StartHardwareCfgConversion();
  WaitForAdcBusy();
  FinishHardwareCfgCycle();
  SetBoardConfiguration();
  
}

static void WaitForAdcBusy(void)
{
  while(ADC12CTL1 & ADC12BUSY);
}

static void StartHardwareCfgConversion(void)
{
  AdcCheck();
  
  /* setup the ADC channel */
  CLEAR_START_ADDR();
  ADC12CTL1 |= ADC12CSTARTADD_0;

  /* enable the ADC to start sampling and perform a conversion */
  ENABLE_ADC();

}

static void FinishHardwareCfgCycle(void)
{
  HardwareConfigurationVolts = AdcCountsToVoltage(ADC12MEM0);
  HARDWARE_CFG_SENSE_DISABLE();
  DISABLE_ADC();
  DISABLE_REFERENCE();
  
  /* semaphore is not used for this cycle */
  
}

/* 80 us 
 * 
 * conversion time = 13 * ADC12DIV *  1/FreqAdcClock
 * 13 * 8 * 1/5e6 = 20.8 us
 */
void BatterySenseCycle(void)
{ 
  xSemaphoreTake(AdcHardwareMutex,portMAX_DELAY);
  
  BATTERY_SENSE_ENABLE();
  ENABLE_REFERENCE();

  /* low_bat_en assertion to bat_sense valid is ~100 ns */
  
  StartBatterySenseConversion();
  WaitForAdcBusy();
  FinishBatterySenseCycle();
  
}

static void StartBatterySenseConversion(void)
{
  AdcCheck();
  
  CLEAR_START_ADDR();
  ADC12CTL1 |= ADC12CSTARTADD_1;

  ENABLE_ADC();
}

static void FinishBatterySenseCycle(void)
{
  BatterySense = AdcCountsToBatteryVoltage(ADC12MEM1);

  if ( QueryCalibrationValid() )
  {
    BatterySense += GetBatteryCalibrationValue();
  }
  
  BatterySenseSamples[BatterySenseSampleIndex++] = BatterySense;
  
  if ( BatterySense < 1000 )
  {
    __no_operation();  
  }
  
  if ( BatterySenseSampleIndex >= MAX_SAMPLES )
  {
    BatterySenseSampleIndex = 0;
    BatterySenseAverageReady = 1;
  }
  
  BATTERY_SENSE_DISABLE();

  EndAdcCycle();

}

void LowBatteryMonitor(void)
{
  
  unsigned int BatteryAverage = ReadBatterySenseAverage();
  

  if ( QueryBatteryDebug() )
  {    
    /* it was not possible to get a reading below 2.8V 
     *
     * the radio does not initialize when the battery voltage (from a supply)
     * is below 2.8 V
     *
     * if the battery is not present then the readings are meaningless
    */  
    PrintStringAndThreeDecimals("Batt Inst: ",BatterySense,
                                " Batt Avg: ",BatteryAverage,
                                " Batt Charge Enable: ", QueryBatteryChargeEnabled());
    
    PrintStringAndDecimal("Power Good: ",QueryPowerGood());
    
  }
  
  /* if the battery is charging then ignore the measured voltage
   * and clear the flags
  */
  if ( QueryPowerGood() )
  {
    /* user must turn radio back on */
    LowBatteryWarningMessageSent = 0;  
    LowBatteryBtOffMessageSent = 0;
  }
  else
  {
    /* 
     * do this check first so the bt off message will get sent first
     * if startup occurs when voltage is below thresholds
     */
    if (   BatteryAverage != 0
        && BatteryAverage < LowBatteryBtOffLevel
        && LowBatteryBtOffMessageSent == 0 )
    {
      LowBatteryBtOffMessageSent = 1;
      
      SetupMessageAndAllocateBuffer(&Msg,LowBatteryBtOffMsgHost,NO_MSG_OPTIONS);
      CopyHostMsgPayload(Msg.pBuffer,(unsigned char *)&BatteryAverage,2);
      Msg.Length = 2;
      RouteMsg(&Msg);
    
      /* send the same message to the display task */
      SetupMessage(&Msg,LowBatteryBtOffMsg,NO_MSG_OPTIONS);
      RouteMsg(&Msg);
      
      /* now send a vibration to the wearer */
      SetupMessageAndAllocateBuffer(&Msg,SetVibrateMode,NO_MSG_OPTIONS);
      
      tSetVibrateModePayload* pMsgData;
      pMsgData = (tSetVibrateModePayload*) Msg.pBuffer;
      
      pMsgData->Enable = 1;
      pMsgData->OnDurationLsb = 0x00;
      pMsgData->OnDurationMsb = 0x01;
      pMsgData->OffDurationLsb = 0x00;
      pMsgData->OffDurationMsb = 0x01;
      pMsgData->NumberOfCycles = 5;
      
      RouteMsg(&Msg);

    }
    
    if (   BatteryAverage != 0
        && BatteryAverage < LowBatteryWarningLevel 
        && LowBatteryWarningMessageSent == 0 )
    {
      LowBatteryWarningMessageSent = 1;

      SetupMessageAndAllocateBuffer(&Msg,LowBatteryWarningMsgHost,NO_MSG_OPTIONS);
      CopyHostMsgPayload(Msg.pBuffer,(unsigned char*)&BatteryAverage,2);
      Msg.Length = 2;
      RouteMsg(&Msg);
    
      /* send the same message to the display task */
      SetupMessage(&Msg,LowBatteryWarningMsg,NO_MSG_OPTIONS);
      RouteMsg(&Msg);
      
      /* now send a vibration to the wearer */
      SetupMessageAndAllocateBuffer(&Msg,SetVibrateMode,NO_MSG_OPTIONS);
      
      tSetVibrateModePayload* pMsgData;
      pMsgData = (tSetVibrateModePayload*) Msg.pBuffer;
      
      pMsgData->Enable = 1;
      pMsgData->OnDurationLsb = 0x00;
      pMsgData->OnDurationMsb = 0x02;
      pMsgData->OffDurationLsb = 0x00;
      pMsgData->OffDurationMsb = 0x02;
      pMsgData->NumberOfCycles = 5;
      
      RouteMsg(&Msg);
      
    }
  
  } /* QueryPowerGood() */
  
}



void LightSenseCycle(void)
{
  xSemaphoreTake(AdcHardwareMutex,portMAX_DELAY);

  LIGHT_SENSOR_L_GAIN();
  ENABLE_REFERENCE();
  
  /* light sensor requires 1 ms to wake up in the dark */
  TaskDelayLpmDisable();
  vTaskDelay(10);
  TaskDelayLpmEnable();
  
  StartLightSenseConversion();
  WaitForAdcBusy();
  FinishLightSenseCycle();

}

void StartLightSenseConversion(void)
{
  AdcCheck();
  
  CLEAR_START_ADDR();
  ADC12CTL1 |= ADC12CSTARTADD_2;

  ENABLE_ADC();

}

/* obtained reading of 91 (or 85) in office 
 * obtained readings from 2000-12000 with droid light in different positions
 */
void FinishLightSenseCycle(void)
{
  LightSense = AdcCountsToVoltage(ADC12MEM2);

  LightSenseSamples[LightSenseSampleIndex++] = LightSense;
  
  if ( LightSenseSampleIndex >= MAX_SAMPLES )
  {
    LightSenseSampleIndex = 0;
    LightSenseAverageReady = 1;
  }
  
  LIGHT_SENSOR_SHUTDOWN();
 
  EndAdcCycle();

#if 0
  PrintStringAndDecimal("LightSenseInstant: ",LightSense);
#endif
  
}

static void EndAdcCycle(void)
{
  DISABLE_ADC();
  DISABLE_REFERENCE();

  /* release the mutex */
  xSemaphoreGive(AdcHardwareMutex);

}

unsigned int ReadBatterySense(void)
{
  return BatterySense;
}

/* this used to return 0 if the measurement is not valid (yet) 
 * but then the display doesn't have a valid value for 80 seconds.
 */
unsigned int ReadBatterySenseAverage(void)
{
  unsigned int SampleTotal = 0;
  unsigned int Result = 0;
  
  if ( BatterySenseAverageReady )
  {
  	unsigned char i;
    for (i = 0; i < MAX_SAMPLES; i++)
    {
      SampleTotal += BatterySenseSamples[i];
    }
    
    Result = SampleTotal/MAX_SAMPLES;
  }
  else
  {
    Result = BatterySense;  
  }

  return Result;
  
}

unsigned int ReadLightSense(void)
{
  return LightSense;  
}


unsigned int ReadLightSenseAverage(void)
{
  unsigned int SampleTotal = 0;
  unsigned int Result = 0;
  
  if ( LightSenseAverageReady )
  {
    unsigned char i;
    for (i = 0; i < MAX_SAMPLES; i++)
    {
      SampleTotal += LightSenseSamples[i];
    }
    
    Result = SampleTotal/MAX_SAMPLES;
  }
  else
  {
    Result = LightSense;  
  }
  
  return Result;
}

/* Set new low battery levels and save them to flash */
void SetBatteryLevels(unsigned char * pData)
{
  LowBatteryWarningLevel = pData[0] * (unsigned int)100;
  LowBatteryBtOffLevel = pData[1] * (unsigned int)100;
  
  OsalNvWrite(NVID_LOW_BATTERY_WARNING_LEVEL,
              NV_ZERO_OFFSET,
              sizeof(LowBatteryWarningLevel),
              &LowBatteryWarningLevel); 
      
  OsalNvWrite(NVID_LOW_BATTERY_BTOFF_LEVEL,
              NV_ZERO_OFFSET,
              sizeof(LowBatteryBtOffLevel),
              &LowBatteryBtOffLevel);    
}

/* Initialize the low battery levels and read them from flash if they exist */
void InitializeLowBatteryLevels(void)
{
  LowBatteryWarningLevel = DEFAULT_LOW_BATTERY_WARNING_LEVEL;
  LowBatteryBtOffLevel = DEFAULT_LOW_BATTERY_BTOFF_LEVEL;
    
  OsalNvItemInit(NVID_LOW_BATTERY_WARNING_LEVEL,
                 sizeof(LowBatteryWarningLevel), 
                 &LowBatteryWarningLevel);
   
  OsalNvItemInit(NVID_LOW_BATTERY_BTOFF_LEVEL, 
                 sizeof(LowBatteryBtOffLevel), 
                 &LowBatteryBtOffLevel);
  
}

/******************************************************************************/


/*! Hardware Configuration is done using a voltage divider
 *
 * Vin * R2/(R1+R2)
 *
 * 2.5 * 3.9e3/103.9e3 = 0.09384 volts ( Config 5 - Rev C digital, Rev E FOSO6 - CC2564 )
 * 2.5 * 4.7/104.7 = 0.1122            ( future )
 * 2.5 * 1.5/101.5 = 0.03694           ( Config 4  - Rev B8 - CC2564 or CC2560 )
 * 2.5 * 1.2/101.2 = 0.02964           ( Config 3  - CC2560 )
 * 
 * comparision could also be done in ADC counts
 * 
 */

#define CFG_WINDOW      ( 50 )
#define CONFIGURATION_1 ( 369 )
#define CONFIGURATION_4 ( 296 )
#define CONFIGURATION_5 ( 938 )

static unsigned char BoardConfiguration = 0;
  
/* This should only get called once */
static void SetBoardConfiguration(void)
{
  if ( HardwareConfigurationVolts > (CONFIGURATION_5 - CFG_WINDOW) )
  {
    BoardConfiguration = 5;    
  }
  else if ( HardwareConfigurationVolts > (CONFIGURATION_1 - CFG_WINDOW) )
  {
    BoardConfiguration = 1;  
  }
  else if ( HardwareConfigurationVolts > (CONFIGURATION_4 - CFG_WINDOW) )
  {
    BoardConfiguration = 4;  
  }

  PrintStringAndDecimal("Board Configuration ",BoardConfiguration);
 
}

unsigned char GetBoardConfiguration(void)
{
  return BoardConfiguration;
}
