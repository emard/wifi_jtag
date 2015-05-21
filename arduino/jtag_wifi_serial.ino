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
// GPIO 13=RXD2 and 15=TXD2 used as Serial.swap() alternate serial port

enum { MODE_JTAG=0, MODE_SERIAL=1 };

// led logic
#define LED_ON LOW
#define LED_OFF HIGH

const char* ssid = "ssid";
const char* password = "password";

// specify TCP port to listen on as an argument
WiFiServer server(3335);
WiFiClient client;

// 0 disabled watchdog because ESP watchdog is not working
#define WATCHDOG 0
// serial RX buffer size
// it must be buffered otherwise ESP8266 reboots
#define RXBUFLEN 1024

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
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  
  // Connect to WiFi network
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    #if WATCHDOG
    ESP.wdtFeed();
    #endif
    digitalWrite(LED, LED_ON);
    delay(10);
    digitalWrite(LED, LED_OFF);
    delay(500);
    //Serial.print(".");
  }
  
  // Start the server
  server.begin();
  //Serial.println("Server started");

  // Print the IP address
  // Serial.println(WiFi.localIP());
  pinMode(LED, INPUT);

  Serial.swap(); // change serial pins to GPIO 13,15
}

void serial_break()
{
  Serial.swap();
  Serial.end();
  Serial.begin(115200);
  pinMode(15, INPUT); // pull down resistor will make serial break
  delay(210);
  Serial.swap(); // break removed. GPIO15 will now become serial port
  Serial.flush();
}

void loop() {
  static uint8_t mode = MODE_JTAG; /* 0-jtag 1-serial */
  static uint8_t rxbuf[RXBUFLEN]; // rx buffer
  static size_t rxbufptr = 0;
  // Check if a client has connected
  #if WATCHDOG
  ESP.wdtFeed();
  #endif
  client = server.available();
  if(client)
  {
    // Serial.println("new client");
    //if(mode == MODE_SERIAL)
    //   Serial.swap();
    mode = MODE_JTAG;
    jtag_on();
    while(client.connected())
    {
      // loop infinitely until incoming data are available
      switch(mode)
      {
        case MODE_JTAG:
          if(client.available())
          {
            char c = client.read();
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
                client.write('0'+jtag_read());
                yield();
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
              case 'K': // serial break
              case '\0':
                serial_break();
                Serial.flush();
                break;
              case '\r':
                jtag_off();
                Serial.flush(); // discard stale buffer
                mode = MODE_SERIAL;
                break;
              case 'Q':
                client.flush();
                client.stop(); // disconnect client's TCP
                break;
            } /* end switch */
          }
          #if WATCHDOG
          ESP.wdtFeed();
          #endif
          yield(); // handle TCP stack
          break;
        case MODE_SERIAL:
          if(Serial.available() > 0)
          {
            // buffering prevents from firmware reboot
            rxbuf[rxbufptr++] = Serial.read();
            if(rxbufptr >= RXBUFLEN)
            { // if buffer is full, send it via network
              client.write((uint8_t *)rxbuf, rxbufptr);
              rxbufptr = 0;
            }
          }
          else
          {
            if(rxbufptr)
            { // if something is left in RX buffer, send it via network
              client.write((uint8_t *)rxbuf, rxbufptr);
              rxbufptr = 0;              
            }
            else
            {
              // if no RX then read from network -> serial TX
              if(client.available() > 0)
              {
                Serial.write(client.read());
              }
            }
          }
          yield(); // handle TCP stack
          #if WATCHDOG
          ESP.wdtFeed();
          #endif
          break;
      } /* end switch */   
    }
    jtag_off();
    client.flush();
    client.stop();
    // Serial.println("client disonnected");
  }
}
