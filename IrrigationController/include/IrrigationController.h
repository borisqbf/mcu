#ifndef IRRIGATIONCONTROLLER_H
#define IRRIGATIONCONTROLLER_H

#include <TimeLib.h>
#include <Chronos.h>
#include "WebController.h"

enum State
{
    Idle = 0,
    OpeningValve,
    Watering,
    ClosingValve
};

#define valveOpenPin 18
#define valveClosePin 19
#define volumeMetterPin 21

class IrrigationController
{
private:
    WebController *webController = NULL;
    enum State currentState = State::Idle;
    Chronos::DateTime stateChangedAt;
    const int maxValveActionTime = 10;
    const int maxWateringTime = 60; // minutes
    float waterVolume = 0.0;
    long lastTimeVolumeMeasured = 0;
    float waterVolumeTarget = 0.0;
    float waterFlow = 0.0;
    long pulseCounter = 0;
    byte lastStateOfvolumeMetterPin = 0;

    bool CheckStartTime();
    bool CheckEndTime();
    bool CheckWaterFlow();
    bool CheckWateringTarget();
    void SetEndTime();
    void SetNextStartTime();
    void InializeFlow();


public:
    IrrigationController(/* args */);
    ~IrrigationController();
    static IrrigationController *GetInstance();
    void Setup();
    void ProcesMainLoop();
    void ValveOpen();
    void ValveClosed();

    float GetWaterFlow();
    const char *GetCurrentState();
    bool IsIdle() { return currentState == State::Idle; };

    const char *GenerateStatusResponse();

    static void CloseValve();
    static void OpenValve();
    static void Reset();
};

#endif