#include "WiFiController.h"
#include "IrrigationController.h"

// Statics
WiFiController *WiFiController::theInstance = NULL;
unsigned int WiFiController::localPort = 2390; // local port to listen for UDP packets
byte WiFiController::packetBuffer[NTP_PACKET_SIZE];

unsigned long WiFiController::previousMillis = 0;
unsigned long WiFiController::interval = 60000;

WiFiUDP WiFiController::Udp;
// Australia Eastern Time Zone (Sydney, Melbourne)

TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660}; // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600}; // UTC + 10 hours
Timezone WiFiController::ausET(aEDT, aEST);

WiFiController *WiFiController::GetInstance()
{
    if (theInstance == NULL)
    {
        theInstance = new WiFiController();

        // returning the instance pointer
        return theInstance;
    }
    else
    {
        return theInstance;
    }
}

WiFiController::WiFiController()
{
}

void WiFiController::Setup()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        // Connect to WPA/WPA2 network
        delay(1000);
        Serial.print(".");
    }

    PrintWifiStatus();
    Udp.begin(localPort);

    setSyncProvider(GetNTPTime);
    setSyncInterval(SECS_PER_HOUR); // every hour
}

void WiFiController::ProcessMainLoop()
{
unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
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
            delay(10);
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
        Serial.print("No NTP Response. Received ");
        Serial.print(size);
        Serial.println(" byte(s)");
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
    NotificationController *n = NotificationController::GetInstance();
    n->Display("IP Address", ip.toString().c_str());
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
