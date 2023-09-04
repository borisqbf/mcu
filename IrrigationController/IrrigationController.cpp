#include "IrrigationController.h"
#include <TimeLib.h>

IrrigationController::IrrigationController(/* args */)
{
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, LOW);
}

IrrigationController::~IrrigationController()
{
}

void IrrigationController::Initialize()
{
    wifiController = WiFiController::GetInstance();
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
        char message[250];
        if (CheckWateringTarget())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            Chronos::Span::Absolute duration = n - stateChangedAt;
            CloseValve();

            sprintf(message, "Watering finished at %0u/%0u/%0u %0u:%0u.  Watering target of %u  liters has been reached. The process took %u minutes.", n.day(), n.month(), n.year(), n.hour(), n.minute(), waterVolumeTarget, duration.minutes());
            wifiController->Alert(message);
        }
        else if (CheckEndTime())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            CloseValve();
            sprintf(message, "Watering aborted at %02u/%02u/%u %02u:%02u after %u minutes. Watering target of %u  liters has not been reached. %u liters have been dispensed", n.day(), n.month(), n.year(), n.hour(), n.minute(), maxWateringTime, waterVolumeTarget, waterVolume);
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
    if (currentState == State::OpeningValve)
    {
        Serial.println("The valve is open");
        currentState = State::Watering;
        stateChangedAt = Chronos::DateTime::now();
        digitalWrite(valveOpenPin, LOW);
        digitalWrite(valveClosePin, LOW);
    }
}

void IrrigationController::ValveClosed()
{
    if (currentState == State::ClosingValve)
    {
        Serial.println("The valve is closed");
        currentState = State::Idle;
        stateChangedAt = Chronos::DateTime::now();
        digitalWrite(valveOpenPin, LOW);
        digitalWrite(valveClosePin, LOW);
    }
}

void IrrigationController::Reset()
{
    Serial.println("Resetting");
    currentState = State::Idle;
}

char *IrrigationController::GetCurrentState()
{
    switch (currentState)
    {
    case State::Idle:
        return "Idle";
    case State::Watering:
        return "Watering";
    case State::OpeningValve:
        return "Opening Valve";
    case State::ClosingValve:
        return "ClosingValve";
    default:
        return "Unknown";
    }
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
    Serial.println("The valve is closing");
    currentState = State::ClosingValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, HIGH);
}

void IrrigationController::OpenValve()
{
    Serial.println("The valve is opening");
    currentState = State::OpeningValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, HIGH);
    digitalWrite(valveClosePin, LOW);
}
