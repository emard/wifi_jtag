/*
 *  Arduino OpenOCD remote_bitbang WIFI-JTAG server for ESP8266
 */

#include <ESP8266WiFi.h>

// cross reference gpio to nodemcu lua pin numbering
// esp gpio:     0, 2,15,13,12,14,16
// lua nodemcu:  3, 4, 8, 7, 6, 5, 0

// GPIO pin assignment (e.g. 15 is GPIO15)
enum { TDO=14, TDI=2, TCK=12, TMS=13, TRST=0, SRST=16, LED=15 };

const char* ssid = "ssid";
const char* password = "password";

// specify TCP port to listen on as an argument
WiFiServer server(3335);

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
  Serial.begin(115200);
  delay(10);

  // set all GPIO as INPUT
  jtag_off();
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
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
            // TODO: need speedup:
            // WIFI TCP seems very slow switching from rx to tx
            // should try some output buffering
            client.write('0'+jtag_read());
            break;
          case 'r':
          case 's':
          case 't':
          case 'u':
            jtag_reset((c-'r') & 3);
            break;
          case 'B':
            digitalWrite(LED, HIGH);
            break;
          case 'b':
            digitalWrite(LED, LOW);
            break;
          case 'Q':
            client.stop(); // disconnect client's TCP
            break;
        }
      }
      // yield();
    }
    jtag_off();
    delay(1);
    // Serial.println("client disonnected");
  }
}
