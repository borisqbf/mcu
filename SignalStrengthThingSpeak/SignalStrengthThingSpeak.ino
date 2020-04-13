
#include "ThingSpeak.h"

unsigned long myChannelNumber = 1036102;
const char * myWriteAPIKey = "QF3O2AHAVC09JUSZ";

#include <ESP8266WiFi.h>

char ssid[] = "QBF";   // your network SSID (name)
char pass[] = "!QbfReward00";   // your network password

WiFiClient  client;

void setup() {
  Serial.begin(115200);
  delay(100);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
}

void reportSignalStrength() {
  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  // Measure Signal Strength (RSSI) of Wi-Fi connection
  long rssi = WiFi.RSSI();

  // Write value to Field 1 of a ThingSpeak Channel
  int httpCode = 0;
  int retryCount = 0;
  do {
    httpCode = ThingSpeak.writeField(myChannelNumber, 1, rssi, myWriteAPIKey);

    if (httpCode != 200) {
      Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
      retryCount++;
      if (retryCount > 10) {
        Serial.println("Giving up retry atempts");
        break;
      }
    }
    delay (30 * 1000);

  } while (httpCode != 200);
}
void loop() {
  reportSignalStrength();
  // Wait 30 minutes to update the channel again
  delay(1000 * 60 * 30);
}
