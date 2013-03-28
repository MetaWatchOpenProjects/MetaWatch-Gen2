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

static const char InvalidMsg[] = "InvalidMsg";

static const tMsgInfo MsgInfo[MAXIMUM_MESSAGE_TYPES] = 
{
  {InvalidMsg,                    FREE_QINDEX,       1 }, /* 0x00 */
  {"GetDevTypeMsg",               DISPLAY_QINDEX,    1 }, /* 0x01 */
  {"GetDevTypeResp",              SPP_TASK_QINDEX,   1 }, /* 0x02 */
  {"GetInfoMsg",                  DISPLAY_QINDEX,    1 }, /* 0x03 */
  {"GetInfoResp",                 SPP_TASK_QINDEX,   1 }, /* 0x04 */
  {"DiagLoopback",                SPP_TASK_QINDEX,   1 }, /* 0x05 */
  {"ShippingModMsg",              SPP_TASK_QINDEX,   1 }, /* 0x06 */
  {"SoftResetMsg",                DISPLAY_QINDEX,    1 }, /* 0x07 */
  {"ConnTimeoutMsg",              SPP_TASK_QINDEX,   1 }, /* 0x08 */
  {"TurnRadioOnMsg",              SPP_TASK_QINDEX,   0 }, /* 0x09 */
  {"TurnRadioOffMsg",             SPP_TASK_QINDEX,   0 }, /* 0x0a */
  {"ReadRssiMsg",                 SPP_TASK_QINDEX,   1 }, /* 0x0b */
  {"PairCtrlMsg",                 SPP_TASK_QINDEX,   0 }, /* 0x0c */
  {"ReadRssiResp",                SPP_TASK_QINDEX,   1 }, /* 0x0d */
  {"SniffCtrlMsg",                SPP_TASK_QINDEX,   1 }, /* 0x0e */
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
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x19 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x1a */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x1b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x1c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x1d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x1e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x1f */
  {"WatchHandMsg",                DISPLAY_QINDEX,    0 }, /* 0x20 */
  {"TermModMsg",                  DISPLAY_QINDEX,    1 }, /* 0x21 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x22 */
  {"SetVbrMsg",                   DISPLAY_QINDEX,    0 }, /* 0x23 */
  {"BtnStateMsg",                 DISPLAY_QINDEX,    0 }, /* 0x24 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x25 */
  {"SetRtcMsg",                   DISPLAY_QINDEX,    0 }, /* 0x26 */
  {"GetRtcMsg",                   DISPLAY_QINDEX,    0 }, /* 0x27 */
  {"GetRtcResp",                  SPP_TASK_QINDEX,   1 }, /* 0x28 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x29 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x2a */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x2b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x2c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x2d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x2e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x2f */
  {"NvalMsg",                     DISPLAY_QINDEX,    0 }, /* 0x30 */
  {"NvalResp",                    SPP_TASK_QINDEX,   1 }, /* 0x31 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x32 */
  {"ModChgIndMsg",                SPP_TASK_QINDEX,   0 }, /* 0x33 */
  {"BtnEventMsg",                 SPP_TASK_QINDEX,   0 }, /* 0x34 */
  {"GeneralPhoneMsg",             SPP_TASK_QINDEX,   0 }, /* 0x35 */
  {"GeneralWatchMsg",             DISPLAY_QINDEX,    0 }, /* 0x36 */
  {"WrpTskMsg",                   SPP_TASK_QINDEX,   0 }, /* 0x37 */
  {"DspTskMsg",                   DISPLAY_QINDEX,    0 }, /* 0x38 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x39 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x3a */  
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x3b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x3c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x3d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x3e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x3f */
  {"WrtBufMsg",                   DISPLAY_QINDEX,    0 }, /* 0x40 */
  {"ConfDispMsg",                 DISPLAY_QINDEX,    1 }, /* 0x41 */
  {"ConfDrwTopMsg",               DISPLAY_QINDEX,    1 }, /* 0x42 */
  {"UpdDispMsg",                  DISPLAY_QINDEX,    0 }, /* 0x43 */
  {"LdTmplMsg",                   DISPLAY_QINDEX,    0 }, /* 0x44 */
  {"ExtAppMsg",                   SPP_TASK_QINDEX,   1 }, /* 0x45 */
  {"EnBtnMsg",                    DISPLAY_QINDEX,    0 }, /* 0x46 */
  {"DisBtnMsg",                   DISPLAY_QINDEX,    1 }, /* 0x47 */
  {"RdBtnConfMsg",                DISPLAY_QINDEX,    1 }, /* 0x48 */
  {"RdBtnConfResp",               SPP_TASK_QINDEX,   1 }, /* 0x49 */
  {"ExtAppIndMsg",                SPP_TASK_QINDEX,   1 }, /* 0x4a */
  {"EraseTmplMsg",                DISPLAY_QINDEX,    1 }, /* 0x4b */
  {"WrtTmplMsg",                  DISPLAY_QINDEX,    1 }, /* 0x4c */
  {"SetClkWgtMsg",                DISPLAY_QINDEX,    1 }, /* 0x4d */
  {"UpdClkWgtMsg",                DISPLAY_QINDEX,    0 }, /* 0x4e */
  {"WrtClkWgtDoneMsg",            DISPLAY_QINDEX,    1 }, /* 0x4f */
  {"SetExtWgtMsg",                SPP_TASK_QINDEX,   1 }, /* 0x50 */
  {"UpdClkWgt",                   DISPLAY_QINDEX,    0 }, /* 0x51 */
  {"BattChrgCtrlMsg",             DISPLAY_QINDEX,    0 }, /* 0x52 */
  {"BattConfMsg",                 DISPLAY_QINDEX,    0 }, /* 0x53 */
  {"LowBattIndMsg",               SPP_TASK_QINDEX,   0 }, /* 0x54 */
  {"LowBattBtOffIndMsg",          SPP_TASK_QINDEX,   0 }, /* 0x55 */
  {"RdBattVoltMsg",               DISPLAY_QINDEX,    0 }, /* 0x56 */
  {"RdBattVoltResp",              SPP_TASK_QINDEX,   1 }, /* 0x57 */
  {"RdLightSensorMsg",            DISPLAY_QINDEX,    0 }, /* 0x58 */
  {"RdLightSensorResp",           SPP_TASK_QINDEX,   1 }, /* 0x59 */
  {"LowBattMsg",                  DISPLAY_QINDEX,    0 }, /* 0x5a */
  {"LowBattBtOffMsg",             DISPLAY_QINDEX,    0 }, /* 0x5b */
  {"AutoBklightMsg",              DISPLAY_QINDEX,    1 }, /* 0x5c */
  {"SetBacklightMsg",             DISPLAY_QINDEX,    1 }, /* 0x5d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x5e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x5f */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x60 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x61 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x62 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x63 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x64 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x65 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x66 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x67 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x68 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x69 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x6a */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x6b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x6c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x6d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x6e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x6f */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x70 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x71 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x72 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x73 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x74 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x75 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x76 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x77 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x78 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x79 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x7a */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x7b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x7c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x7d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x7e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x7f */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x80 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x81 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x82 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x83 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x84 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x85 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x86 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x87 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x88 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x89 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x8a */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x8b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x8c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x8d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x8e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x8f */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x90 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x91 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x92 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x93 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x94 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x95 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x96 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x97 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x98 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x99 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x9a */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x9b */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x9c */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x9d */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x9e */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0x9f */
  {"IdleUpdMsg",                  DISPLAY_QINDEX,    0 }, /* 0xa0 */
  {"SetWgtListMsg",               DISPLAY_QINDEX,    0 }, /* 0xa1 */
  {"WatchDrawnTout",              DISPLAY_QINDEX,    0 }, /* 0xa2 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xa3 */
  {"UnusedMsg",                   DISPLAY_QINDEX,    1 }, /* 0xa4 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xa5 */
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
  {"SetHBMsg",                    SPP_TASK_QINDEX,   0 }, /* 0xb0 */
  {"TunnelToutMsg",               SPP_TASK_QINDEX,   1 }, /* 0xb1 */
  {"UpdConnParamMsg",             SPP_TASK_QINDEX,   0 }, /* 0xb2 */
  {"CallerIdIndMsg",              SPP_TASK_QINDEX,   1 }, /* 0xb3 */
  {"CallerNameMsg",               DISPLAY_QINDEX,    1 }, /* 0xb4 */
  {"CallerIdMsg",                 DISPLAY_QINDEX,    1 }, /* 0xb5 */
  {"HfpMsg",                      SPP_TASK_QINDEX,   0 }, /* 0xb6 */
  {"MapMsg",                      SPP_TASK_QINDEX,   0 }, /* 0xb7 */
  {"MapIndMsg",                   SPP_TASK_QINDEX,   1 }, /* 0xb8 */
  {"ConnChgMsg",                  SPP_TASK_QINDEX,   0 }, /* 0xb9 */
  {"UpdWgtIndMsg",                SPP_TASK_QINDEX,   1 }, /* 0xba */
  {"ConnParamIndMsg",             SPP_TASK_QINDEX,   0 }, /* 0xbb */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xbc */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xbd */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xbe */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xbf */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc0 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc1 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc2 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc3 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc4 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc5 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc6 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc7 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc8 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xc9 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xca */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xcb */
  {"AckMsg",                      SPP_TASK_QINDEX,   1 }, /* 0xcc */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xcd */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xce */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xcf */
  {"QueryMemMsg",                 SPP_TASK_QINDEX,   0 }, /* 0xd0 */
  {"RamTstMsg",                   DISPLAY_QINDEX,    1 }, /* 0xd1 */
  {"RateTstMsg",                  DISPLAY_QINDEX,    1 }, /* 0xd2 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd3 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd4 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd5 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd6 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd7 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd8 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xd9 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xda */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xdb */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xdc */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xdd */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xde */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xdf */
  {"AccelHostMsg",                SPP_TASK_QINDEX,   1 }, /* 0xe0 */
  {"EnAccelMsg" ,                 DISPLAY_QINDEX,    1 }, /* 0xe1 */
  {"DisAccelMsg",                 DISPLAY_QINDEX,    1 }, /* 0xe2 */
  {"AccelSndDataMsg",             DISPLAY_QINDEX,    1 }, /* 0xe3 */
  {"AccelAccessMsg",              DISPLAY_QINDEX,    1 }, /* 0xe4 */
  {"AccelResp",                   SPP_TASK_QINDEX,   1 }, /* 0xe5 */
  {"AccelSetupMsg",               DISPLAY_QINDEX,    1 }, /* 0xe6 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xe7 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xe8 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xe9 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xea */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xeb */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xec */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xed */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xee */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xef */
  {"RadioPwrCtrlMsg",             SPP_TASK_QINDEX,   1 }, /* 0xf0 */
  {"EnableAdvMsg",                SPP_TASK_QINDEX,   1 }, /* 0xf1 */
  {"SetAdvDataMsg",               SPP_TASK_QINDEX,   1 }, /* 0xf2 */
  {"SetScanRespMsg",              SPP_TASK_QINDEX,   1 }, /* 0xf3 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xf4 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xf5 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xf6 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xf7 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xf8 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xf9 */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xfa */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xfb */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xfc */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xfd */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xfe */
  {InvalidMsg,                    FREE_QINDEX,       0 }, /* 0xff */
};

#endif /* MESSAGE_INFORMATION_H */
