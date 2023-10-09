#include "IrrigationController.h"

// Statics
IrrigationController *IrrigationController::theInstance = NULL;
enum State IrrigationController::currentState = State::Idle;
WebController *IrrigationController::webController = NULL;
Chronos::DateTime IrrigationController::stateChangedAt;

float IrrigationController::waterVolume = 0.0;
long IrrigationController::lastTimeVolumeMeasured = 0;
float IrrigationController::waterVolumeTarget = 0.0;
float IrrigationController::waterFlowRate = 0.0;
long IrrigationController::pulseCounter = 0;
bool IrrigationController::flowTooLow = false;
int IrrigationController::maxWateringTime = 60; // minutes
WateringCalendar IrrigationController::wateringCalendar;

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

IrrigationController::IrrigationController()
{
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, LOW);
}

void IrrigationController::Setup()
{
    if (digitalRead(interruptValveOpenPin) == LOW)
    {
        CloseValveInt();
        webController->Alert("Unexpected valve in Open state while powering up. Attempting to close.");
    }
    webController = WebController::GetInstance();
    lastTimeVolumeMeasured = millis();

    WebController::AddAction("/on", &IrrigationController::OpenValve);
    WebController::AddAction("/off", &IrrigationController::CloseValve);
    WebController::AddAction("/reset", &IrrigationController::Reset);
    WebController::AddAction("/set-params", &IrrigationController::SetParams);
    WebController::AddAction("/set-calendar", &IrrigationController::SetCalendar);
    WebController::AddAction("/clear-calendar", &IrrigationController::ClearCalendar);
    WebController::AddAction("/status", &IrrigationController::GetStatus);
}

void IrrigationController::ProcesMainLoop()
{
    if (currentState == State::Idle)
    {
        if (CheckStartTime())
        {
            SetEndTime();
            SetNextStartTime();
            OpenValveInt();
        }
    }
    else if (currentState == State::Watering)
    {
        static char message[150];
        if (CheckWateringTarget())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            Chronos::Span::Absolute duration = n - stateChangedAt;
            CloseValveInt();

            snprintf(message, 150, "Watering finished at %02d/%02d/%d %02d:%02d.  Watering target of %d liters has been reached. The process took %d minutes.", n.day(), n.month(), n.year(), n.hour(), n.minute(), static_cast<int>(waterVolumeTarget), duration.minutes());
            webController->Alert(message);
        }
        else if (CheckEndTime())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            CloseValveInt();
            snprintf(message, 150, "Watering aborted at %02d/%02d/%d %02d:%02d after %d minutes. Watering target of %d liters has not been reached. %d liters have been dispensed", n.day(), n.month(), n.year(), n.hour(), n.minute(), maxWateringTime, static_cast<int>(waterVolumeTarget), static_cast<int>(waterVolume));
            webController->Alert(message);
        }
        if ((millis() - lastTimeVolumeMeasured) > 30000)
        {
            float frequency = (pulseCounter * 1000.0) / (millis() - lastTimeVolumeMeasured);
            waterFlowRate = frequency / 5.5; // liters per minute
            pulseCounter = 0;
            waterVolume += waterFlowRate * 0.5; // as this is executed every 30 sec
            lastTimeVolumeMeasured = millis();

            if (CheckForLowWaterFlow())
            {
                if (!flowTooLow)
                {
                    flowTooLow = true;
                    webController->Alert("Insufficient waterflow.");
                }
            }
            if (CheckForNormalWaterFlow())
            {
                if (flowTooLow)
                {
                    flowTooLow = false;
                    webController->Alert("Waterflow is back to normal.");
                }
            }
        }
    }
    else if (currentState == State::OpeningValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            if (digitalRead(interruptValveOpenPin) != LOW)
                webController->Alert("Unable to open Valve.");
            currentState = State::Watering; // This is the best guess
        }
    }
    else if (currentState == State::ClosingValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            if (digitalRead(interruptValveClosedPin) != LOW)
                webController->Alert("Unable to close Valve");
            currentState = State::Idle; // This is the best guess
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

void IrrigationController::SetParams()
{
    WebController::UrlQueryParameter *params = WebController::GetUrlQueryParams();
    if (params == nullptr)
        webController->SendHttpResponseOK("Missing Parameters\n\n");
    else
    {
        WebController::UrlQueryParameter *paramsToDelete = params;

        while ((*params).p != "")
        {
            if ((*params).p.equalsIgnoreCase("target"))
            {
                waterVolumeTarget = (*params).v.toFloat();
                if (waterVolumeTarget < 1)
                {
                    webController->SendHttpResponseBadRequest("target");
                    break;
                }
            }
            if ((*params).p.equalsIgnoreCase("duration"))
            {
                maxWateringTime = (*params).v.toInt();
                if (maxWateringTime < 1)
                {
                    webController->SendHttpResponseBadRequest("duration");
                    break;
                }
            }
            else
            {
                Serial.print("Unsupported parameter ");
                Serial.print((*params).p);
                Serial.print(":");
                Serial.println((*params).v);
                webController->SendHttpResponseBadRequest((*params).p.c_str());
            }
            params++;
        }
        delete[] paramsToDelete; // deallocate
        paramsToDelete = nullptr;
        webController->SendHttpResponseOK("Configuration successful\n\n");
    }
}

void IrrigationController::SetCalendar()
{
    WebController::UrlQueryParameter *params = WebController::GetUrlQueryParams();
    if (params == nullptr)
        webController->SendHttpResponseOK("Missing Parameters\n\n");
    else
    {
        WebController::UrlQueryParameter *paramsToDelete = params;
        int freq = 2; // every second day
        int minute = 0;
        int hour = 22; // at 22:00

        while ((*params).p != "")
        {
            if ((*params).p.equalsIgnoreCase("at"))
            {
                int columnIndex = (*params).v.indexOf(":");
                if (columnIndex < 1)
                {
                    webController->SendHttpResponseBadRequest("at");
                    break;
                }

                hour = (*params).v.substring(0, columnIndex).toInt();
                minute = (*params).v.substring(columnIndex + 1).toInt();
                if ((hour < 0) || (hour > 24) || (minute < 0) || (minute > 60))
                {
                    webController->SendHttpResponseBadRequest("at");
                    break;
                }
            }
            else if ((*params).p.equalsIgnoreCase("every"))
            {
                freq = (*params).v.toInt();
                if ((freq < 1) || (freq > 3))
                {
                    webController->SendHttpResponseBadRequest("every");
                    break;
                }
            }
            else
            {
                Serial.print("Unsupported parameter ");
                Serial.print((*params).p);
                Serial.print(":");
                Serial.println((*params).v);
                webController->SendHttpResponseBadRequest((*params).p.c_str());
            }
            params++;
        }

        wateringCalendar.add(Chronos::Event(1,
                                            Chronos::Mark::Daily(hour, minute),
                                            Chronos::Span::Minutes(maxWateringTime)));
        delete[] paramsToDelete; // deallocate
        paramsToDelete = nullptr;
        webController->SendHttpResponseOK("Configuration successful\n\n");
    }
}

void IrrigationController::ClearCalendar()
{
    wateringCalendar.clear();
    webController->SendHttpResponseOK("OK\n\n");
}

void IrrigationController::GetStatus()
{
    const byte nEvents = 3;
    Chronos::Event::Occurrence eventArray[nEvents];

    static char response[1024];
    memset(response, '\0', 1024);
    strcat(response, GenerateStatusResponse());
    Chronos::DateTime n = Chronos::DateTime::now();

    int nextEventsNumber = wateringCalendar.listNext(nEvents, eventArray, n);
    if (nextEventsNumber == 0)
    {
        strcat(response, "No scheduled watering events found\n\n");
    }
    else
    {
        strcat(response, "Scheduled watering events:\n");
        char eventInfo[50];
        for (int i = 0; i < nextEventsNumber; i++)
        {
            snprintf(eventInfo, 50, "%02u/%02u/%u %02u:%02u\n", eventArray[i].start.day(), eventArray[i].start.month(), eventArray[i].start.year(), eventArray[i].start.hour(), eventArray[i].start.minute());
            strcat(response, eventInfo);
        }
    }
    webController->SendHttpResponseOK(response);
}

float IrrigationController::GetWaterFlow()
{
    return waterFlowRate;
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

bool IrrigationController::CheckForLowWaterFlow()
{
    return waterFlowRate < lowWaterFlowThreshold * 0.9; // 10% under threshold
}

bool IrrigationController::CheckForNormalWaterFlow()
{
    return waterFlowRate > lowWaterFlowThreshold * 1.1; // 10% under threshold
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

void IrrigationController::CloseValveInt()
{
    currentState = State::ClosingValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, HIGH);
}

void IrrigationController::OpenValveInt()
{
    currentState = State::OpeningValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, HIGH);
    digitalWrite(valveClosePin, LOW);
}

void IrrigationController::CloseValve()
{
    CloseValveInt();
    webController->SendHttpResponseOK(GenerateStatusResponse());
}

void IrrigationController::OpenValve()
{
    OpenValveInt();
    webController->SendHttpResponseOK(GenerateStatusResponse());
}

void IrrigationController::WaterFlowTick()
{
    pulseCounter++;
}

const char *IrrigationController::GenerateStatusResponse()
{
    static char message[250];
    Chronos::DateTime n = Chronos::DateTime::now();
    snprintf(message, 250, "Current time is %02u/%02u/%u %02u:%02u\nCurrent state is %s\nCurrent flow is %d\nWatering target is %d liters\nMax watring duration is %d minutes\n", n.day(), n.month(), n.year(), n.hour(), n.minute(), GetCurrentState(), static_cast<int>(GetWaterFlow()), static_cast<int>(waterVolumeTarget), maxWateringTime);
    Serial.println(message);
    return message;
}

void IrrigationController::InializeFlow()
{
    waterVolume = 0.0;
    lastTimeVolumeMeasured = millis();
    waterFlowRate = 0.0;
    pulseCounter = 0;
    flowTooLow = false;
}
