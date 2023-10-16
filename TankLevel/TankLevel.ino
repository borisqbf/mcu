#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LcdBarGraphX.h>

#define REG_00H 0x0

const int led = 13;

const int maxWaterColumnHeight = 215; // cm
const int bottomDeadSpace = 20;       // cm
const int topDeadSpace = 6;          // cm

const int TOF250address = 0x52;
const int LCDaddress = 0x27;
byte lcdNumCols = 16;

double distance = maxWaterColumnHeight;

// -- number of columns in the LCD
LiquidCrystal_I2C lcd(LCDaddress, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // -- creating LCD instance
LcdBarGraphX lbg(&lcd, 16, 0, 1);

const int pollingPeriodicity = 3 * 1000; // report state every xx seconds
unsigned long lastTimePolled = 0;

const char *ssid = "QBF";          // your network SSID (name)
const char *pass = "!QbfReward00"; // your network password
const char *absLevelURL = "/api/level-abs";
const char *pcLevelURL = "/api/level-pc";


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

  server.on("/", HandleRoot);                // Associate handler function to path
  server.on(pcLevelURL, GetLevelPercentage); // Associate handler function to path
  server.on(absLevelURL, GetLevelAbsolute);  // Associate handler function to path
  server.onNotFound(HandleNotFound);

  server.begin(); // Start server
  Serial.println("HTTP server started");
}

void HandleRoot()
{
  digitalWrite(led, 1);

  String message = "Hello resolved by mDNS!\n\n";
  message += "Available routes are ";
  message += pcLevelURL;
  message += " and ";
  message += absLevelURL;

  server.send(200, "text/plain", message);
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
  message += "\nAvailable routes are ";
  message += pcLevelURL;
  message += " and ";
  message += absLevelURL;

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void GetLevelPercentage()
{
  digitalWrite(led, 1);
  server.send(200, "text/plain", String(waterLevelPercentage, 2).c_str());
  digitalWrite(led, 0);
}

void GetLevelAbsolute()
{
  digitalWrite(led, 1);
  server.send(200, "text/plain", String(distance, 2).c_str());
  digitalWrite(led, 0);
}

double GetLidarDataFromI2C(const int addr)
{
  byte buff[2];
  byte i = 0;
  double d; // current measurement

  Wire.beginTransmission(addr);
  Wire.write(REG_00H);
  Wire.endTransmission();
  Wire.requestFrom(addr, 2);

  while (Wire.available())
  {
    buff[i++] = Wire.read();
    if (i >= 2)
    {
      i = 0;
      d = buff[0] * 256 + buff[1];
      if (d > maxWaterColumnHeight)
        d = maxWaterColumnHeight;
    }
  }
  return d;
}

void CalculatePercentage()
{
  double distanceFromSensorToBottom = maxWaterColumnHeight - distance;
  double usableWaterLevel = distanceFromSensorToBottom - bottomDeadSpace;
  if (usableWaterLevel < 0)
    usableWaterLevel = 0;

  waterLevelPercentage = (usableWaterLevel / (maxWaterColumnHeight - topDeadSpace - bottomDeadSpace)) * 100;

  if (waterLevelPercentage > 100)
  {
    waterLevelPercentage = 100;
  }
  if (waterLevelPercentage < 0)
  {
    waterLevelPercentage = 0;
  }
}

bool IsGoodSample(double sample)
{
  if (sample <= (maxWaterColumnHeight - bottomDeadSpace))
    return true; // when just started any sample is a good one
  else
    return abs(distance - sample) < distance * 0.2; // 20%; Can't use stddev as the distribution is not guaranteed to be normal. E.g. empty tank would have stdev of 0.00
}

double CalculateDistance(double sample)
{
  return (sample + distance) / 2.0;
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
    double currentMeasurement = GetLidarDataFromI2C(TOF250address);
    if (IsGoodSample(currentMeasurement))
    {
      distance = CalculateDistance(currentMeasurement);
    }
    CalculatePercentage();
    int pc = waterLevelPercentage * 100;
    lbg.drawValue(pc, 10000);

    lcd.setCursor(0, 0);
    String val("Level: ");
    val += String(waterLevelPercentage, 2);
    val += "%  ";
    lcd.print(val);
    delay(100);
  }
}
