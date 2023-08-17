#ifndef WIFICONTROLLER_H
#define WIFICONTROLLER_H

#include <stdint.h>
#include <stddef.h>
#include <WiFiESP.h>

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
const unsigned long noWiFiTimeLimit = 6 * 60 * 60 * 1000; // six hours

const uint8_t wifiConnecting = 3;
const uint8_t wifiConnected = 0;
const uint8_t wifiError = 1;

class WiFiController
{
    public:
        bool Setup();
        static WiFiController *GetInstance();
        WiFiController();

    private:
        WiFiClientSecure client;

        WiFiEventHandler wifiConnectHandler;
        WiFiEventHandler wifiDisconnectHandler;
        static void onWifiConnect(const WiFiEventStationModeGotIP &event);
        static void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
        // WI-FI settings
        const char *ssid = "QBF";
        const char *pass = "!QbfReward00";
        unsigned long lastTimeWiFiSuccess;
};

#endif

