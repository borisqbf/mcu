#include "WiFiController.h"
#include "WebController.h"
#include "IrrigationController.h"

// Controllers
IrrigationController *controller = NULL;
WiFiController *wifi = NULL;
WebController *web = NULL;

// Pins
const byte interruptValveOpenPin = 2;
const byte interruptValveClosedPin = 4;

void ValveOpen()
{
  controller->ValveOpen();
}

void ValveClosed()
{
  controller->ValveClosed();
}

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
  controller->Setup();

  web = WebController::GetInstance();

  web->Setup();

  attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpen, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosed, FALLING);
}

void loop()
{
  controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
  web->ProcessMainLoop();
}
