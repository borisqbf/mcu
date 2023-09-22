#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WebServer.h>
#include "IrrigationController.h"

class WebController
{
public:
    WebController();
    void Setup();
    void ProcessMainLoop();
    static WebController *GetInstance();
    void SetOnAction(IrrigationController *controllerInstance, ValveActionFn action);
    void SetOffAction(IrrigationController *controllerInstance, ValveActionFn action);
    void SetResetAction(IrrigationController *controllerInstance, ValveActionFn action);

private:
    WebServer *server;
    void SendHttpResponse(WiFiClient client);
    IrrigationController *actionController;
    ValveActionFn onAction;
    ValveActionFn offAction;
    ValveActionFn resetAction;
    ValveActionFn currentAction;
};

#endif