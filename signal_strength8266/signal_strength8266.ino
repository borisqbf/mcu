#include <LiquidCrystal_I2C.h>
#include <LcdBarGraphX.h>

byte lcdNumCols = 16; // -- number of columns in the LCD

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // -- creating LCD instance

LcdBarGraphX lbg(&lcd, lcdNumCols, 0, 1); // -- Bar graph is on the second line at column 0

#include "ESP8266WiFi.h"
#include "SSIDStrength.h"

SSIDStrength strongestWiFi;
SSIDStrength currentStrength;
bool initialScan;

void setup()
{
  initialScan = true;
  lcd.begin(0, 2, 2, lcdNumCols);
  lcd.clear();
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop()
{
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    lcd.print("No WiFi signal");
    lcd.setCursor(0, 0);
    return;
  }
  else
  {
    if (initialScan)
    {
      strongestWiFi.MaxFromCWLAP(n);
      initialScan = false;
      const int strength = strongestWiFi.getStrength();
      lcd.print(strongestWiFi.getSSID());
      lcd.setCursor(10, 0);
      lcd.print(strength);
      lcd.print("dBm");
    }
    else
    {
      currentStrength.Reset();
      currentStrength.MaxFromCWLAP(n, strongestWiFi.getSSID());
      const int strength = currentStrength.getStrength();

      lcd.setCursor(10, 0);
      if (strength > SSIDStrength::MinRssi)
      {
        lcd.print(strength);
        lbg.drawValue(SSIDStrength::CalculateSignalStrengsPercentage(strength), 100);
      }
      else if (strongestWiFi.getSSID() != UndefinedSSID)
      {
        lcd.print("---");
      }
    }
  }
  // Wait a bit before scanning again
  delay(2000);
}
