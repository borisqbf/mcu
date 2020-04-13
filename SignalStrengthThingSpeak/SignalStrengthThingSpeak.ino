#include "ThingSpeak.h"
#include <ESP8266WiFi.h>

#define ets_wdt_disable ((void (*)(void))0x400030f0)
#define ets_delay_us ((void (*)(int))0x40002ecc)

#define _R (uint32_t *)0x60000700

unsigned long myChannelNumber = 1036102;
const char * myWriteAPIKey = "QF3O2AHAVC09JUSZ";


char ssid[] = "QBF";   // your network SSID (name)
char pass[] = "!QbfReward00";   // your network password


static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D0   = 16;

enum LedColour {
  RED,
  GREEN,
  BLUE
};


void nk_deep_sleep(uint64_t time)
{
  ets_wdt_disable();
  *(_R + 4) = 0;
  *(_R + 17) = 4;
  *(_R + 1) = *(_R + 7) + 5;
  *(_R + 6) = 8;
  *(_R + 2) = 1 << 20;
  ets_delay_us(10);
  *(_R + 39) = 0x11;
  *(_R + 40) = 3;
  *(_R) &= 0xFCF;
  *(_R + 1) = *(_R + 7) + (45 * (time >> 8));
  *(_R + 16) = 0x7F;
  *(_R + 2) = 1 << 20;
  __asm volatile ("waiti 0");
}


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
  reportSignalStrength();
  nk_deep_sleep(300e6);
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
  long rssi = averageSignalStrength();

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
    delay(100);
    digitalWrite(pin, HIGH);
    delay(500);
  }
}

long averageSignalStrength() {
  long retVal = 0;
  for (int i = 0; i < 10; i++) {
    retVal += WiFi.RSSI();
    reportStatus(BLUE, 1);
    delay(1000);
  }
  return retVal / 10;
}
