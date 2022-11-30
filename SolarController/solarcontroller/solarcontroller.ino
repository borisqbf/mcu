#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#pragma region general definitions
const uint8_t TOGGLE_PIN = 14;
const uint8_t MANUAL_PIN = 16;
const uint8_t MODE_PIN = 12;
const uint8_t RELAY_PIN = 13;
const uint8_t RED_LED_PIN = 15;
const uint8_t BLUE_LED_PIN = 2;
const uint8_t GREEN_LED_PIN = 0;

enum LedColour : byte
{
  RED,
  BLUE,
  GREEN
};

uint8_t mode = 0;
unsigned long lastPressed = 0;
#pragma endregion

#pragma region WiFi
// WI-FI settings
char msg[50];
const char *ssid = "QBF";          // your network SSID (name)
const char *pass = "!QbfReward00"; // your network password
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
#pragma endregion

#pragma region MQTT

const char *mqttServer = "299d6fc93f0945089400ce1c143e1ebb.s2.eu.hivemq.cloud";
const int mqttPort = 8883;

const int reportingInterval = 5 * 60; // report state every xx seconds

// MQTT Settings
const char *mqttUser = "boris_qbf";
const char *mqttPassword = "HiveReward00";
const char *family = "solar";
String outTopic(family);
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
PubSubClient mqtt(client);

#pragma endregion

#pragma region Error Codes and Statuses

const uint8_t wifiConnecting = 3;
const uint8_t wifiConnected = 0;
const uint8_t wifiError = 1;

const uint8_t mqttError = 3;

#pragma endregion

void Blink(LedColour c)
{
  uint8_t pin = RED_LED_PIN;
  if (c == BLUE)
    pin = BLUE_LED_PIN;
  else if (c == GREEN)
    pin = GREEN_LED_PIN;

  digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
  delay(300);
  digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
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
    digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
  else
    digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
}

void ReportError(uint8_t errorCode)
{
  for (uint8_t i = 0; i < errorCode; i++)
  {
    Blink(RED);
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
  if (mode == 0)
  {
    mode = 1;
    digitalWrite(MODE_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
  }
  else
  {
    mode = 0;
    digitalWrite(MODE_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
  }
}

void ICACHE_RAM_ATTR IntCallback()
{
  if (digitalRead(MANUAL_PIN))
  {
    if (millis() - lastPressed > 300)
    {
      ToggleMode();
    }
    lastPressed = millis();
  }
}

bool connect2Mqtt()
{
  while (!mqtt.connected())
  {
    Serial.println("Connecting to MQTT Server...");
    String clientId(family);
    clientId += String(" - ");
    clientId += String(random(0xff), HEX);

    if (mqtt.connect(clientId.c_str(), mqttUser, mqttPassword))
    {
      Serial.println("Connected");
      mqtt.publish(outTopic.c_str(), "I am alive");
      return true;
    }
    else
    {
      Serial.print("Failed with state ");
      Serial.println(mqtt.state());
      ReportError(mqttError);
      return false;
    }
  }
}

void SetupWiFiMQTTClient()
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

  // IP address could have been change - update mqtt client
  mqtt.setServer(mqttServer, mqttPort);

  connect2Mqtt();
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
  digitalWrite(RED_LED_PIN, HIGH);

  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(TOGGLE_PIN), IntCallback, RISING);
  SetupWiFiMQTTClient();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    ReportWiFi(wifiError);
    SetupWiFiMQTTClient();
  }
}
