#ifndef WIFICONTROLLER_H
#define WIFICONTROLLER_H

#include <stdint.h>
#include <stddef.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ESP_Mail_Client.h>
#include <TimeLib.h>
#include <Timezone.h>

#define timeServer "time.google.com"
#define UDP_TIMEOUT 2000
#define NTP_PACKET_SIZE 48
#define timeZone 10

class WiFiController
{
public:
    void Setup();
    static WiFiController *GetInstance();
    WiFiController();
    void Alert(const char *message);

private:
    WiFiClient client;
    // WI-FI settings
    const char *ssid = "QBF";
    const char *pass = "!QbfReward00";
    int status = WL_IDLE_STATUS; // the Wifi radio's status

    unsigned int localPort = 2390; // local port to listen for UDP packets

    static void SendNTPpacket(const char *ntpSrv);
    static void PrintWifiStatus();
    static time_t GetNTPTime();
};

#endif
