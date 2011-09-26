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
/*! \file LcdTask.h
 *
 * This task handles writing information to the LCD.  A clear line between the 
 * hardware and application layer is not present.
 *
 * DMA is used to transfer data to the LCD.
 */
/******************************************************************************/

#ifndef LCD_TASK_H
#define LCD_TASK_H

/*! Initialize the task queue, spi peripheral, and LCD pins */
void InitializeLcdTask(void);

/*! Callback from the DMA interrupt service routing that lets LCD task know that 
 * the dma has finished
 */
void LcdDmaIsr(void);

#endif /* LCD_TASK_H */