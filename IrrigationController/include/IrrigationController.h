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
public:
    static IrrigationController *GetInstance();
    void Setup();
    void ProcesMainLoop();
    void ValveOpen();
    void ValveClosed();

    static float GetWaterFlow();
    static const char *GetCurrentState();
    bool IsIdle() { return currentState == State::Idle; };

    static const char *GenerateStatusResponse();

    static void CloseValve();
    static void OpenValve();
    static void Reset();

private:
    IrrigationController(/* args */);
    static WebController *webController;
    static enum State currentState;
    static Chronos::DateTime stateChangedAt;
    const int maxValveActionTime = 10;
    const int maxWateringTime = 60; // minutes
    static float waterVolume;
    static long lastTimeVolumeMeasured;
    static float waterVolumeTarget;
    static float waterFlow;
    static long pulseCounter;

    static byte lastStateOfvolumeMetterPin;
    static IrrigationController *theInstance;

    bool CheckStartTime();
    bool CheckEndTime();
    bool CheckWaterFlow();
    bool CheckWateringTarget();
    void SetEndTime();
    void SetNextStartTime();
    static void InializeFlow();
};

#endif