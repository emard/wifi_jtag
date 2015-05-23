# WIFI-JTAG

ESP8266 (ESP-12) for $4 as WIFI JTAG adapter.
This could be the simplest, the cheapest and the slowest JTAG adapter.

Here's arduino code for ESP8266 which listens to TCP port 3335 
and talks remote_bitbang protocol with OpenOCD over WiFi and a serial port
version of the same, for use with any arduino (without WIFI).

It can upload SVF bistream over WIFI network to the FPGA. 
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

Wifi-jtag can enter tcp-serial bridge mode, thus allowing remote
serial communication to FPGA over WiFi.
Serial break may be issued with K or ctrl-@ at start of telnet session.
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

The GPIO pinout:
    TDO=12, TDI=14, TCK=4, TMS=5, TRST=0, SRST=2, LED=16

The serial pinout is standard RXD and TXD.
The alternte serial pinout (if compiled with SERIAL_SWAP 1):
    RXD=13, TXD=15

![ESP-12 pinout](/pic/esp-12_pindef.png)
![Altera 10-pin and Xilinx 14-pin](/pic/altera10pin_xilinx14pin.jpg)

  PIN   | nodemcu |   ESP-12E  | wire   |TB276 10-pin |TB299 14-pin
--------|---------|------------|--------|-------------|----------------
  GND   |   GND   |   GND      | black  |      10     | 1,3,5,7,9,11,13
  TMS   |   D1    |   GPIO5    | violet |       5     |    4
  TDI   |   D5    |   GPIO14   | blue   |       9     |   10
  TDO   |   D6    |   GPIO12   | green  |       3     |    8
  TCK   |   D2    |   GPIO4    | yellow |       1     |    6
  VCC   |   3V3   |   VCC      | red    |       4     |    2
  TXD0  |   D10   |   GPIO1    | orange |             |RXD 94
  RXD0  |   D9    |   GPIO3    | white  |             |TXD 97
  TXD2  |   D8    |   GPIO15   | orange |             |RXD 94
  RXD2  |   D7    |   GPIO13   | white  |             |TXD 97
  GND   |         |   GPIO15   | 1k     |             |
  VCC   |         | EN or CH_PD| 1k     |             |
  VCC   |         |   GPIO0    | 15k    |             |
  VCC   |         |   GPIO2    | 15k    |             |
  VCC   |         |   REST     | 15k    |             |

Warning1: use either TXD0/RXD0 or TXD2/RXD2 not 0 and 2 at the same time.
If ESP8266 doesn't boot at power up, you mignt try the
other pin or disconnect both GPIO2 and GPIO15.

Warning2: Older ESP-12 modules have GPIO4 and GPIO5 swapped.
There are some ESP-12E modules and breakout boards with
old labeling for ESP-12. If it doesn't work, try swapping GPIO4 and GPIO5.
See ESP8266 family pinouts:
http://www.esp8266.com/wiki/doku.php?id=esp8266-module-family

Avoid connecting GPIO 0, 2, 15, 16 to target FPGA, as those
pins need to be at some default state at power on for ESP8266
to boot firmware. Use ESP-12 as it has has plenty of GPIO.
There's complete development board with micro usb:
https://github.com/nodemcu/nodemcu-devkit

TRST and SRST are reset signals usually used for ARM debugging.
Most FPGA don't need them. LED may be left unconnected too.
WIFI-JTAG board can be directly powered from JTAG connector.

# Flashing

To flash firmware into ESP8266 connect 3.3V USBSERIAL adapter:

USBSERIAL | wire   |ESP-12E  
----------|--------|-----------
GND       | black  | GND
GND       | 1k     | GPIO15
VCC       | red    | VCC 3.3V
VCC       | 1k     | EN or CH_PD
RXD       | orange | TXD
TXD       | white  | RXD
DTR       | green  | GPIO0
RTS       | yellow | REST


# Compiling

Precompiled binary is available in bin/

Default SSID=ssid PASSWORD=password

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


# Speed

JTAG upload is slow because OpenOCD creates network 
traffic with short packets of 1-3 bytes sending to and 
from WIFI-JTAG. Watch the traffic:

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
to WIFI access point even if JTAG is not used.
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
boundary scan. Works unstable and often reboots. 
It is here just as a proof-of-concept.

# Similar JTAG projects:

https://github.com/sowbug/JTAGWhisperer
