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
#include "hal_miscellaneous.h"

#include "Messages.h"
#include "MessageQueues.h"
#include "DebugUart.h"
#include "Utilities.h"
#include "Adc.h"

#define HARDWARE_CFG_INPUT_CHANNEL  ( ADC12INCH_13 )
#define BATTERY_SENSE_INPUT_CHANNEL ( ADC12INCH_15 )
#define LIGHT_SENSE_INPUT_CHANNEL   ( ADC12INCH_1 )

#define ENABLE_REFERENCE()  {  }
#define DISABLE_REFERENCE() {  }

/* start conversion */
#define ENABLE_ADC()       { ADC12CTL0 |= ADC12ON; ADC12CTL0 |= ADC12ENC + ADC12SC; }
#define DISABLE_ADC()      { ADC12CTL0 &= ~ADC12ENC; ADC12CTL0 &= ~ADC12ON; }
#define CLEAR_START_ADDR() { ADC12CTL1 &= 0x0FFF; }

#define WARN_LEVEL_CRITICAL  (0)
#define WARN_LEVEL_RADIO_OFF (1)
#define WARN_LEVEL_NONE      (2)

typedef struct
{
  unsigned char MsgType1;
  unsigned char MsgType2;
  tSetVibrateModePayload VibrateData;
} LowBattWarning_t;

static const LowBattWarning_t LowBattWarning[] =
{
  {LowBatteryWarningMsgHost, LowBatteryWarningMsg, {1, 0x00, 0x02, 0x00, 0x02, 5}},
  {LowBatteryBtOffMsgHost, LowBatteryBtOffMsg, {1, 0x00, 0x01, 0x00, 0x01, 5}}
};

/*! Hardware Configuration is done using a voltage divider
 * Vin * R2/(R1+R2) volts
 *
 * 2.5 * 4.7/(100 + 4.7) = 0.11223    (Config 3 - future)
 * 2.5 * 3.9/(100 + 3.9) = 0.09384    (Config 2 - Rev C digital, Rev E FOSO6 - CC2564)
 * 2.5 * 1.5/(100 + 1.5) = 0.03694    (Config 1 - Rev B8 - CC2564 or CC2560)
 * 2.5 * 1.2/(100 + 1.2) = 0.02964    (Config 0 - CC2560)
 * 
 * comparision could also be done in ADC counts
 */  
#define CFG_BASE        ( 50 )

static const unsigned int HwConfig[] =
{
  296 - CFG_BASE,
  369 - CFG_BASE,
  938 - CFG_BASE,
  1122 - CFG_BASE
};
#define HW_CONFIG_NUM     (sizeof(HwConfig) / sizeof(int))

extern unsigned char BoardType;

static xSemaphoreHandle AdcMutex = 0;

#define MAX_SAMPLES (8)
#define SAMPLE_GAP  (10)
static unsigned int Sample[MAX_SAMPLES];

#define DEFAULT_WARNING_LEVEL         ( 3468 ) //20% * (4140 - 3300) + 3300
#define DEFAULT_RADIO_OFF_LEVEL       ( 3300 )

static unsigned int WarningLevel;
static unsigned int RadioOffLevel;

static void AdcCheck(void);
static void WaitForAdcBusy(void);
static void EndAdcCycle(void);

#if ENABLE_LIGHT_SENSOR
static unsigned int LightSense = 0;
static unsigned int LightSenseSamples[MAX_SAMPLES];
static unsigned char LightSenseIndex;
static unsigned char LightSenseAverageReady = 0;

static void StartLightSenseConversion(void);
static void FinishLightSenseCycle(void);
#endif

/*! the voltage from the battery is divided
 * before it goes to ADC (so that it is less than
 * 2.5 volt reference)
 * The output of this function is Voltage * 1000
 */
const double CONVERSION_FACTOR_BATTERY = 
  ((24300.0+38300.0)*2.5*1000.0)/(4095.0*24300.0);

/*! conversion factor */
const double CONVERSION_FACTOR =  2.5*10000.0/4096.0;

/*! Initialize the Analog-to-Digital Conversion peripheral.  Set the outputs from
 * the micro to the correct state.  The ADC is used to read the 
 * hardware configuration registers, the battery voltage, and the value from the
 * light sensor.
*/
void InitAdc(void)
{
  /* 
   * the internal voltage reference is not used ( AVcc is used )
   * reference is controlled by ADC12MCTLx register
   * 2.5 volt reference cannot be used because it requires external AVcc of 2.8
   * disable temperature sensor 
   */
  REFCTL0 = REFMSTR | REFTCOFF; 

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

  unsigned char i = 0; for (; i < MAX_SAMPLES; ++i) Sample[i] = 0;

#if ENABLE_LIGHT_SENSOR
  LightSenseIndex = 0;
  LightSense = 0;
  LightSenseAverageReady = 0;
#endif

  /* control access to adc peripheral */
  AdcMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(AdcMutex);
  
  WarningLevel = DEFAULT_WARNING_LEVEL;
  RadioOffLevel = DEFAULT_RADIO_OFF_LEVEL;
  /*
   * A voltage divider on the board is populated differently
   * for each revision of the board.
   *
   * determine configuration at start-up
  */
  HARDWARE_CFG_SENSE_ENABLE();
  ENABLE_REFERENCE();

  /* Start Hardware Configuration Cycle */
  AdcCheck();

  /* setup the ADC channel */
  CLEAR_START_ADDR();
  ADC12CTL1 |= ADC12CSTARTADD_0;

  /* enable the ADC to start sampling and perform a conversion */
  ENABLE_ADC();
  WaitForAdcBusy();
  
  /* Finish HardwareConfiguration Cycle */
  /* Convert ADC counts to a voltage (truncates)
   * ADC12MEM0: Voltage in ADC counts
   * result: Voltage in millivolts * 10
   */
  unsigned int HwCfgVolt = (unsigned int)(CONVERSION_FACTOR * (double)ADC12MEM0);
  HARDWARE_CFG_SENSE_DISABLE();
  DISABLE_ADC();
  DISABLE_REFERENCE();
  
  BoardType = HW_CONFIG_NUM - 1;
  for (; BoardType; --BoardType) if (HwCfgVolt > HwConfig[BoardType]) break;

  ConfigureDefaultIO();
}

/* 80 us
 * 
 * conversion time = 13 * ADC12DIV *  1/FreqAdcClock
 * 13 * 8 * 1/5e6 = 20.8 us
 */
void BatterySenseCycle(void)
{ 
  static unsigned char Index = 0;

  xSemaphoreTake(AdcMutex, portMAX_DELAY);
  
  BATTERY_SENSE_ENABLE();
  ENABLE_REFERENCE();

  /* low_bat_en assertion to bat_sense valid is ~100 ns */
  /* Start battery sense conversion */
  AdcCheck();
  CLEAR_START_ADDR();
  ADC12CTL1 |= ADC12CSTARTADD_1;
  ENABLE_ADC();

  WaitForAdcBusy();

  /* Convert the ADC count for the battery input into a voltage 
   * ADC12MEM1: Counts Battery Voltage in ADC counts
   * Result: Battery voltage in millivolts */
  unsigned int Value = (unsigned int)(CONVERSION_FACTOR_BATTERY * (double)ADC12MEM1);

  if (ValidCalibration()) Value += GetBatteryCalibrationValue();
  PrintDecimal(Value);

  /* smoothing algorithm: cut extreme values (gap > 20) */
  unsigned char Prev = (Index == 0) ? MAX_SAMPLES - 1: Index - 1;
  if (Sample[Prev])
  {
    int Gap = Value - Sample[Prev];
    
    if (Gap > SAMPLE_GAP) Gap = SAMPLE_GAP;
    else if (Gap < -1 * SAMPLE_GAP) Gap = -1 * SAMPLE_GAP;
    Sample[Index] = Sample[Prev] + Gap;
  }
  else Sample[Index] = Value;

  if (++Index >= MAX_SAMPLES) Index = 0;

  BATTERY_SENSE_DISABLE();
  EndAdcCycle(); //xSemaphoreGive()
}

/* this used to return 0 if the measurement is not valid (yet) 
 * but then the display doesn't have a valid value for 80 seconds.
 */
unsigned int BatteryLevel(void)
{
  unsigned long Total = 0;
  unsigned char i = 0;
//  unsigned int Max = 0, Min = 9999;

  /* 5 points moving average */
  while (i < MAX_SAMPLES && Sample[i])
  {
//    if (Sample[i] > Max) Max = Sample[i];
//    if (Sample[i] < Min) Min = Sample[i];
    Total += Sample[i++];
  }
  
//  if (i == MAX_SAMPLES) Total -= Charging() ? Min : Max;

  return (Total / i);
}

unsigned char BatteryPercentage(void)
{
  int BattVal = BatteryLevel();
  
  if (BattVal > BATTERY_FULL_LEVEL) BattVal = BATTERY_FULL_LEVEL;
  BattVal -= BATTERY_CRITICAL_LEVEL;
  BattVal = BattVal > 0 ? BattVal * 10 / (BATTERY_LEVEL_RANGE / 10) : 0;
    
  return (unsigned char)BattVal;
}

// Shall be called only when clip is off
void CheckBatteryLow(void)
{
  static unsigned char Severity = WARN_LEVEL_NONE;

  unsigned int Average = BatteryLevel();

  /* it was not possible to get a reading below 2.8V
   * the radio does not initialize when the battery voltage (from a supply)
   * is below 2.8 V
   * if the battery is not present then the readings are meaningless
   * if the battery is charging then ignore the measured voltage
   * and clear the flags
  */
  if (Average < WarningLevel)
  {
    if (Average < RadioOffLevel && Severity != WARN_LEVEL_RADIO_OFF)
      Severity = WARN_LEVEL_RADIO_OFF;
    
    else if (Average >= RadioOffLevel && Severity != WARN_LEVEL_CRITICAL)
      Severity = WARN_LEVEL_CRITICAL;
    else return;

    tMessage Msg;
    SetupMessageAndAllocateBuffer(&Msg, LowBattWarning[Severity].MsgType1, MSG_OPT_NONE);
    Msg.pBuffer[0] = (unsigned char)Average;
    Msg.pBuffer[1] = (unsigned char)(Average >> 8);
    Msg.Length = 2;
    RouteMsg(&Msg);
  
    /* send the same message to the display task */
    SendMessage(&Msg, LowBattWarning[Severity].MsgType2, MSG_OPT_NONE);

    /* now send a vibration to the wearer */
    SetupMessageAndAllocateBuffer(&Msg, SetVibrateMode, MSG_OPT_NONE);

    unsigned char i = 0;
    for (; i < sizeof(tSetVibrateModePayload); ++i)
      Msg.pBuffer[i] = *((unsigned char *)&LowBattWarning[Severity].VibrateData + i);

    RouteMsg(&Msg);
  }
  // re-enable alarm only if charging
  else if (Charging()) Severity = WARN_LEVEL_NONE;
}

/* Set new low battery levels and save them to flash */
void SetBatteryLevels(unsigned char * pData)
{
  WarningLevel = pData[0] * (unsigned int)100;
  RadioOffLevel = pData[1] * (unsigned int)100;  
}

unsigned int BatteryCriticalLevel(unsigned char Type)
{
  return Type == CRITICAL_BT_OFF ? RadioOffLevel : WarningLevel;
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

static void WaitForAdcBusy(void)
{
  while (ADC12CTL1 & ADC12BUSY);
}

static void EndAdcCycle(void)
{
  DISABLE_ADC();
  DISABLE_REFERENCE();

  /* release the mutex */
  xSemaphoreGive(AdcMutex);
}

#if ENABLE_LIGHT_SENSOR
void LightSenseCycle(void)
{
  xSemaphoreTake(AdcMutex,portMAX_DELAY);

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

  LightSenseSamples[LightSenseIndex++] = LightSense;
  
  if ( LightSenseIndex >= MAX_SAMPLES )
  {
    LightSenseIndex = 0;
    LightSenseAverageReady = 1;
  }
  
  LIGHT_SENSOR_SHUTDOWN();
 
  EndAdcCycle();

#if 0
  PrintStringAndDecimal("LightSenseInstant: ",LightSense);
#endif
  
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

/*! Initiate a light sensor cycle.  Then send the instantaneous and average
 * light sense values to the host.
 *
 * \param tHostMsg* pMsg is unused
 *
 */
void ReadLightSensorHandler(void)
{
  /* start cycle and wait for it to finish */
  LightSenseCycle();

  /* send message to the host */
  tMessage Msg;
  SetupMessageAndAllocateBuffer(&Msg, ReadLightSensorResponse, MSG_OPT_NONE);

  /* instantaneous value */
  unsigned int lv = ReadLightSense();
  Msg.pBuffer[0] = lv & 0xFF;
  Msg.pBuffer[1] = (lv >> 8 ) & 0xFF;

  /* average value */
  lv = ReadLightSenseAverage();
  Msg.pBuffer[2] = lv & 0xFF;
  Msg.pBuffer[3] = (lv >> 8 ) & 0xFF;
  Msg.Length = 4;
  RouteMsg(&Msg);
}
#endif
