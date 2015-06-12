# WIFI-JTAG

ESP8266 (ESP-12) for $4 as WiFi JTAG adapter.
This could be the simplest, the cheapest and the slowest JTAG adapter.

Here's arduino code for ESP8266 which listens to TCP port 3335 
and talks remote_bitbang protocol with OpenOCD http://openocd.org over WiFi 
and usb-serial port version of the same, for use with any arduino (without WiFi).

It can upload SVF bistream over WiFi network to the FPGA. 
more on http://www.nxlab.fer.hr/fpgarduino/linux_bitstreams.html

Tested and working on FPGA boards TB276 (Altera Cyclone-4) and TB299 (Xilinx Spartan-6)

    interface remote_bitbang
    remote_bitbang_port 3335
    remote_bitbang_host jtag.lan
    jtag newtap tb276 tap -expected-id 0x020f10dd -irlen 10
    init
    scan_chain
    svf -tap tb276.tap project.svf
    shutdown

OpenOCD log of remote_bitbang JTAG adapter made with
Arduino ESP8266.

    Warn : Adapter driver 'remote_bitbang' did not declare which transports it allows; assuming legacy JTAG-only
    Info : only one transport option; autoselect 'jtag'
    adapter speed: 1000 kHz
    Info : Initializing remote_bitbang driver
    Info : Connecting to jtag.lan:3335
    Info : remote_bitbang driver initialized
    Info : This adapter doesn't support configurable speed
    Info : JTAG tap: tb276.tap tap/device found: 0x020f10dd (mfg: 0x06e, part: 0x20f1, ver: 0x0)
    Warn : gdb services need one or more targets defined
       TapName             Enabled  IdCode     Expected   IrLen IrCap IrMask
    -- ------------------- -------- ---------- ---------- ----- ----- ------
     0 tb276.tap              Y     0x020f10dd 0x020f10dd    10 0x01  0x03
    shutdown command invoked
    Info : remote_bitbang interface quit

What works for one FPGA, doesn't neccessary work for the other.

# TCP-Serial

By simply pressing ENTER in telnet session Wifi-jtag will enter tcp-serial bridge mode, 
thus allowing remote serial communication to FPGA over WiFi.
Serial break may be issued with ctrl-@ at start of telnet session.
Serial break resets FPGArduino F32C CPU and enters bootloader,
which can accept a hex or binary executable file.

    telnet jtag.lan 3335
    ctrl-]
    telnet> mode char
    ctrl-@
    mi32l>
    mi32l>
    mi32l>
  
virtual serial port:

    socat -d -d pty,link=/dev/ttyS5,raw,echo=0  tcp:xilinx.lan:3335
  
sending ascii file over tcp. (executable compiled hex file).
For serial break to enter bootloader before upload, first
char of the file should be a null char (ascii 0 aka \0)

    socat -u FILE:blink.cpp.hex TCP:jtag.lan:3335

# Pinout

The pinout with standard TXD and RXD:

    TDO=14, TDI=16, TCK=12, TMS=13, TRST=4, SRST=5, TXD=1, RXD=3, LED=15

Alternte serial pinout can be defined at compile time:

    TXD=15, RXD=13

![ESP-12 pinout](/pic/esp-12_pindef.png)
![Altera 10-pin and Xilinx 14-pin](/pic/altera10pin_xilinx14pin.jpg)

  PIN   | nodemcu |   ESP-12   | ESP-201   | wire    |TB276 10-pin |TB299 14-pin
--------|---------|------------|-----------|---------|-------------|----------------
  GND   |   GND   |   GND      | GND       | black   |      10     | 1,3,5,7,9,11,13
  TMS   |   D0    |   GPIO16   | XPD       | violet  |       5     |    4
  TCK   |   D5    |   GPIO14   | 1014      | yellow  |       1     |    6
  TDO   |   D6    |   GPIO12   | 1012      | green   |       3     |    8
  TDI   |   D7    |   GPIO13   | 1013      | blue    |       9     |   10
  VCC   |   3V3   |   VCC      | 3.3V      | red     |       4     |    2
  TXD0  |   D10   |TXD or GPIO1| TX        | orange  |             |RXD 94
  RXD0  |   D9    |RXD or GPIO3| RX        | white   |             |TXD 97
  GND   |         |GPIO15 or NC| 1015      | 15k     |             |
  VCC   |         | EN or CH_PD| CHIP_EX   | 15k     |             |
  VCC   |         | GPIO0 or NC| 100       | 15k     |             |
  VCC   |         | GPIO2 or NC| 102       | 15k     |             |
  VCC   |         |   REST     | RST       | 15k     |             |

Warning: Some ESP-12 modules and breakout boards have GPIO4 and GPIO5 swapped
and wrong labeling for them. If it doesn't work, try swapping GPIO4 and GPIO5.
See ESP8266 family pinouts:
http://www.esp8266.com/wiki/doku.php?id=esp8266-module-family

Avoid connecting GPIO 0, 2, 15 (and 16 on some ESP) to target FPGA, 
as those pins need to be at some default state during first second 
of power on for ESP8266 to boot firmware. If it doesn't boot, 
they need to be disconnected during first second of powering up the ESP8266.

TRST and SRST are reset signals usually used for ARM debugging.
Most FPGA don't need them. LED may be left unconnected too.
WiFi-JTAG board can be directly powered from JTAG connector.

We recommend ESP-12 or nodemcu as they have plenty of free GPIO.
They need external usbserial to flash them, see next section "Flashing".

"nodemcu-devkit" board has onboard 3.3V serial over micro usb:
https://github.com/nodemcu/nodemcu-devkit but if TXD/RXD need to be
used to communicate with FPGA then TXD must be GPIO15 and manually 
disconnected during power up.

ESP-201 is currently not recommended. Those boards are different.
Boundary scan works but TCP becomes slow and stalls after some time.

# Flashing

To flash firmware into ESP8266 connect 3.3V USBSERIAL adapter:

USBSERIAL | wire   | ESP-12      | ESP-201
----------|--------|-------------|---------
 GND      | black  | GND         | GND
 GND      | 1k     | GPIO15      | 1015
 VCC      | 1k     | EN or CH_PD | CHIP_EX
 VCC      | red    | VCC 3.3V    | 3.3V
 RXD      | orange | TXD         | TX
 TXD      | white  | RXD         | RX
 DTR      | green  | GPIO0       | 100
 RTS      | yellow | REST        | RST


# Compiling

Precompiled binary is available in bin/

Default SSID is "jtag" (without quotes) and no password
for connecting to free access point

It has to be compiled to change SSID and PASSWORD.

To compile from source you need ESP8266 Arduino.
Download from http://arduino.cc and unpack arduino-1.6.4 or higher.
Then add support for ESP8266

This is the main project that started it all
https://github.com/esp8266/Arduino
Automatic installation using JSON is now avaialble
in File->Default Settings->Additional Boards Manager URLs enter:

    http://arduino.esp8266.com/package_esp8266com_index.json

Select pull down menu Tools->Board->Board Manager
and instal ESP8266 (cca 30MB).
Read more on
https://github.com/arduino/Arduino/wiki/Unofficial-list-of-3rd-party-boards-support-urls#list-of-3rd-party-boards-support-urls

https://github.com/sandeepmistry/esp8266-Arduino
version 0.0.5 is simple to manually install 
(just copy to ~/Arduino/hardware) 
and has pretty stable and fast TCP stack.

Adafruit has some ESP8266 has JSON install but 
speed and stability of TCP is not the best (old library?).
in File->Default Settings->Additional Boards Manager URLs enter:

    https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

Select pull down menu Tools->Board->Board Manager and instal ESP8266 (cca 30MB). 

If you have ESP-12 or nodemcu, choose board "Generic ESP8266 Module".



# Speed

JTAG upload is slow because OpenOCD creates network 
traffic with short packets of 1-3 bytes sending to and 
from WiFi-JTAG. Watch the traffic:

    tcpdump -A port 3335

Switching direction from receive to transmit takes time
1-300 ms.

Time uploading FPGArduino https://github.com/f32c 

    750KB SVF file to TB276 (Altera Cyclone-4)
    WiFi sandeepmistry 0.0.5 : 1:45 minutes
    Serial 230400 baud       : 4:20 minutes

    1.4MB SVF file to TB299 (Xilinx Spartan-6)
    WiFi sandeepmistry 0.0.5 : 1:30 minutes

# Reliability

Firmware is useable but not completely stable.
After one JTAG TCP connection, it is good
to wait few seconds for the sockets to close
properly before next connection.
It may sometimes stop responding when just connected 
to WiFi access point even if JTAG is not used.
We recommend to power it on just before use.

# Improvement

There is much room for improvement of course: network protocol
from OpenOCD could be optimized to allow longer packets, buffering,
compression (instead of sending same sequence many times), use some
hardware optimization in the ESP8266 like SPI as JTAG accelerator, 
or SVF player approach from JTAGWhisperer.

# LUA (not useable)

LUA version of the above does the same protocol but in 
practice it will not work for longer transfers, only
boundary scan. TCP disconnects within a minute.
It is here just as a proof-of-concept.

# Similar JTAG projects:

Xilinx virtual cable in ESP nodemcu LUA
https://github.com/wzab/esp-wifi-xvc
http://forums.xilinx.com/t5/Configuration/Problems-with-XVC-plugin-both-in-ISE-Impact-and-in-Vivado/m-p/631321

JTAG SVF player
https://github.com/sowbug/JTAGWhisperer

Xilinx virtual cable
https://github.com/Xilinx/XilinxVirtualCable
https://github.com/tmbinc/xvcd

FT232R bitbang
http://vak.ru/doku.php/proj/bitbang/bitbang-jtag

JTAGduino
https://github.com/balau/JTAGduino
