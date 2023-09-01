#ifndef WIFICONTROLLER_H
#define WIFICONTROLLER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>

class WiFiController
{
    public:
        void Setup();
        static WiFiController *GetInstance();
        WiFiController();
        time_t GetNTPTime();
        void Alert(const char*message);

    private:
        WiFiEspClient client;
        // WI-FI settings
        const char *ssid = "QBF";
        const char *pass = "!QbfReward00";
        int status = WL_IDLE_STATUS; // the Wifi radio's status
        const int timeZone = 10;
        const char *timeServer = "au.pool.ntp.org"; // NTP server
        unsigned int localPort = 2390;            // local port to listen for UDP packets

        const int NTP_PACKET_SIZE = 48; // NTP timestamp is in the first 48 bytes of the message
        const int UDP_TIMEOUT = 2000;   // timeout in miliseconds to wait for an UDP packet to arrive

        byte *packetBuffer = NULL; // buffer to hold incoming and outgoing packets
        WiFiEspUDP Udp;

        void SendNTPpacket(const char *ntpSrv);
        void PrintWifiStatus();
};

#endif

