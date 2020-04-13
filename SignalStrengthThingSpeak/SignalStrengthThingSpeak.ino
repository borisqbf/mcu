
#include "ThingSpeak.h"

unsigned long myChannelNumber = 1036102;
const char * myWriteAPIKey = "QF3O2AHAVC09JUSZ";

#include <ESP8266WiFi.h>

static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;

enum LedColour {
  RED,
  GREEN,
  BLUE
};

char ssid[] = "QBF";   // your network SSID (name)
char pass[] = "!QbfReward00";   // your network password

WiFiClient  client;

void setup() {
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  delay(100);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
}

void reportSignalStrength() {
  // Connect or reconnect to WiFi
  reportStatus(BLUE, 3);
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network

      delay(5000);
    }
  }
  reportStatus(GREEN, 3);

  // Measure Signal Strength (RSSI) of Wi-Fi connection
  long rssi = WiFi.RSSI();

  // Write value to Field 1 of a ThingSpeak Channel
  int httpCode = 0;
  int retryCount = 0;
  reportStatus(BLUE, 1);

  do {
    httpCode = ThingSpeak.writeField(myChannelNumber, 1, rssi, myWriteAPIKey);

    if (httpCode != 200) {
      reportStatus(RED, 1);
      retryCount++;
      if (retryCount > 10) {
        reportStatus(RED, 5);
        break;
      }
    } else {
      reportStatus(GREEN, 1);
    }

    delay (30 * 1000);

  } while (httpCode != 200);
}
void loop() {
  reportSignalStrength();
  // Wait 30 minutes to update the channel again
  delay(1000 *  30);
}

void reportStatus(LedColour c, int n) {
  uint8_t pin;
  if (c == GREEN)
    pin = D6;
  else if (c == RED)
    pin = D5;
  else
    pin = D7;

  for (int i = 0; i < n; i++) {
    digitalWrite(pin, LOW);
    delay(300);
    digitalWrite(pin, HIGH);
    delay(500);
  }
}
