#include <ESPmDNS.h>
#include <TimeLib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DateTimeLib.h"
#include "WebController.h"


// Static members
WebController *WebController::theInstance = NULL;
WebController::Route WebController::routes[MAX_ROUTES];
int WebController::nextRouteIndex = 1; // one for root
WebServer *WebController::server = NULL;

WebController::WebController()
{
    server = new WebServer(80);
    routes[0] = {"/", &WebController::HandleRoot};
    for (int i = 1; i < MAX_ROUTES; i++)
    {
        routes[i].url = nullptr;
        routes[i].handler = nullptr;
    }
}

WebController *WebController::GetInstance()
{
    if (theInstance == NULL)
    {
        theInstance = new WebController();

        // returning the instance pointer
        return theInstance;
    }
    else
    {
        return theInstance;
    }
}

void WebController::HandleRoot()
{
    String message = "Hello resolved by mDNS!\n\n";
    message += "\nAvailable routes are:\n";
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == nullptr)
            break;
        message += routes[i].url;
        message += "\n";
    }

    server->send(200, "text/plain", message);
}

void WebController::HandleNotFound()
{
    String message = "Rote Not Found\n\n";
    message += "URI: ";
    message += server->uri();
    message += "\nMethod: ";
    message += (server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server->args();
    message += "\n";
    for (uint8_t i = 0; i < server->args(); i++)
    {
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    message += "\nAvailable routes are:\n";
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == nullptr)
            break;
        message += routes[i].url;
        message += "\n";
    }

    DateTime n(now());
    message += "Current time is ";
    message += n.day();
    message += "/";
    message += n.month();
    message += "/";
    message += n.year();
    message += " ";
    message += n.hour();
    message += ":";
    if (n.minute() < 10)
        message += "0";
    message += n.minute();
    message += "\n";

    server->send(404, "text/plain", message);
}

void WebController::AddAction(const char *url, WebServer::THandlerFunction action)
{
    if (nextRouteIndex < MAX_ROUTES)
    {
        routes[nextRouteIndex++] = {url, action};
        server->on(url, action);
    }
    else
        Serial.println("No more route slots available");
}

WebController::UrlQueryParameter *WebController::GetUrlQueryParams()
{
    if (server->args() == 0)
        return NULL;
    else
    {
        WebController::UrlQueryParameter *retVal = new WebController::UrlQueryParameter[server->args() + 1];
        uint8_t i = 0;
        for (i = 0; i < server->args(); i++)
        {
            retVal[i] = {server->argName(i), server->arg(i)};
        }
        retVal[i] = {"", ""};
        return retVal;
    }
}

void WebController::Setup()
{
    if (MDNS.begin("irrigation-controller"))
    { // Start mDNS

        Serial.println("MDNS started");
    }

    server->onNotFound(HandleNotFound);

    server->begin(); // Start server
    Serial.println("HTTP server started");
}

void WebController::ProcessMainLoop()
{
    server->handleClient();
    delay(2); // allow the cpu to switch to other tasks
}

void WebController::SendHttpResponseOK(const char *message)
{
    server->send(200, "text/plain", message);
}

void WebController::SendHttpResponseBadRequest(const char *message)
{
    server->send(200, "text/plain", message);
}

int WebController::GetWaterTankLevel()
{
    HTTPClient http;
    http.begin(TANK_LEVEL_URL); // HTTP
    int retVal = -1;
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            Serial.println(payload);
            retVal = payload.toInt();
            if (retVal == 0)
                retVal = -1;
        }
        else
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET from %s code: %d\n", TANK_LEVEL_URL, httpCode);
        }
    }
    else
    {
        Serial.printf("[HTTP] GET from %s failed, error: %s\n", TANK_LEVEL_URL, http.errorToString(httpCode).c_str());
    }

    http.end();
    return retVal;
}

float WebController::GetRainForecast()
{
    HTTPClient http;
    http.begin(WEATHER_FORECAST_URL); // HTTP
    float retVal = 0;
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            StaticJsonDocument<0> filter;
            filter.set(true);

            DynamicJsonDocument doc(24576);

            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

            if (error)
            {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return 0;
            }

            JsonObject forecast = doc["forecast"]["forecastday"][0];
            retVal = forecast["totalprecip_mm"];
        }
        else
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET from %s code: %d\n", WEATHER_FORECAST_URL, httpCode);
        }
    }
    else
    {
        Serial.printf("[HTTP] GET from %s failed, error: %s\n", WEATHER_FORECAST_URL, http.errorToString(httpCode).c_str());
    }

    http.end();
    return retVal;
}
