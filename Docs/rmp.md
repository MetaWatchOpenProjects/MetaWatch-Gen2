# MetaWatch Remote Message Protocol

1 Introduction
==============

This protocol provides the serial message format for the MetaWatch project.

2 Revision History
==================

Revision | Details | Author | Date
:---: | :--- | :---: | :--- 
1.0 | Original Release | Andrew Hedin, David Rosales | October 3, 2011
1.0.1 | Redefine Button Event Message (0x34) payload to include same set of info as that of Enable Button (0x46). | Mu Yang | June 11, 2012
1.0.2 | Add a Nval identifier “language” to support local “day of week” format | Mu Yang | June 18, 2012
1.0.3 | Add two parameters: start row and number of rows to “Update LCD Display” (0x43). | Mu Yang | June 28, 2012
1.1.0 | Update “WriteBuffer” and “UpdateDisplay” supporting new Widget UI | Mu Yang |September, 2012
1.1.1 | Add “Set Widget List” message | Mu Yang | September 25, 2012
1.1.2 | Correct “UpdateDisplay” bits definition errors. | Mu Yang  | October 18, 2012
1.1.3 | Update “UpdateDisplay” and “Get Information String Response”  | Mu Yang | November 5, 2012
1.1.4 | Update “SetWidgetList”: List shall be in ascend sorting order | Mu Yang | November 5, 2012
1.1.5 | Update “SetWidgetList”: 0-15: home widget IDs | Mu Yang | November 7, 2012
1.1.6 | Add “Music play state message”; update “WriteBuffer” and “UpdateDisplay” | Mu Yang | November 16, 2012
1.1.7 | Change typo “7:0” to “0:7” | Mu Yang | January 15, 2013
1.1.8 | Add new format for Nval operation message (0x30) | Mu Yang | January 15, 2013
1.1.9 | Deprecate Idle buffer configuration Nval key (0x30) | Mu Yang | January 17, 2013
1.2.0 | Update 0x30 | Mu Yang | February 28, 2013
1.2.1 | Update 5.30 (0x57) Read Battery Voltage | Mu Yang | February 28, 2013
1.2.2 | Update 5.33 SetMsgList to include clock widget settings | Mu Yang | March 12, 2013
2.0.0 | Rewritten in markdown format for easy maintenance. | Mu Yang | March 15, 2013
2.0.1 | Add light sensor messages. | Mu Yang | March 18, 2013
2.0.2 | Add enable auto backlight property. | Mu Yang | March 20, 2013
2.0.3 | Add ControlFullScreen message. | Mu Yang | March 26, 2013
2.0.4 | Add accelerometer message | Mu Yang | April 8, 2013
2.0.5 | Remove Get Real Time Clock / Response | Mu Yang | April 9, 2013
2.0.6 | Update the Accelerometer messages | Mu Yang | April 23, 2013
2.0.7 | Update the Accelerometer messages | Mu Yang | May 1, 2013
2.0.8 | Use GetDeviceType to tell watch the phone type info. | Mu Yang | May 6, 2013
2.0.9 | Update GetVersionInfo. | Mu Yang | May 8, 2013
2.1.0 | Support "clear widgets" in Set Widget List message. | Mu Yang | June 16, 2013

3 Abbreviation
===============
Word | Definition
:--- | :---
BLE | Bluetooth Low Energy
BR | Bluetooth Basic Rate
CRC | Cyclic Redundancy Check
GATT | Generic Attribute Profile
HFP | Hands-free Profile
LSB | Less Significant Bit/Byte
MAP | Messaging Access Profile
MSB | Most Significant Bit/Byte
RTC | Real-time Clock
SPP | Serial Port Profile

4 Packet Format
===============

Messages are sent using GATT over BLE or SPP over BR. Most messages originate from the phone but the watch can also send messages. For this system the message size is limited to 32 bytes. The minimum message length is 6 bytes (4 bytes header + 2 bytes CRC). Therefore the message payload length: 0 \<= n \<= 26.

Byte | Name | Value | Description
:---: | :---: | :---: | :--- 
0    | Start | 0x01 | start of the frame
1 | Length | 6 ~ 32 | total length including frame header and CRC
2 | Type | 0 ~ 0xFF | message types detailed in Chapter 4
3 | Options | 0 ~ 0xFF | additional options for the message
0 : n | Payload | - | message specific data. See chapter 4
n+1 : n+2 | CRC | - | 16 bits CRC of the frame

4.1 CRC Generation
------------------

The phone must generate the CRC that matches the MSP430. It uses CRC-CCITT with a starting value of 0xFFFF with reverse input bit order. For example, the Get Device Type message is: 0x01, 0x06, 0x01, 0x00, 0x0B, 0xD9. The CRC is 0xD90B.

5 Message Definitions
=====================

Message Type | Code | Source
:--- | :---: | :---:
Get Device Type | 0x01 | Phone
Get Device Type Response | 0x02 | Watch
Get Version Info | 0x03 | Both
Get Version Info Response | 0x04 | Both
Control Full Screen | 0x42 | Phone
Set Vibrate Mode | 0x23 | Phone
Set Real Time Clock | 0x26 | Phone
Watch Property Operation | 0x30 | Phone
Watch Property Operation Response | 0x31 | Watch
Status Change Event | 0x33 | Watch
Enable Button | 0x46 | Phone
Disable Button | 0x47 | Phone
Button Event Message | 0x34 | Watch
Write LCD Buffer | 0x40 | Phone
Update LCD Display | 0x43 | Phone
Set Widget List Message | 0xA1 | Phone
Load Template | 0x44 | Phone
Set Battery Warning Level Message | 0x53 | Phone
Low Battery Warning Message | 0x54 | Watch
Low Battery Bluetooth off Message | 0x55 | Watch
Get Battery Status Message | 0x56 | Phone
Get Battery Status Response | 0x57 | Watch
Get Light Sensor Value Message | 0x58 | Phone
Get Light Sensor Value Response | 0x59 | Watch
Music Playing State Message | 0x18 | Phone
Setup Accelerometer Message | 0xE1 | Phone
Accelerometer Data Response | 0xE0 | Watch
Write OLED Buffer | 0x10 | Phone
Change OLED Mode | 0x12 | Phone
Write OLED Scroll Buffer | 0x13 | Phone
Advance Watch Hands | 0x20 | Phone

5.1 Get Device Type (0x01)
--------------------------

This message is used to query the type of watch that is connected.

**Options:**

Bit | Description
:---: | :---
0 ~ 5 | Reserved.
6 | HFP/MAP Connected needed.
7 | Support ACK for SPP connection.

**Payload:** not used.

5.2 Get Device Type Response (0x02)
-----------------------------------

This is the response for the message "Get Device Type" from the watch to the phone.

**Options:** Device type of the connected watch

Bit | Description
:---: | :---
0 ~ 3 | Device Type
4 ~ 6 | Reserved
7 | Support ACK for SPP connection

**Device Type:**

* 0 - Reserved
* 1 - Analog watch
* 2 - Digital watch (Gen1)
* 3 - Digital development board (Gen1)
* 4 - Analog development board
* 5 - Digital watch (Gen2)
* 6 - Digital development board (Gen2) 

**Payload:** not used.


5.3 Get Version Info (0x03)
---------------------------------

This message is used for fetching the firmware version of the watch.

**Options:** not used.

**Payload:** not used.

5.4 Get Version Info Response (0x04)
------------------------------------------

This is the response to the message **Get Version Info** from the watch to the phone.

**Options:** not used.

**Payload:**

Byte | Value | Description
:---: | :---: | :---
0 ~ 2 | '0' ~ '9' | 3 digits of general firmware build number (one digit per byte)
3 ~ 5 | '0' ~ '9' | 3 digits of custom build number starting from 001 
6 ~ 8 | 0 ~ 255 | 3 bytes for major, minor and patch version (1 byte each)

5.5 Control Full Screen (0x42)
----------------------------

This message tells the watch that phone would draw full screen.

**Options:**

* 0 - Watch is responsible of drawing the top 1/3 screen (default).
* 1 - Phone is responsible of drawing the full screen.

**Payload:** not used.

5.6 Set Vibrate Mode (0x23)
----------------------------

This message causes the watch to vibrate.

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2 | Byte3 | Byte4 | Byte5
:---: | :---: | :---: | :---: | :---: | :---: 
Enable | ON duration (LSB) | ON duration (MSB) | OFF duration (LSB) | OFF duration (MSB) | Number of ON/OFF cycles


5.7 Set Real Time Clock (0x26)
-------------------------------

This message sets the real time clock in the MSP430.

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2 | Byte3 | Byte4 | Byte5 | Byte6 | Byte7
:---: | :---: | :---: | :---: | :---: | :---: | :---: | :---:
Year (MSB) | Year (LSB) | Month (1~12) | Day of Month (1~31) | Day of Week (0~6) | Hour (0~23) | Minute (0~59) | Second (0~59)

5.8 Watch Property Operation (0x30)
--------------------------

The message is used to get and set watch properties. The properties values retain till the battery is depleted. Please note that all property bits are valid when the message is sent to the watch.

**Options:**

Bit | Description
:---: | :---
0 | Show date in 24H format. Default is 12H (with AM and PM)
1 | Show date in DDMM format. Default is MMDD.
2 | Show seconds in the clock. Default is not-show.
3 | Show separation line between widgets in idle mode. Default is set.
4 | Enable auto-backlight (backlight switches on automatically when notification comes and the surrounding is too dark).
5 | Reserved.
6 | Reserved.
7 | Read operation. Default is Write.

**Payload:** not used.


5.8 Watch Property Operation Response (0x31)
-----------------------------------

For Set Property Response, the **Options** byte contains the result code: 0 - Success; 1 - Failure.

For Read Property Response, the **Options** byte contains the property value.

5.9 Status Change Event (0x33)
-------------------------------

This message is used to notify the phone about the watch's status change. The **Options** byte contains the current mode and idle page (if in idle mode) when the status change event occurs. The **Payload** data tells what kind of event it is.

**Options:**

Bit7 ~ Bit4 | Bit3 ~ Bit0
:---: | :---:
Idle page | Mode

**Mode:**

* 0 - Idle Mode is the default mode for showing status information.
* 1 - App Mode is drawn by apps running on the phone.
* 2 - Notification Mode is for showing phone notifications, e.g. caller ID, SMS, etc.
* 3 - Music Mode is for remote controlling the music player on the phone.

**Payload:**

Byte | Description
:---: | :---
0 | Event Type

**Event Type:**

* 0 - Reserved
* 1 - Mode switching complete
* 2 - Mode timeout (not applicable for the Idle mode)

5.10 Enable Button (0x46)
-------------------------

Each button press type (immediate, release and hold) can generate an event. In addition, each button press type can have a different event for each of the display modes (Idle, Application, Notification and Music). For example, the following **Payload** data are sent to the phone when button A  is pressed in the Notification mode: 0x2, 0x0, 0x0, 0x34, 0x00. The **Button Event Message (0x34)** will be sent to the phone once the button press has been detected (without waiting for the button to be released).

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2 | Byte3 | Byte4
:---: | :---: | :---: | :---: | :---:
Button index | Mode | Event Type | Callback Message Type | Callback Message Options

**Button index:**

Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0
:--: | :--: | :--: | :--: | :--: | :--: | :--: | :--:
Reserved | F | E | Reserved | D | C | B | A

**Event Type:**

* 0 - immediate, button is pressed.
* 1 - release, button is released.
* 2 - hold, button is pressed and held for 6 seconds.

**Callback Message Type** and **Callback Message Options** are 
for messages which are sent out when the button event occurs.

5.11 Disable Button (0x47)
--------------------------

The message is used to remove the association of a message with a button event.

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2
:---: | :---: | :---:
Button index | Mode | Event Type

5.12 Button Event Message (0x34)
--------------------------------

This message is sent from the watch when a button event occurs.

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2 | Byte3 | Byte4
:---: | :---: | :---: | :---: | :---:
Button index | Mode | Event Type | Callback Message Type | Callback Message Options

5.13 Write LCD Buffer (0x40)
----------------------------

This message is used to send the display data (e.g. widget data) from the phone to the corresponding mode screen (Idle, Application, Notification and Music mode) or Idle mode pages (Gen2 UI style only).

**Options:**

Bit7 | Bit6 ~ Bit5 | Bit4 | Bit3 ~ Bit2 | Bit1 ~ Bit0
:---: | :---: | :---: | :---: | :---:
UI Style | Reserved | One Row | Reserved | Mode

**UI Style:**

* 0 - Gen1's three-row UI
* 1 - Gen2's four-quad UI

**One Row:** valid only for SPP.

* 0 - the Payload contains two display rows of data.
* 1 - the Payload contains only one display row of data (12 bytes).

**Payloads:** the payload definition is different from Gen1 UI and Gen2 UI.

* **Gen1 UI:**

Byte0 | Byte1 ~ Byte12 | Byte13 | Byte14 ~ Byte25
:---: | :---: | :---: | :---:
Row A | Data A | Row B | Data B

**Row A/B:** the row number (0 ~ 95) of the display which the data is to be drawn.

**Data A/B:** the data for **Row A/B**.

If **One Row** is set, only **Row A** is valid.

* **Gen2 UI:**

Byte0 | Byte1 | Byte2 ~ Byte13
:---: | :---: | :---:
Widget ID | Row | Data

**Widget ID:** there are Clock Widget ID and Normal Widget ID.

* Clock Widget - drawn by the watch itself. The lower 4 bits of the ID is the Widget ID (0 ~ 15); the higher 4 bits is the clock Face ID (0 ~ 15).
* Normal Widget - drawn by the phone. The Widget ID range is 16 ~ 254.

**Row:** the row number (0 ~ 191) of the widget area. Please note: one widget row is half the size of the display row (96 bits / 8 bits per byte = 12 bytes). The row number counts continuously quad by quad from left to right and up to bottom of the display.

**Data:** two adjacent widget rows of data (12 bytes).


5.14 Update LCD Display (0x43)
------------------------------

This message is used to tell the watch data of which display mode or idle mode page (if in idle mode) shall be drawn to the LCD display.

**Options:**

Bit7 | Bit6 | Bit5 | Bit4 | Bit3 ~ Bit2 | Bit1 ~ Bit0
:---: | :---: | :---: | :---: | :---: | :---:
UI Style | Reserved | Buffer Type | Reserved | Page | Mode

**Buffer Type:** specify which type of buffer's data shall be drawn to the display.

* 0 - Mode buffer specified by **Mode**.
* 1 - Page buffer specified by idle mode **Page** if it's for idle mode     .

**Page:** 0 ~ 3: one of the four idle mode page.

**Payload:** specify which rows of the display shall be updated. Used only for Gen1 UI.

Byte0 | Byte1
:---- | :---
Start Row (0~95) | Row number (1~96)

5.15 Set Widget List Message (0xA1)
-----------------------------------

The message is used to send a list of widgets’ properties to configure up to 4 pages of idle mode screen. There could be totally at most 16 1Q widgets on 4 pages of the Idle mode screen. There are two bytes of each widget’s property: first one is the widget ID and the other is the widget setting (e.g. invert color, layout type, clock widget, etc.). The payload of one message is 14 bytes which can contains 7 widgets’ properties (2 bytes for each widget). So it requires at most 3 messages for sending max 16 widgets’ properties. The order of widgets’ properties in the list shall be according to the widget IDs in ascending order.
Note that if the message contains only one widget property with the invalid widget ID (0xFF), all previously configured widgets would be removed from the watch.

**Options:**

Bit7 ~ Bit4 | Bit3 ~ Bit2 | Bit1 ~ Bit0
:---: | :---: | :---:
Reserved | Total messages | Index of the message (0~2)

**Payload:**

Byte0 | Byte1 | Byte2 | Byte3 | … | Byte n | Byte n+1
:---: | :---: | :---: | :---: | :---: | :---: | :---
Widget0 ID | Widget0 setting | Widget1 ID | Widget1 setting | … | Widget n/2 ID | Widget n/2 setting  

**Widget setting:**

Bit7 | Bit6 | Bit5 ~ Bit4 | Bit3 ~ Bit2 | Bit1 ~ Bit0
:---: | :---: | :---: | :---: | :---: | :---:
Clock/Normal | Invert Color | Page Number | Layout | Position

**Clock/Normal:** 

* 0 - normal widget
* 1 - clock widget

**Invert Color:**

* 0 - not invert the widget colour
* 1 - invert the widget colour 

**Page Number:**

* 0 ~ 3 - which one of the 4 Idle mode pages the widget is to be drawn.

**Layout:**

* 0 - 1Q (one quad)
* 1 - 2Q (two horizontal quads)
* 2 - 2Q (two vertical quads)
* 3 - 4Q (full screen)

**Position:** the starting quad of the page the widget is to be drawn.

* 0 - up-left quad 
* 1 - up-right quad 
* 2 - bottom-left quad
* 3 - bottom-right quad

5.16 Load Template (0x44)
------------------------------------

Currently it's used for setting whole display white (0) or black (1).

**Options:**

Bit7 ~ Bit2 | Bit1 ~ Bit0
:---: | :---:
Reserved | Mode

**Payload:**

Byte0:

* 0 - set display white
* 1 - set display black

5.17 Set Battery Warning Level Message (0x53)
-----------------------------------------

This determines at what charge level (in percentage) a warning message is sent to the phone indicating a low battery event. This message also determines at what level the Bluetooth radio will be shut off to conserve battery
power for watch-only operations. The default warning level 20% and the default Bluetooth off level is 0%. The message overwrites the defaults.

**Options:** not used.

**Payload:**

Byte0 | Byte1
:---: | :---
Warning Level (%)| Radio Off Level (%)

5.18 Low Battery Warning Message (0x54)
---------------------------------------

The message is sent to the phone when the battery is at warning level (see **Set Battery Warning Level Message (0x53)**).

**Options:** not used.

**Payload:** not used.

5.19 Low Battery Bluetooth off Message (0x55)
---------------------------------------------

The message is sent to the phone when the battery is at Bluetooth radio off level (see **Set Battery Warning Level Message (0x53)**).

**Options:** not used.

**Payload:** not used.

5.20 Get Battery Status Message (0x56)
----------------------------------------

The message is used to get the battery status including clip attach, charging, current charge and battery voltage (see **Get Battery Status Response (0x57)**).

**Options:** not used.

**Payload:** not used.

5.21 Read Battery Status Response (0x57)
-----------------------------------------

The message contains the results of the battery status. Battery voltage value is the volts times 100. For example, a value of 4100 means 4.1 volts. 

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2 | Byte3 | Byte4 | Byte5
:---: | :---: | :---: | :---: | :---: | :---:
Clip attached | Charging | Charge (%) | Reserved | Voltage (LSB) | Voltage (MSB)

5.22 Get Light Sensor Value Message (0x58)
----------------------------------------

The message is used to read the light sensor value (see **Get Light Sensor Value Response (0x59)**).

**Options:** not used.

**Payload:** not used.

5.23 Get Light Sensor Value Response (0x59)
-----------------------------------------

The message contains the instant value of the light sensor.

**Options:** not used.

**Payload:**

Byte0 | Byte1
:---: | :---:
Value (LSB) | Value (MSB)

5.24 Music Playing State Message (0x18)
---------------------------------------

This message is used to tell the watch about current music playing state: either “play” or “stop”.

**Options:**

Bit7 ~ Bit1 | Bit0
:---: | :---:
Reserved | Music playing state (0: stopped; 1: playing)

**Payload:** not used.

5.25 Setup Accelerometer Message (0xE1)
--------------------------------------

This message is used for enabling, disabling and setting parameters of the accelerometer. There are two modes: Streaming and Motion Detection mode. In the Streaming mode, the data are sent from watch to phone in 25Hz continuously. In Motion Detection Mode, the data are only sent when it's over a threshold (default is 0.5g) and stopped when it's below.

**Options:**

* 0 - Disable accelerometer
* 1 - Enable accelerometer in Motion Detection mode with threshold (in Payload)
* 2 - Enable accelerometer in Streaming mode (default)

**Payload:**

Byte0 | Byte1|
:---: | :---:
Range | Threshold

**Range:**

* 0 - +/- 2g
* 1 - +/-4g
* 2 - +/-8g (default)

**Threshold (Motion Detection mode):** 

* 0~255. Default value is 16 (0.5g in +/-8g range).


5.26 Accelerometer Data Response (0xE0)
--------------------------------------

This message is used for sending accelerometer date to the phone.

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2
:---: | :---: | :---:
X | Y | Z

5.27 Write OLED Buffer (0x10)
----------------------------
The message is used to send data to the analog watch's OLED display buffer.

**Options:**

Bit7 | Bit6 ~ Bit4 | Bit3 ~ Bit0
:---: | :---: | :---
- | Page Control | Mode

**Page Control:** control the status of the page (specified by **Buffer Select** below).  

* 0 - no action
* 1 - invalidate page
* 2 - Invalidate and Clear Page
* 3 - Invalidate and Fill Page
* 4 - Activate page (validates page also)

**Payload:**

Byte 0 | Byte 1 | Byte 2 | Bytes 3:n
:--: | :--: | :--:
Buffer Select | Column Index | Size | Data

**Buffer Select:** which display buffer to apply

* 0 - Top Page 1
* 1 - Bottom Page 1
* 2 - Top Page 2
* 3 - Bottom Page 2


**Column Index:** which column of the buffer to apply

* 0 ~ 79 - Top Row
* 80 ~ 159 - Bottom Row

**Size:** the length of the data in bytes (0 ~ 23).

**Data:** data to be sent to the watch display buffer.


Only the idle mode contains buffers for the top and bottom page 2. When any byte in a page is written the page is validated. If a page is invalid then it will not be displayed when in idle mode and the middle button is pressed. When the phone wishes to display a page it should set the activate page control bits in the final command it sends.

Scroll control is only valid for the Bottom Page 1 in Notification mode.

If a page is activated and the current mode is not active then the current mode will be changed before displaying the page.

Each display consists of two rows of 80 characters.

5.28 Change OLED Mode (0x12)
----------------------------------

Change the mode of the watch. This command does not cause an update of the top or bottom OLED. It does change how the buttons are handled. When a mode other than IDLE is selected its mode timer is started.

**Options:**

Bit7 ~ Bit4 | Bit3 ~ Bit0
:---: | :---:
Reserved | Mode

**Payload:** not used.

5.29 Write OLED Scroll Buffer (0x13)
-----------------------------------

**Options:**

Bit7 ~ Bit2 | Bit1 | Bit0
:---: | :---: | :---:
Reserved | Scroll Control | Scroll Complete

**Scroll Complete:**

* 0 - This is not the last packet of scroll information
* 1 - This is the last packet of scroll information

**Scroll Control:**

* 0 - No action
* 1 - Scroll Start

**Payload:**

Byte0 | Byte1 ~ Byte n
:---: | :---:
Size of data in bytes | data

The scroll buffer contains 240 bytes that are used to display scroll information. This information is tied to the bottom row of the bottom OLED. This buffer can be written indefinitely.

If the scroll state machine runs out of data then the scroll will be terminated.

The scroll state machine will send a scroll request status message each time it scrolls 80 characters (OLED display columns). The phone is responsible for not writing too many characters to the scroll buffer.

When a scroll is started if the top OLED is on then it will remain on for the duration of the scroll.

5.30 Advance Watch Hands (0x20)
------------------------------

This command will advance the watch hands by the specified amount.

**Options:** not used.

**Payload:**

Byte0 | Byte1 | Byte2
:---: | :---: | :---:
Hour (0~12) | Minute (0~59) | Second (0~59)

