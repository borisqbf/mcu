#include "WiFiController.h"
#include "IrrigationController.h"
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <Timezone.h>

SoftwareSerial Serial1(6, 7); // RX, TX

static WiFiController theInstance;
byte packetBuffer[NTP_PACKET_SIZE];
WiFiEspUDP Udp;
// Australia Eastern Time Zone (Sydney, Melbourne)
TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660}; // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600}; // UTC + 10 hours
Timezone ausET(aEDT, aEST);

WiFiController *WiFiController::GetInstance()
{
    return &theInstance;
}

WiFiController::WiFiController()
{
}

void WiFiController::Setup()
{
    Serial1.begin(9600);
    WiFi.init(&Serial1);

    if (WiFi.status() == WL_NO_SHIELD)
    {
        Serial.println("WiFi shield not present");
        // don't continue
        return;
    }
    // attempt to connect to WiFi network
    while (status != WL_CONNECTED)
    {
        status = WiFi.begin(ssid, pass);
        // Connect to WPA/WPA2 network
        delay(500);
        Serial.print(".");
    }
    Serial.println("You're connected to the network");
    Udp.begin(localPort);
    PrintWifiStatus();
    setSyncProvider(GetNTPTime);
    setSyncInterval(SECS_PER_HOUR); // every hour
}

time_t WiFiController::GetNTPTime()
{
    if (!IrrigationController::GetInstance()->IsIdle())
        return 0;
    else
    {
        Serial.println("Transmit NTP Request");
        SendNTPpacket(timeServer);
        unsigned long startMs = millis();
        while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT)
        {
        }

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
            const unsigned long seventyYears = 2208988800UL;
            unsigned long epoch = secsSince1900 - seventyYears;
            return ausET.toLocal(epoch);
        }
        Serial.println("No NTP Response");
        return 0; // return 0 if unable to get the time
    }
}

void WiFiController::PrintWifiStatus()
{
    // print the SSID of the network you're attached to
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
}
void WiFiController::Alert(const char *message)
{
}

void WiFiController::SendNTPpacket(const char *ntpSrv)
{
    memset(&packetBuffer, 0, NTP_PACKET_SIZE);
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
