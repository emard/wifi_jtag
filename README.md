# WIFI-JTAG

ESP8266 as wifi jtag adapter.

Here's lua code for nodemcu that listens to port 3335 
and talks remote_bitbang protocol of OpenOCD

It should be used to send SVF bistream to the FPGA.

Currently it is just proof-of-concept which does boundary 
scan. At least it shows that input/output wiring is ok.

interface remote_bitbang
remote_bitbang_port 3335
remote_bitbang_host jtag.lan
jtag newtap tb276 tap -expected-id 0x020f10dd -irlen 10
init
scan_chain
# svf -tap tb276.tap project.svf
shutdown

Too bad SVF upload will not go.

Might be LUA is too slow. It should be rewritten in C
