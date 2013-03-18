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

#ifndef VERSION_H
#define VERSION_H

/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/* Version mm/dd/yy  Author        Description of Modification                */
/* ------ --------  -------  ------------------------------------------------ */
/* 3.5.0  07/03/12  Mu Yang  Support Gen2                                     */
/* 3.6.0  08/23/12  Mu Yang  Support DUO mode                                 */
/* 3.6.1  08/25/12  Mu Yang  Refactor MessageQueue.c                          */
/* 3.6.2  08/28/12  Mu Yang  Add code for new Quard UI                        */
/* 3.6.3  09/03/12  Mu Yang  Integrated WatchFaces                            */
/* 3.6.4  09/05/12  Mu Yang  Widget update works, fix Android 2-line-draw     */
/* 3.6.5  09/06/12  Mu Yang  Support switching idle screens                   */
/* 3.6.6  09/07/12  Mu Yang  Fix showing phone control date/time mess-up      */
/* 3.6.7  09/07/12  Mu Yang  Support widget removal                           */
/* 3.6.8  09/07/12  Mu Yang  Use msg->pBuffer for WriteBuf()                  */
/* 3.6.9  09/08/12  Mu Yang  Add support for Clock widget                     */
/* 3.7.0  09/09/12  Mu Yang  Add Clock widget                                 */
/* 3.7.1  09/10/12  Mu Yang  Fix "WgtLst out of sync" by semaphore            */
/* 3.7.2  09/10/12  Mu Yang  Fix "show notification too short"                */
/* 3.7.3  09/12/12  Mu Yang  Tap to light-on; device name to "SW12".          */
/* 3.7.4  09/12/12  Mu Yang  Add widget boarder.                              */
/* 3.7.5  09/13/12  Mu Yang  Draw white dots of vertical boarder as well.     */
/* 3.7.6  09/13/12  Mu Yang  Support MUX; fix "UART ERROR".                   */
/* 3.7.7  09/17/12  Mu Yang  Support grid on/off and pixel inversion.         */
/* 3.7.8  09/18/12  Mu Yang  Merge 3.7.6a watchdog and DebugUart change.      */
/* 3.7.9  09/19/12  Mu Yang  Fix bug: late update clock and conn status.      */
/* 3.8.0  09/19/12  Mu Yang  redefine buttons for music control               */
/* 3.8.1  09/21/12  Mu Yang  Set WDT to 0x02 (4mins)                          */
/* 3.8.2  09/24/12  Mu Yang  Fix no vibration: msginfo.h: type mismatch.      */
/* 3.8.3  09/24/12  Mu Yang  Fix menu item mismatch.                          */
/* 3.8.4  09/25/12  Mu Yang  Fix mem leak on open/closing SPP;new 1316 patch. */
/* 3.8.5  09/25/12  Mu Yang  Suppport button-hold event for music control     */
/* 3.8.6  09/25/12  Mu Yang  Merge bootloader from ah2 branch.                */
/* 3.8.7  10/02/12  Mu Yang  Fix buttons' assignments.                        */
/* 3.8.8  10/03/12  Mu Yang  Fix show-caller-info notification bug.           */
/* 3.8.9  10/04/12  Mu Yang  Share button buffer for none-idle modes.         */
/* 3.9.0  10/08/12  Mu Yang  Support 2Q and 4Q clock widgets.                 */
/* 3.9.1  10/10/12  A. Hedin Fix WDT lockup; Setting capacitor value 0        */
/* 3.9.2  10/10/12  Mu Yang  Change definition of button D, E                 */
/* 3.9.3  10/12/12  Mu Yang  Optimize update idle pages and home widgets      */
/* 3.9.4  10/16/12  Mu Yang  Merge Background task into Display task.         */
/* 3.9.5  10/18/12  Mu Yang  Key press keep LED on;keys same for RadioOff     */
/* 3.9.6  10/19/12  Mu Yang  Add empty/loading widget buffer templates.       */
/* 3.9.7  10/22/12  Mu Yang  Update menu and add Service menu.                */
/* 3.9.8  10/23/12  Mu Yang  Add play/stop icons; 2Q clock/menu order.        */
/* 3.9.9  10/23/12  Mu Yang  Screen update by mode priority;invert disp.      */
/* 4.0.0  10/24/12  Mu Yang  Fix change mode loop; Service menu.              */
/* 4.0.1  10/24/12  Mu Yang  Add ChangeModeMsg current idle screen no.        */
/* 4.0.2  10/24/12  Mu Yang  No DrawClock if no clock on current page.        */
/* 4.0.3  10/25/12  Mu Yang  New status/un/pair screens; enter SrvMenu.       */
/* 4.0.4  10/25/12  Mu Yang  SHow battery %; charge icons; SrvMenu order.     */
/* 405    10/26/12  Mu Yang  Idle>Music mode; new versioning.                 */
/* 406    10/26/12  Mu Yang  Fix update overtaking non-idle page;apply to call*/
/* 407    10/29/12  Mu Yang  New button handling (free 175b RAM); B-E reset.  */
/* 408    10/30/12  Mu Yang  Stop showing clock when conn and watch-ctrl-top. */
/* 409    10/30/12  Mu Yang  Battery % and icons corrected; Bootloader blink. */
/* 410    10/31/12  Mu Yang  show right icon if battery below warning level.  */
/* 411    10/31/12  Mu Yang  Add build no. to infoStringResp.                 */
/* 412    10/31/12  Mu Yang  Fix ServiceMenu button combination.              */
/* 413    11/01/12  Mu Yang  Change ALL_TASKS_HAVE_CHECKED_IN from 0x7 to 0x3.*/
/* 414    11/01/12  Mu Yang  Add version info to infoStringResp.              */
/* 415    11/01/12  Mu Yang  Integrate Andrew's bootloader fix.               */
/* 416    11/02/12  Mu Yang  Handle TermMode message.                         */
/* 417    11/03/12  Mu Yang  Press A-B-C to exit bootloader mode.             */
/* 418    11/04/12  Mu Yang  Fix battery below warning level herizontal icon. */
/* 419    11/04/12  Mu Yang  Fix caller notification mixed with status screen.*/
/* 420    11/05/12  Mu Yang  Add ChangeMuxMode() to BattChrgCtrlHdlr().       */
/* 421    11/05/12  Mu Yang  Fix: reset mode timeout on new notification.     */
/* 422    11/06/12  Mu Yang  Fix: pressing A during booting mess-up UI.       */
/* 423    11/07/12  Mu Yang  BC to G1/1.5; Fix:HmWgtId 15; call in other mode.*/
/* 424    11/08/12  Mu Yang  Reject call on call notif page; correct tVersion.*/
/* 425    11/10/12  Mu Yang  Caller name aligns center.                       */
/* 426    11/19/12  Mu Yang  Fix Nval settings for sec/month/12h.             */
/* 427    11/21/12  Mu Yang  Use quewaiting for handing long MAP message.     */
/* 428    11/22/12  Mu Yang  New TI 1316 patch to fix random disconnection.   */
/* 429    11/22/12  Mu Yang  Use ResetModeTimer() directly from UpdDspHdl().  */
/* 430    11/22/12  Mu Yang  Show splash till BT is successfully turned on.   */
/* 431    11/24/12  Mu Yang  Metaboot cmdbuf to 8; no reject call.            */
/* 432    11/24/12  Mu Yang  V1.1:Call ends 5s timeout; APP_VER for metaboot. */
/* 433    11/28/12  Mu Yang  Receiving multi-msgs handling for Android 4.2.   */
/* 434    11/28/12  Mu Yang  SPP receving handling support for Android 4.2.   */
/* 435    12/06/12  Mu Yang  Back compatible with MWM-CE.                     */
/* 436    12/07/12  Mu Yang  Add BLE/BR mark to Status screen.                */
/* 437    12/07/12  Mu Yang  Replace WDReset(PUC) by SwReset(BOR).            */
/* 438    12/11/12  Mu Yang  Add battery icon to initial screens.             */
/* 439    12/12/12  Mu Yang  Integrate Andrew's changes to WDT:               */
/*                            - remove ResetWatchdog() from idle loop         */
/*                              [WDT controlled entirely by task check-in]    */
/*                            - remove set all pins to output function        */
/*                            - don't touch watchdog in Flash routines        */
/*                            - fix macro for enabling accelerometer power    */
/* 440    12/13/12  Mu Yang  set midium interval for music mode.              */
/* 441    12/14/12  Mu Yang  Increase Display message queue to 30.            */
/* 442    12/15/12  Mu Yang  Master reset when entering bootloader mode.      */
/* 443    12/15/12  Mu Yang  Fix vibrating after flashing: InitBtn;Splshing.  */
/* 444    12/18/12  Mu Yang  Move InitBtn/InitRtc to end of main().           */
/* 445    12/20/12  Mu Yang  Fix button not responsive after flashing.        */
/* 446    12/22/12  Mu Yang  Fix wrongly entering Ship mode after flashing.   */
/* 447    12/25/12  Mu Yang  Fix 1st-boot-after-flashing-fail by changing     */
/*                           heap size 12328 to 12334 (diff learned from 439) */
/* 448    12/26/12  Mu Yang  Integrate new patch from TI.                     */
/* 449    12/31/12  Mu Yang  Optimise MAP message handing.                    */
/* 450    12/31/12  Mu Yang  Change WDT reset from PUC to BOR.                */
/* 451    01/09/13  Mu Yang  Integrate Andrew's fix for UCS7 and 10.          */
/* 452    01/11/13  Mu Yang  Integrate SS1-417 HCI code.                      */
/* 453    01/13/13  Mu Yang  Remove UCS7/10 fix due to "Not HCILL Action" err.*/
/* 454    01/13/13  Mu Yang  Add fix to hal_ucs for UCS7/10.                  */
/* 455    01/15/13  Mu Yang  Add Andrew's complete fix for UCS7/10.           */
/* 456    01/15/13  Mu Yang  Extends call notification timeout from 5s to 10s.*/
/* 457    01/16/13  Mu Yang  V1.2: Integrate TI's new release v2.8.           */
/* 457a   01/19/13  Mu Yang  New bootloader 1.6.                              */
/* 457b   01/19/13  Mu Yang  Add Andrew's FLL fix (except LCD_SPI_SOMI_BIT).  */
/* 457c   01/22/13  Mu Yang  Disable interrupts in main(); add task debugging.*/
/* 457d   01/22/13  Mu Yang  Interval change only for BLE in Music mode.      */
/* 458    01/27/13  Mu Yang  Move initialisation from main to Disaplay task to*/
/*                           fix CSTACK overflow during first time boot;      */
/*                           Replace 6 boolean type with one byte type Nvals; */
/*                           Add show-widget-grid setting to Nval.            */
/* 459    01/30/13  Mu Yang  V1.3: Fix no-serial-receiving problem.           */
/* 460    02/04/13  Mu Yang  Replace Nval with _no_init RAM; Add FLASH reset; */
/*                           Opt battery monitoring; Exclude max/min voltage. */
/* 461    02/11/13  Mu Yang  Fix boot-hang by delay InitAccel/BattSenseCycle; */
/*                           Fix battery not charging.                        */
/* 462    02/12/13  Mu Yang  Fix SecInvertMsg not supported for Android.      */
/* 463    02/12/13  Mu Yang  Fine tune MonitorBattery().                      */
/* 464    02/13/13  Mu Yang  Fine tune BoardConfiguration.                    */
/* 465    02/13/13  Mu Yang  Move ChgMux/ChgTerm to CheckClip; Chg Init() seq.*/
/* 466    02/14/13  Mu Yang  Refactor TermMode to enable serial-receiving.    */
/* 467    02/14/13  Mu Yang  V1.34: Refactor hal_battery to avoid battery     */
/*                           reading on init; Short battery life issue fixed. */
/* 468    02/15/13  Mu Yang  V1.35: Find the root cause of TermMode fail:     */
/*                           CSTACK overflow overwrite "RevCmpltd" init value;*/
/*                           BattAlgo: duplicate previous sample if the jump  */
/*                           is too big; then do 5 samples average.           */
/* 469    02/16/13  Mu Yang  BattAlgo:set range +/-20; 8 points moving average*/
/*                           Set battery full level from 4000 to 4140.        */
/* 470    02/20/13  Mu Yang  Fix no-UpdateDisplay-on-set-grid.                */
/* 471    02/21/13  Mu Yang  BattAlgo starts from 2nd sample.                 */
/*                           Re-enable battery low alarm only when charging.  */
/* 472    02/26/13  Mu Yang  4Q clock widget using TimeK font.                */
/* 473    02/26/13  Mu Yang  Fix 2Q Hori/Vert mess-up.                        */
/* 474    02/28/13  Mu Yang  Fix 1Q date/grid overlapping; BattPercent(0x57). */
/* 475    03/03/13  Mu Yang  Support CCS5.                                    */
/* 476    03/04/13  Mu Yang  Separate clock drawing from LcdDisplay.          */
/* 477    03/06/13  Mu Yang  Fix modify-time-cause-WDT-reset.                 */
/* 478    03/08/13  Mu Yang  Separate LcdBuffer from LcdDisplay.              */
/* 479    03/08/13  Mu Yang  v1.36: Support multiple clock watch faces;       */
/*                                  sample gap 20 -> 10 (8.4 is 1%).          */
/* 480    03/12/13  Mu Yang  Add logs for response messages.                  */
/* 481    03/14/13  Mu Yang  v1.4: Fix VBatRespMsg for backward compatable.   */
/* 482    03/18/13  Mu Yang  v1.41: Enable Light Sensor.                      */
/******************************************************************************/

/** WRAPPER VERSION HISTORY ***************************************************/
/*                                                                            */
/*   Version  mm/dd/yy   Author         Description of Modification           */
/*   -------  --------- -------- -------------------------------------------  */
/*   3.0.5    08/10/12  Mu Yang   Fix mem leak in switching on/off radio      */
/*   3.1.0    08/15/12  Mu Yang   Support HFP                                 */
/*   3.1.1    08/22/12  Mu Yang   Move heartbeat to Tunnel.c                  */
/*   3.2.0    08/23/12  Mu Yang   Support MAP                                 */
/*   3.2.2    08/23/12  Mu Yang   Support DUO MODE                            */
/*   3.2.3    08/23/12  Mu Yang   Add HFP/MAP/SPP/BLE connection management   */
/*   3.2.4    08/23/12  Mu Yang   Fix connection state mismatch bug           */
/*   3.2.5    08/28/12  Mu Yang   Fix MAP delay: use Notif for Indication;    */
/*                                Avoid direct call Host_Wrt() from HFP evt   */
/*   3.2.6    08/30/12  Mu Yang   Add memory checking                         */
/*   3.2.7    08/31/12  Mu Yang   Add HFP/MAP auto reconnect                  */
/*   3.2.8    09/03/12  Mu Yang   Add Ring-Once to avoid BLE disconnecting    */
/*   3.2.9    09/04/12  Mu Yang   Fix disconnect-fail; replace function call  */
/*                                with msg handling on connection state change*/
/*   3.3.0    09/07/12  Mu Yang   Fix heartbeat timeout using high/low bytes  */
/*   3.3.1    09/10/12  Mu Yang   Fix 'HFP show notification too short'       */
/*   3.3.2    09/13/12  Mu Yang   add more logs to monitor BTPS alloc         */
/*   3.3.3    09/13/12  Mu Yang   Add back MAP for iOS 6                      */
/*   3.3.4    09/13/12  Mu Yang   support for MUX; fix "UART ERROR"           */
/*   3.3.5    09/17/12  Mu Yang   Use timer to retry failed HFP/MAP connection*/
/*   3.3.6    09/18/12  Mu Yang   Support sending connection status to phone  */
/*   3.3.6a   09/19/12  Mu Yang   Temporarily disable SPP                     */
/*   3.3.7    09/20/12  Mu Yang   Don't connect HFP/MAP before pairing        */
/*   3.3.8    09/24/12  Mu Yang   Fix HFP/MAP conn stuck by adding back       */
/*                                dynamic channnel discovery.                 */
/*   3.3.9    09/25/12  Mu Yang   Disable MTU request for checking disconn.   */
/*   3.4.0    09/25/12  Mu Yang   Add 2 times 25s timer for keeping iOS dev's */
/*                                sending data when setting up BLE connection.*/
/*   3.4.1    09/25/12  Mu Yang   Merge bootloader from ah2 branch.           */
/*   3.4.2    09/27/12  Mu Yang   Add GAPS primary service device name for iOS*/
/*   3.4.3    10/02/12  Mu Yang   Fix WrapperInit semaphore cause radioon fail*/
/*   3.4.4    10/03/12  Mu Yang   Update libBluetopia support "service change"*/
/*   3.4.5    10/05/12  Mu Yang   Add Device Information Profile              */
/*   3.4.6    10/06/12  Mu Yang   Change GAP-Dev-Name-handle to avoid iOS bug */
/*   3.4.7    10/08/12  Mu Yang   Support BLE/SPP exclusively                 */
/*   3.4.8    10/08/12  Mu Yang   Increase connection latency from 0 to 4     */
/*   3.4.9    10/11/12  Mu Yang   Send UpdWgtIndMsg to iOS after Client Config*/
/*   3.5.0    10/12/12  Mu Yang   Set conn params to 20, 40, 4, 400.          */
/*   3.5.1    10/15/12  Mu Yang   Set conn params to 40ms, 60ms, 3, 5s.       */
/*   3.5.2    10/16/12  Mu Yang   Fix SPP by rewritting GAP Service.          */
/*   3.5.3    10/16/12  Mu Yang   auto change conn interval from 60ms to 480ms*/
/*   3.5.4    10/17/12  Mu Yang   Open HF server when BLE time connected.     */
/*   3.5.5    10/18/12  Mu Yang   Add delay-disconnect.                       */
/*   3.5.6    10/20/12  Mu Yang   Fix pairable for MAP and visibility for HFP */
/*   3.5.7    10/20/12  Mu Yang   Tell connect/disconnect in ConnChangeMsg.   */
/*   3.5.8    10/22/12  Mu Yang   Connectable/Discoverable if not connected.  */
/*   3.5.9    10/24/12  Mu Yang   Ask to send curr idle page no when reconn.  */
/*   3.6.0    10/24/12  Mu Yang   Fix OutOfRange radio off; interval to 120ms.*/
/*   361      10/28/12  Mu Yang   auto reconnect HFP/MAP at heartbeat.        */
/*   362      10/29/12  Mu Yang   Set interval to 500ms.                      */
/*   363      10/30/12  Mu Yang   stop connection timer when SPP connected.   */
/*   364      10/30/12  Mu Yang   Set interval to 300ms.                      */
/*   365      10/30/12  Mu Yang   Set SHORT for MapIndMsg.                    */
/*   366      10/30/12  Mu Yang   Set 300ms interval after connected 40s.     */
/*   367      11/01/12  Mu Yang   Stop handling MAP msg when no msg buffer;   */
/*                                New libSS1 fix IntervalRequest() mem leak;  */
/*   368      11/05/12  Mu Yang   Add BatteryPercentage() to heartbeat's opt. */
/*   369      11/08/12  Mu Yang   Fix:Save LnkInfo(DevType+Addr);tVersion upd.*/
/*   370      11/13/12  Mu Yang   Add 3rd party advertising API.              */
/*   371      11/19/12  Mu Yang   Support for disconnect MWM cmd.             */
/*   372      11/19/12  Mu Yang   Support for half-connect handling.          */
/*   373      11/21/12  Mu Yang   Queue waiting for long MAP                  */
/*   374      11/22/12  Mu Yang   Switch BT off/on when HCI HW error.         */
/*   375      11/22/12  Mu Yang   Reboot till no boot error.                  */
/*   376      11/24/12  Mu Yang   V1.1:Friendly name overwrites PairedDevType.*/
/*   377      11/28/12  Mu Yang   SPP data receiving handling for multi-msg.  */
/*   378      11/30/12  Mu Yang   HFP disconn SCO on receiving caller ID.     */
/*   379      12/05/12  Mu Yang   Msg que:30 stack size:450; No radioOff timer*/
/*                                3s SCO timer; KNL/TRANS WDT reset back.     */
/*   380      12/06/12  Mu Yang   Make sure stored device type is valid.      */
/*   381      12/07/12  Mu Yang   Rm log frm Tunnel/ToutHdlIsr;stack size:350.*/
/*   382      12/07/12  Mu Yang   Replace KNL/TRANS WDT PUC by SW BOR reset.  */
/*   383      12/11/12  Mu Yang   Mv UpdWgtInd to HB hdl.                     */
/*   384      12/12/12  Mu Yang   Mv UpdConnParam frm MapInd to SND_EVT_IND.  */
/*   385      12/14/12  Mu Yang   Mv conn HFP ahead of MAP; Delay conn BR 10s.*/
/*   386      12/18/12  Mu Yang   Adv min interval 20->100 for lower Tx power;*/
/*                                BLE power level 12*2 -> 2*2 in RfTest.c     */
/*   387      12/19/12  Mu Yang   add back MTU for iOS6b4; BLE pwr -> 12 * 2  */
/*   388      12/21/12  Mu Yang   BLE pwr level -> 6 * 2                      */
/*   389      12/21/12  Mu Yang   Move SetConnected() from CliConf to HBHdl   */
/*   390      12/31/12  Mu Yang   Fix memleak @Close_Conn() Map.c;Fix MAP port*/
/*   391      12/31/12  Mu Yang   Fix multi-small-msgs crash.                 */
/*   392      01/04/12  Mu Yang   SS1 4-17 fix MAP memleak / GOEP_Find_Header */
/*   393      01/09/13  Mu Yang   Send conn_interval in HBHdl.                */
/*   394      01/10/13  Mu Yang   Add iAP support.                            */
/*   395      01/11/13  Mu Yang   Use RouteMsg in 2nd Request of MAP MsgBody. */
/*   396      01/13/13  Mu Yang   Increase stack size 400->500.               */
/*   397      01/14/13  Mu Yang   Get rid of crystal timer to use same OST.   */
/*   398      01/15/13  Mu Yang   Fix bug when two MsgEvt comes in a row.     */
/*   400      01/15/13  Mu Yang   V1.2:BLE pwr level 6->4;BR max level 15->13 */
/*                                Add BLE disconnection counter.              */
/*   401      01/18/13  Mu Yang   Remove semaphore in SPP sniff control.      */
/*   402      01/24/13  Mu Yang   Optimise sniff control and apply to MNS;    */
/*                                Support Tunnel read-cli-config-descriptor;  */
/*                                Enter-Sniff-timeout 60s -> 3s.              */
/*   403      01/24/13  Mu Yang   Entering sniff for SPP -> BR.               */
/*   404      01/24/13  Mu Yang   Add missing Cli-Char-Config to ServiceChgd  */
/*   405      01/29/13  Mu Yang   Enable MITM; Rmv ChangeSniffState()         */
/*   406      01/29/13  Mu Yang   V1.3:Remove ServiceChanged frm SS1 GATT     */
/*   407      01/30/13  Mu Yang   Force pairing by insufficient authentication*/
/*   408      02/04/13  Mu Yang   Add Directed Advertising.                   */
/*   409      02/11/13  Mu Yang   Add more log for sniff; BR IOCap: NoInOut   */
/*   410      02/11/13  Mu Yang   Fix Conn_Handle missing for HFP/MAP;        */
/*                                StartSniffTimer when ACL connected.         */
/*   411      02/15/13  Mu Yang   Add exit_sniff on data-rcv for Android 4.2  */
/*   412      03/05/13  Mu Yang   Chg SPP rcv buf & max frame to 32B; pBuf+=i */
/*   413      03/08/13  Mu Yang   Set discoverable whenever SPP disconnected. */
/*   414      03/13/13  Mu Yang   Inc SPP rcv buf to 32x2B (due MsgAlloc fail)*/
/*   415      03/16/13  Mu Yang   Change back SPP rcv buf to 32B (no help)    */
/******************************************************************************/

#define APP_VER     "1.41"
#define BUILD_VER   "481.415"

#endif /* VERSION_H */
