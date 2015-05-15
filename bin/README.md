Precompiled binaries. Upload them like this:

    esptool -vv -cd ck -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf jtag.cpp_00000.bin -ca 0x40000 -cf jtag.cpp_40000.bin
