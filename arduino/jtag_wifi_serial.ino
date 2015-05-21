/*
 *  Arduino OpenOCD remote_bitbang WIFI-JTAG server for ESP8266
 *  LICENSE=GPL
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

#include <ESP8266WiFi.h>

// cross reference gpio to nodemcu lua pin numbering
// esp gpio:     0, 2, 4, 5,15,13,12,14,16
// lua nodemcu:  3, 4, 2, 1, 8, 7, 6, 5, 0

// GPIO pin assignment
// Try to avoid connecting JTAG to GPIO 0, 2, 15, 16 (board may not boot)
enum { TDO=12, TDI=14, TCK=4, TMS=5, TRST=0, SRST=2, LED=16 };
// RXD=GPIO13 and TXD=GPIO15 used as Serial.swap() alternate serial port
// additional TXD=GPIO2 (#define TXD_GPIO2 1)

enum { MODE_JTAG=0, MODE_SERIAL=1 };

const char* ssid = "ssid";
const char* password = "password";

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

// specify TCP port to listen on as an argument
WiFiServer server(3335);
WiFiClient clients[MAX_SRV_CLIENTS];

// 0: disabled watchdog because ESP watchdog is not working
// 1: enable watchdog
#define WATCHDOG 0

// *** serial port settings ***
#define BAUDRATE 115200

// 0: use standard  serial pins TX=GPIO1  RX=GPIO3
// 1: use alternate serial pins TX=GPIO15 RX=GPIO13
#define SERIAL_SWAP 1

// 0: don't use additional TX
// 1: use additional TX at GPIO2
// this option currently doesn't work, leaving it 0
#define TXD_GPIO2 0

// serial RX buffer size
// it must be buffered otherwise ESP8266 reboots
// default 1024 bytes
#define RXBUFLEN 1024

// led logic
#define LED_ON LOW
#define LED_OFF HIGH

uint8_t jtag_state = 0;

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

void setup() {
  jtag_off();
  #if WATCHDOG
  ESP.wdtEnable(1024);
  #endif
  Serial.begin(BAUDRATE);
  #if TXD_GPIO2
  Serial1.begin(BAUDRATE);
  #endif
  #if SERIAL_SWAP
  Serial.swap();
  #endif
  
  pinMode(LED, OUTPUT);
  
  // Connect to WiFi network
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    #if WATCHDOG
    ESP.wdtFeed();
    #endif
    // blink when trying to connect
    digitalWrite(LED, LED_ON);
    delay(10);
    digitalWrite(LED, LED_OFF);
    delay(500);
    //Serial.print(".");
  }
  
  // Start the server
  server.begin();
  server.setNoDelay(true);
  //Serial.println("Server started");

  // Print the IP address
  // Serial.println(WiFi.localIP());
  pinMode(LED, INPUT);
}

void serial_break()
{
  #if SERIAL_SWAP
  Serial.swap(); // second swap should un-swap
  #endif
  Serial.end(); // shutdown serial port
  #if TXD_GPIO2
  // if we want to drive additional tx line
  Serial1.end(); // shutdown it too
  #endif
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW); // line LOW is serial break
  #if TXD_GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  #endif
  delay(210); // at least 200ms we must wait for BREAK to take effect
  Serial.begin(BAUDRATE); // start port just before use
  #if SERIAL_SWAP
  Serial.swap(); // port started, break removed at GPIO15 (will now become serial TX)
  #endif
  #if TXD_GPIO2
  Serial1.begin(BAUDRATE); // port started, break removed at GPIO2 (will now become serial TX)
  #endif
  Serial.flush();
}

void loop() {
  static uint8_t mode = MODE_JTAG; // start with JTAG remote_bitbang mode
  uint8_t i;
  uint8_t nconn = 0;

  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!clients[i] || !clients[i].connected()){
        if(clients[i]) clients[i].stop();
        clients[i] = server.available();
        // Serial1.print("New client: "); Serial1.print(i);
        mode = MODE_JTAG;
        if(jtag_state == 0)
          jtag_on();
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient client_reject = server.available();
    client_reject.stop();
    delay(1);
  }
  
  #if WATCHDOG
  ESP.wdtFeed();
  #endif
  
  nconn = 0; // counts number of clients currently connected
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++)
  {
    if (clients[i] && clients[i].connected())
    {
        nconn++;
        // client has connected. if JTAG interface was off, turn it on 
        while(clients[i].available())
        {
          if(mode == MODE_JTAG)
          {
            char c = clients[i].read();
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
                clients[i].write('0'+jtag_read());
                break;
              case 'r':
              case 's':
              case 't':
              case 'u':
                jtag_reset((c-'r') & 3);
                break;
              case 'B':
                digitalWrite(LED, LED_ON);
                break;
              case 'b':
                digitalWrite(LED, LED_OFF);
                break;
              case '\0': // ctrl-@ (byte 0x00) sends serial BREAK
                serial_break();
                break;
              case '\r':
                jtag_off();
                mode = MODE_SERIAL;
                break;
              case 'Q':
                clients[i].stop(); // disconnect client's TCP
                break;
            } /* end switch */
          } // end mode == JTAG
          else
          {
            // get data from the telnet client and push it to the UART
            if(mode == MODE_SERIAL)
              Serial.write(clients[i].read());
          }
         
          #if WATCHDOG
          ESP.wdtFeed();
          #endif
        }
    }
  }

  if(nconn == 0)
  {
    // all clients disconnected
    // JTAG interface off
    if(jtag_state != 0)
      jtag_off();
  }

  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    if(mode == MODE_SERIAL)
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        if (clients[i] && clients[i].connected()){
          clients[i].write(sbuf, len);
          delay(1);
        }
      }
  }
}
