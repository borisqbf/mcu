
#include <Servo.h>

byte servoPin = 9; // signal pin for the ESC.
byte controlPin = 7;
Servo servo;

void setup() {
pinMode (7, INPUT);
servo.attach(servoPin);
servo.writeMicroseconds(1500); // send "stop" signal to ESC. Also necessary to arm the ESC.

delay(7000); // delay to allow the ESC to recognize the stopped signal.
}

void loop() {

int controlVal = digitalRead(controlPin); // read input from potentiometer.

if (controlVal == LOW)
  servo.writeMicroseconds(1800); // Send signal to ESC.
  else
  servo.writeMicroseconds(1500); 
}
