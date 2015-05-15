# WIFI-JTAG

ESP8266 for $3 as WIFI JTAG adapter.
This could be the simplest, the cheapest and the slowest JTAG adapter.

Here's arduino code for ESP8266 which listens to port 3335 
and talks remote_bitbang protocol with OpenOCD.

It can upload SVF bistream to the FPGA 
(Currently tested only with Altera Cyclone-4 board TB276).

    interface remote_bitbang
    remote_bitbang_port 3335
    remote_bitbang_host jtag.lan
    jtag newtap tb276 tap -expected-id 0x020f10dd -irlen 10
    init
    scan_chain
    svf -tap tb276.tap project.svf
    shutdown

Here's OpenOCD log of remote_bitbang JTAG adapter made with
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

LUA version of the above does the same protocol but in 
practice it will not work for longer transfers, only
boundary scan. It is here just as a proof-of-concept.

For real JTAG use Arduino C code - very slow but seems to work.

This is the pinout (e.g. 15 is GPIO15)
    TDO=2, TDI=14, TCK=12, TMS=13, TRST=0, SRST=16, LED=15

Precompiled binary is available. Upload them like this:
    esptool -vv -cd ck -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf jtag.cpp_00000.bin -ca 0x40000 -cf jtag.cpp_40000.bin

TRST and SRST are reset signals for ARM. Most FPGA don't need them.
LED is activity LED, may be left unconnected too.

Get ESP8266 Arduino i also very simple:
Fastest way is to install arduino-1.6.4 or higher is to download from
http://arduino.cc and add support for ESP8266 by following instructions on
https://github.com/arduino/Arduino/wiki/Unofficial-list-of-3rd-party-boards-support-urls#list-of-3rd-party-boards-support-urls

If you'd like to compile all tools from source
this project will build them all and also
add 8266 in arduino IDE and eclipse:
https://github.com/esp8266/Arduino

