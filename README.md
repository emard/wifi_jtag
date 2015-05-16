# WIFI-JTAG

ESP8266 for $4 as WIFI JTAG adapter.
This could be the simplest, the cheapest and the slowest JTAG adapter.

Here's arduino code for ESP8266 which listens to port 3335 
and talks remote_bitbang protocol with OpenOCD.

It can upload SVF bistream over WIFI network to the FPGA. 
Tested on boards: TB276 (Altera Cyclone-4) and TB299 
(Xilinx Spartan-6), more on http://www.nxlab.fer.hr/fpgarduino/linux_bitstreams.html

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

# Speed

JTAG upload is slow because OpenOCD creates network 
traffic with short packets of 1-3 bytes sending to and 
from WIFI-JTAG.
Switching direction from receive to transmit takes lot of
time (100-300 ms). This is main reason for lack of decent speed.

Uploading FPGArduino https://github.com/f32c takes

1.4MB SVF to Xilinx in 90 seconds.

750KB SVF to Altera in 250 seconds.

# Reliability

Firmware is useable but not completely stable,
sometimes freezes or reboots, even if not used
for JTAG but just connected to WIFI access point.
We recommend to power it on just before upload.

The GPIO pinout:
    TDO=14, TDI=2, TCK=12, TMS=13, TRST=0, SRST=16, LED=15

TRST and SRST are reset signals usually used for ARM debugging.
Most FPGA don't need them. LED may be left unconnected too.

# Improvement

There is much room for improvement of course: network protocol
from OpenOCD could be optimized to allow longer packets, buffering,
compression (instead of sending same sequence many times), use some
hardware optimization in the ESP8266 like SPI as JTAG accelerator, 
or SVF player approach from JTAGWhisperer.

# Compiling

Precompiled binary is available in bin/
but you probably will have to compile it to 
set your WIFI ssid and password.

To compile from source you need ESP8266 Arduino.
Download from http://arduino.cc and unpack arduino-1.6.4 or higher.
Then add support for ESP8266 in File->Default Settings->Additional Boards Manager URLs
enter URL:

    https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

Select pull down menu Tools->Board->Board Manager
and instal ESP8266 (cca 30MB). Read more on
https://github.com/arduino/Arduino/wiki/Unofficial-list-of-3rd-party-boards-support-urls#list-of-3rd-party-boards-support-urls

If you'd like to compile all tools from source
this project will build them all and also
add 8266 in arduino IDE and eclipse:
https://github.com/esp8266/Arduino

# LUA (not useable)

LUA version of the above does the same protocol but in 
practice it will not work for longer transfers, only
boundary scan. Works unstable and often reboots. 
It is here just as a proof-of-concept.

# Similar JTAG projects:

https://github.com/sowbug/JTAGWhisperer
