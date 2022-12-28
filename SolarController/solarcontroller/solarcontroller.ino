#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>

#pragma region general definitions
const uint8_t TOGGLE_PIN = 14;
const uint8_t MANUAL_PIN = 16;
const uint8_t MODE_PIN = 12;
const uint8_t RELAY_PIN = 13;
const uint8_t RED_LED_PIN = 15;
const uint8_t BLUE_LED_PIN = 2;
const uint8_t GREEN_LED_PIN = 0;

const int pollingPeriodicity = 3 * 60 * 1000; // in milliseconds

unsigned long lastTimeTelemetrySent;

enum LedColour : byte
{
  RED,
  BLUE,
  GREEN
};
enum PowerMode : byte
{
    OnSolar = 0,
    OnMain = 1
};

PowerMode mode = OnSolar;
unsigned long lastPressed = 0;
#pragma endregion

#pragma region WiFi
// WI-FI settings
char msg[50];
const char *ssid = "QBF";          // your network SSID (name)
const char *pass = "!QbfReward00"; // your network password

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClientSecure client;
#pragma endregion

#pragma region MQTT

const char *mqttServer = "299d6fc93f0945089400ce1c143e1ebb.s2.eu.hivemq.cloud";
const int mqttPort = 8883;

const int reportingInterval = 5 * 60; // report state every xx seconds

// MQTT Settings
const char *mqttUser = "boris_qbf";
const char *mqttPassword = "mqttReward00";
const char *family = "solar";
String outTopic(family);

PubSubClient *mqtt;

#pragma endregion

#pragma region Error Codes and Statuses

const uint8_t wifiConnecting = 3;
const uint8_t wifiConnected = 0;
const uint8_t wifiError = 1;

const uint8_t mqttError = 3;
const uint8_t mqttSentOK = 5;

#pragma endregion

void Blink(LedColour c)
{
  uint8_t pin = RED_LED_PIN;
  if (c == BLUE)
    pin = BLUE_LED_PIN;
  else if (c == GREEN)
    pin = GREEN_LED_PIN;

  digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
  delay(300);
  digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
  delay(500);
}

void LEDOnOff(LedColour c, bool on = true)
{
  uint8_t pin = RED_LED_PIN;
  if (c == BLUE)
    pin = BLUE_LED_PIN;
  else if (c == GREEN)
    pin = GREEN_LED_PIN;
  if (on)
    digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
  else
    digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
}

void ReportError(uint8_t errorCode)
{
  for (uint8_t i = 0; i < errorCode; i++)
  {
    Blink(RED);
  }
  LEDOnOff(RED);
}

void ReportStatus(uint8_t status)
{
  for (uint8_t i = 0; i < status; i++)
  {
    Blink(GREEN);
  }
}

void ReportWiFi(uint8_t status)
{
  if (status == 0)
    LEDOnOff(BLUE);
  else if (status == 1)
    LEDOnOff(BLUE, false);
  else
  {
    for (uint8_t i = 0; i < status; i++)
    {
      Blink(BLUE);
    }
  }
}

void ToggleMode()
{ 
  if (mode == OnSolar)
  {
    mode = OnMain;
    digitalWrite(MODE_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
  }
  else
  {
    mode = OnSolar;
    digitalWrite(MODE_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
  }
}

void ICACHE_RAM_ATTR IntCallback()
{
  long int m =  millis();
  Serial.printf("Got interupt. Millis %ld\n",m);
  if (digitalRead(MANUAL_PIN))
  {
    if (millis() - lastPressed > 400)
    {
      ToggleMode();
    }
    lastPressed = millis();
  }
}

bool Connect2Mqtt()
{
  randomSeed(micros());
  SetDateTime();
  // IP address could have been change - update mqtt client

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0)
  {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return false; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  mqtt = new PubSubClient(*bear);

  mqtt->setServer(mqttServer, mqttPort);
  while (!mqtt->connected())
  {
    Serial.println("Connecting to MQTT Server...");
    String clientId(family);
    clientId += String(" - ");
    clientId += String(random(0xff), HEX);

    if (mqtt->connect(clientId.c_str(), mqttUser, mqttPassword))
    {
      Serial.println("Connected");
      mqtt->publish(outTopic.c_str(), "I am alive");
      return true;
    }
    else
    {
      Serial.print("Failed with state ");
      Serial.println(mqtt->state());
      ReportError(-1 * mqtt->state());
      return false;
    }
  }
}

void SetupWiFiClient()
{
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    ReportWiFi(wifiConnecting);
    Serial.print("*");
  }

  Serial.println("WiFi connection Successful");
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.println(WiFi.localIP()); // Print the IP address
  ReportWiFi(wifiConnected);
}

void InitLEDs()
{
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(MODE_PIN, HIGH); // Initial state is on solar so green LED is on
}

void SetDateTime()
{
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Australia_Melbourne, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}

void setup()
{
  Serial.begin(9600);
  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  pinMode(MANUAL_PIN, INPUT_PULLDOWN_16);
  pinMode(MODE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(RED_LED_PIN, OUTPUT);

  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  InitLEDs();
  attachInterrupt(digitalPinToInterrupt(TOGGLE_PIN), IntCallback, RISING);

  SetupWiFiClient();

  LittleFS.begin();
  Connect2Mqtt();
}

void loop()
{
  mqtt->loop();
  if (WiFi.status() != WL_CONNECTED)
  {
    ReportWiFi(wifiError);
    SetupWiFiClient();
  }

  if ((millis() - lastTimeTelemetrySent) > pollingPeriodicity)
  {
    mqtt->publish(outTopic.c_str(), "Periodic poll");
    lastTimeTelemetrySent = millis();
    Serial.println("Peiodic sent");
    ReportStatus(mqttSentOK);
  }
}
