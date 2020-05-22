
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

const char *mqttServer = "homeassistant";
const int mqttPort = 1883;
const int relayPin = 0;
const int reportingInterval = 30; //report state every xx seconds


long lastMsg = 0;
byte state = 0;

char msg[50];
const char *ssid = "QBF";   // your network SSID (name)
const char *pass = "!QbfReward00";   // your network password
const char *mqttUser = "boris";
const char *mqttPassword = "HAReward00";
char *family = "wifi-relay";
String outTopic(family);
String inTopic(family);


// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
PubSubClient mqtt(client);


void setup() {
  pinMode(relayPin, OUTPUT);
  EEPROM.begin(4);
  state = EEPROM.read(0);
  digitalWrite(relayPin, state ? LOW : HIGH);

  outTopic += "/frontyard/state";
  inTopic += "/frontyard/command";
  // Connect to WiFi
  Serial.begin(74880);
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

void loop() {
  mqtt.loop();
  long now = millis();
  if (now - lastMsg > reportingInterval * 1000) {
    lastMsg = now;
    snprintf (msg, 50, "current state %s", state ? "On" : "Off");
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqtt.publish(outTopic.c_str(), msg);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  byte tempState = (length >= 2) && (((char)payload[0] == 'O') || ((char)payload[0] == 'o')) && ((char)payload[1] == 'n')? 1 : 0;
  if (tempState != state)
  {
    state = tempState;
    EEPROM.write(0, state);
    EEPROM.commit();
  }
 
  digitalWrite(relayPin, state ? LOW : HIGH);
}
