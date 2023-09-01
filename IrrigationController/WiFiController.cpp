#include "WiFiController.h"

static WiFiController theInstance;

WiFiController* WiFiController::GetInstance()
{
    return &theInstance;
}

WiFiController::WiFiController()
{
    packetBuffer = new byte[NTP_PACKET_SIZE];
}

void WiFiController::Setup()
{
    WiFi.init(&Serial);
 

    // attempt to connect to WiFi network
    while (status != WL_CONNECTED)
    {
        status = WiFi.begin(ssid, pass);
        // Connect to WPA/WPA2 network
        delay(500);
        Serial.print(".");
    }
}

time_t WiFiController::GetNTPTime()
{
    while (Udp.parsePacket() > 0)
        ; // discard any previously received packets
    Serial.println("Transmit NTP Request");
    SendNTPpacket(timeServer);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500)
    {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            Serial.println("Receive NTP Response");
            Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return secsSince1900 - 2208988800UL + timeZone * 3600;
        }
    }
    Serial.println("No NTP Response :-(");
    return 0; // return 0 if unable to get the time
}

void WiFiController::Alert(const char *message)
{
}

void WiFiController::SendNTPpacket(const char *ntpSrv)
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)

    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(ntpSrv, 123); // NTP requests are to port 123

    Udp.write(packetBuffer, NTP_PACKET_SIZE);

    Udp.endPacket();
}
