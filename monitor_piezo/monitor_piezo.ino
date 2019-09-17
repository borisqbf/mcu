#include <Wire.h>

int solenoidPin = 9;                    //This is the output pin on the Arduino

void setup()
{
  pinMode(solenoidPin, OUTPUT);          //Sets that pin as an output
  pinMode(LED_BUILTIN, OUTPUT);          //Sets that pin as an output
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(500000);  // start serial for output
}


int read_output(int addr) {
  Wire.requestFrom(addr, 2);    // request 2 bytes from piezo
  int piezo_out = 0;
  byte b = Wire.read();
  piezo_out |= (b << 8);
  b = Wire.read();
  piezo_out |= b;
  return piezo_out;
}

void loop()
{
  digitalWrite(solenoidPin, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(80);
  digitalWrite(solenoidPin, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  for (int y = 0; y < 100; y++) {
    int out2 = read_output(0x50);
    int out1 = read_output(0x5a);

    if (!(out1 <= 0 && out2 <= 0)) {
      Serial.print(out2);
      Serial.print(' ');
      Serial.println(out1);
    }
  }
  delay(500);
}
