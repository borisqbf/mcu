#include <arduino.h>
#define TRANSMITTER_PIN 10
#define zero LOW
#define one HIGH
#define N_REPEATS 10000
#define PULSE1_DURATION 350
#define PULSE0_DURATION 140

void setup() {
  Serial.begin(9600);
  pinMode(TRANSMITTER_PIN, OUTPUT);
  digitalWrite(TRANSMITTER_PIN, zero);
}

void loop() {
  int choice = showMenu();

  switch (choice)
  {
    case 0:
      break;
    case 1:
      transmit32bitRepeated(0xFBDEEFF4, N_REPEATS); // On
      break;
    case 2:
      transmit32bitRepeated(0xFBDEEFEC, N_REPEATS); // Off
      break;
    case 3:
      transmit32bitRepeated(0xFBDEEFDC, N_REPEATS); // minus
      break;
    case 4:
      transmit32bitRepeated(0xFBDEEFBC, N_REPEATS); // Plus
      break;
    case 5:
      transmit32bitRepeated(0xFBDEEFEA, N_REPEATS); // Up
      break;
    case 6:
      transmit32bitRepeated(0xFBDEEFD5, N_REPEATS); // Down
      break;
    case 7:
      transmit32bitRepeated(0xFBDEEFD6, N_REPEATS); //pattern
      break;
    case 8:
      transmit32bitRepeated(0xFBDEEFDA, N_REPEATS); //red
      break;
    case 9:
      transmit32bitRepeated(0xFBDEEFAE, N_REPEATS); //green
      break;
    case 10:
      transmit32bitRepeated(0xFBDEEFB5, N_REPEATS); //blue
      break;
    case 11:
      transmit32bitRepeated(0xFBDEEFBA, N_REPEATS); //white
      break;
defult:
      break;
  }
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

int showMenu()
{
  Serial.println("Please enter command:");
  Serial.println("1 On");
  Serial.println("2 Off");
  Serial.println("3 -");
  Serial.println("4 +");
  Serial.println("5 Up");
  Serial.println("6 Down");
  Serial.println("7 Change Pattern");
  Serial.println("8 Red");
  Serial.println("9 Green");
  Serial.println("10 Blue");
  Serial.println("11 White");
  while (Serial.available() == 0) {
    /* do nothing */
  }
  while (Serial.available())
  {
    String ch = Serial.readString();// read the incoming data as string
    int selection = ch.toInt();
    if ((selection < 1) || (selection > 11))
    {
      Serial.println("Incorrect command: " + ch);
    }
    else
    {
      Serial.println("Executing command: " + ch);
      return selection;
    }
  }
}
