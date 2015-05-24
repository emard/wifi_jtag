/*
 *  Arduino OpenOCD remote_bitbang SERIAL-JTAG for ESP8266
 *  LICENSE=GPL
 *
 *  Needs TCP socket-to-serial bridge:
 *
 *  socat TCP4-LISTEN:3335,fork /dev/ttyUSB0,b230400,raw,echo=0,crnl
 *
 *  OpenOCD interface:
 *
 *  interface remote_bitbang
 *  remote_bitbang_host localhost
 *  remote_bitbang_port 3335
 */

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

// cross reference gpio to nodemcu lua pin numbering
// esp       GPIO: 0, 2, 4, 5,15,13,12,14,16
// lua nodemcu  D: 3, 4, 2, 1, 8, 7, 6, 5, 0

// GPIO pin assignment (e.g. 15 is GPIO15)
// Try to avoid connecting JTAG to GPIO 0, 2, 15, (and 16 on some boards), as board may not boot

// bare ESP-07 or ESP-12
enum { TDO=14, TDI=16, TCK=12, TMS=13, TRST=4, SRST=5, LED=15 };

// nodemcu devikit
// enum { TDO=12, TDI=14, TCK=4, TMS=5, TRST=0, SRST=2, LED=16 };

// led logic
#define LED_ON LOW
#define LED_OFF HIGH

uint8_t jtag_state;

// activate JTAG outputs
void jtag_on(void)
{
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
  pinMode(TCK, INPUT);
  pinMode(TDO, INPUT);
  pinMode(TDI, INPUT);
  pinMode(TMS, INPUT);
  pinMode(TRST, INPUT);
  pinMode(SRST, INPUT);
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
  #ifdef ESP8266
  WiFi.mode(WIFI_AP);
  #endif
  Serial.begin(230400);
}

void loop() {
      static char c;
      // loop infinitely until incoming data are available
      if(Serial.available())
      {
        c = Serial.read();
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
            // TODO: need speedup:
            // WIFI TCP seems very slow switching from rx to tx
            // should try some output buffering
            Serial.write('0'+jtag_read());
            break;
          case 'r':
          case 's':
          case 't':
          case 'u':
            jtag_reset((c-'r') & 3);
            break;
          case 'B':
            digitalWrite(LED, LED_ON);
            if(jtag_state == 0)
              jtag_on();
            break;
          case 'b':
            digitalWrite(LED, LED_OFF);
            if(jtag_state == 0)
              jtag_on();
            break;
          case 'Q':
            jtag_off();
            break;
        }
      }
}
