-- jtag: listen to OpenOCD remote_bitbang tcp port (jtag)
--
-- WARNING: this code doesn't work for long JTAG transfers
-- boundary scan works as a proof-of-concept
-- when JTAG transfers longer file, nodemcu reboots or
-- stops working
--
-- TCP port server will listen to
port=3335
-- timeout in seconds for inactive client to disconnect
timeout=1000
-- esp       GPIO: 0, 2, 4, 5,15,13,12,14,16
-- lua nodemcu  D: 3, 4, 2, 1, 8, 7, 6, 5, 0
-- pin assignment
tdo  = 5
tdi  = 0
tck  = 6
tms  = 7
trst = 3
srst = 4

jtag_off = function(conn)
  print("jtag off")
  gpio.mode(tdo,  gpio.INPUT)
  gpio.mode(tdi,  gpio.INPUT)
  gpio.mode(tck,  gpio.INPUT)
  gpio.mode(tms,  gpio.INPUT)
  gpio.mode(trst, gpio.INPUT)
  gpio.mode(srst, gpio.INPUT)
end

jtag_on = function(conn)
  print("jtag on")
  gpio.mode(tdo,  gpio.INPUT)
  gpio.mode(tdi,  gpio.OUTPUT)
  gpio.mode(tck,  gpio.OUTPUT)
  gpio.mode(tms,  gpio.OUTPUT)
  gpio.mode(trst, gpio.OUTPUT)
  gpio.mode(srst, gpio.OUTPUT)
end

-- Sample the value of tdo
jtag_read = function()
  return gpio.read(tdo)
end

-- write tdi, tms, tck
jtag_write = function(tck_tms_tdi)
  -- print("write", tck_tms_tdi)
  val_tck = bit.rshift(bit.band(tck_tms_tdi,4),2)
  val_tms = bit.rshift(bit.band(tck_tms_tdi,2),1)
  val_tdi =            bit.band(tck_tms_tdi,1)
  gpio.write(tdi, val_tdi)
  gpio.write(tms, val_tms)
  gpio.write(tck, val_tck)
  -- print(val_tck, val_tms, val_tdi)
end

jtag_reset = function(trst_srst)
  -- print("reset", trst_srst)
  val_trst = bit.rshift(bit.band(trst_srst,2),1)
  val_srst =            bit.band(trst_srst,1)
  gpio.write(srst, val_srst)
  gpio.write(trst, val_trst)
  -- print(val_trst, val_srst)
end

jtag_parse = function(conn, payload)
   for i = 1, #payload do
     local c = payload:sub(i,i)
     -- print(c)
     if c >= "0" and c <= "7" then
       jtag_write(bit.band(string.byte(c),7))
     end
     if c >= "r" and c <= "u" then
       jtag_reset(bit.band(string.byte(c)-114,3))
     end
     if c == "R" then
       conn:send(jtag_read())
     end
     if c == "B" then
       print("blink on")
     end
     if c == "b" then
       print("blink off")
     end
     if c == "Q" then
       conn:close()
     end
     tmr.wdclr()
   end
end

jtag_off()

-- wifi.setmode(wifi.STATION)
-- wifi.sta.config("ssid","password")

ipaddress=wifi.sta.getip()
if not ipaddress then
  ipaddress="0.0.0.0"
end
print("OpenOCD remote_bitbang "..ipaddress..":"..port)
print("TDO:",tdo,"TDI:",tdi,"TCK:",tck,"TMS:",tms,"TRST:",trst,"SRST:",srst)

-- A simple TCP server
srv=net.createServer(net.TCP, timeout)
srv:listen(port,function(conn)
  jtag_on()
  conn:on("receive", jtag_parse)
  conn:on("disconnection", jtag_off)
end)
