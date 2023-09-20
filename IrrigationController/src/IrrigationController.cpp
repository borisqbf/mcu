
#include "IrrigationController.h"


static IrrigationController theInstance;

IrrigationController *IrrigationController::GetInstance()
{
    return &theInstance;
}

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
    lastTimeVolumeMeasured = millis();
}

void IrrigationController::ProcesMainLoop()
{
    if (currentState == State::Idle)
    {
        if (CheckStartTime())
        {
            SetEndTime();
            SetNextStartTime();
            OpenValve();
        }
    }
    else if (currentState == State::Watering)
    {
        char message[150];
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
        if ((millis() - lastTimeVolumeMeasured) > 30000)
        {
            float frequency = (pulseCounter * 1000.0) / (millis() - lastTimeVolumeMeasured);
            waterFlow = frequency / 5.5;
            pulseCounter = 0;
            waterVolume += (waterFlow * 2);
            lastTimeVolumeMeasured = millis();
        }
        if (digitalRead(volumeMetterPin) != lastStateOfvolumeMetterPin)
        {
            lastStateOfvolumeMetterPin = digitalRead(volumeMetterPin);
            if (lastStateOfvolumeMetterPin == HIGH)
            {
                pulseCounter++;
            }
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
        currentState = State::Watering;
        stateChangedAt = Chronos::DateTime::now();
        digitalWrite(valveOpenPin, LOW);
        digitalWrite(valveClosePin, LOW);
        InializeFlow();
    }
}

void IrrigationController::ValveClosed()
{
    if (currentState == State::ClosingValve)
    {
        currentState = State::Idle;
        stateChangedAt = Chronos::DateTime::now();
        digitalWrite(valveOpenPin, LOW);
        digitalWrite(valveClosePin, LOW);
    }
}

void IrrigationController::Reset()
{
    currentState = State::Idle;
    InializeFlow();
}

float IrrigationController::GetWaterFlow()
{
    return waterFlow;
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
        return "Closing Valve";
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
    currentState = State::ClosingValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, HIGH);
}

void IrrigationController::OpenValve()
{
    currentState = State::OpeningValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, HIGH);
    digitalWrite(valveClosePin, LOW);
}

void IrrigationController::InializeFlow()
{
    waterVolume = 0.0;
    lastTimeVolumeMeasured = millis();
    waterFlow = 0.0;
    lastStateOfvolumeMetterPin = LOW;
    pulseCounter = 0;
}
