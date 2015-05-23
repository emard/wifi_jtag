/*  
 *  JTAG-WIFI with TCP-SERIAL by Emard
 *
 *  OpenOCD compatible remote_bitbang WIFI-JTAG server
 *  for ESP8266 Arduino
 *
 *  OpenOCD interface:
 *
 *  interface remote_bitbang
 *  remote_bitbang_host jtag.lan
 *  remote_bitbang_port 3335
 *
 *  telnet jtag.lan 3335
 *  <ctrl-]>
 *  telnet> mode c
 *  <ctrl-@>
 *  mi32l>
 *
 *  virtual serial port:
 *  socat -d -d pty,link=/dev/ttyS5,raw,echo=0  tcp:xilinx.lan:3335
 *
 *  send file to tcp
 *  socat -u FILE:blink.cpp.hex TCP:xilinx.lan:3335
*/

/* 
  based on
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266
  by Hristo Gochkov

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <ESP8266WiFi.h>

const char* ssid = "ssid";
const char* password = "password";

// cross reference gpio to nodemcu lua pin numbering
// esp gpio:     0, 2, 4, 5,15,13,12,14,16
// lua nodemcu:  3, 4, 2, 1, 8, 7, 6, 5, 0

// GPIO pin assignment
// Try to avoid connecting JTAG to GPIO 0, 2, 15, 16 (board may not boot)
enum { TDO=12, TDI=14, TCK=4, TMS=5, TRST=0, SRST=2, LED=16 };
// RXD=GPIO13 and TXD=GPIO15 used as Serial.swap() alternate serial port
// additional TXD=GPIO2 (#define TXD_GPIO2 1)

enum { MODE_JTAG=0, MODE_SERIAL=1 };

uint8_t mode = MODE_JTAG; // input parser mode JTAG (remote bitbang)

// *** serial port settings ***
#define BAUDRATE 115200

// use standard  serial pins TX=GPIO1  RX=GPIO3
#define SERIAL_SWAP 0
// use alternate serial pins TX=GPIO15 RX=GPIO13
//#define SERIAL_SWAP 1

// 0: don't use additional TX
// 1: use additional TX at GPIO2
// this option currently doesn't work, leaving it 0
#define TXD_GPIO2 0

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

WiFiServer server(3335);
WiFiClient serverClients[MAX_SRV_CLIENTS];

// led logic
#define LED_ON LOW
#define LED_OFF HIGH
#define LED_DIM INPUT_PULLDOWN

uint8_t jtag_state = 0; // 0:off 1:on

// activate JTAG outputs
void jtag_on(void)
{
  // Serial.println("jtag on");
  pinMode(TDO, INPUT);
  pinMode(TDI, OUTPUT);
  pinMode(TMS, OUTPUT);
  pinMode(TRST, OUTPUT);
  pinMode(SRST, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(TCK, OUTPUT);
  jtag_state = 1;
}

// deactivate JTAG outputs: all pins input
void jtag_off(void)
{
  // Serial.println("jtag off");
  pinMode(TCK, INPUT);
  pinMode(TDO, INPUT);
  pinMode(TDI, INPUT);
  pinMode(TMS, INPUT);
  pinMode(TRST, INPUT);
  pinMode(SRST, INPUT);
  digitalWrite(LED, LED_OFF);
  pinMode(LED, INPUT);
  jtag_state = 0;
}

uint8_t jtag_read(void)
{
  return digitalRead(TDO);
}

void jtag_write(uint8_t tck_tms_tdi)
{
  digitalWrite(TDI, tck_tms_tdi & 1 ? HIGH : LOW);
  digitalWrite(TMS, tck_tms_tdi & 2 ? HIGH : LOW);
  digitalWrite(TCK, tck_tms_tdi & 4 ? HIGH : LOW);
}

void jtag_reset(uint8_t trst_srst)
{
  digitalWrite(SRST, trst_srst & 1 ? HIGH : LOW);
  digitalWrite(TRST, trst_srst & 2 ? HIGH : LOW);
}


void serial_break()
{
  pinMode(LED, LED_DIM);
  digitalWrite(LED, LED_ON);
  #if SERIAL_SWAP
  Serial.swap();
  #endif
  Serial.end(); // shutdown serial port
  #if TXD_GPIO2
  // if we want to drive additional tx line
  Serial1.end(); // shutdown it too
  #endif
  #if SERIAL_SWAP
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW); // line LOW is serial break
  #else
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW); // line LOW is serial break  
  #endif
  #if TXD_GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  #endif
  delay(210); // at least 200ms we must wait for BREAK to take effect
  Serial.begin(BAUDRATE); // start port just before use
  // remove serial break either above or after swap
  #if SERIAL_SWAP
  Serial.swap();
  #endif
  #if TXD_GPIO2
  Serial1.begin(BAUDRATE); // port started, break removed at GPIO2 (will now become serial TX)
  #endif
  pinMode(LED, INPUT);
  digitalWrite(LED, LED_OFF);
  Serial.flush();
}

void tcp_parser(WiFiClient *client)
{
  if(mode == MODE_JTAG)
  {
    char c = client->read();
    switch(c)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        jtag_write(c & 7); // it's the same as ((c-'0') & 7)
        break;
      case 'R':
        client->write('0'+jtag_read());
        break;
      case 'r':
      case 's':
      case 't':
      case 'u':
        jtag_reset((c-'r') & 3);
        break;
      case 'B':
        if(jtag_state == 0)
          jtag_on();
        digitalWrite(LED, LED_ON);
        break;
      case 'b':
        digitalWrite(LED, LED_OFF);
        break;
      case '\0': // ctrl-@ (byte 0x00) sends serial BREAK
        serial_break();
        break;
      case '\r': // press enter to tcp-serial mode
        jtag_off();
        mode = MODE_SERIAL;
        break;
      case 'Q':
        jtag_off();
        client->stop(); // disconnect client's TCP
        break;
    } /* end switch */
  } // end mode == JTAG
  else
  {
    // get data from the telnet client and push it to the UART
    if(mode == MODE_SERIAL)
    {
      pinMode(LED, OUTPUT);
      digitalWrite(LED, LED_ON);
      Serial.write(client->read());
      digitalWrite(LED, LED_OFF);
    }
  }
}

void setup() {
  jtag_off();
  pinMode(LED, OUTPUT);
  Serial1.begin(BAUDRATE);
  WiFi.begin(ssid, password);
  Serial1.print("\nConnecting to "); Serial1.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  { // blink LED when trying to connect
    digitalWrite(LED, LED_ON);
    delay(10);
    digitalWrite(LED, LED_OFF);
    delay(500);
  }
  //start UART and the server
  Serial.begin(BAUDRATE);
  #if SERIAL_SWAP
  Serial.swap();
  #endif
  server.begin();
  server.setNoDelay(true);
  
  Serial1.print("Ready! Use 'telnet ");
  Serial1.print(WiFi.localIP());
  Serial1.println(" 3335' to connect");
}

void loop() {
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        Serial1.print("New client: "); Serial1.print(i);
        mode = MODE_JTAG;
        if(jtag_state == 0)
          jtag_on();
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        //get data from the telnet client and push it to the UART
        // while(serverClients[i].available()) Serial.write(serverClients[i].read());
        while(serverClients[i].available()) tcp_parser(&(serverClients[i]));
      }
    }
  }
  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (mode == MODE_SERIAL)
      {
        if (serverClients[i] && serverClients[i].connected()){
          pinMode(LED, LED_DIM);
          serverClients[i].write(sbuf, len);
          delay(1);
          pinMode(LED, INPUT);
        }
      }
    }
  }
}
