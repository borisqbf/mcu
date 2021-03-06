#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include "SSIDStrength.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin) (no difference)
#define SSD1306_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



SSIDStrength strongestWiFi;
SSIDStrength currentStrength;
bool initialScan;

void setup()
{
  initialScan = true;
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Wire.begin(0, 2);

  delay(2000);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, false);
 
//  delay(2000);
//  Send1306Command(0xE4);
  display.clearDisplay();
  display.display();
}

void loop()
{
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    printLeftTopCorner("No WiFi signal");
    return;
  }
  else
  {
      
    if (initialScan)
    {
      strongestWiFi.MaxFromCWLAP(n);
      initialScan = false;
      printSignalStrength(strongestWiFi.getStrength(), strongestWiFi.getSSID());
    }
    else
    {
      currentStrength.Reset();
      const String ssid = strongestWiFi.getSSID();
      currentStrength.MaxFromCWLAP(n, ssid);
      printSignalStrength(currentStrength.getStrength(), ssid);
    }
  }
  // Wait a bit before scanning again
  delay(2000);
}


void printLeftTopCorner(const char* str)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(str);
  display.display();
}

///
/// strenght between -15 and - 100
///
void printSignalStrength(int strength, String ssid)
{
  display.clearDisplay();
  int percentStrength = (strength < SSIDStrength::MinRssi) ? SSIDStrength::MinRssi : strength;
  percentStrength = (strength > SSIDStrength::MaxRssi) ? SSIDStrength::MaxRssi : strength;
  float pc = SSIDStrength::CalculateSignalStrengsPercentage(percentStrength);
  char buffer[3];
  buffer[2] = '\0';
  itoa(strength * -1, buffer, 10);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  if (ssid == UndefinedSSID)
    display.println("---");
  else 
    display.println(ssid);
  display.setCursor(92, 0);
  display.setTextSize(1);

  display.print("-");
  display.print(buffer);
  display.print("dBm");
  if (ssid != UndefinedSSID)
    display.fillRect(0, 18, pc * 1.28, 10, WHITE);

  display.display();
}

void Send1306Command(int command)
{
  Wire.beginTransmission(SSD1306_I2C_ADDRESS);  // START I2C COMMUNICATION WITH OLED (0x3c=OLED ADDRESS)
  Wire.write(0x80);                             // WRITE I2C COMMAND WORD TO OLED
  Wire.write(command);                        
  Wire.endTransmission();                       // STOP I2C COMMUNICATION
  delay(500);
}
