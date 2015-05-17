# WIFI-JTAG

ESP8266 (ESP-12) for $4 as WIFI JTAG adapter.
This could be the simplest, the cheapest and the slowest JTAG adapter.

Here's arduino code for ESP8266 which listens to TCP port 3335 
and talks remote_bitbang protocol with OpenOCD over WiFi and a serial port
version of the same, for use with any arduino (without WIFI).

It can upload SVF bistream over WIFI network to the FPGA. 
more on http://www.nxlab.fer.hr/fpgarduino/linux_bitstreams.html

Works for FPGA board TB276 (Altera Cyclone-4).

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
FPGA board TB299 (Xilinx Spartan-6) seems like everything is fine:

    svf file programmed successfully for 59 commands with 0 errors

But FPGA will not start. Maybe upload too slow?

# Pinout

The GPIO pinout:
    TDO=12, TDI=13, TCK=14, TMS=5, TRST=0, SRST=2, LED=16

TRST and SRST are reset signals usually used for ARM debugging.
Most FPGA don't need them. LED may be left unconnected too.
WIFI-JTAG board can be directly powered from JTAG connector.

Avoid using GPIO 0, 2, 15, 16 for JTAG signals, as those
pins need to be at some defalut state at power on for ESP8266
to boot firmware. Use ESP-12 as it has has plenty of GPIO.
There's complete development board with micro usb:
https://github.com/nodemcu/nodemcu-devkit

# Compiling

Precompiled binary is available in bin/

Default SSID=ssid PASSWORD=password

It has to be compiled to change SSID and PASSWORD.

To compile from source you need ESP8266 Arduino.
Download from http://arduino.cc and unpack arduino-1.6.4 or higher.
Then add support for ESP8266

https://github.com/sandeepmistry/esp8266-Arduino
version 0.0.5 is simple to install and has pretty 
stable and fast TCP stack. 

Adafruit has some ESP8266 which is easy to install but 
speed and stability of TCP is not the best.
in File->Default Settings->Additional Boards Manager URLs enter URL:

    https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

Select pull down menu Tools->Board->Board Manager
and instal ESP8266 (cca 30MB). Read more on
https://github.com/arduino/Arduino/wiki/Unofficial-list-of-3rd-party-boards-support-urls#list-of-3rd-party-boards-support-urls

This is the main project that started it all
If you'd like to compile all tools from source
this project will build them all and also
add 8266 in arduino IDE and eclipse:
https://github.com/esp8266/Arduino

# Speed

JTAG upload is slow because OpenOCD creates network 
traffic with short packets of 1-3 bytes sending to and 
from WIFI-JTAG. Watch the traffic:

    tcpdump -A port 3335

Switching direction from receive to transmit takes lot of
some time.

Time uploading FPGArduino https://github.com/f32c 750KB SVF
file to TB276 (Altera Cyclone-4)

    WiFi sandeepmistry 0.0.5 : 2 minutes
    Serial 230400 baud       : 4 minutes

# Reliability

Firmware is useable but not completely stable.
It may sometimes stop working when just connected 
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
