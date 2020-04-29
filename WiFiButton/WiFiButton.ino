
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


const char *mqttServer = "10.0.0.175";
const int mqttPort = 1883;


const char *ssid = "QBF";   // your network SSID (name)
const char *pass = "!QbfReward00";   // your network password
const char *mqttUser = "boris";
const char *mqttPassword = "HAReward00";

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
PubSubClient mqtt(client);


void setup() {
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
  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT Server...");
    String clientId = "WiFiButton-";
    clientId += String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed with state ");
      Serial.println(mqtt.state());
    }
  }

  Serial.print("Sending button pressed");
  String message = "Office-";
  message += millis();
  Serial.println(message);
 
  mqtt.publish("buttons/test", message.c_str());
  mqtt.loop();

  ESP.deepSleep(0);
}

void loop() {
  Serial.println("You should never see this");
}
