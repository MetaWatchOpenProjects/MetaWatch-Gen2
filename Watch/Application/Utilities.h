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
/*! \file Utilities.h
 *
 * Includes functions to check on task memory usage and prepare buffers
 * for transmission to the host
 *
 */
/******************************************************************************/

#ifndef UTILITIES_H
#define UTILITIES_H

#ifndef TASK_H
  #error "task.h must be included before Utilities.h"
#endif

#ifndef DEBUG_UART_H
  #error "DebugUart.h must be included before Utilities.h"
#endif
  
/*! This determines how many tasks can be kept track of in the utility functions */
#define MAX_NUM_TASK_LOG_ENTRIES  ( TOTAL_TASKS )

/*! Task Information Structure */
typedef struct
{
  void * taskHandle;
  unsigned int Depth;
  unsigned int FreeEntries;

} tTaskInfo;


#if TASK_DEBUG
/*! Cycle through all of the tasks and display how much stack they are using 
 * (Prints information for one task each call)
 */
void UTL_FreeRtosTaskStackCheck(void);
#endif

/*! Check how much statck a task is currently using
 *
 * \param TaskHandle points to the task
 * \param TaskName is a string representation of the task
 */
void CheckStackUsage(xTaskHandle TaskHandle,tString * TaskName);


/*! memcopy
 *
 * \param pBuffer is the destination
 * \param pSource is a pointer to the source
 * \param Size is the size
 */
void CopyHostMsgPayload(unsigned char* pBuffer, 
                        unsigned char* pSource, 
                        unsigned char Size);

/*void CopyBytes(unsigned char* pDest, unsigned char* pSource, unsigned char Size); */

/*! \return a pointer to the device name string */
unsigned char * GetDeviceNameString(void);

/*! Check the configuration of the power management module to make sure that
 * it is not setup in a mode that will cause problems
 */
unsigned char PMM15Check(void);

/******************************************************************************/

/*! check how full a queue is */
void CheckQueueUsage(xQueueHandle Qhandle);


/******************************************************************************/

void GenerateDisplayStringMessage(tString* pString,unsigned char Options);

/******************************************************************************/

#define MUX_MODE_OFF         ( 0 )
#define MUX_MODE_SERIAL      ( 1 )
#define MUX_MODE_GND         ( 2 ) 
#define MUX_MODE_SPY_BI_WIRE ( 3 )

#define MUX_MODE_DEFAULT_5V     ( MUX_MODE_SERIAL )
#define MUX_MODE_DEFAULT_NORMAL ( MUX_MODE_GND )

/*! Initialize NV value that controls the mux */
void InitializeMuxMode(void);

/*! return the mode the mux control should be in
 * (output of micro can be overriden by pressing S1 (top right) during power-up
 *
 * \param PowerGood is 1 when 5V is present, 0 otherwise
 * 
 * \return 0 = mux off, 1 = serial, 2 = gnd, 3 = spy-bi-wire
 */
unsigned char GetMuxMode(unsigned char PowerGood);

void SetMuxMode(unsigned char MuxMode, unsigned char PowerGood);

/*! Initialize the state of the mux that allows the case back pins 
 * to be serial port, GND, or spy-bi-wire
 */
void ChangeMuxMode(void);

/******************************************************************************/

#endif /* UTILITIES_H */
