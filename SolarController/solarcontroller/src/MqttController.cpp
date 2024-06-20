#include <ESP8266WiFi.h>
#include "MqttController.h"
#include "LEDStatusReporter.h"

static MqttController theInstance;

MqttController *MqttController::GetInstance()
{
    return &theInstance;
};

MqttController::MqttController()
{
    mqtt = nullptr;
    lastTimeTelemetrySent = 0;
    wifiController = WiFiController::GetInstance();
}

bool MqttController::PublishMessage(const char *msg)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!mqtt->connected())
        {
            if (!Connect())
                return false;
        }
        return mqtt->publish(family, msg);
    }
    else
    {
         return false;
    }
}

void MqttController::HandleMQTTError(int errorCode)
{
    switch (errorCode)
    {
    case MQTT_CONNECTION_TIMEOUT:
        Serial1.println("MQTT Connection timeout");
        LEDStatusReporter::ReportError(mqttConnectionTimeout);
        break;
    case MQTT_CONNECTION_LOST:
        Serial1.println("MQTT Connection lost");
        LEDStatusReporter::ReportError(mqttConnectionLost);
        break;
    case MQTT_CONNECT_FAILED:
        Serial1.println("MQTT Connect failed");
        LEDStatusReporter::ReportError(mqttConnectFailed);
        break;
    case MQTT_DISCONNECTED:
        Serial1.println("MQTT Disconnected");
        LEDStatusReporter::ReportError(mqttDisconnected);
        break;
    case MQTT_CONNECTED:
        Serial1.println("MQTT Connected");
        break;
    case MQTT_CONNECT_BAD_PROTOCOL:
        Serial1.println("MQTT Connect bad protocol. The server doesn't support the requested version of MQTT");
        LEDStatusReporter::ReportError(mqttConnectBadProtocol);
        break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
        Serial1.println("MQTT Connect bad client ID. The server rejected the client identifier");
        LEDStatusReporter::ReportError(mqttConnectBadClientId);
        break;
    case MQTT_CONNECT_UNAVAILABLE:
        Serial1.println("MQTT Connect unavailable. The server was unable to accept the connection");
        LEDStatusReporter::ReportError(mqttConnectUnavailable);
        break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
        Serial1.println("MQTT Connect bad credentials. The username/password were rejected");
        LEDStatusReporter::ReportError(mqttConnectBadCredentials);
        break;
    case MQTT_CONNECT_UNAUTHORIZED:
        Serial1.println("MQTT Connect unauthorized. The client was not authorized to connect");
        LEDStatusReporter::ReportError(mqttConnectUnauthorized);
        break;
    default:
        Serial1.println("Unknown error");
        LEDStatusReporter::ReportError(mqttUnknownError);
        break;
    }
}

bool MqttController::Connect()
{
    LittleFS.begin();
    randomSeed(micros());
    SetDateTime();
    // IP address could have been change - update mqtt client

    int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial1.printf("Number of CA certs read: %d\n", numCerts);
    if (numCerts == 0)
    {
        Serial1.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
        return false; // Can't connect to anything w/o certs!
    }

    BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
    // Integrate the cert store with this connection
    bear->setCertStore(&certStore);

    if (mqtt != nullptr)
        delete mqtt;
        
    mqtt = new PubSubClient(*bear);

    mqtt->setServer(mqttServer, mqttPort);
    mqtt->setKeepAlive(6 * mqttSendingPeriodicity / 1000);
    while (!mqtt->connected())
    {
        Serial1.println("Connecting to MQTT Server...");

        if (mqtt->connect(family, mqttUser, mqttPassword))
        {
            Serial1.println("Connected");
            PublishMessage("Renogy monitor connected");
            return true;
        }
        else
        {
            HandleMQTTError(mqtt->state());
            return false;
        }
    }
    return false;
}

void MqttController::SetDateTime()
{
    // You can use your own timezone, but the exact time is not used at all.
    // Only the date is needed for validating the certificates.
    configTime(TZ_Australia_Melbourne, "pool.ntp.org", "time.nist.gov");

    Serial1.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2)
    {
        delay(100);
        Serial1.print(".");
        now = time(nullptr);
    }
    Serial1.println();

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial1.printf("%s %s", tzname[0], asctime(&timeinfo));
}

bool MqttController::IsUpdateRequired()
{
    if ((millis() - lastTimeTelemetrySent) > mqttSendingPeriodicity)
    {
        lastTimeTelemetrySent = millis();
        return true;
    }
    else
        return false;
}
