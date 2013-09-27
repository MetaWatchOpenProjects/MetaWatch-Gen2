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

#ifndef CALL_NOTIFIER_H
#define CALL_NOTIFIER_H

void HandleCallNotification(unsigned char Opt, unsigned char *pVal, unsigned char Len);
void SetCallerNumber(unsigned char *pNumber, unsigned char Len);
unsigned char Ringing(void);

#endif // CALL_NOTIFIER_H

