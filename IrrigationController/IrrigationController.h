#ifndef IRRIGATIONCONTROLLER_H
#define IRRIGATIONCONTROLLER_H

#include <time.h>

class IrrigationController
{
private:
    const char *ntpServer1 = "au.pool.ntp.org";
    const char *ntpServer2 = "pool.ntp.org";

    const long gmtOffset_sec = 10 * 60 * 60;
    const int daylightOffset_sec = 3600;
    struct tm timeinfo;
    enum State currentState = State.Idle;
    DateTime stateChangedAt;
    const int maxValveActionTime = 10;
    const int maxWateringTime = 60; // minutes
    float waterVolume = 0.0;
    float waterVolumeTarget = 0.0;

    bool CheckStartTime();
    bool CheckEndTime();
    bool CheckWaterFlow();
   
    void PrintLocalTime();
    void SetEndTime();
    void SetNextStartTime();
    void CloseValve();
    

public:
    IrrigationController(/* args */);
    ~IrrigationController();
    void Initialize();
    void ProcesMainLoop();
    void ValveOpen();
    void ValveClosed();
};

enum State
{
    Idle = 0,
    OpeningValve,
    Watering,
    ClosingValve
};
#endif