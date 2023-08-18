#include "IrrigationController.h"

IrrigationController::IrrigationController(/* args */)
{
    wifiController = WiFiController::GetInstance();
}

IrrigationController::~IrrigationController()
{
}

void IrrigationController::Initialize()
{
    wifiController->GetNTPTime();
}

void IrrigationController::ProcesMainLoop()
{
    if (currentState == State::Idle)
    {
        if (CheckStartTime())
        {
            waterVolume = 0.0;
            SetEndTime();
            SetNextStartTime();
            OpenValve();
        }
    }
    else if (currentState == State::Watering)
    {
        if (CheckWateringTarget())
        {
            Chronos::Span::Absolute duration = Chronos::DateTime::now() - stateChangedAt;
            CloseValve();
            wifiController->Alert("Watering finished at " + Chronos::DateTime::now() + " Watering target of " + waterVolumeTarget + " liters has been reached. The process took " + duration.minutes() + " minutes.");
        }
        else if (CheckEndTime())
        {
            CloseValve();
            wifiController->Alert("Watering aborted at " + Chronos::DateTime::now() + " after " + maxWateringTime + " minutes. Watering target of " + waterVolumeTarget + " liters has not been reached. " + waterVolume + " liters have been dispensed");
        }
        else
        {
            if (!CheckWaterFlow())
                wifiController->Alert("Insufficient waterflow.");
        }
    }
    else if (currentState == State::OpeningValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            wifiController->Alert("Unable to open Valve.");
        }
    }
    else if (currentState == State::ClosingValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            wifiController->Alert("Unable to close Valve");
        }
    }
}

void IrrigationController::ValveOpen()
{
    currentState = State::Watering;
    stateChangedAt = Chronos::DateTime::now();
}

void IrrigationController::ValveClosed()
{
    currentState = State::Idle;
    stateChangedAt = Chronos::DateTime::now();
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

bool IrrigationController::CheckWateringTarget()
{
    return false;
}

void IrrigationController::SetEndTime()
{
}

void IrrigationController::SetNextStartTime()
{
}

void IrrigationController::CloseValve()
{
    currentState = State::ClosingValve;
    stateChangedAt = Chronos::DateTime::now();
}

void IrrigationController::OpenValve()
{
    currentState = State::OpeningValve;
    stateChangedAt = Chronos::DateTime::now();
}
