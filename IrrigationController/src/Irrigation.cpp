#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include <Arduino.h>

#include "WiFiController.h"
#include "WebController.h"
#include "NotificationController.h"
#include "IrrigationController.h"

// Controllers
IrrigationController *controller = NULL;
WiFiController *wifi = NULL;
WebController *web = NULL;
NotificationController *notifications = NULL;

volatile bool valveOpen = false;
volatile bool valveClosed = false;
volatile long valveOpenChangedAt = 0;
volatile long valveClosedChangedAt = 0;

IRAM_ATTR void ValveOpen()
{
  valveOpen = true;
  valveOpenChangedAt = millis();
}

IRAM_ATTR void ValveClosed()
{
  valveClosed = true;
  valveClosedChangedAt = millis();
}

IRAM_ATTR void WaterFlowTick()
{
  controller->WaterFlowTick();
}

void setup()
{
  pinMode(valveOpenPin, OUTPUT);
  pinMode(valveClosePin, OUTPUT);

  pinMode(interruptWaterFlowTickPin, INPUT_PULLUP);
  pinMode(interruptValveOpenPin, INPUT);
  pinMode(interruptValveClosedPin, INPUT);

  Serial.begin(115200);
  delay(500);

  wifi = WiFiController::GetInstance();
  wifi->Setup();

  web = WebController::GetInstance();
  web->Setup();

  notifications = NotificationController::GetInstance();
  notifications->Setup();

  controller = IrrigationController::GetInstance();
  controller->Setup();

  attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpen, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosed, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptWaterFlowTickPin), WaterFlowTick, FALLING);

  notifications->Alert("Irrigation controller has started.");
  notifications->Display("Ready.", "");
}

void loop()
{
  if (valveClosed && ((millis() - valveClosedChangedAt) > 300) && (digitalRead(interruptValveClosedPin) == LOW))
  {
    valveClosed = false;
    Serial.println("Int - closed");
    controller->ValveClosed();
  }
  if (valveOpen && ((millis() - valveOpenChangedAt) > 300) && (digitalRead(interruptValveOpenPin) == LOW))
  {
    valveOpen = false;
    Serial.println("Int - open");
    controller->ValveOpen();
  }
  controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
  wifi->ProcessMainLoop();
  notifications->ProcessMainLoop();
  web->ProcessMainLoop();
}
