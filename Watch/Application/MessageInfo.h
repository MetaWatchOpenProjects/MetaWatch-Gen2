//==============================================================================
//  Copyright 2012 Meta Watch Ltd. - http://www.MetaWatch.org/
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

#ifndef MESSAGE_INFORMATION_H
#define MESSAGE_INFORMATION_H

/*! structure contains message name as a string, 
 * queue to route message to,
 * and if it should be printed to the debug terminal
 */
typedef struct
{
  const char *MsgStr;
  unsigned char MsgQueue;
  unsigned char Log;

} tMsgInfo;

static const char UnusedMsg[] = "UnusedMsg";

static const tMsgInfo MsgInfo[MAXIMUM_MESSAGE_TYPES] = 
{
  {"InvalidMsg",                  FREE_QINDEX,       1 }, /* 0x00 */
  {"GetDevTypeMsg",               DISPLAY_QINDEX,    1 }, /* 0x01 */
  {"GetDevTypeResp",              WRAPPER_QINDEX,    1 }, /* 0x02 */
  {"GetInfoMsg",                  DISPLAY_QINDEX,    1 }, /* 0x03 */
  {"GetInfoResp",                 WRAPPER_QINDEX,    1 }, /* 0x04 */
  {"DiagLoopback",                WRAPPER_QINDEX,    1 }, /* 0x05 */
  {"ShippingModMsg",              WRAPPER_QINDEX,    1 }, /* 0x06 */
  {"SoftResetMsg",                DISPLAY_QINDEX,    1 }, /* 0x07 */
  {"ConnTimeoutMsg",              WRAPPER_QINDEX,    1 }, /* 0x08 */
  {"TurnRadioOnMsg",              WRAPPER_QINDEX,    1 }, /* 0x09 */
  {"TurnRadioOffMsg",             WRAPPER_QINDEX,    0 }, /* 0x0a */
  {"ReadRssiMsg",                 WRAPPER_QINDEX,    1 }, /* 0x0b */
  {"PairCtrlMsg",                 WRAPPER_QINDEX,    0 }, /* 0x0c */
  {"ReadRssiResp",                WRAPPER_QINDEX,    1 }, /* 0x0d */
  {"SniffCtrlMsg",                WRAPPER_QINDEX,    1 }, /* 0x0e */
  {"LnkAlmMsg",                   DISPLAY_QINDEX,    1 }, /* 0x0f */
  {"OledWrtBufMsg",               DISPLAY_QINDEX,    0 }, /* 0x10 */
  {"OleConfModMsg",               DISPLAY_QINDEX,    0 }, /* 0x11 */
  {"OleChgModMsg",                DISPLAY_QINDEX,    0 }, /* 0x12 */
  {"OleWrtScrlBufMsg",            DISPLAY_QINDEX,    0 }, /* 0x13 */
  {"OleScrlMsg",                  DISPLAY_QINDEX,    0 }, /* 0x14 */
  {"OleShowIdleBufMsg",           DISPLAY_QINDEX,    0 }, /* 0x15 */
  {"OleCrwnMenuMsg",              DISPLAY_QINDEX,    0 }, /* 0x16 */
  {"OleCrwnMenuBtnMsg",           DISPLAY_QINDEX,    0 }, /* 0x17 */
  {"MusicStateMsg",               DISPLAY_QINDEX,    1 }, /* 0x18 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x19 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x1a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x1b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x1c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x1d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x1e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x1f */
  {"WatchHandMsg",                DISPLAY_QINDEX,    0 }, /* 0x20 */
  {"TermModMsg",                  DISPLAY_QINDEX,    1 }, /* 0x21 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x22 */
  {"SetVbrMsg",                   DISPLAY_QINDEX,    0 }, /* 0x23 */
  {"BtnStateMsg",                 DISPLAY_QINDEX,    0 }, /* 0x24 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x25 */
  {"SetRtcMsg",                   DISPLAY_QINDEX,    0 }, /* 0x26 */
  {"GetRtcMsg",                   DISPLAY_QINDEX,    0 }, /* 0x27 */
  {"GetRtcResp",                  WRAPPER_QINDEX,    1 }, /* 0x28 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x29 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x2a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x2b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x2c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x2d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x2e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x2f */
  {"PropMsg",                     DISPLAY_QINDEX,    0 }, /* 0x30 */
  {"PropResp",                    WRAPPER_QINDEX,    1 }, /* 0x31 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x32 */
  {"ModChgIndMsg",                WRAPPER_QINDEX,    0 }, /* 0x33 */
  {"BtnEventMsg",                 WRAPPER_QINDEX,    0 }, /* 0x34 */
  {"GeneralPhoneMsg",             WRAPPER_QINDEX,    0 }, /* 0x35 */
  {"GeneralWatchMsg",             DISPLAY_QINDEX,    0 }, /* 0x36 */
  {"WrpTskMsg",                   WRAPPER_QINDEX,    0 }, /* 0x37 */
  {"DspTskMsg",                   DISPLAY_QINDEX,    0 }, /* 0x38 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x39 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x3a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x3b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x3c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x3d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x3e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x3f */
  {"WrtBufMsg",                   DISPLAY_QINDEX,    0 }, /* 0x40 */
  {"ConfDispMsg",                 DISPLAY_QINDEX,    1 }, /* 0x41 */
  {"ConfDrwTopMsg",               DISPLAY_QINDEX,    1 }, /* 0x42 */
  {"UpdDispMsg",                  DISPLAY_QINDEX,    0 }, /* 0x43 */
  {"LdTmplMsg",                   DISPLAY_QINDEX,    1 }, /* 0x44 */
  {"ExtAppMsg",                   WRAPPER_QINDEX,    1 }, /* 0x45 */
  {"EnBtnMsg",                    DISPLAY_QINDEX,    0 }, /* 0x46 */
  {"DisBtnMsg",                   DISPLAY_QINDEX,    1 }, /* 0x47 */
  {"RdBtnConfMsg",                DISPLAY_QINDEX,    1 }, /* 0x48 */
  {"RdBtnConfResp",               WRAPPER_QINDEX,    1 }, /* 0x49 */
  {"ExtAppIndMsg",                WRAPPER_QINDEX,    1 }, /* 0x4a */
  {"EraseTmplMsg",                DISPLAY_QINDEX,    1 }, /* 0x4b */
  {"WrtTmplMsg",                  DISPLAY_QINDEX,    1 }, /* 0x4c */
  {"SetClkWgtMsg",                DISPLAY_QINDEX,    1 }, /* 0x4d */
  {"UpdClkWgtMsg",                DISPLAY_QINDEX,    0 }, /* 0x4e */
  {"WrtClkWgtDoneMsg",            DISPLAY_QINDEX,    1 }, /* 0x4f */
  {"SetExtWgtMsg",                WRAPPER_QINDEX,    1 }, /* 0x50 */
  {"UpdClkWgt",                   DISPLAY_QINDEX,    0 }, /* 0x51 */
  {"BattChrgCtrlMsg",             DISPLAY_QINDEX,    0 }, /* 0x52 */
  {"BattConfMsg",                 DISPLAY_QINDEX,    0 }, /* 0x53 */
  {"LowBattIndMsg",               WRAPPER_QINDEX,    0 }, /* 0x54 */
  {"LowBattBtOffIndMsg",          WRAPPER_QINDEX,    0 }, /* 0x55 */
  {"RdBattVoltMsg",               DISPLAY_QINDEX,    0 }, /* 0x56 */
  {"RdBattVoltResp",              WRAPPER_QINDEX,    1 }, /* 0x57 */
  {"RdLightSensorMsg",            DISPLAY_QINDEX,    0 }, /* 0x58 */
  {"RdLightSensorResp",           WRAPPER_QINDEX,    1 }, /* 0x59 */
  {"LowBattMsg",                  DISPLAY_QINDEX,    0 }, /* 0x5a */
  {"LowBattBtOffMsg",             DISPLAY_QINDEX,    0 }, /* 0x5b */
  {"AutoBklightMsg",              DISPLAY_QINDEX,    1 }, /* 0x5c */
  {"SetBacklightMsg",             DISPLAY_QINDEX,    1 }, /* 0x5d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x5e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x5f */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x60 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x61 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x62 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x63 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x64 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x65 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x66 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x67 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x68 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x69 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x6a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x6b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x6c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x6d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x6e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x6f */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x70 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x71 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x72 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x73 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x74 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x75 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x76 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x77 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x78 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x79 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x7a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x7b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x7c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x7d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x7e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x7f */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x80 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x81 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x82 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x83 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x84 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x85 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x86 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x87 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x88 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x89 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x8a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x8b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x8c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x8d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x8e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x8f */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x90 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x91 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x92 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x93 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x94 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x95 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x96 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x97 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x98 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x99 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x9a */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x9b */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x9c */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x9d */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x9e */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0x9f */
  {"IdleUpdMsg",                  DISPLAY_QINDEX,    0 }, /* 0xa0 */
  {"SetWgtListMsg",               DISPLAY_QINDEX,    0 }, /* 0xa1 */
  {"WatchDrawnTout",              DISPLAY_QINDEX,    0 }, /* 0xa2 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xa3 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xa4 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xa5 */
  {"ChgModMsg",                   DISPLAY_QINDEX,    0 }, /* 0xa6 */
  {"ModeTimeoutMsg",              DISPLAY_QINDEX,    0 }, /* 0xa7 */
  {"WatchStatusMsg",              DISPLAY_QINDEX,    0 }, /* 0xa8 */
  {"MenuModeMsg",                 DISPLAY_QINDEX,    0 }, /* 0xa9 */
  {"TrigSrvMenuMsg",              DISPLAY_QINDEX,    1 }, /* 0xaa */
  {"LstPairedDevMsg",             DISPLAY_QINDEX,    0 }, /* 0xab */
  {"BTStateChgMsg",               DISPLAY_QINDEX,    0 }, /* 0xac */
  {"ModifyTimeMsg",               DISPLAY_QINDEX,    1 }, /* 0xad */
  {"MenuBtnMsg",                  DISPLAY_QINDEX,    1 }, /* 0xae */
  {"ToggleSecMsg",                DISPLAY_QINDEX,    1 }, /* 0xaf */
  {"SetHBMsg",                    WRAPPER_QINDEX,    0 }, /* 0xb0 */
  {"HBToutMsg",                   WRAPPER_QINDEX,    1 }, /* 0xb1 */
  {"UpdConnParamMsg",             WRAPPER_QINDEX,    0 }, /* 0xb2 */
  {"CallerIdIndMsg",              WRAPPER_QINDEX,    1 }, /* 0xb3 */
  {"CallerNameMsg",               DISPLAY_QINDEX,    1 }, /* 0xb4 */
  {"CallerIdMsg",                 DISPLAY_QINDEX,    1 }, /* 0xb5 */
  {"HfpMsg",                      WRAPPER_QINDEX,    0 }, /* 0xb6 */
  {"MapMsg",                      WRAPPER_QINDEX,    0 }, /* 0xb7 */
  {"MapIndMsg",                   WRAPPER_QINDEX,    1 }, /* 0xb8 */
  {"ConnChgMsg",                  WRAPPER_QINDEX,    0 }, /* 0xb9 */
  {"UpdWgtIndMsg",                WRAPPER_QINDEX,    1 }, /* 0xba */
  {"ConnParamIndMsg",             WRAPPER_QINDEX,    0 }, /* 0xbb */
  {"TunnelToutMsg",               WRAPPER_QINDEX,    0 }, /* 0xbc */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xbd */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xbe */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xbf */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc0 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc1 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc2 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc3 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc4 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc5 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc6 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc7 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc8 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xc9 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xca */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xcb */
  {"AckMsg",                      WRAPPER_QINDEX,    1 }, /* 0xcc */
  {"CntdwnMsg",                   DISPLAY_QINDEX,    1 }, /* 0xcd */
  {"SetDoneMsg",                  DISPLAY_QINDEX,    1 }, /* 0xce */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xcf */
  {"QueryMemMsg",                 WRAPPER_QINDEX,    0 }, /* 0xd0 */
  {"ConnTypeMsg",                 WRAPPER_QINDEX,    1 }, /* 0xd1 */
  {"RateTstMsg",                  DISPLAY_QINDEX,    1 }, /* 0xd2 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd3 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd4 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd5 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd6 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd7 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd8 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xd9 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xda */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xdb */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xdc */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xdd */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xde */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xdf */
  {"AccelIndMsg",                 WRAPPER_QINDEX,    0 }, /* 0xe0 */
  {"AccelMsg" ,                   DISPLAY_QINDEX,    0 }, /* 0xe1 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe2 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe3 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe4 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe5 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe6 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe7 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe8 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xe9 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xea */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xeb */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xec */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xed */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xee */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xef */
  {"RadioPwrCtrlMsg",             WRAPPER_QINDEX,    1 }, /* 0xf0 */
  {"EnableAdvMsg",                WRAPPER_QINDEX,    1 }, /* 0xf1 */
  {"SetAdvDataMsg",               WRAPPER_QINDEX,    1 }, /* 0xf2 */
  {"SetScanRespMsg",              WRAPPER_QINDEX,    1 }, /* 0xf3 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xf4 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xf5 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xf6 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xf7 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xf8 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xf9 */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xfa */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xfb */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xfc */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xfd */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xfe */
  {UnusedMsg,                     FREE_QINDEX,       0 }, /* 0xff */
};

#endif /* MESSAGE_INFORMATION_H */
