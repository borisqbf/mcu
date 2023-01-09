#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>

#include <ModbusMaster.h>

#pragma region general definitions
const uint8_t TOGGLE_PIN = 14;
const uint8_t MANUAL_PIN = 16;
const uint8_t MODE_PIN = 12;
const uint8_t RELAY_PIN = 13;
const uint8_t RED_LED_PIN = 15;
const uint8_t BLUE_LED_PIN = 5;
const uint8_t GREEN_LED_PIN = 0;

enum LedColour : byte
{
  RED,
  BLUE,
  GREEN
};
enum PowerMode : byte
{
  OnSolar = 0,
  OnMain = 1
};

PowerMode mode = OnSolar;
float OnMainThreshold = 10.5;
float OnSolarThreshold = 12.0;
unsigned long lastPressed = 0;
#pragma endregion

#pragma region ModBus -Renogy

const uint32_t numDataRegisters = 35;
const uint32_t numInfoRegisters = 17;

// A struct to hold the controller data
struct ControllerData
{
  uint8_t batteryCapacitySoc;            // percent
  float batteryVoltage;                  // volts
  float batteryChargingCurrent;          // amps
  uint8_t batteryTemperature;            // celcius
  uint8_t controllerTemperature;         // celcius
  float loadVoltage;                     // volts
  float loadCurrent;                     // amps
  uint8_t loadPower;                     // watts
  float panelVoltage;                    // volts
  float panelCurrent;                    // amps
  uint8_t panelPower;                    // watts
  float minBatteryVoltageToday;          // volts
  float maxBatteryVoltageToday;          // volts
  float maxChargingCurrentToday;         // amps
  float maxDischargingCurrentToday;      // amps
  uint8_t maxChargePowerToday;           // watts
  uint8_t maxDischargePowerToday;        // watts
  uint8_t chargeAmphoursToday;           // amp hours
  uint8_t dischargeAmphoursToday;        // amp hours
  uint8_t powerGenerationToday;          // watt hours
  uint8_t powerConsumptionToday;         // watt hours
  uint8_t totalNumOperatingDays;         // days
  uint8_t totalNumBatteryOverDischarges; // count
  uint8_t totalNumBatteryFullCharges;    // count
  uint32_t totalChargingAmphours;        // amp hours
  uint32_t totalDischargingAmphours;     // amp hours
  uint32_t cumulativePowerGeneration;    // kW hours
  uint32_t cumulativePowerConsumption;   // kW hours
  uint8_t loadState;                     // on-off, street light brightness
  uint8_t chargingState;
  uint16_t controllerFaultsHi;
  uint16_t controllerFaultsLo; // Reserved

  String toJSON()
  {
    char buffer[500];
    sprintf(buffer, "{\n\"BatteryVoltage\":\"%f\",\n\"BatteryChargingCurrent\":\"%f\",\n\"PanelVoltage\":\"%f\",\n\"PanelCurrent\":\"%f\"\n }\n",
            batteryVoltage, batteryChargingCurrent, panelVoltage, panelCurrent);
    return (String(buffer));
  }
};

// A struct to hold the controller info params
struct ControllerInfo
{
  uint8_t maxSupportedVltage;       // volts
  uint8_t chargingCurrentRating;    // amps
  uint8_t dischargingCurrentRating; // amps
  uint8_t productType;
  char productModel[17];
  char softwareVersion[5];
  char hardwareVersion[5];
  char serialNumber[5];
  uint8_t modbusAddress;
};

ControllerInfo renogyInfo;
ControllerData renogyData;

const uint8_t modbusAddress = 255;
const int modbusBaudRate = 9600;

const int modbusPollingPeriodicity = 2 * 60 * 1000; // report state every xx minutes
unsigned long lastTimeRenogyPolled = 0;

ModbusMaster node;

#pragma endregion

#pragma region WiFi
// WI-FI settings
char msg[50];
const char *ssid = "QBF";          // your network SSID (name)
const char *pass = "!QbfReward00"; // your network password

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClientSecure client;
#pragma endregion

#pragma region MQTT

const char *mqttServer = "299d6fc93f0945089400ce1c143e1ebb.s2.eu.hivemq.cloud";
const int mqttPort = 8883;

const int mqttSendingPeriodicity = 3 * 60 * 1000; // in minutes
unsigned long lastTimeTelemetrySent = 0;

// MQTT Settings
const char *mqttUser = "boris_qbf";
const char *mqttPassword = "mqttReward00";
const char *family = "solar";
String outTopic(family);

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

PubSubClient *mqtt;

#pragma endregion

#pragma region Error Codes and Statuses

const uint8_t wifiConnecting = 3;
const uint8_t wifiConnected = 0;
const uint8_t wifiError = 1;

const uint8_t modbusUnknownError = 10;
const uint8_t modbusIllegalDataAddressError = 10;
const uint8_t modbusIllegalDataValueError = 11;
const uint8_t modbusSlaveDeviceFailure = 12;
const uint8_t modbusIllegalFunctionError = 13;
const uint8_t modbusnvalidSlaveIDError = 14;
const uint8_t modbusInvalidFunction = 15;
const uint8_t modbusResponseTimedOutError = 16;
const uint8_t modbusInvalidCRCError = 17;

const uint8_t modbusReadOK = 1;
const uint8_t mqttSentOK = 3;

#pragma endregion

void Blink(LedColour c)
{
  uint8_t pin = RED_LED_PIN;
  if (c == BLUE)
    pin = BLUE_LED_PIN;
  else if (c == GREEN)
    pin = GREEN_LED_PIN;

  digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
  delay(300);
  digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
  delay(500);
}

void LEDOnOff(LedColour c, bool on = true)
{
  uint8_t pin = RED_LED_PIN;
  if (c == BLUE)
    pin = BLUE_LED_PIN;
  else if (c == GREEN)
    pin = GREEN_LED_PIN;
  if (on)
    digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
  else
    digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
}

void ReportError(uint8_t errorCode)
{
  for (uint8_t i = 0; i < errorCode; i++)
  {
    Blink(RED);
  }
  LEDOnOff(RED);
}

void ReportStatus(uint8_t status)
{
  for (uint8_t i = 0; i < status; i++)
  {
    Blink(GREEN);
  }
}

void ReportWiFi(uint8_t status)
{
  if (status == 0)
    LEDOnOff(BLUE);
  else if (status == 1)
    LEDOnOff(BLUE, false);
  else
  {
    for (uint8_t i = 0; i < status; i++)
    {
      Blink(BLUE);
    }
  }
}

void ToSolarMode()
{
  if (mode == OnSolar)
    return;
  mode = OnSolar;
  digitalWrite(MODE_PIN, HIGH);
  digitalWrite(RELAY_PIN, HIGH);
}

void ToMainMode()
{
  if (mode == OnMain)
    return;
  mode = OnMain;
  digitalWrite(MODE_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
}

void ToggleMode()
{
  if (mode == OnSolar)
  {
    ToMainMode();
  }
  else
  {
    ToSolarMode();
  }
}

void ICACHE_RAM_ATTR IntCallback()
{
  long int m = millis();
  Serial1.printf("Got interupt. Millis %ld\n", m);
  if (digitalRead(MANUAL_PIN))
  {
    if (millis() - lastPressed > 400)
    {
      ToggleMode();
    }
    lastPressed = millis();
  }
}

void HandleMQTTError(int errorCode)
{
  switch (errorCode)
  {
  case MQTT_CONNECTION_TIMEOUT:
    Serial1.println("MQTT Connection timeout");
    ReportError(mqttConnectionTimeout);
    break;
  case MQTT_CONNECTION_LOST:
    Serial1.println("MQTT Connection lost");
    ReportError(mqttConnectionLost);
    break;
  case MQTT_CONNECT_FAILED:
    Serial1.println("MQTT Connect failed");
    ReportError(mqttConnectFailed);
    break;
  case MQTT_DISCONNECTED:
    Serial1.println("MQTT Disconnected");
    ReportError(mqttDisconnected);
    break;
  case MQTT_CONNECTED:
    Serial1.println("MQTT Connected");
    break;
  case MQTT_CONNECT_BAD_PROTOCOL:
    Serial1.println("MQTT Connect bad protocol. The server doesn't support the requested version of MQTT");
    ReportError(mqttConnectBadProtocol);
    break;
  case MQTT_CONNECT_BAD_CLIENT_ID:
    Serial1.println("MQTT Connect bad client ID. The server rejected the client identifier");
    ReportError(mqttConnectBadClientId);
    break;
  case MQTT_CONNECT_UNAVAILABLE:
    Serial1.println("MQTT Connect unavailable. The server was unable to accept the connection");
    ReportError(mqttConnectUnavailable);
    break;
  case MQTT_CONNECT_BAD_CREDENTIALS:
    Serial1.println("MQTT Connect bad credentials. The username/password were rejected");
    ReportError(mqttConnectBadCredentials);
    break;
  case MQTT_CONNECT_UNAUTHORIZED:
    Serial1.println("MQTT Connect unauthorized. The client was not authorized to connect");
    ReportError(mqttConnectUnauthorized);
    break;
  default:
    Serial1.println("Unknown error");
    ReportError(mqttUnknownError);
    break;
  }
}

bool Connect2Mqtt()
{
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

  mqtt = new PubSubClient(*bear);

  mqtt->setServer(mqttServer, mqttPort);
  mqtt->setKeepAlive(2 * mqttSendingPeriodicity / 1000);
  while (!mqtt->connected())
  {
    Serial1.println("Connecting to MQTT Server...");
    String clientId(family);
    clientId += String(" - ");
    clientId += String(random(0xff), HEX);

    if (mqtt->connect(clientId.c_str(), mqttUser, mqttPassword))
    {
      Serial1.println("Connected");
      mqtt->publish(outTopic.c_str(), "Renogy monitor connected");
      return true;
    }
    else
    {
      HandleMQTTError(mqtt->state());
      return false;
    }
  }
}

void SetupWiFiClient()
{
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    ReportWiFi(wifiConnecting);
    Serial1.print("*");
  }

  Serial1.println("WiFi connection Successful");
  Serial1.print("The IP Address of ESP8266 Module is: ");
  Serial1.println(WiFi.localIP()); // Print the IP address
  ReportWiFi(wifiConnected);
}

void InitLEDs()
{
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(MODE_PIN, HIGH); // Initial state is on solar so green LED is on
}

void SetDateTime()
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

void SetupModbus()
{
  node.begin(modbusAddress, Serial);
}

void HandleModbusError(uint8_t errorCode)
{
  switch (errorCode)
  {
  case node.ku8MBIllegalDataAddress:
    Serial1.println("Illegal data address");
    ReportError(modbusIllegalDataAddressError);
    break;
  case node.ku8MBIllegalDataValue:
    Serial1.println("Illegal data value");
    ReportError(modbusIllegalDataValueError);
    break;
  case node.ku8MBIllegalFunction:
    Serial1.println("Illegal function");
    ReportError(modbusIllegalFunctionError);
    break;
  case node.ku8MBSlaveDeviceFailure:
    Serial1.println("Slave device failure");
    ReportError(modbusSlaveDeviceFailure);
    break;
  case node.ku8MBSuccess:
    Serial1.println("Success");
    break;
  case node.ku8MBInvalidSlaveID:
    Serial1.println("Invalid slave ID: The slave ID in the response does not match that of the request.");
    ReportError(modbusnvalidSlaveIDError);
    break;
  case node.ku8MBInvalidFunction:
    Serial1.println("Invalid function: The function code in the response does not match that of the request.");
    ReportError(modbusInvalidFunction);
    break;
  case node.ku8MBResponseTimedOut:
    Serial1.println("Response timed out");
    ReportError(modbusResponseTimedOutError);
    break;
  case node.ku8MBInvalidCRC:
    Serial1.println("InvalidCRC");
    ReportError(modbusInvalidCRCError);
    break;
  default:
    Serial1.println("Unknown error");
    ReportError(modbusUnknownError);
    break;
  }
}

void RenogyReadDataRegisters()
{
  uint8_t result;
  uint16_t dataRegisters[numDataRegisters];
  char buffer1[40], buffer2[40];
  uint8_t rawData;

  result = node.readHoldingRegisters(0x100, numDataRegisters);
  if (result != node.ku8MBSuccess)
  {
    HandleModbusError(result);
    renogyData.batteryCapacitySoc = 0;
    renogyData.batteryVoltage = 0;
    renogyData.batteryChargingCurrent = 0;
    renogyData.controllerTemperature = 0;
    renogyData.batteryTemperature = 0;
    renogyData.loadVoltage = 0;
    renogyData.loadCurrent = 0;
    renogyData.loadPower = 0;
    renogyData.panelVoltage = 0;
    renogyData.panelCurrent = 0;
    renogyData.panelPower = 0;
    renogyData.minBatteryVoltageToday = 0;
    renogyData.maxBatteryVoltageToday = 0;
    renogyData.maxChargingCurrentToday = 0;
    renogyData.maxDischargingCurrentToday = 0;
    renogyData.maxChargePowerToday = 0;
    renogyData.maxDischargePowerToday = 0;
    renogyData.chargeAmphoursToday = 0;
    renogyData.dischargeAmphoursToday = 0;
    renogyData.powerGenerationToday = 0;
    renogyData.powerConsumptionToday = 0;
    renogyData.totalNumOperatingDays = 0;
    renogyData.totalNumBatteryOverDischarges = 0;
    renogyData.totalNumBatteryFullCharges = 0;
    renogyData.loadState = 0;
    renogyData.chargingState = 0;
    renogyData.controllerFaultsHi = 0;
    renogyData.controllerFaultsLo = 0;
    renogyData.totalChargingAmphours = 0;
    renogyData.totalDischargingAmphours = 0;
    renogyData.cumulativePowerGeneration = 0;
    renogyData.cumulativePowerConsumption = 0;
  }
  else
  {
    for (uint8_t i = 0; i < numDataRegisters; i++)
    {
      dataRegisters[i] = node.getResponseBuffer(i);
    }

    renogyData.batteryCapacitySoc = dataRegisters[0];
    renogyData.batteryVoltage = dataRegisters[1] * 0.1;
    renogyData.batteryChargingCurrent = dataRegisters[2] * 0.01;

    // 0x103 returns two bytes, one for battery and one for controller temp in c
    uint16_t rawData = dataRegisters[3]; // eg 5913
    renogyData.controllerTemperature = rawData / 256;
    renogyData.batteryTemperature = rawData % 256;

    renogyData.loadVoltage = dataRegisters[4] * 0.1;
    renogyData.loadCurrent = dataRegisters[5] * 0.01;
    renogyData.loadPower = dataRegisters[6];
    renogyData.panelVoltage = dataRegisters[7] * 0.1;
    renogyData.panelCurrent = dataRegisters[8] * 0.01;
    renogyData.panelPower = dataRegisters[9];
    // Register 0x10A - Turn on load, write register - 10
    renogyData.minBatteryVoltageToday = dataRegisters[11] * 0.1;
    renogyData.maxBatteryVoltageToday = dataRegisters[12] * 0.1;
    renogyData.maxChargingCurrentToday = dataRegisters[13] * 0.01;
    renogyData.maxDischargingCurrentToday = dataRegisters[14] * 0.01;
    renogyData.maxChargePowerToday = dataRegisters[15];
    renogyData.maxDischargePowerToday = dataRegisters[16];
    renogyData.chargeAmphoursToday = dataRegisters[17];
    renogyData.dischargeAmphoursToday = dataRegisters[18];
    renogyData.powerGenerationToday = dataRegisters[19];
    renogyData.powerConsumptionToday = dataRegisters[20];
    renogyData.totalNumOperatingDays = dataRegisters[21];
    renogyData.totalNumBatteryOverDischarges = dataRegisters[22];
    renogyData.totalNumBatteryFullCharges = dataRegisters[23];

    memcpy(&renogyData.totalChargingAmphours, &(dataRegisters[24]), 4);
    memcpy(&renogyData.totalDischargingAmphours, &(dataRegisters[26]), 4);
    memcpy(&renogyData.cumulativePowerGeneration, &(dataRegisters[28]), 4);
    memcpy(&renogyData.cumulativePowerConsumption, &(dataRegisters[30]), 4);

    rawData = dataRegisters[32];
    renogyData.loadState = rawData / 256;
    renogyData.chargingState = rawData % 256;
    renogyData.controllerFaultsHi = dataRegisters[33];
    renogyData.controllerFaultsLo = dataRegisters[34];
  }
}

void RenogyReadInfoRegisters()
{
  uint8_t result;
  uint16_t infoRegisters[numInfoRegisters];
  char buffer1[40], buffer2[40];
  uint8_t rawData;

  result = node.readHoldingRegisters(0x00A, numInfoRegisters);
  if (result != node.ku8MBSuccess)
  {
    HandleModbusError(result);
  }
  else
  {
    for (uint8_t i = 0; i < numInfoRegisters; i++)
    {
      infoRegisters[i] = node.getResponseBuffer(i);
    }

    // read and process each value
    // Register 0x0A - Controller voltage and Current Rating - 0
    rawData = infoRegisters[0];
    renogyInfo.maxSupportedVltage = rawData / 256;
    renogyInfo.chargingCurrentRating = rawData % 256;

    // Register 0x0B - Controller discharge current and productType - 1
    rawData = infoRegisters[1];
    renogyInfo.dischargingCurrentRating = rawData / 256;
    renogyInfo.productType = rawData % 256;

    // Registers 0x0C to 0x13 - Product Model String - 2-9
    char *modelNo = (char *)&(infoRegisters[2]);
    strncpy((char *)&(renogyInfo.productModel), modelNo, 16);
    renogyInfo.productModel[16] = '\0';

    // Registers 0x014 to 0x015 - Software Version - 10-11
    itoa(infoRegisters[10], buffer1, 10);
    itoa(infoRegisters[11], buffer2, 10);
    strcat(buffer1, buffer2);
    strcpy(renogyInfo.softwareVersion, buffer1);

    // Registers 0x016 to 0x017 - Hardware Version - 12-13
    itoa(infoRegisters[12], buffer1, 10);
    itoa(infoRegisters[13], buffer2, 10);
    strcat(buffer1, buffer2);
    strcpy(renogyInfo.hardwareVersion, buffer1);

    // Registers 0x018 to 0x019 - Product Serial Number - 14-15
    itoa(infoRegisters[14], buffer1, 10);
    itoa(infoRegisters[15], buffer2, 10);
    strcat(buffer1, buffer2);
    strcpy(renogyInfo.serialNumber, buffer1);

    renogyInfo.modbusAddress = infoRegisters[16] % 256;
  }
}

void GetRenogyData()
{
  Serial1.println("Polling Renogy");
  static uint32_t i;
  i++;

  // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
  node.setTransmitBuffer(0, lowWord(i));
  // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
  node.setTransmitBuffer(1, highWord(i));

  RenogyReadInfoRegisters();

  Serial1.println("Info Registers");
  Serial1.println("Voltage rating: " + String(renogyInfo.maxSupportedVltage));
  Serial1.println("Amp rating: " + String(renogyInfo.chargingCurrentRating));
  Serial1.println("Voltage rating: " + String(renogyInfo.dischargingCurrentRating));
  Serial1.println("Product type: " + String(renogyInfo.productType));
  Serial1.println(strcat("Controller name: ", renogyInfo.productModel));
  Serial1.println(strcat("Software version: ", renogyInfo.softwareVersion));
  Serial1.println(strcat("Hardware version: ", renogyInfo.hardwareVersion));
  Serial1.println(strcat("Serial number: ", renogyInfo.serialNumber));
  Serial1.println("Modbus address: " + String(renogyInfo.modbusAddress));

  RenogyReadDataRegisters();

  Serial1.println("Battery voltage: " + String(renogyData.batteryVoltage));
  Serial1.println("Battery charge current: " + String(renogyData.batteryChargingCurrent));
  Serial1.println("Battery charge level: " + String(renogyData.batteryCapacitySoc) + "%");
  Serial1.println("Battery temp: " + String(renogyData.batteryTemperature));
  Serial1.println("Controller temp: " + String(renogyData.controllerTemperature));
  Serial1.println("Panel voltage: " + String(renogyData.panelVoltage));
  Serial1.println("Panel current: " + String(renogyData.panelCurrent));
  Serial1.println("Panel power: " + String(renogyData.panelPower));
  Serial1.println("Min Battery voltage today: " + String(renogyData.minBatteryVoltageToday));
  Serial1.println("Max charging current today: " + String(renogyData.maxChargingCurrentToday));
  Serial1.println("Max discharging current today: " + String(renogyData.maxDischargingCurrentToday));
  Serial1.println("Max charging power today: " + String(renogyData.maxChargePowerToday));
  Serial1.println("Max discharging power today: " + String(renogyData.maxDischargePowerToday));
  Serial1.println("Charging amphours today: " + String(renogyData.chargeAmphoursToday));
  Serial1.println("Discharging amphours today: " + String(renogyData.dischargeAmphoursToday));
  Serial1.println("Charging power today: " + String(renogyData.powerGenerationToday));
  Serial1.println("Discharging power today: " + String(renogyData.powerConsumptionToday));
  Serial1.println("Controller uptime: " + String(renogyData.totalNumOperatingDays) + " days");
  Serial1.println("Total Battery fullcharges: " + String(renogyData.totalNumBatteryFullCharges));
  Serial1.println("Total Battery overDischarges: " + String(renogyData.totalNumBatteryOverDischarges));
  Serial1.println("Load State: " + String(renogyData.loadState));
  Serial1.println("Charging State: " + String(renogyData.chargingState));
  Serial1.println("Controller Faults High Word: " + String(renogyData.controllerFaultsHi));
  Serial1.println("Controller Faults Low Word: " + String(renogyData.controllerFaultsLo));

  Serial1.println("-----------------------------------------------------");
}

void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  pinMode(MANUAL_PIN, INPUT_PULLDOWN_16);
  pinMode(MODE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(RED_LED_PIN, OUTPUT);

  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  InitLEDs();
  attachInterrupt(digitalPinToInterrupt(TOGGLE_PIN), IntCallback, RISING);

  SetupModbus();

  SetupWiFiClient();

  LittleFS.begin();
  Connect2Mqtt();
}

void loop()
{
  mqtt->loop();
  if (WiFi.status() != WL_CONNECTED)
  {
    ReportWiFi(wifiError);
    SetupWiFiClient();
  }

  if ((millis() - lastTimeRenogyPolled) > modbusPollingPeriodicity)
  {
    lastTimeRenogyPolled = millis();
    GetRenogyData();
    ReportStatus(modbusReadOK);
    if (!digitalRead(MANUAL_PIN))
    {
      if (renogyData.batteryVoltage < OnMainThreshold)
      {
        ToMainMode();
      }
      else if (renogyData.batteryVoltage > OnSolarThreshold)
      {
        ToSolarMode();
      }
    }
  }

  if ((millis() - lastTimeTelemetrySent) > mqttSendingPeriodicity)
  {
    lastTimeTelemetrySent = millis();
    if (mqtt->publish(outTopic.c_str(), renogyData.toJSON().c_str()))
    {
      Serial1.println("Telemetry sent");
      ReportStatus(mqttSentOK);
    }
    else
    {
      Serial1.println("Error sending Telemetry");
      HandleMQTTError(mqtt->state());
    }
  }
}
