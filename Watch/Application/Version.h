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
/* 477    03/06/13  Mu Yang  RELEASE: Fix modify-time-cause-WDT-reset.        */
/* 478    03/08/13  Mu Yang  Separate LcdBuffer from LcdDisplay.              */
/* 479    03/08/13  Mu Yang  v1.36: Support multiple clock watch faces;       */
/*                           sample gap 20 -> 10 (8.4 is 1%).                 */
/* 480    03/12/13  Mu Yang  Add logs for response messages.                  */
/* 481    03/14/13  Mu Yang  v1.40: Fix VBatRespMsg for backward compatable.  */
/* 482    03/18/13  Mu Yang  v1.41: Enable Light Sensor.                      */
/* 483    03/20/13  Mu Yang  v1.42: Auto-backlight;Fix AllocTimer fail.       */
/* 484    03/22/13  Mu Yang  v1.43: Optimise one-second-timer usage.          */
/* 485    03/26/13  Mu Yang  v1.44: Support timestamp log.                    */
/* 486    03/27/13  Mu Yang  v1.45: Optimise log print using vPrintF;         */
/*                           Fix: clock-on-menu, repeat battery alarm;        */
/*                           Support DUO/BLE/BR.                              */
/* 487    03/29/13  Mu Yang  v1.45: Fix time-adjust-not-update-clock;         */
/* 488    03/29/13  Mu Yang  Fix "show 100% when volt < critical level".      */
/* 489    03/30/13  Mu Yang  Ignore msg when msg que overflow.                */
/* 490    04/02/13  Mu Yang  v1.46: Support accelerometer.                    */
/* 491    04/03/13  Mu Yang  Fix flow control in hal_battery.                 */
/* 492    04/05/13  Mu Yang  Fix Backlight-always-on;Disable BT_FLOW and      */
/*                           vTaskDelay for Fll in ChargeControl().           */
/* 493    04/07/13  Mu Yang  Change battery up/down gap limits to 10/5.       */
/* 494    04/08/13  Mu Yang  v1.47: Enable accelerometer.                     */
/* 495    04/09/13  Mu Yang  Fix clock-not-update when SetProperty.           */
/* 497    04/18/13  Mu Yang  v1.49: fmt chg to n.n.nd;							            */
/*                           SrvMenu Test vibration; Rm Tout for connecting   */
/* 499    04/20/13  Mu Yang  Pass BootVer to App to be shown in "Ready...". 	*/
/* 500    04/23/13  Mu Yang  Fix ClearLcd in splash screen sometimes fails. 	*/
/* 501    04/23/13  Mu Yang  Change OneSecondTimer TOUT_ONCE to be 1;         */
/* 502    04/23/13  Mu Yang  Fix always use same timeout for all modes;       */
/* 506    04/26/13  Mu Yang  Support ACK in SPP.                              */
/* 507    04/29/13  Mu Yang  Fix HFP/MAP auto-reconnect.                      */
/* 508    04/30/13  Mu Yang  Fix 0am -> 12am.                                 */
/* 512    05/01/13  Mu Yang  Set MAP connected after Notif Server registered; */
/*                           Support Disconnect BT state for CDT.             */
/* 513    05/01/13  Mu Yang  Fix BR auto-connect regression.                  */
/* 515    05/01/13  Mu Yang  Add G-range mask for configuring accelerometer.  */
/* 520    05/06/13  Mu Yang  Tell phone if ACK is supported.                  */
/* 521    05/06/13  Mu Yang  Print CSTACK once.                               */
/* 522    05/06/13  Mu Yang  Fix GetDevTypeMsg sent to Wrapper queue.         */
/* 523    05/08/13  Mu Yang  Correct accel output to direct XOUT_HPF_H.       */
/* 524    05/10/13  Mu Yang  Accel streaming no need for clearing int.        */
/* 525    05/10/13  Mu Yang  Fix caller name not shown if it's too long.      */
/* 526    05/11/13  Mu Yang  New call notification UI.                        */
/* 527    05/12/13  Mu Yang  Fix Accel streaming reset by postpone long intvl.*/
/* 528    05/13/13  Mu Yang  Fix Accel streaming long intvl timeout.          */
/* 529    05/14/13  Mu Yang  Clear BLE adv flags: simu BR/BLE.                */
/* 530    05/16/13  Mu Yang  Add Hanzi watch face.                            */
/* 531    05/16/13  Mu Yang  TermMode:strcmp() -> strstr();                   */
/*                           Detail outgoing log in wrapper.c.                */
/* 532    06/03/13  Mu Yang  Change battery averaging to BattSenseCycle();    */
/*                           Batt sample gap 10 -> 20; Fix interval stay      */
/*                           short by seperating HB and interval timers.      */
/* 533    06/04/13  Mu Yang  Enable all 4 watchfaces; Spp actv mode bf write()*/
/* 534    06/06/13  Mu Yang  Fix block at whoami() with VERSION string.			  */
/* 535    06/06/13  Mu Yang  Support DayOfWeek in German and Finnish.			    */
/* 536		06/07/13  Mu Yang  Merge with fix for reducing CCS build warning.   */
/* 537		06/10/13  Mu Yang  Spp try more times if part of msg is read;				*/
/*													 Change watch faces' order for backward compatible*/
/* 538		06/10/13  Mu Yang  Send current page indication to phone;						*/
/* 539		06/12/13  Mu Yang  Fix not-change-to-conn-state if not SetCliConfig.*/
/* 540    06/13/13  Mu Yang  Fix 'BLE' not shown in status screen for Android.*/
/* 541    06/18/13  Mu Yang  Fix Accel data only out via BLE.                 */
/* 542    06/27/13  Mu Yang  Refactor SerialRam.c; Bypass CRC.	              */
/* 543    06/28/13  Mu Yang  Set adv flags to support both BR/BLE.            */
/* 544    07/01/13  Mu Yang  Apply errata 1 to revision F as well.            */
/******************************************************************************/



#endif /* VERSION_H */
