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
/*! \file OneSecondTimers.h
 *
 * Software based timers with 1 second resolution.  These use the 1 second tick
 * from the Real Time Clock.
 * 
 * The number of timers is directly proportional to execution time.  Simple loops
 * are used (not linked lists).
 *
 */
/******************************************************************************/


#ifndef ONE_SECOND_TIMERS_H
#define ONE_SECOND_TIMERS_H

/*! setting the repeat count to 0xFF causes a timer to repeat forever */
#define TOUT_ONCE      (1)
#define REPEAT_FOREVER (0xFF)

/* Timeout for OneSecondTimer */
#define TOUT_MONITOR_BATTERY          (14) //second
#define TOUT_BACKLIGHT                (5) //second
#define TOUT_IDLE_MODE                (0xFFFF)
#define TOUT_APP_MODE                 (600)
#define TOUT_NOTIF_MODE               (30)
#define TOUT_MUSIC_MODE               (600)
#define TOUT_RING                     (2)
#define TOUT_CONN_HFP_MAP_LONG        (10)
#define TOUT_CONN_HFP_MAP_SHORT       (1)
#define TOUT_DISCONNECT               (10)
#define TOUT_TUNNEL_CONNECTING        (5)
#define TOUT_INTERVAL_LONG            (40) //60
#define TOUT_HEARTBEAT                (1) 
#define TOUT_FIELD_TEST               (1)
#define TOUT_TO_SNIFF                 (21)

typedef enum
{
  BatteryTimer,
  ModeTimer,
//  ShowCallTimer,
  BacklightTimer,
  WrapperTimer,
  ConnIntvTimer,
  HeartbeatTimer,
  VibraTimer,
//  FieldTestTimer,
  SniffTimer,
} eTimerId;

/*! One Second Timer handler that occurs in interrupt context */
unsigned char OneSecondTimerHandlerIsr(void);

void StartTimer(eTimerId Id);

/*! Stop timer associated with TimerId */
void StopTimer(eTimerId Id);


/*! Reset and start a timer
 *
 * \param Timeout in seconds
 * \param Repeat Number of times to count
*/
void ResetTimer(eTimerId Id, unsigned int Timeout, unsigned char Repeat);

void SetTimer(eTimerId Id, unsigned int Timeout, unsigned char Repeat,
              unsigned char Que, unsigned char Msg, unsigned char Option);

#endif /* ONE_SECOND_TIMERS_H */
