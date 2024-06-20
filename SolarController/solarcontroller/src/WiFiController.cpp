#include "WiFiController.h"
#include "LEDStatusReporter.h"

static WiFiController theInstance;


WiFiController* WiFiController::GetInstance()
{
    return &theInstance;
};

WiFiController::WiFiController()
{
    previousMillis = 0;
    interval = 60000;
}

void WiFiController::onWifiConnect(const WiFiEventStationModeGotIP &event)
{
    Serial1.println("Connected to Wi-Fi sucessfully.");
    Serial1.print("IP address: ");
    Serial1.println(WiFi.localIP());
    LEDStatusReporter::ReportWiFi(wifiConnected);
}

void WiFiController::onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    LEDStatusReporter::ReportWiFi(wifiError);
    Serial1.println("Disconnected from Wi-Fi, trying to connect...");
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
        LEDStatusReporter::ReportWiFi(wifiConnecting);
        Serial1.print("*");
        delay(1000);
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        Serial1.println("Wi-Fi connection Successful");
        Serial1.print("The IP Address of ESP8266 Module is: ");
        Serial1.println(WiFi.localIP()); // Print the IP address
        LEDStatusReporter::ReportWiFi(wifiConnected);
        return true;
    }
    else
    {
        return false;
    }
}

void WiFiController::ProcessMainLoop()
{
    unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every interval seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}
