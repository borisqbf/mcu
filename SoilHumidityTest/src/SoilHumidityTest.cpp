#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>


#define humidityInputPin 36
#define humidityPowerPin 26


LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
  Serial.begin(115200);
  pinMode(humidityInputPin, INPUT);
  delay(500);

    lcd.init();
    lcd.clear();
    lcd.backlight();
    lcd.home();
}


int GetHumidityImp()
{
    digitalWrite(humidityPowerPin, HIGH);
    delay (10000);
    int h = 0;

    for (int i = 0; i < 100; i++)
    {
        h += analogRead(humidityInputPin);
        delay(10);
    }
    return h / 100;
    digitalWrite(humidityPowerPin, LOW);
}
void loop()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(GetHumidityImp());
    delay(30000);
}
