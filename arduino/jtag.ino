/*
 *  Arduino OpenOCD remote_bitbang WIFI-JTAG server
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>

const char* ssid = "ssid";
const char* password = "password";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(3335);

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);
  
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
    // Wait until the client sends some data
    Serial.println("new client");
    while(client.connected())
    {
      if(client.available())
      {
        char c = client.read();
        // Serial.write(c); 
        client.write(c); // seems very slow
      }
      // yield();
    }
    delay(1);
    client.stop();
    Serial.println("client disonnected");
  }
  
}
