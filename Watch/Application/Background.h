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
/*! \file Background.h
 *
 * The background task handles functions that don't belong anywhere else. The 
 * battery monitoring is handled from this task.
 */
/******************************************************************************/

#ifndef BACKGROUND_H
#define BACKGROUND_H

#ifndef MESSAGES_H
  #error "Messages.h must be included before Background.h"
#endif

#ifndef QUEUE_H
  #error "queue.h must be included before Background.h"
#endif

/*! Setup the queue for the background task */
void InitializeBackgroundTask(void);

/*! Save the RstNmi pin configuration value in non-volatile menu */
void SaveRstNmiConfiguration(void);

#endif //BACKGROUND_H

