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
/*! \file Accelerometer.h
 *
 */
/******************************************************************************/

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

/******************************************************************************/

#define ACCELEROMETER_POWER_UP_TIME_MS ( 20 )

/******************************************************************************/
   
void InitializeAccelerometer(void);

/*! The accelerometer is configured to wake up on a certain threshold */
void AccelerometerIsr(void);

void AccelerometerSendDataHandler(void);
void AccelerometerSetupHandler(tMessage* pMsg);
void AccelerometerAccessHandler(tMessage* pMsg);
void AccelerometerEnable(void);
void AccelerometerDisable(void);

/******************************************************************************/


#endif /*ACCELEROMETER_H */
