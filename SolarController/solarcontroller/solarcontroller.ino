#include "RenogyData.h"
#include "RenogyInfo.h"
#include "renogyController.h"
#include "MqttController.h"
#include "LEDStatusReporter.h"
#include "WiFiController.h"

#pragma region general definitions
const uint8_t TOGGLE_PIN = 14;
const uint8_t MANUAL_PIN = 16;
const uint8_t MODE_PIN = 12;
const uint8_t RELAY_PIN = 13;

enum PowerMode : byte
{
  OnSolar = 0,
  OnMain = 1
};

PowerMode mode = OnSolar;
float OnMainThreshold = 21;
float OnSolarThreshold = 24.0;
unsigned long lastPressed = 0;
#pragma endregion

RenogyController *renogyController = RenogyController::GetInstance();
WiFiController *wifiController = WiFiController::GetInstance();
MqttController *mqttController = MqttController::GetInstance();

void ToSolarMode()
{
  if (mode == OnSolar)
    return;
  mode = OnSolar;
  digitalWrite(MODE_PIN, HIGH);
  digitalWrite(RELAY_PIN, HIGH);
}

void ToMainMode()
{
  if (mode == OnMain)
    return;
  mode = OnMain;
  digitalWrite(MODE_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
}

void ToggleMode()
{
  if (mode == OnSolar)
  {
    ToMainMode();
  }
  else
  {
    ToSolarMode();
  }
}

void ICACHE_RAM_ATTR IntCallback()
{
  long int m = millis();
  if (digitalRead(MANUAL_PIN))
  {
    if (millis() - lastPressed > 400)
    {
      ToggleMode();
    }
    lastPressed = millis();
  }
}

void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  pinMode(MANUAL_PIN, INPUT_PULLDOWN_16);
  pinMode(MODE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(MODE_PIN, HIGH); // Initial state is on solar so green LED is on
  attachInterrupt(digitalPinToInterrupt(TOGGLE_PIN), IntCallback, RISING);

  LEDStatusReporter::Setup();
  renogyController->Setup();
  if (wifiController->Setup())
    mqttController->Connect();
  else
    mqttController->RequestDelayedConnect();
}

void loop()
{
  mqttController->Loop();

  if (renogyController->IsUpdateRequired())
  {
    if (renogyController->GetRenogyData())
    {
      LEDStatusReporter::ReportStatus(modbusReadOK);
      LEDStatusReporter::ReportError(0);
      if (!digitalRead(MANUAL_PIN))
      {
        float batteryVoltage = renogyController->GetBatteryVoltage();
        if (batteryVoltage < OnMainThreshold)
        {
          mqttController->PublishMessage("Switching to mains power");
          ToMainMode();
        }
        else if (batteryVoltage > OnSolarThreshold)
        {
          mqttController->PublishMessage("Switching to solar");
          ToSolarMode();
        }
      }
    }
  }
  if (mqttController->IsUpdateRequired())
  {
    if (renogyController->PublishRenogyData())
    {
      Serial1.println("Telemetry sent");
      LEDStatusReporter::ReportStatus(mqttSentOK);
    }
    else
    {
      Serial1.println("Error sending Telemetry");
      mqttController->HandleMQTTError();
    }
  }
}
