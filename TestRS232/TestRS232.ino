void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
Serial.println("Hello");
delay(1000);
String echo = Serial.readString();
Serial.print("Received ");
Serial.println(echo);
}
