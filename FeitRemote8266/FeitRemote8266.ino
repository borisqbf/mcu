#include <arduino.h>
#define TRANSMITTER_PIN 2 //GPIO2 of ESP8266
#define zero LOW
#define one HIGH
#define N_REPEATS 10
#define PULSE1_DURATION 350
#define PULSE0_DURATION 140

void setup() {
  pinMode(TRANSMITTER_PIN, OUTPUT);
   //transmit32bitRepeated(0xFBDEEFEC, N_REPEATS); // Off
}

void loop() {


   transmit32bitRepeated(0xFBDEEFEC, N_REPEATS); // Off
}

void transmitOneBit(bool v)
{
  if (v)
  {
    digitalWrite(TRANSMITTER_PIN, one);
    delayMicroseconds(PULSE1_DURATION);
  }
  else
  {
    digitalWrite(TRANSMITTER_PIN, zero);
    delayMicroseconds(PULSE0_DURATION);
  }
  digitalWrite(TRANSMITTER_PIN, zero);
  delayMicroseconds(PULSE0_DURATION);
}

// Transmits 32 bit int value
void transmit32bit(long v)
{
  digitalWrite(TRANSMITTER_PIN, zero);
  unsigned long mask = 0x80000000;
  for (int i = 0; i < 32; i++)
  {
    bool b = (mask & v) != 0;
    transmitOneBit(b);
    mask = mask >> 1;
  }
}

void transmit32bitRepeated(long v, int nRepeats)
{
  for (int i = 0; i < nRepeats; i++)
  {
    transmit32bit(v);
    delayMicroseconds(5000);
  }
}
