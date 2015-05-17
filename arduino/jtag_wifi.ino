/*
 *  Arduino OpenOCD remote_bitbang WIFI-JTAG server for ESP8266
 *  LICENSE=GPL
 *
 *  OpenOCD interface:
 *
 *  interface remote_bitbang
 *  remote_bitbang_host localhost
 *  remote_bitbang_port 3335
 */

#include <ESP8266WiFi.h>

// cross reference gpio to nodemcu lua pin numbering
// esp gpio:     0, 2,15,13,12,14,16
// lua nodemcu:  3, 4, 8, 7, 6, 5, 0

// GPIO pin assignment
// Try to avoid connecting JTAG to GPIO 0, 2, 15, 16 (board may not boot)
enum { TDO=12, TDI=13, TCK=14, TMS=5, TRST=0, SRST=2, LED=16 };

// led logic
#define LED_ON LOW
#define LED_OFF HIGH

const char* ssid = "ssid";
const char* password = "password";

// specify TCP port to listen on as an argument
WiFiServer server(3335);
WiFiClient client;

#define WATCHDOG 0 // disabled because ESP watchdog is not working

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
  delay(10);

  pinMode(LED, OUTPUT);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    #if WATCHDOG
    ESP.wdtFeed();
    #endif
    digitalWrite(LED, LED_ON);
    delay(10);
    digitalWrite(LED, LED_OFF);
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  //server.setNoDelay(true);
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  pinMode(LED, INPUT);
}

void loop() {
  // Check if a client has connected
  #if WATCHDOG
  ESP.wdtFeed();
  #endif
  client = server.available();
  if(client)
  {
    // Serial.println("new client");
    jtag_on();
    while(client.connected())
    {
      // loop infinitely until incoming data are available
      if(client.available())
      {
        char c = client.read();
        // Serial.write(c); 
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
          case 'Q':
            client.flush();
            client.stop(); // disconnect client's TCP
            break;
        }
      }
      #if WATCHDOG
      ESP.wdtFeed();
      #endif
      yield();
    }
    jtag_off();
    client.flush();
    client.stop();
    // Serial.println("client disonnected");
  }
}
