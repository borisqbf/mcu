#include "RenogyData.h"
#include "RenogyInfo.h"
#include "RenogyController.h"
#include "MqttController.h"
#include "LEDStatusReporter.h"
#include "WiFiController.h"

const uint8_t TOGGLE_PIN = 14;
const uint8_t MANUAL_PIN = 16;
const uint8_t MODE_PIN = 12;
const uint8_t RELAY_PIN = 13;

enum PowerMode : byte
{
  OnSolar = 0,
  OnGrid = 1
};

PowerMode mode = OnSolar;
float OnGridThreshold = 23.5;
float OnSolarThreshold = 25.0;
unsigned long lastPressed = 0;

RenogyController *renogyController = RenogyController::GetInstance();
WiFiController *wifiController = WiFiController::GetInstance();
MqttController *mqttController = MqttController::GetInstance();

void ToSolarMode()
{
  mqttController->PublishMessage("Switching to solar");
  mode = OnSolar;
  digitalWrite(MODE_PIN, HIGH);
  digitalWrite(RELAY_PIN, HIGH);
}

void ToMainMode()
{
  mqttController->PublishMessage("Switching to mains power");
  mode = OnGrid;
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

void IRAM_ATTR IntCallback()
{
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
  wifiController->Setup();
  mqttController->Connect();
}

void loop()
{
  wifiController->ProcessMainLoop();
  mqttController->ProcessMainLoop();

  if (renogyController->IsUpdateRequired())
  {
    if (renogyController->GetRenogyData())
    {
      LEDStatusReporter::ReportStatus(modbusReadOK);
      LEDStatusReporter::ReportError(0);
      if (!digitalRead(MANUAL_PIN))
      {
        float batteryVoltage = renogyController->GetBatteryVoltage();
        if ((batteryVoltage < OnGridThreshold) && (batteryVoltage > 7.0)) // to protect from bad reads, etc
        {
          ToMainMode();
        }
        if (batteryVoltage > OnSolarThreshold)
        {
          ToSolarMode();
        }
      }
    }
  }
  if (mqttController->IsUpdateRequired())
  {
    Serial1.println("Sending telemetry");
 
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
