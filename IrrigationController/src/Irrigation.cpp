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
  pinMode(valveOpenPin, OUTPUT);
  pinMode(valveClosePin, OUTPUT);
  pinMode(interruptValveOpenPin, INPUT_PULLUP);
  pinMode(interruptValveClosedPin, INPUT_PULLUP);
  pinMode(volumeMetterPin, INPUT_PULLUP);
  
  Serial.begin(115200);
  wifi = WiFiController::GetInstance();
  wifi->Setup();

  controller = IrrigationController::GetInstance();
  controller->Initialize();

  web = WebController::GetInstance();
  web->Setup();
  web->SetOnAction(controller, &(controller->OpenValve));
  web->SetOffAction(controller, &(controller->CloseValve));
  web->SetResetAction(controller, &(controller->Reset));
  attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpen, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosed, FALLING);
}

void loop()
{
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