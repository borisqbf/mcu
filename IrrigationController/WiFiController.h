#ifndef WIFICONTROLLER_H
#define WIFICONTROLLER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>

#define timeServer "au.pool.ntp.org"
#define UDP_TIMEOUT 2000
#define NTP_PACKET_SIZE 48
#define timeZone 10

class WiFiController
{
    public:
        void Setup();
        static WiFiController *GetInstance();
        WiFiController();
        void Alert(const char*message);

    private:
        WiFiEspClient client;
        // WI-FI settings
        const char *ssid = "QBF";
        const char *pass = "!QbfReward00";
        int status = WL_IDLE_STATUS; // the Wifi radio's status


        unsigned int localPort = 2390;            // local port to listen for UDP packets

        static void SendNTPpacket(const char *ntpSrv);
        static void PrintWifiStatus();
        static time_t GetNTPTime();
};

#endif

