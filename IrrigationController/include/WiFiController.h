#ifndef WIFICONTROLLER_H
#define WIFICONTROLLER_H

#include <stdint.h>
#include <stddef.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>

#define timeServer "au.pool.ntp.org"
#define UDP_TIMEOUT 20000
#define NTP_PACKET_SIZE 48
#define timeZone 10

class WiFiController
{
public:
    static WiFiController *GetInstance();
    void Setup();
    void ProcessMainLoop();

private:
    WiFiController();
    static WiFiClient client;
    static WiFiController *theInstance;
    // WI-FI settings
    const char *ssid = "QBF";
    const char *pass = "!QbfReward00";

    static byte packetBuffer[];
    static WiFiUDP Udp;
    static Timezone ausET;

    static unsigned long previousMillis;
    static unsigned long interval;

    static unsigned int localPort; // local port to listen for UDP packets

    static void SendNTPpacket(const char *ntpSrv);
    static void PrintWifiStatus();
    static time_t GetNTPTime();
};

#endif
