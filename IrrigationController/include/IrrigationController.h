#ifndef IRRIGATIONCONTROLLER_H
#define IRRIGATIONCONTROLLER_H

#include <TimeLib.h>
#include <Chronos.h>
#include "WebController.h"
#include "NotificationController.h"

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

#define interruptWaterFlowTickPin 24
#define interruptValveOpenPin 2
#define interruptValveClosedPin 23

DefineCalendarType(WateringCalendar, 4);

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

    //HTTP request handlers
    static void CloseValve();
    static void OpenValve();
    static void Reset();
    static void SetParams();
    static void SetCalendar();
    static void ClearCalendar();
    static void GetStatus();
    static void WaterFlowTick();

private:
    IrrigationController();
    static IrrigationController *theInstance;
    static WebController *webController;
    static NotificationController *notificationController;
    static enum State currentState;
    static Chronos::DateTime stateChangedAt;
    const int maxValveActionTime = 20; //sec
    const int lowWaterFlowThreshold = 10; // l/min
    static bool flowTooLow;
    static float waterVolume;
    static long lastTimeVolumeMeasured;
    static float waterVolumeTarget;
    static float waterFlowRate;
    static long pulseCounter;
    static int maxWateringTime; // minutes

    static WateringCalendar wateringCalendar;

    bool CheckStartTime();
    bool CheckEndTime();
    bool CheckForLowWaterFlow();
    bool CheckForNormalWaterFlow();
    bool CheckWateringTarget();
    static void CloseValveInt();
    static void OpenValveInt();
    static void InializeFlow();
};

#endif