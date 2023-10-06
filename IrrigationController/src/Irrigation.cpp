#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include <Arduino.h>

#include "WiFiController.h"
#include "WebController.h"
#include "IrrigationController.h"

// Controllers
IrrigationController *controller = NULL;
WiFiController *wifi = NULL;
WebController *web = NULL;

void ValveOpen()
{
  controller->ValveOpen();
}

void ValveClosed()
{
  controller->ValveClosed();
}

void WaterFlowTick()
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

  controller = IrrigationController::GetInstance();
  controller->Setup();

  web = WebController::GetInstance();

  web->Setup();

  attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpen, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosed, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptWaterFlowTickPin), WaterFlowTick, FALLING);

  web->Alert("Irrigation controller has started.");
}

void loop()
{
    controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
    web->ProcessMainLoop();
}
