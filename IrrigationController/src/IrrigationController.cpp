#include "IrrigationController.h"

// Statics
IrrigationController *IrrigationController::theInstance = NULL;
enum State IrrigationController::currentState = State::Idle;
WebController *IrrigationController::webController = NULL;
NotificationController *IrrigationController::notificationController = NULL;
Chronos::DateTime IrrigationController::stateChangedAt;

float IrrigationController::waterVolume = 0.0;
long IrrigationController::lastTimeVolumeMeasured = 0;
float IrrigationController::waterVolumeTarget = 100.0;
float IrrigationController::waterFlowRate = 0.0;
long IrrigationController::pulseCounter = 0;
bool IrrigationController::flowTooLow = false;
int IrrigationController::maxWateringTime = 60; // minutes

// Volatile non-member vars for ISRs

volatile bool valveOpen = false;
volatile bool valveClosed = false;
volatile long valveOpenChangedAt = 0;
volatile long valveClosedChangedAt = 0;

IRAM_ATTR void ValveOpenISR()
{
    valveOpen = true;
    valveOpenChangedAt = millis();
}

IRAM_ATTR void ValveClosedISR()
{
    valveClosed = true;
    valveClosedChangedAt = millis();
}

IRAM_ATTR void WaterFlowTickISR()
{
    IrrigationController::GetInstance()->WaterFlowTick();
}

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
    webController = WebController::GetInstance();
    notificationController = NotificationController::GetInstance();
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, LOW);
}

void IrrigationController::Setup()
{
    pinMode(valveOpenPin, OUTPUT);
    pinMode(valveClosePin, OUTPUT);

    pinMode(interruptWaterFlowTickPin, INPUT_PULLUP);
    pinMode(interruptValveOpenPin, INPUT);
    pinMode(interruptValveClosedPin, INPUT);

    pinMode(humidityInputPin, INPUT);
    pinMode(humidityPowerPin, OUTPUT);
    digitalWrite(humidityPowerPin, LOW);

    attachInterrupt(digitalPinToInterrupt(interruptWaterFlowTickPin), WaterFlowTickISR, FALLING);

    if (digitalRead(interruptValveClosedPin) != LOW)
    {
        CloseValveInt();
        notificationController->Alert("Unexpected valve in Open state while powering up. Attempting to close.");
        notificationController->Display("Valve error", "Valve is open");
    }
    lastTimeVolumeMeasured = millis();

    WebController::AddAction("/on", &IrrigationController::OpenValve);
    WebController::AddAction("/off", &IrrigationController::CloseValve);
    WebController::AddAction("/reset", &IrrigationController::Reset);
    WebController::AddAction("/set-params", &IrrigationController::SetParams);
    WebController::AddAction("/set-calendar", &IrrigationController::SetCalendar);
    WebController::AddAction("/clear-calendar", &IrrigationController::ClearCalendar);
    WebController::AddAction("/status", &IrrigationController::GetStatus);
    WebController::AddAction("/humidity", &IrrigationController::GetSHumidity);

    notificationController->Alert("Irrigation controller has started.");
    notificationController->Display("Ready.", "");
}

void IrrigationController::ProcesMainLoop()
{
    if (valveClosed && ((millis() - valveClosedChangedAt) > 300) && (digitalRead(interruptValveClosedPin) == LOW))
    {
        detachInterrupt(interruptValveClosedPin);
        valveClosed = false;
        Serial.println("Int - closed");
        ValveClosed();
    }
    if (valveOpen && ((millis() - valveOpenChangedAt) > 300) && (digitalRead(interruptValveOpenPin) == LOW))
    {
        detachInterrupt(interruptValveOpenPin);
        valveOpen = false;
        Serial.println("Int - open");
        ValveOpen();
    }
    if (currentState == State::Idle)
    {
        if (CheckStartTime())
        {
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
            notificationController->Alert(message);
            notificationController->Display("Ready", "Finished");
        }
        else if (CheckEndTime())
        {
            Chronos::DateTime n = Chronos::DateTime::now();
            CloseValveInt();
            snprintf(message, 150, "Watering aborted at %02d/%02d/%d %02d:%02d after %d minutes. Watering target of %d liters has not been reached. %d liters have been dispensed", n.day(), n.month(), n.year(), n.hour(), n.minute(), maxWateringTime, static_cast<int>(waterVolumeTarget), static_cast<int>(waterVolume));
            notificationController->Alert(message);
            notificationController->Display("Ready", "Aborted");
        }
        if (((millis() - lastTimeVolumeMeasured) > 30000) &&
            ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(2 * maxValveActionTime)))
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
                    notificationController->Alert("Insufficient waterflow.");
                }
            }
            if (CheckForNormalWaterFlow())
            {
                if (flowTooLow)
                {
                    flowTooLow = false;
                    notificationController->Alert("Waterflow is back to normal.");
                }
            }

            char msg1[20];
            char msg2[20];
            snprintf(msg1, 20, "Flow %d L/min", static_cast<int>(waterFlowRate));
            snprintf(msg2, 20, "%d min", (Chronos::DateTime::now() - stateChangedAt).totalSeconds() / 60);
            notificationController->Display(msg1, msg2);
        }
    }
    else if (currentState == State::OpeningValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            if (digitalRead(interruptValveOpenPin) != LOW)
            {
                notificationController->Alert("Unable to open Valve.");
                notificationController->Display("Valve Error", "Unable to open");
            }
            currentState = State::Watering; // This is the best guess
        }
    }
    else if (currentState == State::ClosingValve)
    {
        if ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Seconds(maxValveActionTime))
        {
            if (digitalRead(interruptValveClosedPin) != LOW)
            {
                notificationController->Alert("Unable to close Valve");
                notificationController->Display("Valve error", "Unable to close");
            }
            currentState = State::Idle; // This is the best guess
        }
    }
}

void IrrigationController::ValveOpen()
{
    if (currentState == State::OpeningValve)
    {
        notificationController->Alert("Watering has started.");
        notificationController->Display("Watering", "has started.");
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
        notificationController->Display("Watering", "has ended.");
        currentState = State::Idle;
        waterFlowRate = 0;
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
            else if ((*params).p.equalsIgnoreCase("duration"))
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
                                            Chronos::Mark::Daily(hour, minute, 0, freq),
                                            Chronos::Span::Minutes(1)));
        delete[] paramsToDelete; // deallocate
        paramsToDelete = nullptr;
        webController->SendHttpResponseOK("Watering Schedule set\n\n");
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

    static char response[512];
    memset(response, '\0', 512);
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

void IrrigationController::GetSHumidity()
{
    char resp[50];
    digitalWrite(humidityPowerPin, HIGH);
    delay(500);
    unsigned int humidity = analogRead(humidityInputPin);
    snprintf(resp, 50, "Humidity: %u\n\n", humidity);
    webController->SendHttpResponseOK(resp);

    digitalWrite(humidityPowerPin, LOW);
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
    const byte nEvents = 1;
    Chronos::Event::Occurrence eventArray[nEvents];

    Chronos::DateTime n = Chronos::DateTime::now();

    return (wateringCalendar.listOngoing(nEvents, eventArray, n) > 0);
}

bool IrrigationController::CheckEndTime()
{
    if (currentState != State::Watering)
        return false;
    else
    {
        return ((Chronos::DateTime::now() - stateChangedAt) > Chronos::Span::Minutes(maxWateringTime));
    }
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

void IrrigationController::CloseValveInt()
{
    currentState = State::ClosingValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, HIGH);

    attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosedISR, FALLING);
}

void IrrigationController::OpenValveInt()
{
    currentState = State::OpeningValve;
    stateChangedAt = Chronos::DateTime::now();
    digitalWrite(valveOpenPin, HIGH);
    digitalWrite(valveClosePin, LOW);

    attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpenISR, FALLING);
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
    snprintf(message, 250, "Current time is %02u/%02u/%u %02u:%02u\nCurrent state is %s\nCurrent flow is %d\nWatering target is %d liters\nMax watering duration is %d minutes\n", n.day(), n.month(), n.year(), n.hour(), n.minute(), GetCurrentState(), static_cast<int>(GetWaterFlow()), static_cast<int>(waterVolumeTarget), maxWateringTime);
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
