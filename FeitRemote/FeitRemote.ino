
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

// RF settings
// inverse polarity
#define zero HIGH
#define one LOW

const int N_REPEATS = 30;
const int TRANSMITTER_PIN = 2;
const int PULSE1_DURATION = 400;
const int PULSE0_DURATION = 80;

const char *mqttServer = "homeassistant";
const int mqttPort = 1883;

const int reportingInterval = 30; //report state every xx seconds

// MQTT
long lastMsg = 0;
byte state = 0;

char msg[50];
const char *ssid = "QBF";   // your network SSID (name)
const char *pass = "!QbfReward00";   // your network password
const char *mqttUser = "boris";
const char *mqttPassword = "HAReward00";
char *family =  "rf-remote";
String outTopic(family);
String inTopic(family);


// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
PubSubClient mqtt(client);

void setup() {
  Serial.begin(9600);
  pinMode(TRANSMITTER_PIN, OUTPUT);
  digitalWrite(TRANSMITTER_PIN, zero);
  setupWiFiMQTTClient();
  EEPROM.begin(4);
  state = EEPROM.read(0);
  transmitCommand(state);
}


void loop() {
  mqtt.loop();
  long now = millis();
  if (now - lastMsg > reportingInterval * 1000) {
    lastMsg = now;
    snprintf (msg, 50, "current state %s", state2String(state));
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqtt.publish(outTopic.c_str(), msg);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char* command = new char[length + 1];
  for (int i = 0; i < length; i++) {
    command[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }

  Serial.println();
  command[length] = '\0';

  byte tempState = decodeCommand(command);
  transmitCommand(tempState);
  if (tempState != state)
  {
    state = tempState;
    EEPROM.write(0, state);
    EEPROM.commit();
  }
}

byte decodeCommand(const char* command) {
  if (strcasecmp(command, "On") == 0)
    return 1;
  else if (strcasecmp(command, "Off") == 0)
    return 2;
  else if (strcasecmp(command, "Minus") == 0)
    return 3;
  else if (strcasecmp(command, "Plus") == 0)
    return 4;
  else if (strcasecmp(command, "Up") == 0)
    return 5;
  else if (strcasecmp(command, "Down") == 0)
    return 6;
  else if (strcasecmp(command, "Pattern") == 0)
    return 7;
  else if (strcasecmp(command, "Red") == 0)
    return 8;
  else if (strcasecmp(command, "Green") == 0)
    return 9;
  else if (strcasecmp(command, "Blue") == 0)
    return 10;
  else if (strcasecmp(command, "White") == 0)
    return 11;
  else return 0;
}

const char* state2String(int state) {
  switch (state)
  {
    case 0:
      return "Empty";
    case 1:
      return "On";
      break;
    case 2:
      return "Off";
      break;
    case 3:
      return "Minus";
      break;
    case 4:
      return "Plus";
      break;
    case 5:
      return "Up";
      break;
    case 6:
      return "Down";
      break;
    case 7:
      return "Pattern";
      break;
    case 8:
      return "Red";
      break;
    case 9:
      return "Green";
      break;
    case 10:
      return "Blue";
      break;
    case 11:
      return "White";
      break;
defult:
      break;
  }
}

void transmitCommand(int state) {
  switch (state)
  {
    case 1:
      transmit32bitRepeated(0xFBDEEFF4, N_REPEATS); // On
      break;
    case 2:
      transmit32bitRepeated(0xFBDEEFEC, N_REPEATS); // Off
      break;
    case 3:
      transmit32bitRepeated(0xFBDEEFDC, N_REPEATS); // Minus
      break;
    case 4:
      transmit32bitRepeated(0xFBDEEFBC, N_REPEATS); // Plus
      break;
    case 5:
      transmit32bitRepeated(0xFBDEEFEA, N_REPEATS); // Up
      break;
    case 6:
      transmit32bitRepeated(0xFBDEEFD5, N_REPEATS); // Down
      break;
    case 7:
      transmit32bitRepeated(0xFBDEEFD6, N_REPEATS); // Pattern
      break;
    case 8:
      transmit32bitRepeated(0xFBDEEFF4, N_REPEATS); // On
      delay(1000);
      transmit32bitRepeated(0xFBDEEFDA, N_REPEATS); // Red
      break;
    case 9:
      transmit32bitRepeated(0xFBDEEFF4, N_REPEATS); // On
      delay(1000);
      transmit32bitRepeated(0xFBDEEFAE, N_REPEATS); // Green
      break;
    case 10:
      transmit32bitRepeated(0xFBDEEFF4, N_REPEATS); // On
      delay(1000);
      transmit32bitRepeated(0xFBDEEFB5, N_REPEATS); // Blue
      break;
    case 11:
      transmit32bitRepeated(0xFBDEEFF4, N_REPEATS); // On
      delay(1000);
      transmit32bitRepeated(0xFBDEEFBA, N_REPEATS); // White
      break;
defult:
      transmit32bitRepeated(0xFBDEEFEC, N_REPEATS); // Off
      break;
  }
  snprintf (msg, 50, "current state %s", state2String(state));
  mqtt.publish(outTopic.c_str(), msg);
}

void transmitOneBit(bool v) {
  if (v)
  {
    digitalWrite(TRANSMITTER_PIN, one);
    delayMicroseconds(PULSE1_DURATION);
  }
  else
  {
    digitalWrite(TRANSMITTER_PIN, zero);
    delayMicroseconds(PULSE1_DURATION);
  }
  digitalWrite(TRANSMITTER_PIN, zero);
  delayMicroseconds(PULSE0_DURATION);
}


// Transmits 32 bit int value
void transmit32bit(long v) {
  digitalWrite(TRANSMITTER_PIN, zero);
  unsigned long mask = 0x80000000;
  for (int i = 0; i < 32; i++)
  {
    bool b = (mask & v) != 0;
    transmitOneBit(b);
    mask = mask >> 1;
  }
}

void transmit32bitRepeated(long v, int nRepeats) {
  for (int i = 0; i < nRepeats; i++)
  {
    transmit32bit(v);
    delay(4);
  }
}


void   setupWiFiMQTTClient() {
  outTopic += "/sideyard/state";
  inTopic += "/sideyard/command";

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("*");
  }

  Serial.println("WiFi connection Successful");
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.println(WiFi.localIP());// Print the IP address
  // Now we can publish stuff!
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);

  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT Server...");
    String clientId(family);
    clientId += String(" - ");
    clientId += String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed with state ");
      Serial.println(mqtt.state());
    }
  }
  mqtt.publish(outTopic.c_str(), "I am alive");

  mqtt.subscribe(inTopic.c_str());
}
