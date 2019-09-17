int solenoidPin = 9;                    //This is the output pin on the Arduino

void setup() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(solenoidPin, OUTPUT);          //Sets that pin as an output
}

void loop() 
{
  digitalWrite(solenoidPin, HIGH);      //Switch Solenoid ON
  digitalWrite(LED_BUILTIN, HIGH);      //Switch Solenoid ON
  delay(20);                           //Wait 0.1 Second
  digitalWrite(solenoidPin, LOW);       //Switch Solenoid OFF
  digitalWrite(LED_BUILTIN, LOW);       //Switch Solenoid OFF
  delay(200);                          //Wait 1 Second
}
