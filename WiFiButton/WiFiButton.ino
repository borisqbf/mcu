#include <ESP8266WiFi.h>

char ssid[] = "QBF";   // your network SSID (name)
char pass[] = "!QbfReward00";   // your network password
void setup() {
 // Connect to WiFi
  Serial.begin(74880);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
     delay(500);
     Serial.print("*");
  }
  
  Serial.println("");
  Serial.println("WiFi connection Successful");
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.print(WiFi.localIP());// Print the IP address
  ESP.deepSleep(0);
}

void loop() {
  Serial.println("You should never see this");
}
