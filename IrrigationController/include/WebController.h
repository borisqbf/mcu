#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WebServer.h>
#include <Arduino.h>
#include <WiFi.h>

#define MAX_ROUTES 15

#define TANK_LEVEL_URL "http://rain-tank-level.local/api/level-abs"
#define WEATHER_FORECAST_URL "http://api.weatherapi.com/v1/forecast.json?key=db03e9230fea4859a9c74636233110&q=-37.9157,145.0160&days=1&aqi=no&alerts=no"

class WebController
{
public:
    struct Route
    {
        const char *url;
        WebServer::THandlerFunction handler;
    };

    struct UrlQueryParameter
    {
        String p;
        String v;
    };

public:
    void Setup();
    void ProcessMainLoop();
    static WebController *GetInstance();
    static void AddAction(const char *url, WebServer::THandlerFunction action);
    static UrlQueryParameter *GetUrlQueryParams();
    void SendHttpResponseOK(const char *message);
    void SendHttpResponseBadRequest(const char *message);
    int GetWaterTankLevel();
    float GetRainForecast();

private:
    WebController();
    static WebServer *server;

    static void HandleRoot();
    static void HandleNotFound();
    static WebController *theInstance;
    static Route routes[MAX_ROUTES];
    static int nextRouteIndex; // one for root
};

#endif