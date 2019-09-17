#include <Wire.h>

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(57600);  // start serial for output
}

double average = 0;
int N = 3000;

int read_output(int addr) {
  Wire.requestFrom(addr, 2);    // request 2 bytes from piezo
  int piezo_out = 0;
  byte b = Wire.read();
  piezo_out |= (b << 8);
  b = Wire.read();
  piezo_out |= b;
  return piezo_out;
}

double approxRollingAverage (double avg, double new_sample) {

  avg -= avg / N;
  avg += new_sample / N;
  return avg;
}

void loop() {
int piezo_out=read_output(0x50);
  if (piezo_out > 0) {
  average = approxRollingAverage(average, piezo_out);
  Serial.print(piezo_out);
  }

  delay(1);
}
