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

IRAM_ATTR void ValveOpen()
{
  controller->ValveOpen();
}

IRAM_ATTR void ValveClosed()
{
  controller->ValveClosed();
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
  pinMode(interruptValveOpenPin, INPUT_PULLUP);
  pinMode(interruptValveClosedPin, INPUT_PULLUP);

  Serial.begin(115200);
  delay (500);

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
    controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
    wifi->ProcessMainLoop();
    notifications->ProcessMainLoop();
    web->ProcessMainLoop();
}
