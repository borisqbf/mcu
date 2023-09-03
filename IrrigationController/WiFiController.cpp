#include "WiFiController.h"
#include "SoftwareSerial.h"
#include <TimeLib.h>
SoftwareSerial Serial1(6, 7); // RX, TX

static WiFiController theInstance;
byte packetBuffer[NTP_PACKET_SIZE];
WiFiEspUDP Udp;

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
    setSyncInterval(3600); //every hour
}

time_t WiFiController::GetNTPTime()
{
    Serial.println("Transmit NTP Request");
    SendNTPpacket(timeServer);
    unsigned long startMs = millis();
    while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT)
    {
    }

    Serial.println(Udp.parsePacket());
    if (Udp.parsePacket())
    {
        Serial.println("packet received");
        // We've received a packet, read the data from it into the buffer
        Udp.read(packetBuffer, NTP_PACKET_SIZE);

        // the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        Serial.print("Seconds since Jan 1 1900 = ");
        Serial.println(secsSince1900);

        // now convert NTP time into everyday time:
        Serial.print("Unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        // print Unix time:
        Serial.println(epoch);

        // print the hour, minute and second:
        Serial.print("The UTC time is ");      // UTC is the time at Greenwich Meridian (GMT)
        Serial.print((epoch % 86400L) / 3600); // print the hour (86400 equals secs per day)
        Serial.print(':');
        if (((epoch % 3600) / 60) < 10)
        {
            // In the first 10 minutes of each hour, we'll want a leading '0'
            Serial.print('0');
        }
        Serial.print((epoch % 3600) / 60); // print the minute (3600 equals secs per minute)
        Serial.print(':');
        if ((epoch % 60) < 10)
        {
            // In the first 10 seconds of each minute, we'll want a leading '0'
            Serial.print('0');
        }
        Serial.println(epoch % 60); // print the second
    }
    // wait ten seconds before asking for the time again
    delay(10000);
    return 0; // return 0 if unable to get the time
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
