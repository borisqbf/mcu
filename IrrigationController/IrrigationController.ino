#include "WiFiController.h"
#include "IrrigationController.h"
#include "WebController.h"

// Controllers
IrrigationController *controller = NULL;
WiFiController *wifi = NULL;
WebController *web = NULL;

// Pins
const byte interruptValveOpenPin = 2;
const byte interruptValveClosedPin = 3;


void setup()
{
  Serial.begin(115200);
  wifi = WiFiController::GetInstance();
  wifi->Setup();
  
  controller = new IrrigationController();
  controller->Initialize();

  web = WebController::GetInstance();
  web->Setup();

  attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpen, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosed, RISING);
  web->SetOnAction(controller, &(controller->OpenValve));
  web->SetOffAction(controller, &(controller->CloseValve));
}

void loop() {
  controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
  web->ProcessMainLoop();
}

void ValveOpen()
{
  controller->ValveOpen();
}

void ValveClosed()
{
  controller->ValveClosed();
}