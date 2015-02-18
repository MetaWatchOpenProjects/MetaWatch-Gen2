//==============================================================================
//  Copyright 2013 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#define CDT_START         0xFF
#define CDT_ENTER         0xFE
#define CDT_COUNT         0xFD

#define CDT_HHMM          0x00
#define CDT_INC           0x01
#define CDT_DEC           0x02

void CountdownHandler(unsigned char Option);
void SetCountdownTimer(unsigned char Set);
void CountingDown(void);

#endif /* COUNTDOWN_H */
