#ifndef MQTTCONTROLLER_H
#define MQTTCONTROLLER_H

#include <stdint.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>

#include "WiFiController.h"

const int mqttSendingPeriodicity = 3 * 60 * 1000; // in minutes

const int mqttPort = 8883;

const uint8_t mqttUnknownError = 20;
const uint8_t mqttConnectionTimeout = 21;
const uint8_t mqttConnectionLost = 22;
const uint8_t mqttConnectFailed = 23;
const uint8_t mqttDisconnected = 24;
const uint8_t mqttConnected = 25;
const uint8_t mqttConnectBadProtocol = 26;
const uint8_t mqttConnectBadClientId = 27;
const uint8_t mqttConnectUnavailable = 28;
const uint8_t mqttConnectBadCredentials = 29;
const uint8_t mqttConnectUnauthorized = 30;

class MqttController
{
public:
    MqttController();
    bool Connect();

    bool PublishMessage(const char *msg);
    void ProcessMainLoop() { mqtt->loop(); };
    void HandleMQTTError(){HandleMQTTError(mqtt->state());};
    bool IsUpdateRequired();
    static MqttController *GetInstance();

private:
    PubSubClient *mqtt;
    void HandleMQTTError(int errorCode);
    void SetDateTime();
    // A single, global CertStore which can be used by all connections.
    // Needs to stay live the entire time any of the WiFiClientBearSSLs
    // are present.
    BearSSL::CertStore certStore;

    const char *mqttServer = "299d6fc93f0945089400ce1c143e1ebb.s2.eu.hivemq.cloud";

    const char *mqttUser = "boris_qbf";
    const char *mqttPassword = "mqttReward00";
    const char *family = "solar";
    unsigned long lastTimeTelemetrySent;
    WiFiController *wifiController;
};
#endif
