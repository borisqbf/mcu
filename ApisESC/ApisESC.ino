
#include <Servo.h>

byte servoPinL = 9; // signal pin for the ESC.
byte servoPinR = 8; // signal pin for the ESC.
byte controlPin = 7;
Servo servoL;
Servo servoR;

void setup()
{
  pinMode(7, INPUT);
  servoL.attach(servoPinL);
  servoR.attach(servoPinR);
  servoL.writeMicroseconds(1500); // send "stop" signal to ESC. Also necessary to arm the ESC.
  servoR.writeMicroseconds(1500);
  .

      delay(7000); // delay to allow the ESC to recognize the stopped signal.
}

void loop()
{

  int controlVal = digitalRead(controlPin); // read input from potentiometer.

  if (controlVal == LOW)
  {
    servoL.writeMicroseconds(2000); // Send signal to ESC.
    servoR.writeMicroseconds(2000); 
  }
  else
  {
    servoL.writeMicroseconds(1500);
    servoR.writeMicroseconds(1500);
  }
}
