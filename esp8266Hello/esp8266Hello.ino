#include <LiquidCrystal_I2C.h>
#include <LcdBarGraphX.h>

byte lcdNumCols = 16; // -- number of columns in the LCD

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);   // -- creating LCD instance

LcdBarGraphX lbg(&lcd, lcdNumCols, 0, 1); // -- Second line at column 0

byte i = 0;

void setup() {

  // -- initializing the LCD  
  lcd.begin(0, 2, 2, lcdNumCols);
  lcd.clear();
  lcd.print("G'Day");
  // -- do some delay: some time I've got broken visualization
  delay(100);

}

void loop()
{
  // -- draw bar graph from the analog value readed
  lbg.drawValue( i, 255);
  // -- do some delay: frequent draw may cause broken visualization
  delay(100);

  i += 5;
   Serial.println("blink");
}
