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
/*! \file Statistics.h
 *
 * These global variables are used for housekeeping.
 */
/******************************************************************************/

#ifndef STATISTICS_H
#define STATISTICS_H

/*! Structure for storing Bluetooth Serial Port statistics
 *
 * \param UpTime is how long the stack has been connected
 * \param MaxUpTime is the longest the stack has been connected
 * \param MallocFailed indicates that the serial port profile received a malloc failure (flag)
 * \param TxDataOverrun indicates that data was sent to the stack faster than it could sent it. (counter)
 * \param TxDataStalled indicates that the stack is busy and no more data should be sent. (flag)
 * \param RxDataOverrun indicates that rx data was not processed by the stack fast enough (flag)
 * \param RxInvalidStartCallback indicates that characters were lost on the HCI interface (flag)
 */
typedef struct
{
  unsigned int UpTime;
  unsigned int MaxUpTime;
  unsigned char MallocFailed;
  unsigned int TxDataOverrun;
  unsigned char TxDataStalled;
  unsigned char TxDataError;
  unsigned char RxDataOverrun;
  unsigned char RxInvalidStartCallback;
  unsigned int RxCrcFailureCount;
  
} tBluetoothStatistics;

/*! Structure for storing application statistics
 *
 * \param DebugUartOverflow is a flag indicating that too much data was sent to
 * debug uart
 *
 * \param BufferPoolFailure indicates that a buffer was not available when a task
 * requested it.
 */
typedef struct
{
  unsigned char DebugUartOverflow;
  unsigned char BufferPoolFailure;
  unsigned char QueueOverflow;
  unsigned char FllFailure;
  
} tApplicationStatistics;


/*! Global variable for holding the application statistics */
extern tApplicationStatistics gAppStats;

/*! Global variable for holding the Bluetooth Statistics */
extern tBluetoothStatistics gBtStats;

/*! Keeps track of how long the phone was connected
 * and the maximum amount of time the phone was connected
 */
void IncrementUpTime(void);

/*! Keeps track of CRC failures during receive message processing
 */
void IncrementRxCrcFailureCount(void);

#endif /* STATISTICS_H */
