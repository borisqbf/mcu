#include "IrrigationController.h"

IrrigationController::IrrigationController(/* args */)
{
}

IrrigationController::~IrrigationController()
{
}

void IrrigationController::Initialize()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    PrintLocalTime();
}

void IrrigationController::ProcesMainLoop()
{
    if (currentState == State.Idle)
    {
        if (CheckStartTime())
        {
            waterVolume = 0.0;
            SetEndTime();
            SetNextStartTime();
            OpenValve();
        }
    }
    else if (currentState == State.Watering)
    {
        if (CheckWateringTarget())
        {
            CloseValve();
            Alert("Watering finished at " + Now() + " Watering target of " + waterVolumeTarget + " liters has been reached.");
        }
        else if (CheckEndTime())
        {
            CloseValve();
            Alert("Watering aborted at " + Now() + " after " + maxWateringTime + " minutes. Watering target of " + waterVolumeTarget + " liters has not been reached. " + waterVolume + " liters have been dispensed");
        }
        else
        {
            if(!CheckWaterFlow())
                Alert("Insufficient waterflow.")
        }
    }
    else if (currentState == State.OpeningValve)
    {
        if ((Now() - stateChangedAt) > TimeSpan(0,0, maxValveActionTime))
        {
            Alert("Unable to open Valve.")
        }
    }
    else if (currentState == State.ClosingValve)
    {
        if ((Now() - stateChangedAt) > TimeSpan(0, 0, maxValveActionTime))
        {
            Alert("Unable to close Valve")
        }
    }
}

void IrrigationController::ValveOpen()
{
    currentState = State.Watring;
    stateChangedAt = Now();
}

void IrrigationController::ValveClosed()
{
    currentState = State.Idle;
    stateChangedAt = Now();
}

bool IrrigationController::CheckStartTime()
{
    return false;
}

bool IrrigationController::CheckEndTime()
{
    return false;
}

bool IrrigationController::CheckWaterFlow()
{
    return false;
}

void IrrigationController::PrintLocalTime()
{
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.print("Day of week: ");
    Serial.println(&timeinfo, "%A");
    Serial.print("Month: ");
    Serial.println(&timeinfo, "%B");
    Serial.print("Day of Month: ");
    Serial.println(&timeinfo, "%d");
    Serial.print("Year: ");
    Serial.println(&timeinfo, "%Y");
    Serial.print("Hour: ");
    Serial.println(&timeinfo, "%H");
    Serial.print("Minute: ");
    Serial.println(&timeinfo, "%M");
    Serial.print("Second: ");
    Serial.println(&timeinfo, "%S");
}

void IrrigationController::SetEndTime()
{
}

void IrrigationController::SetNextStartTime()
{
}

void IrrigationController::CloseValve()
{
    currentState = State.ClosingValve;
    stateChangedAt = Now();
}

void IrrigationController::OpenValve()
{
    currentState = State.OpeningValve;
    stateChangedAt = Now();
}
