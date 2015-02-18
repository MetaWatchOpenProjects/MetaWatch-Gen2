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

#ifndef LOG_H
#define LOG_H

/*! This function prints about the cause of a watchdog reset
 * It also saves the values to non-volatile memory for the last occurence
 * of a watchdog reset. */
void SaveStateInfo(void);
void ShowStateInfo(void);

void UpdateLog(unsigned char MsgType, unsigned char MsgOpt);
void InitStateLog(void);
void SaveStateLog(void);
void SendStateLog(void);
void ShowStateLog(void);

#endif /* LOG_H */
