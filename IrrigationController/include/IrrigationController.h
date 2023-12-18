#ifndef IRRIGATIONCONTROLLER_H
#define IRRIGATIONCONTROLLER_H

#include <TimeLib.h>
#include "DateTimeLib.h"
#include "WebController.h"
#include "NotificationController.h"

enum State
{
    Idle = 0,
    OpeningValve,
    Watering,
    ClosingValve
};

enum SkipReason
{
    None = 0,
    RainBefore,
    RainForecast,
    SoilHumidity
};

// Pins
#define valveOpenPin 18
#define valveClosePin 19

#define interruptWaterFlowTickPin 32
#define interruptValveOpenPin 5
#define interruptValveClosedPin 23

#define humidityInputPin 36
#define humidityPowerPin 26

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

    // HTTP request handlers
    static void CloseValve();
    static void OpenValve();
    static void Reset();
    static void SetParams();
    static void SetCalendar();
    static void ClearCalendar();
    static void GetStatus();
    static void WaterFlowTick();
    static void GetHumidity();
    static void SelfTest();

private:
    IrrigationController();
    static IrrigationController *theInstance;
    static WebController *webController;
    static NotificationController *notificationController;
    static enum State currentState;
    static bool inSelfTestMode;
    static DateTime stateChangedAt;
    static DateTime startTime;
    const int maxValveActionTime = 30;    // sec
    const int lowWaterFlowThreshold = 15; // l/min
    static bool flowTooLow;
    static float waterVolume;
    static long lastTimeVolumeMeasured;
    static float waterVolumeTarget;
    static float waterFlowRate;
    static float deltaWaterTankLeveThreshold;
    static float rainForecastThreshold;
    static int soilHumidityThreshold;
    static long pulseCounter;
    static int maxWateringTime; // minutes
    static int wateringFrequency;
    static int waterTankLevel;
    
    // temp storage of irrigation parameters during self-test
    static DateTime tmpStartTime;
    static float tmpWaterVolumeTarget;
    static float tmpDeltaWaterTankLeveThreshold;
    static float tmpRainForecastThreshold;
    static int tmpSoilHumidityThreshold;
    static int tmpMaxWateringTime;

    bool CheckStartTime();
    void SetNextStartTime();
    static void SkipToNext();
    static void SkipDayImp();
    static void SkipDay();
    static void SetNextStartTime(int hour, int mm);
    bool CheckEndTime();
    bool CheckForLowWaterFlow();
    bool CheckForNormalWaterFlow();
    bool CheckWateringTarget();

    static int GetHumidityImp();
    static int GetWaterTankLevel();
    SkipReason WateringRequired(int newWaterTankLevel, float rainForecast, int soilHumidity);
    const char *GetSkipReasonDescription(SkipReason reason);

    static void CloseValveInt();
    static void OpenValveInt();
    static void InializeFlow();
    static void EndSelfTest();
};

#endif