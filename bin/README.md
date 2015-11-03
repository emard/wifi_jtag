Precompiled binaries. Upload them like this:

    esptool -vv -cd ck -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf jtag_wifi_serial.cpp.bin
