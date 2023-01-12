#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LcdBarGraphX.h>


    // WI-FI settings
    const int led = 13;
    byte lcdNumCols = 16;                                          // -- number of columns in the LCD
    LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // -- creating LCD instance
    LcdBarGraphX lbg(&lcd, 16, 0, 1);

    const int pollingPeriodicity = 7 * 1000; // report state every xx seconds
    unsigned long lastTimePolled = 0;

    const char *ssid = "QBF";          // your network SSID (name)
    const char *pass = "!QbfReward00"; // your network password
    // Create an ESP8266 WiFiClient class to connect to the MQTT server.
    WiFiClient client;
    ESP8266WebServer server(80);

    double waterLevelPercentage = 0.00;

    void SetupWiFiClient()
    {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass);
      while (WiFi.status() != WL_CONNECTED)
      {
        Serial.print("*");
      }

      Serial.println("WiFi connection Successful");
      Serial.print("The IP Address of ESP8266 Module is: ");
      Serial.println(WiFi.localIP()); // Print the IP address
      if (MDNS.begin("rain-tank-level"))
      { // Start mDNS

        Serial.println("MDNS started");
      }

      server.on("/", HandleRoot);        // Associate handler function to path
      server.on("/api/level", GetLevel); // Associate handler function to path
      server.onNotFound(HandleNotFound);

      server.begin(); // Start server
      Serial.println("HTTP server started");
}

void HandleRoot()
{
  digitalWrite(led, 1);
  server.send(200, "text/plain", "Hello resolved by mDNS !");
  digitalWrite(led, 0);
}

void HandleNotFound()
{
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void GetLevel()
{
  digitalWrite(led, 1);
  server.send(200, "text/plain", String(waterLevelPercentage, 2).c_str());
  digitalWrite(led, 0);
}

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  SetupWiFiClient();
  lcd.begin(0, 2, 2, lcdNumCols);
  lcd.clear();
  lcd.backlight(); // open the backlight
  delay(100);
}

void loop() 
{
  server.handleClient();
  MDNS.update();
  if ((millis() - lastTimePolled) > pollingPeriodicity)
  {
    lastTimePolled = millis();
    waterLevelPercentage +=1;
    if (waterLevelPercentage > 98)
    {
      waterLevelPercentage = 0;  
    }
    int pc = waterLevelPercentage * 100;
    lbg.drawValue(pc, 10000);

    lcd.setCursor(0, 0); 
    String val("Level: ");
    val += String(waterLevelPercentage, 2);
    val += "%";
    lcd.print(val);
    delay(100);
  }
}
