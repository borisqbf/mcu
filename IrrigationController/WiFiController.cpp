#include "WiFiController.h"

static WiFiController theInstance;

WiFiController* WiFiController::GetInstance()
{
    return &theInstance;
};

WiFiController::WiFiController()
{
    lastTimeWiFiSuccess = 0;
}

void WiFiController::onWifiConnect(const WiFiEventStationModeGotIP &event)
{
    Serial.println("Connected to Wi-Fi sucessfully.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    theInstance.lastTimeWiFiSuccess = millis();
}

void WiFiController::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    Serial.println("Disconnected from Wi-Fi, trying to connect...");
    WiFi.disconnect();
    WiFi.begin(theInstance.ssid, theInstance.pass);
}

bool WiFiController::Setup()
{
    // Register event handlers
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    // max wait to connect 1 minute

    uint8_t i = 0;
    while ((WiFi.status() != WL_CONNECTED) && (i < 60))
    {
        i++;
        Serial.print("*");
        delay(1000);
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        Serial.println("Wi-Fi connection Successful");
        Serial.print("The IP Address of ESP8266 Module is: ");
        Serial.println(WiFi.localIP()); // Print the IP address
        return true;
    }
    else
    {
        return false;
    }
}
