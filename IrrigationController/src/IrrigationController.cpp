
#include "IrrigationController.h"

// Statics
IrrigationController *IrrigationController::theInstance = NULL;
enum State IrrigationController::currentState = State::Idle;
WebController *IrrigationController::webController= NULL;
Chronos::DateTime IrrigationController::stateChangedAt;

float IrrigationController::waterVolume = 0.0;
long IrrigationController::lastTimeVolumeMeasured = 0;
float IrrigationController::waterVolumeTarget = 0.0;
float IrrigationController::waterFlow = 0.0;
long IrrigationController::pulseCounter = 0;
byte IrrigationController::lastStateOfvolumeMetterPin = 0;

IrrigationController *IrrigationController::GetInstance()
{
    if (theInstance == NULL)
    {
        theInstance = new IrrigationController();

        // returning the instance pointer
        return theInstance;
    }
    else
    {
        return theInstance;
    }
}

IrrigationController::IrrigationController(/* args */)
{
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, LOW);
}

void IrrigationController::Setup()
{
    webController = WebController::GetInstance();
    lastTimeVolumeMeasured = millis();

    WebController::AddAction("/on", &IrrigationController::OpenValve);
    WebController::AddAction("/off", &IrrigationController::CloseValve);
    WebController::AddAction("/reset", &IrrigationController::Reset);
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
        static char message[150];
        if (CheckWateringTarget())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            Chronos::Span::Absolute duration = n - stateChangedAt;
            CloseValve();

            snprintf(message, 150, "Watering finished at %02d/%02d/%d %02d:%02d.  Watering target of %d liters has been reached. The process took %d minutes.", n.day(), n.month(), n.year(), n.hour(), n.minute(), static_cast<int>(waterVolumeTarget), duration.minutes());
            webController->Alert(message);
        }
        else if (CheckEndTime())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            CloseValve();
            snprintf(message, 150, "Watering aborted at %02d/%02d/%d %02d:%02d after %d minutes. Watering target of %d liters has not been reached. %d liters have been dispensed", n.day(), n.month(), n.year(), n.hour(), n.minute(), maxWateringTime, static_cast<int>(waterVolumeTarget), static_cast<int>(waterVolume));
            webController->Alert(message);
        }
        else
        {
            if (!CheckWaterFlow())
                webController->Alert("Insufficient waterflow.");
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
            webController->Alert("Unable to open Valve.");
        }
    }
    else if (currentState == State::ClosingValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            webController->Alert("Unable to close Valve");
        }
    }
}

void IrrigationController::ValveOpen()
{
    if (currentState == State::OpeningValve)
    {
        webController->Alert("Watering has started.");
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

const char *IrrigationController::GetCurrentState()
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

const char *IrrigationController::GenerateStatusResponse()
{
    static char message[250];
    Chronos::DateTime n = Chronos::DateTime::now();
    snprintf(message, 250, "Current time is %02u/%02u/%u %02u:%02u\nCurrent state is %s\nCurrent flow is %d\n", n.day(), n.month(), n.year(), n.hour(), n.minute(), GetCurrentState(), static_cast<int>(GetWaterFlow()));
    Serial.println(message);
    return message;
}

void IrrigationController::CloseValve()
{
    currentState = State::ClosingValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, HIGH);

    webController->SendHttpResponse(GenerateStatusResponse());
}

void IrrigationController::OpenValve()
{
    currentState = State::OpeningValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, HIGH);
    digitalWrite(valveClosePin, LOW);

    webController->SendHttpResponse(GenerateStatusResponse());
}

void IrrigationController::InializeFlow()
{
    waterVolume = 0.0;
    lastTimeVolumeMeasured = millis();
    waterFlow = 0.0;
    lastStateOfvolumeMetterPin = LOW;
    pulseCounter = 0;
}
