#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WiFiEsp.h>
#include "IrrigationController.h"

class WebController
{
public:
    WebController();
    void Setup();
    void ProcessMainLoop();
    static WebController *GetInstance();
    SetOnAction(IrrigationController *controllerInstance, ValveActionFn action);
    SetOffAction(IrrigationController *controllerInstance, ValveActionFn action);
    SetResetAction(IrrigationController *controllerInstance, ValveActionFn action);

private:
    WiFiEspServer *server;
    void SendHttpResponse(WiFiEspClient client);
    IrrigationController *actionController;
    ValveActionFn OnAction;
    ValveActionFn OffAction;
    ValveActionFn ResetAction;
};


#endif