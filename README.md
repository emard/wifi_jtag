# WIFI-JTAG

ESP8266 as wifi jtag adapter.

Here's lua code for nodemcu that listens to port 3335 
and talks remote_bitbang protocol of OpenOCD

When sufficiently developed and optimized, it 
should be used to send SVF bistream to the FPGA.

    interface remote_bitbang
    remote_bitbang_port 3335
    remote_bitbang_host jtag.lan
    jtag newtap tb276 tap -expected-id 0x020f10dd -irlen 10
    init
    scan_chain
    svf -tap tb276.tap project.svf
    shutdown

Currently it is just proof-of-concept which does boundary 
scan. At least it seems that LUA gpio works and wiring is ok.

Here's OpenOCD log of ESP8266 running nodemcu LUA code
with remote_bitbang protocol.

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

Too bad SVF upload will not go.
Might be LUA is slow? 


Now I've rewritten this in C for Arduino ESP8266 (but not yet tested :)

Fastest way is to install normal arduino-1.6.x or higher 
and add support for ESP8266 by following instructions on
https://github.com/arduino/Arduino/wiki/Unofficial-list-of-3rd-party-boards-support-urls#list-of-3rd-party-boards-support-urls

This project will compile all tools for ESP8266 and
add 8266 in arduino IDE and eclipse:
https://github.com/esp8266/Arduino
