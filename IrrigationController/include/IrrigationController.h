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

// Pins
#define valveOpenPin 18
#define valveClosePin 19

#define interruptWaterFlowTickPin 21
#define interruptValveOpenPin 22
#define interruptValveClosedPin 23


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
    static void WaterFlowTick();
    static void Reset();

private:
    IrrigationController();
    static IrrigationController *theInstance;
    static WebController *webController;
    static enum State currentState;
    static Chronos::DateTime stateChangedAt;
    const int maxValveActionTime = 20;
    const int maxWateringTime = 60;       // minutes
    const int lowWaterFlowThreshold = 10; // l/min
    static bool flowTooLow;
    static float waterVolume;
    static long lastTimeVolumeMeasured;
    static float waterVolumeTarget;
    static float waterFlowRate;
    static long pulseCounter;


    bool CheckStartTime();
    bool CheckEndTime();
    bool CheckForLowWaterFlow();
    bool CheckForNormalWaterFlow();
    bool CheckWateringTarget();
    void SetEndTime();
    void SetNextStartTime();
    static void CloseValveInt();
    static void OpenValveInt();
    static void InializeFlow();
};

#endif