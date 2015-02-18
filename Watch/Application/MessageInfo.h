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
  char const * const MsgStr;
  unsigned char MsgQueue;
  unsigned char Log;
} tMsgInfo;

static char const ReservedMsg[] = "ReservedMsg";

static tMsgInfo const MsgInfo[MAXIMUM_MESSAGE_TYPES] =
{
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x00 */
  {"GetDevTypeMsg",               DISPLAY_QINDEX,    1 }, /* 0x01 */
  {"GetDevTypeResp",              WRAPPER_QINDEX,    1 }, /* 0x02 */
  {"GetInfoMsg",                  DISPLAY_QINDEX,    1 }, /* 0x03 */
  {"GetInfoResp",                 WRAPPER_QINDEX,    1 }, /* 0x04 */
  {"DiagLoopback",                WRAPPER_QINDEX,    1 }, /* 0x05 */
  {"ShippingModMsg",              WRAPPER_QINDEX,    1 }, /* 0x06 */
  {"SoftResetMsg",                DISPLAY_QINDEX,    1 }, /* 0x07 */
  {"ConnTimeoutMsg",              WRAPPER_QINDEX,    1 }, /* 0x08 */
  {"TurnRadioOnMsg",              WRAPPER_QINDEX,    1 }, /* 0x09 */
  {"TurnRadioOffMsg",             WRAPPER_QINDEX,    1 }, /* 0x0a */
  {"ReadRssiMsg",                 WRAPPER_QINDEX,    1 }, /* 0x0b */
  {"PairCtrlMsg",                 WRAPPER_QINDEX,    0 }, /* 0x0c */
  {"ReadRssiResp",                WRAPPER_QINDEX,    1 }, /* 0x0d */
  {"SniffCtrlMsg",                WRAPPER_QINDEX,    1 }, /* 0x0e */
  {"LnkAlmMsg",                   DISPLAY_QINDEX,    0 }, /* 0x0f */
  {"OledWrtBufMsg",               DISPLAY_QINDEX,    0 }, /* 0x10 */
  {"OleConfModMsg",               DISPLAY_QINDEX,    0 }, /* 0x11 */
  {"OleChgModMsg",                DISPLAY_QINDEX,    0 }, /* 0x12 */
  {"OleWrtScrlBufMsg",            DISPLAY_QINDEX,    0 }, /* 0x13 */
  {"OleScrlMsg",                  DISPLAY_QINDEX,    0 }, /* 0x14 */
  {"OleShowIdleBufMsg",           DISPLAY_QINDEX,    0 }, /* 0x15 */
  {"OleCrwnMenuMsg",              DISPLAY_QINDEX,    0 }, /* 0x16 */
  {"OleCrwnMenuBtnMsg",           DISPLAY_QINDEX,    0 }, /* 0x17 */
  {"MusCtlMsg",                   DISPLAY_QINDEX,    1 }, /* 0x18 */
  {"DrwMsg",                      DISPLAY_QINDEX,    0 }, /* 0x19 */
  {"SetCliCfgMsg",                WRAPPER_QINDEX,    0 }, /* 0x1a */
  {"HidMsg",                      WRAPPER_QINDEX,    0 }, /* 0x1b */
  {"MusicIcon",                   DISPLAY_QINDEX,    0 }, /* 0x1c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x1d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x1e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x1f */
  {"WatchHandMsg",                DISPLAY_QINDEX,    0 }, /* 0x20 */
  {"TermModMsg",                  DISPLAY_QINDEX,    1 }, /* 0x21 */
  {"FtmMsg",                      DISPLAY_QINDEX,    1 }, /* 0x22 */
  {"SetVbrMsg",                   DISPLAY_QINDEX,    0 }, /* 0x23 */
  {"BtnStateMsg",                 DISPLAY_QINDEX,    0 }, /* 0x24 */
  {"StopTimerMsg",                DISPLAY_QINDEX,    0 }, /* 0x25 */
  {"SetRtcMsg",                   DISPLAY_QINDEX,    0 }, /* 0x26 */
  {"GetRtcMsg",                   DISPLAY_QINDEX,    0 }, /* 0x27 */
  {"GetRtcResp",                  WRAPPER_QINDEX,    1 }, /* 0x28 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x29 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x2a */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x2b */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x2c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x2d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x2e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x2f */
  {"PropMsg",                     DISPLAY_QINDEX,    0 }, /* 0x30 */
  {"PropResp",                    WRAPPER_QINDEX,    0 }, /* 0x31 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x32 */
  {"ModChgIndMsg",                WRAPPER_QINDEX,    0 }, /* 0x33 */
  {"BtnEventMsg",                 WRAPPER_QINDEX,    0 }, /* 0x34 */
  {"GeneralPhoneMsg",             WRAPPER_QINDEX,    0 }, /* 0x35 */
  {"GeneralWatchMsg",             DISPLAY_QINDEX,    0 }, /* 0x36 */
  {"WrpTskMsg",                   WRAPPER_QINDEX,    0 }, /* 0x37 */
  {"DspTskMsg",                   DISPLAY_QINDEX,    0 }, /* 0x38 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x39 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x3a */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x3b */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x3c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x3d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x3e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x3f */
  {"WrtBufMsg",                   DISPLAY_QINDEX,    0 }, /* 0x40 */
  {"ConfDispMsg",                 DISPLAY_QINDEX,    1 }, /* 0x41 */
  {"ConfDrwTopMsg",               DISPLAY_QINDEX,    1 }, /* 0x42 */
  {"UpdDispMsg",                  DISPLAY_QINDEX,    0 }, /* 0x43 */
  {"LdTmplMsg",                   DISPLAY_QINDEX,    1 }, /* 0x44 */
  {"ExtAppMsg",                   WRAPPER_QINDEX,    1 }, /* 0x45 */
  {"EnBtnMsg",                    DISPLAY_QINDEX,    0 }, /* 0x46 */
  {"DisBtnMsg",                   DISPLAY_QINDEX,    0 }, /* 0x47 */
  {"RdBtnConfMsg",                DISPLAY_QINDEX,    0 }, /* 0x48 */
  {"RdBtnConfResp",               WRAPPER_QINDEX,    0 }, /* 0x49 */
  {"ExtAppIndMsg",                WRAPPER_QINDEX,    1 }, /* 0x4a */
  {"EraseTmplMsg",                DISPLAY_QINDEX,    1 }, /* 0x4b */
  {"WrtTmplMsg",                  DISPLAY_QINDEX,    1 }, /* 0x4c */
  {"SetClkWgtMsg",                DISPLAY_QINDEX,    0 }, /* 0x4d */
  {"DrwClkWgtMsg",                DISPLAY_QINDEX,    0 }, /* 0x4e */
  {"LogMsg",                      WRAPPER_QINDEX,    1 }, /* 0x4f */
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
  {"AutoBklightMsg",              DISPLAY_QINDEX,    0 }, /* 0x5c */
  {"SetBacklightMsg",             DISPLAY_QINDEX,    0 }, /* 0x5d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x5e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x5f */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x60 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x61 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x62 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x63 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x64 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x65 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x66 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x67 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x68 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x69 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x6a */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x6b */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x6c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x6d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x6e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x6f */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x70 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x71 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x72 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x73 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x74 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x75 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x76 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x77 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x78 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x79 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x7a */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x7b */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x7c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x7d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x7e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x7f */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x80 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x81 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x82 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x83 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x84 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x85 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x86 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x87 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x88 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x89 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x8a */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x8b */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x8c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x8d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x8e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x8f */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x90 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x91 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x92 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x93 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x94 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x95 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x96 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x97 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x98 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x99 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x9a */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x9b */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x9c */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x9d */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x9e */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0x9f */
  {"IdleUpdMsg",                  DISPLAY_QINDEX,    0 }, /* 0xa0 */
  {"SetWgtListMsg",               DISPLAY_QINDEX,    0 }, /* 0xa1 */
  {"WatchDrawnTout",              DISPLAY_QINDEX,    0 }, /* 0xa2 */
  {"AncsNtfMsg",                  WRAPPER_QINDEX,    0 }, /* 0xa3 */
  {"AncsGetAttrMsg",              WRAPPER_QINDEX,    1 }, /* 0xa4 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xa5 */
  {"ChgModMsg",                   DISPLAY_QINDEX,    0 }, /* 0xa6 */
  {"ModeTimeoutMsg",              DISPLAY_QINDEX,    0 }, /* 0xa7 */
  {"WatchStatusMsg",              DISPLAY_QINDEX,    0 }, /* 0xa8 */
  {"MenuModeMsg",                 DISPLAY_QINDEX,    0 }, /* 0xa9 */
  {"TrigSrvMenuMsg",              DISPLAY_QINDEX,    1 }, /* 0xaa */
  {"LstPairedDevMsg",             DISPLAY_QINDEX,    0 }, /* 0xab */
  {"BTStateChgMsg",               DISPLAY_QINDEX,    0 }, /* 0xac */
  {"ModifyTimeMsg",               DISPLAY_QINDEX,    0 }, /* 0xad */
  {"MenuBtnMsg",                  DISPLAY_QINDEX,    0 }, /* 0xae */
  {"ToggleSecMsg",                DISPLAY_QINDEX,    0 }, /* 0xaf */
  {"HBMsg",                       WRAPPER_QINDEX,    0 }, /* 0xb0 */
  {"HBToutMsg",                   WRAPPER_QINDEX,    1 }, /* 0xb1 */
  {"UpdIntvMsg",                  WRAPPER_QINDEX,    1 }, /* 0xb2 */
  {"CallerIdIndMsg",              WRAPPER_QINDEX,    1 }, /* 0xb3 */
  {"ShowCallMsg",                 DISPLAY_QINDEX,    1 }, /* 0xb4 */
  {"CallerIdMsg",                 DISPLAY_QINDEX,    1 }, /* 0xb5 */
  {"HfpMsg",                      WRAPPER_QINDEX,    0 }, /* 0xb6 */
  {"MapMsg",                      WRAPPER_QINDEX,    0 }, /* 0xb7 */
  {"MapIndMsg",                   WRAPPER_QINDEX,    0 }, /* 0xb8 */
  {"ConnChgMsg",                  WRAPPER_QINDEX,    0 }, /* 0xb9 */
  {"UpdWgtIndMsg",                WRAPPER_QINDEX,    0 }, /* 0xba */
  {"IntvChgIndMsg",               WRAPPER_QINDEX,    1 }, /* 0xbb */
  {"TunnelToutMsg",               WRAPPER_QINDEX,    0 }, /* 0xbc */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xbd */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xbe */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xbf */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc0 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc1 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc2 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc3 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc4 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc5 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc6 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc7 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc8 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xc9 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xca */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xcb */
  {"AckMsg",                      WRAPPER_QINDEX,    1 }, /* 0xcc */
  {"CntdwnMsg",                   DISPLAY_QINDEX,    0 }, /* 0xcd */
  {"SetDoneMsg",                  DISPLAY_QINDEX,    0 }, /* 0xce */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xcf */
  {"QueryMemMsg",                 WRAPPER_QINDEX,    0 }, /* 0xd0 */
  {"ConnTypeMsg",                 WRAPPER_QINDEX,    1 }, /* 0xd1 */
  {"RateTstMsg",                  DISPLAY_QINDEX,    1 }, /* 0xd2 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd3 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd4 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd5 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd6 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd7 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd8 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xd9 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xda */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xdb */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xdc */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xdd */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xde */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xdf */
  {"AccelIndMsg",                 WRAPPER_QINDEX,    0 }, /* 0xe0 */
  {"AccelMsg" ,                   DISPLAY_QINDEX,    0 }, /* 0xe1 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe2 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe3 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe4 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe5 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe6 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe7 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe8 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xe9 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xea */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xeb */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xec */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xed */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xee */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xef */
  {"RadioPwrCtrlMsg",             WRAPPER_QINDEX,    1 }, /* 0xf0 */
  {"EnableAdvMsg",                WRAPPER_QINDEX,    1 }, /* 0xf1 */
  {"SetAdvDataMsg",               WRAPPER_QINDEX,    1 }, /* 0xf2 */
  {"SetScanRespMsg",              WRAPPER_QINDEX,    1 }, /* 0xf3 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xf4 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xf5 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xf6 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xf7 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xf8 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xf9 */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xfa */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xfb */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xfc */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xfd */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xfe */
  {ReservedMsg,                   FREE_QINDEX,       0 }, /* 0xff */
};

#endif /* MESSAGE_INFORMATION_H */
