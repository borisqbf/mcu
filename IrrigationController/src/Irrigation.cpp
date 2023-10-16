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

void setup()
{
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
}

void loop()
{
  controller->ProcesMainLoop(); // put your main code here, to run repeatedly:
  wifi->ProcessMainLoop();
  notifications->ProcessMainLoop();
  web->ProcessMainLoop();
}
