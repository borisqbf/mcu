
#include "WiFiController.h"
#include "IrrigationController.h"

IrrigationController *controller = NULL;
WiFiController *wifi = NULL;
const byte interruptValveOpenPin = 2;
const byte interruptValveClosedPin = 3;


void setup()
{
  wifi = WiFiController::GetInstance();
  wifi->Setup();
  
  controller = new IrrigationController();
  controller->Initialize();
  attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpen, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosed, RISING);
}

void loop() {
  controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
}

void ValveOpen()
{
  controller->ValveOpen();
}

void ValveClosed()
{
  controller->ValveClosed();
}