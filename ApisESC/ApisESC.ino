
#include <Servo.h>

byte servoPinL = 9; // signal pin for the ESC.
byte servoPinR = 8; // signal pin for the ESC.
byte controlPin = 7;
unsigned long lastUpdate = 0;
Servo servoL;
Servo servoR;

int speed = 1950;

void setup()
{
  pinMode(7, INPUT);
  delay(7000);// wait untill ESCs boot up
  servoL.attach(servoPinL);
  servoR.attach(servoPinR);
  servoL.writeMicroseconds(1500); // send "stop" signal to ESC. Also necessary to arm the ESC.
  servoR.writeMicroseconds(1500);
  delay(7000); // delay to allow the ESC to recognize the stopped signal.
}

void loop()
{
  int controlVal = digitalRead(controlPin); // read input from potentiometer.

  if (controlVal == LOW)
  {
      servoL.writeMicroseconds(speed); // Send signal to ESC.
      servoR.writeMicroseconds(speed); 
  }
  else
  {
    servoL.writeMicroseconds(1500);
    servoR.writeMicroseconds(1500);
  }
}
