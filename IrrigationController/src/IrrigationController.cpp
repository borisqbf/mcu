#include "IrrigationController.h"

// Statics
IrrigationController *IrrigationController::theInstance = NULL;
enum State IrrigationController::currentState = State::Idle;
bool IrrigationController::inSelfTestMode = false;
WebController *IrrigationController::webController = NULL;
NotificationController *IrrigationController::notificationController = NULL;
DateTime IrrigationController::stateChangedAt;
DateTime IrrigationController::startTime;

float IrrigationController::waterVolume = 0.0;
long IrrigationController::lastTimeVolumeMeasured = 0;
float IrrigationController::waterVolumeTarget = 200.0;
float IrrigationController::waterFlowRate = 0.0;
float IrrigationController::deltaWaterTankLeveThreshold = 2.0;
float IrrigationController::rainForecastThreshold = 5.0;
int IrrigationController::soilHumidityThreshold = 3000.0;

long IrrigationController::pulseCounter = 0;
bool IrrigationController::flowTooLow = false;
int IrrigationController::maxWateringTime = 20;  // minutes
int IrrigationController::wateringFrequency = 2; // every two days by default
int IrrigationController::waterTankLevel = 0;

// temp storage of irrigation parameters during self-test
DateTime IrrigationController::tmpStartTime;
float IrrigationController::tmpWaterVolumeTarget;
float IrrigationController::tmpDeltaWaterTankLeveThreshold;
float IrrigationController::tmpRainForecastThreshold;
int IrrigationController::tmpSoilHumidityThreshold;
int IrrigationController::tmpMaxWateringTime;

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
    startTime = DateTime(2099, 12, 12); // in a distant future
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
    int wl = GetWaterTankLevel();
    if (wl > 0)
        waterTankLevel = wl;

    WebController::AddAction("/on", &IrrigationController::OpenValve);
    WebController::AddAction("/off", &IrrigationController::CloseValve);
    WebController::AddAction("/reset", &IrrigationController::Reset);
    WebController::AddAction("/set-params", &IrrigationController::SetParams);
    WebController::AddAction("/set-calendar", &IrrigationController::SetCalendar);
    WebController::AddAction("/clear-calendar", &IrrigationController::ClearCalendar);
    WebController::AddAction("/status", &IrrigationController::GetStatus);
    WebController::AddAction("/humidity", &IrrigationController::GetHumidity);
    WebController::AddAction("/next", &IrrigationController::SkipToNext);
    WebController::AddAction("/skip-day", &IrrigationController::SkipDay);
    WebController::AddAction("/selftest", &IrrigationController::SelfTest);

    notificationController->Alert("Irrigation controller has started.");
    notificationController->Display("Ready.", "");
}

void IrrigationController::InializeFlow()
{
    waterVolume = 0.0;
    lastTimeVolumeMeasured = millis();
    waterFlowRate = 0.0;
    pulseCounter = 0;
    flowTooLow = false;
}

void IrrigationController::WaterFlowTick()
{
    pulseCounter++;
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
            int newTankLevel = GetWaterTankLevel();
            float rainfallExpected = GetRainForecast();
            int soilHumidity = GetHumidityImp();
            SkipReason reason = WateringRequired(newTankLevel, rainfallExpected, soilHumidity);
            if (reason == SkipReason::None)
            {
                SetNextStartTime();
                OpenValveInt();
            }
            else
            {
                if (reason == SkipReason::RainForecast)
                    SkipDayImp();
                else
                    SkipToNext();
                char message[150];
                strcpy(message, "No watering required. ");
                strcat(message, GetSkipReasonDescription(reason));
                strcat(message, " Next scheduled watering event: hh:mmAP on DD MMM YY");
                startTime.toString(message);

                notificationController->Alert(message);
                if (newTankLevel > 0)
                    waterTankLevel = newTankLevel;
            }
        }
    }
    else if (currentState == State::Watering)
    {
        static char message[150];
        if (CheckWateringTarget())
        {
            DateTime n(now());
            TimeSpan duration = n - stateChangedAt;
            CloseValveInt();

            snprintf(message, 150, "Watering finished at %02d/%02d/%d %02d:%02d.  Watering target of %d liters has been reached. The process took %d minutes.", n.day(), n.month(), n.year(), n.hour(), n.minute(), static_cast<int>(waterVolumeTarget), duration.totalseconds() / 60);
            notificationController->Alert(message);
            notificationController->Display("Ready", "Finished");
            if (inSelfTestMode)
            {
                EndSelfTest();
            }
        }
        else if (CheckEndTime())
        {
            DateTime n(now());
            CloseValveInt();
            snprintf(message, 150, "Watering aborted at %02d/%02d/%d %02d:%02d after %d minutes. Watering target of %d liters has not been reached. %d liters have been dispensed", n.day(), n.month(), n.year(), n.hour(), n.minute(), maxWateringTime, static_cast<int>(waterVolumeTarget), static_cast<int>(waterVolume));
            notificationController->Alert(message);
            notificationController->Display("Ready", "Aborted");
            if (inSelfTestMode)
            {
                EndSelfTest();
            }
        }
        if (((millis() - lastTimeVolumeMeasured) > 30000) &&
            ((DateTime(now()) - stateChangedAt).totalseconds() > 2 * maxValveActionTime))
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
            snprintf(msg1, 20, "Flow %d Tot. %d", static_cast<int>(waterFlowRate), static_cast<int>(waterVolume));
            snprintf(msg2, 20, "%d min", ((DateTime(now()) - stateChangedAt).totalseconds() / 60));
            notificationController->Display(msg1, msg2);
        }
    }
    else if (currentState == State::OpeningValve)
    {
        if ((DateTime(now()) - stateChangedAt).totalseconds() > maxValveActionTime)
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
        if ((DateTime(now()) - stateChangedAt).totalseconds() > maxValveActionTime)
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

void IrrigationController::Reset()
{
    currentState = State::Idle;
    InializeFlow();
}

void IrrigationController::SetParams()
{
    WebController::UrlQueryParameter *params = WebController::GetUrlQueryParams();
    if (params == nullptr)
        webController->SendHttpResponseOK("Missing Parameter. Available parameters are:\ntarget\nmax-duration\nrain-forecast\nsoil-humidity\ndelta-tank\n\n");
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
            else if ((*params).p.equalsIgnoreCase("max-duration"))
            {
                maxWateringTime = (*params).v.toInt();
                if (maxWateringTime < 1)
                {
                    webController->SendHttpResponseBadRequest("max-duration");
                    break;
                }
            }
            else if ((*params).p.equalsIgnoreCase("rain-forecast"))
            {
                rainForecastThreshold = (*params).v.toFloat();
                if (rainForecastThreshold < 0.0)
                {
                    webController->SendHttpResponseBadRequest("rain-forecast");
                    break;
                }
            }
            else if ((*params).p.equalsIgnoreCase("soil-humidity"))
            {
                soilHumidityThreshold = (*params).v.toInt();
                if (soilHumidityThreshold < 0.0)
                {
                    webController->SendHttpResponseBadRequest("soil-humidity");
                    break;
                }
            }
            else if ((*params).p.equalsIgnoreCase("delta-tank"))
            {
                deltaWaterTankLeveThreshold = (*params).v.toFloat();
                if (deltaWaterTankLeveThreshold < 0.0)
                {
                    webController->SendHttpResponseBadRequest("delta-tank");
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
        int mins = 0;
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
                mins = (*params).v.substring(columnIndex + 1).toInt();
                if ((hour < 0) || (hour > 24) || (mins < 0) || (mins > 60))
                {
                    webController->SendHttpResponseBadRequest("at");
                    break;
                }
            }
            else if ((*params).p.equalsIgnoreCase("every"))
            {
                wateringFrequency = (*params).v.toInt();
                if ((wateringFrequency < 1) || (wateringFrequency > 3))
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

        IrrigationController::SetNextStartTime(hour, mins);

        delete[] paramsToDelete; // deallocate
        paramsToDelete = nullptr;
        webController->SendHttpResponseOK("Watering Schedule set\n\n");
    }
}

void IrrigationController::ClearCalendar()
{
    startTime = DateTime(2099, 12, 12); // in a distant future
    webController->SendHttpResponseOK("OK\n\n");
}

void IrrigationController::GetStatus()
{
    static char response[512];
    memset(response, '\0', 512);
    strcat(response, GenerateStatusResponse());

    if (startTime >= DateTime(2099, 12, 12))
    {
        strcat(response, "No scheduled watering events found\n\n");
    }
    else
    {
        char eventInfo[] = "Next scheduled watering event: hh:mmAP on DD MMM YY\n\n";

        strcat(response, startTime.toString(eventInfo));
    }
    webController->SendHttpResponseOK(response);
}

int IrrigationController::GetHumidityImp()
{
    int h = 0;
    digitalWrite(humidityPowerPin, HIGH);
    delay(1000);
    for (int i = 0; i < 100; i++)
    {
        h += analogRead(humidityInputPin);
        delay(10);
    }
    return h / 100;
    digitalWrite(humidityPowerPin, LOW);
}

void IrrigationController::GetHumidity()
{
    static char resp[50];

    snprintf(resp, 50, "Humidity: %d\n\n", GetHumidityImp());
    webController->SendHttpResponseOK(resp);
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

const char *IrrigationController::GenerateStatusResponse()
{
    static char message[350];
    DateTime n(now());
    snprintf(message, 350, "Current time is %02u/%02u/%u %02u:%02u\nCurrent state: %s\nCurrent flow: %d l/min\nWatering target: %d liters\nMax watering duration: %d min\nDelta Watertamk level threshold: %.2f cm\nRain forecast threshold: %.2f mm\nSoil humidity threshold: %d\nDelta tank level: %d cm\nRain forecast: %.2f mm\nSoil humidity: %d\n", n.day(), n.month(), n.year(), n.hour(), n.minute(), GetCurrentState(), static_cast<int>(GetWaterFlow()), static_cast<int>(waterVolumeTarget), maxWateringTime, deltaWaterTankLeveThreshold, rainForecastThreshold, soilHumidityThreshold, waterTankLevel - GetWaterTankLevel(), GetRainForecast(), GetHumidityImp());

    Serial.println(message);
    return message;
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

const char *IrrigationController::GetSkipReasonDescription(SkipReason reason)
{
    switch (reason)
    {
    case SkipReason::RainBefore:
        return "Sufficient rainfall since last watering.";
    case SkipReason::RainForecast:
        return "Significant rainfall is expected in the next 24 hours.";
    case SkipReason::SoilHumidity:
        return "Soil moisture is sufficient.";
    default:
        return "Reason unknown.";
    }
}

float IrrigationController::GetWaterFlow()
{
    return waterFlowRate;
}

bool IrrigationController::CheckStartTime()
{
    DateTime n(now());
    return (n > startTime) && (startTime > n - TimeSpan(maxWateringTime * 60 * 2));
}

void IrrigationController::SetNextStartTime()
{
    startTime = startTime + TimeSpan(wateringFrequency, 0, 0, 0);
}

void IrrigationController::SkipToNextImp()
{
    startTime = startTime + TimeSpan(wateringFrequency, 0, 0, 0);
}

void IrrigationController::SkipToNext()
{
    SkipToNextImp();
    char response[] = "Next scheduled watering event: hh:mmAP on DD MMM YY\n\n";
    startTime.toString(response);
    webController->SendHttpResponseOK(response);
}

void IrrigationController::SkipDayImp()
{
    startTime = startTime + TimeSpan(1, 0, 0, 0);
}

void IrrigationController::SkipDay()
{
    SkipDayImp();
    char response[] = "Next scheduled watering event: hh:mmAP on DD MMM YY\n\n";
    startTime.toString(response);
    webController->SendHttpResponseOK(response);
}

void IrrigationController::SelfTest()
{
    inSelfTestMode = true;

    // temp storage of irrigation parameters during self-test
    tmpStartTime = startTime;
    tmpWaterVolumeTarget = waterVolumeTarget;
    tmpDeltaWaterTankLeveThreshold = deltaWaterTankLeveThreshold;
    tmpRainForecastThreshold = rainForecastThreshold;
    tmpSoilHumidityThreshold = soilHumidityThreshold;
    tmpMaxWateringTime = maxWateringTime;

    waterVolumeTarget = 10; // Displense 10 liters in max 3 minutes
    maxWateringTime = 3;

    // ensure watering start conditions
    deltaWaterTankLeveThreshold = 300;
    rainForecastThreshold = 100;
    soilHumidityThreshold = 10000;
    DateTime n(now());
    startTime = n + TimeSpan(20);
    webController->SendHttpResponseOK("Self-Test started");
}

void IrrigationController::EndSelfTest()
{
    inSelfTestMode = false;

    // restore saved irrigation params
    startTime = tmpStartTime;
    waterVolumeTarget = tmpWaterVolumeTarget;
    deltaWaterTankLeveThreshold = tmpDeltaWaterTankLeveThreshold;
    rainForecastThreshold = tmpRainForecastThreshold;
    soilHumidityThreshold = tmpSoilHumidityThreshold;
    maxWateringTime = tmpMaxWateringTime;

    notificationController->Alert("Self-Test is complete");
}

void IrrigationController::SetNextStartTime(int hour, int minute)
{
    DateTime n(now());
    startTime = DateTime(n.year(), n.month(), n.day(), hour, minute);
    if (startTime < n)
    {
        startTime = startTime + TimeSpan(1, 0, 0, 0);
    }
}

bool IrrigationController::CheckEndTime()
{
    if (currentState != State::Watering)
        return false;
    else
    {
        return ((DateTime(now()) - stateChangedAt).totalseconds() > maxWateringTime * 60);
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
    return waterVolumeTarget <= waterVolume;
}

void IrrigationController::ValveOpen()
{
    if (currentState == State::OpeningValve)
    {
        notificationController->Alert("Watering has started.");
        notificationController->Display("Watering", "has started.");
        currentState = State::Watering;
        stateChangedAt = DateTime(now());
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
        stateChangedAt = DateTime(now());
        digitalWrite(valveOpenPin, LOW);
        digitalWrite(valveClosePin, LOW);
        int wl = GetWaterTankLevel();
        if (wl > 0)
            waterTankLevel = wl;
    }
}
int IrrigationController::GetWaterTankLevel()
{
    return webController->GetWaterTankLevel();
}

float IrrigationController::GetRainForecast()
{
    DateTime n(now());
    DateTime noon(n.year(), n.month(), n.day(), 12, 0, 0);
    return webController->GetRainForecast(n > noon);
}

SkipReason IrrigationController::WateringRequired(int newWaterTankLevel, float rainfallExpected, int soilHumidity)
{
    if (newWaterTankLevel < 0)
        return SkipReason::None;
    else if (((newWaterTankLevel + deltaWaterTankLeveThreshold) < waterTankLevel) && newWaterTankLevel < 180 && waterTankLevel < 180)
        return SkipReason::RainBefore; // considerable rainfall since last watering. Level readings above 180 are unreliable
    else if (rainfallExpected > rainForecastThreshold)
        return SkipReason::RainForecast;
    else if (soilHumidity > soilHumidityThreshold)
        return SkipReason::SoilHumidity;
    else
        return SkipReason::None;
}

void IrrigationController::CloseValveInt()
{
    currentState = State::ClosingValve;
    stateChangedAt = DateTime(now());
    digitalWrite(valveOpenPin, LOW);
    digitalWrite(valveClosePin, HIGH);

    attachInterrupt(digitalPinToInterrupt(interruptValveClosedPin), ValveClosedISR, FALLING);
}

void IrrigationController::OpenValveInt()
{
    currentState = State::OpeningValve;
    stateChangedAt = DateTime(now());
    digitalWrite(valveOpenPin, HIGH);
    digitalWrite(valveClosePin, LOW);

    attachInterrupt(digitalPinToInterrupt(interruptValveOpenPin), ValveOpenISR, FALLING);
}
