#include "IrrigationController.h"

IrrigationController *controller = null;

void setup()
{
  controller = new IrrigationController();
  controller->Initialize();
}

void loop() {
  controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
}