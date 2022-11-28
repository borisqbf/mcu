uint8_t TOGGLE_PIN = 14;
uint8_t MANUAL_PIN = 16;
uint8_t MODE_PIN = 12;
uint8_t RELAY_PIN = 13;
uint8_t ERROR_PIN = 15;
uint8_t WIFI_PIN = 2;
uint8_t MODBUS_PIN = 0;

int mode = 0;
long int lastPressed = 0;

void ToggleMode()
{
  if (mode == 0)
  {
    mode = 1;
    digitalWrite(MODE_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);

    digitalWrite(ERROR_PIN, HIGH);
    digitalWrite(WIFI_PIN, LOW);
    digitalWrite(MODBUS_PIN, LOW);
  }
  else
  {
    mode = 0;
    digitalWrite(MODE_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);

    digitalWrite(ERROR_PIN, LOW);
    digitalWrite(WIFI_PIN, HIGH);
    digitalWrite(MODBUS_PIN, HIGH);
  }
}

void ICACHE_RAM_ATTR IntCallback()
{
  if (digitalRead(MANUAL_PIN))
  {
    if (millis() - lastPressed > 300)
    {
      ToggleMode();
    }
    lastPressed = millis();
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  pinMode(MANUAL_PIN, INPUT_PULLDOWN_16);
  pinMode(MODE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(ERROR_PIN, OUTPUT);
  digitalWrite(ERROR_PIN, HIGH);

  pinMode(WIFI_PIN, OUTPUT);
  pinMode(MODBUS_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(TOGGLE_PIN), IntCallback, RISING);
}

void loop()
{
}
