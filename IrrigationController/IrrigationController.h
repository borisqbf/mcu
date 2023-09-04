#ifndef IRRIGATIONCONTROLLER_H
#define IRRIGATIONCONTROLLER_H

#include <TimeLib.h>
#include <Chronos.h>
#include "WiFiController.h"

enum State
{
    Idle = 0,
    OpeningValve,
    Watering,
    ClosingValve
};


#define valveOpenPin 4
#define valveClosePin 5

class IrrigationController
{
private:
    WiFiController *wifiController = NULL;
    enum State currentState = State::Idle;
    Chronos::DateTime stateChangedAt;
    const int maxValveActionTime = 10;
    const int maxWateringTime = 60; // minutes
    float waterVolume = 0.0;
    float waterVolumeTarget = 0.0;

    bool CheckStartTime();
    bool CheckEndTime();
    bool CheckWaterFlow();
    bool CheckWateringTarget();
    void SetEndTime();
    void SetNextStartTime();
    void CloseValve();
    void OpenValve();
 
public:
    IrrigationController(/* args */);
    ~IrrigationController();
    void Initialize();
    void ProcesMainLoop();
    void ValveOpen();
    void ValveClosed();
};

typedef void (IrrigationController::*ValveActionFn)(void);

#endif