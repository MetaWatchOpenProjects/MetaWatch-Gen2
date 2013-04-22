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
/*! \file hal_rtc.h
 *
 * Real Time Clock (RTC)
 *
 * This module contains more than the hardware abstraction. The real time clock
 * is always running off of the 32.768 kHz crystal.  The micro contains a 
 * calendar function that keeps track of month, date, and year.
 */
/******************************************************************************/

#ifndef HAL_RTC_H
#define HAL_RTC_H

#define RTC_SEC   (0)
#define RTC_MIN   (1)
#define RTC_HOUR  (2)
#define RTC_DAY   (3)
#define RTC_DOW   (4)
#define RTC_MON   (5)

#define BCD_H(_x)  (_x >> 4)
#define BCD_L(_x)  (_x & 0x0F)

/*!
 * \param Year is a 12 bit value
 * \param Month of the year - 1 to 12
 * \param Day is 1 to 31
 * \param DayOfWeek is 0 to 6
 * \param Hour is 0 to 24
 * \param Minute is 0 to 59
 * \param Second is 0 to 59
 */
typedef struct
{
  unsigned char YearMsb;
  unsigned char YearLsb;
  unsigned char Month;
  unsigned char Day;
  unsigned char DayOfWeek;
  unsigned char Hour;
  unsigned char Minute;
  unsigned char Second;
} Rtc_t;

/*! Enables the prescale one RTC interrupt
 *
 * The prescale one interrupt is used because the RTC runs even when the
 * processor is in LPM3.  It is used as a 32 Hz system timer.

 * \param user one of defined users (see hal_rtc.h)
 *
 */
void EnableRtcPrescaleInterruptUser(unsigned char user);


/*! Disables the prescale one RTC interrupt user
 *
 * Each interrupt takes the processor out of the low power mode so we
 * can't leave it running all the time. Multiple things use the timer so we
 * make sure everyone is done before turning it off.
 *
 * \param user one of defined users (see hal_rtc.h)
 */
void DisableRtcPrescaleInterruptUser(unsigned char user);

/*! Initialize the RTC for normal watch operation
 *
 * This function also sets up the static prescale one and 1ppS messages as well
 * as enable the 1 Hz RTC output on P2.4
 *
 * \note This function does not change the value of the RTC
 */
void InitRealTimeClock(void);

/*! Sets the RTC 
 *
 * \note There could be a separate struct to abstract this, however the message
 *  data was laid out to exactly match the MSP430 RTC registers.  The asm level
 *  patch functions may not be needed on newer versions of the MSP430
 *
 * \param pRtcData
 */
void SetRtc(Rtc_t *pRtcData);

void BackupRtc(void);

void IncRtc(unsigned char Index);

unsigned char ToBCD(unsigned char Dec);
unsigned char ToBin(unsigned char Bcd);

unsigned char To12H(unsigned char H24);

// The exact value is 31.25 mS
#define RTC_TIMER_MS_PER_TICK       31   

/*! Users of the RTC prescaler timer 0 interrupt.  This interrupt occurs at
 * 128 kHz and is divided down to occur at 32 khZ.
 */
#define RTC_TIMER_VIBRATION       ( BIT0 )
#define RTC_TIMER_RESERVED        ( BIT1 )
#define RTC_TIMER_BUTTON          ( BIT2 )
#define RTC_TIMER_RESERVED3       ( BIT3 )
#define RTC_TIMER_RESERVED4       ( BIT4 )
#define RTC_TIMER_RESERVED5       ( BIT5 )


#endif /* HAL_RTC_H */
