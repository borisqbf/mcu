#include <Wire.h>

#define REG_00H 0x00
#define REG_00L 0x00

const int TOF250address = 0x52;

unsigned int distance = 0;

void setup()
{
Wire.begin();
Serial.begin(115200);
delay(100);
}

void loop()
{
GetLidarDataFromI2C(TOF250address);
Serial.println("Distance " + String(distance) + " cm");
}

void GetLidarDataFromI2C(const int addr)
{
                byte buff[2];
                byte i = 0;

                Wire.beginTransmission(addr);
                Wire.write(REG_00H);
                Wire.endTransmission();
                Wire.requestFrom(addr, 2);
                
                 while(Wire.available())
                {
                                buff[i++] = Wire.read();
                                if (i >= 2)
                                {
                                                i = 0;
                                                distance = buff[0] * 256 + buff[1];
                                }
                }
}
