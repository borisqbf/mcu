
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SSD1306_I2C_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



void setup() {
  Wire.begin(0, 2);
  Serial.begin(9600);
  delay (2000);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, false)) {
    Serial.println("SSD1306 allocation failed");
  }
  else
  {
    Serial.println(F("SSD1306 allocation OK"));
  }
}
int i = 15;
float pc = 0;

void loop() {
  char buffer[3];
  buffer[2] = '\0';
  itoa(i++, buffer, 10);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Qbf");
  display.setCursor(92, 0);
  display.setTextSize(1);

  display.print("-");
  display.print(buffer);
  display.print("dBm");
  pc += 100.0 / 85.0 * 1.28;
  Serial.println((int)pc);
  display.fillRect(0, 18, (int)pc, 10, WHITE);

  display.display();
  if (i == 100) {
    i = 15;
    pc = 0;
  }

  delay (500);

}
