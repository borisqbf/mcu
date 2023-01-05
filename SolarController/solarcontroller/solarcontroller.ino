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
const uint8_t BLUE_LED_PIN = 2;
const uint8_t GREEN_LED_PIN = 0;
// const uint8_t SOFT_RX_PIN = 5;
// const uint8_t SOFT_TX_PIN = 4;

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

#pragma region ModBus-Renogy

const uint32_t num_data_registers = 35;
const uint32_t num_info_registers = 17;

// A struct to hold the controller data
struct Controller_data
{
  uint8_t battery_soc;               // percent
  float battery_voltage;             // volts
  float battery_charging_amps;       // amps
  uint8_t battery_temperature;       // celcius
  uint8_t controller_temperature;    // celcius
  float load_voltage;                // volts
  float load_amps;                   // amps
  uint8_t load_watts;                // watts
  float solar_panel_voltage;         // volts
  float solar_panel_amps;            // amps
  uint8_t solar_panel_watts;         // watts
  float min_battery_voltage_today;   // volts
  float max_battery_voltage_today;   // volts
  float max_charging_amps_today;     // amps
  float max_discharging_amps_today;  // amps
  uint8_t max_charge_watts_today;    // watts
  uint8_t max_discharge_watts_today; // watts
  uint8_t charge_amphours_today;     // amp hours
  uint8_t discharge_amphours_today;  // amp hours
  uint8_t charge_watthours_today;    // watt hours
  uint8_t discharge_watthours_today; // watt hours
  uint8_t controller_uptime_days;    // days
  uint8_t total_battery_overcharges; // count
  uint8_t total_battery_fullcharges; // count
  uint32_t total_charging_amphours;  // amp hours
  uint32_t total_discharging_amphours;  // amp hours
  uint32_t total_power_generated;       // kW hours
  uint32_t total_power_consumed;        // kW hours
  float battery_charging_watts;         // watts
  uint8_t loadState;                    // on-off, brightness
  uint8_t chargingState;
  uint16_t controllerFaultsHi;
  uint16_t controllerFaultsLo; //Reserved

  long last_update_time;         // millis() of last update time
  String toJSON()
  {
    char buffer[100];
    sprintf(buffer, "{\n\"BatteryVoltage\":\"%f\",\n\"BatteryChargingCurrent\":\"%f\",\n\"PanelVoltage\":\"%f\",\n\"PanelCurrent\":\"%f\",\n }\n", 
          battery_voltage, battery_charging_amps, solar_panel_voltage, solar_panel_amps);
    return (String(buffer));
  }
};

// A struct to hold the controller info params
struct Controller_info
{
  uint8_t voltage_rating;       // volts
  uint8_t amp_rating;           // amps
  uint8_t discharge_amp_rating; // amps
  uint8_t productType;
  char controller_name[40];
  char software_version[40];
  char hardware_version[40];
  char serial_number[40];
  uint8_t modbus_address;

  float wattage_rating;
  long last_update_time; // millis() of last update time
};

Controller_info renogy_info;
Controller_data renogy_data;

const uint8_t modbus_address = 255;
const int modbusBaudRate = 9600;
const int modbusPollingPeriodicity = 30 * 1000; // in milliseconds;
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

unsigned long lastTimeTelemetrySent = 0;
const char *mqttServer = "299d6fc93f0945089400ce1c143e1ebb.s2.eu.hivemq.cloud";
const int mqttPort = 8883;

const int mqttSendingPeriodicity = 3 * 60 * 1000; // in milliseconds

const int reportingInterval = 5 * 60; // report state every xx seconds

// MQTT Settings
const char *mqttUser = "boris_qbf";
const char *mqttPassword = "mqttReward00";
const char *family = "solar";
String outTopic(family);

PubSubClient *mqtt;

#pragma endregion

#pragma region Error Codes and Statuses

const uint8_t wifiConnecting = 3;
const uint8_t wifiConnected = 0;
const uint8_t wifiError = 1;
const uint8_t mqttError = 3;
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
  long int m =  millis();
  Serial.printf("Got interupt. Millis %ld\n",m);
  if (digitalRead(MANUAL_PIN))
  {
    if (millis() - lastPressed > 400)
    {
      ToggleMode();
    }
    lastPressed = millis();
  }
}

bool Connect2Mqtt()
{
  randomSeed(micros());
  SetDateTime();
  // IP address could have been change - update mqtt client

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0)
  {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return false; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  mqtt = new PubSubClient(*bear);

  mqtt->setServer(mqttServer, mqttPort);
  while (!mqtt->connected())
  {
    Serial.println("Connecting to MQTT Server...");
    String clientId(family);
    clientId += String(" - ");
    clientId += String(random(0xff), HEX);

    if (mqtt->connect(clientId.c_str(), mqttUser, mqttPassword))
    {
      Serial.println("Connected");
      mqtt->publish(outTopic.c_str(), "I am alive");
      return true;
    }
    else
    {
      Serial.print("Failed with state ");
      Serial.println(mqtt->state());
      ReportError(-1 * mqtt->state());
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
    Serial.print("*");
  }

  Serial.println("WiFi connection Successful");
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.println(WiFi.localIP()); // Print the IP address
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

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}

void SetupModbus()
{
  node.begin(modbus_address, Serial);
}

void HandleModbusError(uint8_t errorCode)
{
  switch (errorCode)
  {
  case node.ku8MBIllegalDataAddress:
    Serial.println("Illegal data address");
    ReportError(modbusIllegalDataAddressError);
    break;
  case node.ku8MBIllegalDataValue:
    Serial.println("Illegal data value");
    ReportError(modbusIllegalDataValueError);
    break;
  case node.ku8MBIllegalFunction:
    Serial.println("Illegal function");
    ReportError(modbusIllegalFunctionError);
    break;
  case node.ku8MBSlaveDeviceFailure:
    Serial.println("Slave device failure");
    ReportError(modbusSlaveDeviceFailure);
    break;
  case node.ku8MBSuccess:
    Serial.println("Success");
    break;
  case node.ku8MBInvalidSlaveID:
    Serial.println("Invalid slave ID: The slave ID in the response does not match that of the request.");
    ReportError(modbusnvalidSlaveIDError);
    break;
  case node.ku8MBInvalidFunction:
    Serial.println("Invalid function: The function code in the response does not match that of the request.");
    ReportError(modbusInvalidFunction);
    break;
  case node.ku8MBResponseTimedOut:
    Serial.println("Response timed out");
    ReportError(modbusResponseTimedOutError);
    break;
  case node.ku8MBInvalidCRC:
    Serial.println("InvalidCRC");
    ReportError(modbusInvalidCRCError);
    break;
  default:
    Serial.println("Unknown error");
    ReportError(modbusUnknownError);
    break;
  }
}

void renogy_read_data_registers()
{
  uint8_t j, result;
  uint16_t data_registers[num_data_registers];
  char buffer1[40], buffer2[40];
  uint8_t raw_data;

  result = node.readHoldingRegisters(0x100, num_data_registers);
  if (result != node.ku8MBSuccess)
  {
    HandleModbusError(result);
    renogy_data.battery_soc = 0;
    renogy_data.battery_voltage = 0; 
    renogy_data.battery_charging_amps = 0;
    renogy_data.battery_charging_watts = 0;
    renogy_data.controller_temperature = 0;
    renogy_data.battery_temperature = 0;
    renogy_data.load_voltage = 0;
    renogy_data.load_amps = 0;
    renogy_data.load_watts = 0;
    renogy_data.solar_panel_voltage = 0;
    renogy_data.solar_panel_amps = 0;
    renogy_data.solar_panel_watts = 0; 
    renogy_data.min_battery_voltage_today = 0;
    renogy_data.max_battery_voltage_today = 0;
    renogy_data.max_charging_amps_today = 0;
    renogy_data.max_discharging_amps_today = 0;
    renogy_data.max_charge_watts_today = 0;
    renogy_data.max_discharge_watts_today = 0;
    renogy_data.charge_amphours_today = 0;
    renogy_data.discharge_amphours_today = 0;
    renogy_data.charge_watthours_today = 0;
    renogy_data.discharge_watthours_today = 0;
    renogy_data.controller_uptime_days = 0;
    renogy_data.total_battery_overcharges = 0;
    renogy_data.total_battery_fullcharges = 0;
    renogy_data.loadState = 0;
    renogy_data.chargingState = 0;
    renogy_data.controllerFaultsHi = 0;
    renogy_data.controllerFaultsLo = 0;
    renogy_data.total_charging_amphours = 0;
    renogy_data.total_discharging_amphours = 0;
    renogy_data.total_power_generated = 0;
    renogy_data.total_power_consumed = 0;
  }
  else
  {
    for (j = 0; j < num_data_registers; j++)
    {
      data_registers[j] = node.getResponseBuffer(j);
    }

    renogy_data.battery_soc = data_registers[0];
    renogy_data.battery_voltage = data_registers[1] * 0.1; 
    renogy_data.battery_charging_amps = data_registers[2] * 0.01;

    renogy_data.battery_charging_watts = renogy_data.battery_voltage * renogy_data.battery_charging_amps;

    // 0x103 returns two bytes, one for battery and one for controller temp in c
    uint16_t raw_data = data_registers[3]; // eg 5913
    renogy_data.controller_temperature = raw_data / 256;
    renogy_data.battery_temperature = raw_data % 256;

    renogy_data.load_voltage = data_registers[4] * 0.1;
    renogy_data.load_amps = data_registers[5] * 0.01;
    renogy_data.load_watts = data_registers[6];
    renogy_data.solar_panel_voltage = data_registers[7] * 0.1;
    renogy_data.solar_panel_amps = data_registers[8] * 0.01;
    renogy_data.solar_panel_watts = data_registers[9];
    // Register 0x10A - Turn on load, write register - 10
    renogy_data.min_battery_voltage_today = data_registers[11] * 0.1;
    renogy_data.max_battery_voltage_today = data_registers[12] * 0.1;
    renogy_data.max_charging_amps_today = data_registers[13] * 0.01;
    renogy_data.max_discharging_amps_today = data_registers[14] * 0.01;
    renogy_data.max_charge_watts_today = data_registers[15];
    renogy_data.max_discharge_watts_today = data_registers[16];
    renogy_data.charge_amphours_today = data_registers[17];
    renogy_data.discharge_amphours_today = data_registers[18];
    renogy_data.charge_watthours_today = data_registers[19];
    renogy_data.discharge_watthours_today = data_registers[20];
    renogy_data.controller_uptime_days = data_registers[21];
    renogy_data.total_battery_overcharges = data_registers[22];
    renogy_data.total_battery_fullcharges = data_registers[23];

    memcpy(&renogy_data.total_charging_amphours, &(data_registers[24]), 4);
    memcpy(&renogy_data.total_discharging_amphours, &(data_registers[26]), 4);
    memcpy(&renogy_data.total_power_generated, &(data_registers[28]), 4);
    memcpy(&renogy_data.total_power_consumed, &(data_registers[30]), 4);

    raw_data = data_registers[32];
    renogy_data.loadState = raw_data / 256;
    renogy_data.chargingState = raw_data % 256;
    renogy_data.controllerFaultsHi = data_registers[33];
    renogy_data.controllerFaultsLo = data_registers[34];
    renogy_data.last_update_time = millis();
  }
}

void renogy_read_info_registers()
{
  uint8_t j, result;
  uint16_t info_registers[num_info_registers];
  char buffer1[40], buffer2[40];
  uint8_t raw_data;

  result = node.readHoldingRegisters(0x00A, num_info_registers);
  if (result != node.ku8MBSuccess)
  {
    HandleModbusError(result);
  }
  else
  {
    for (j = 0; j < num_info_registers; j++)
    {
      info_registers[j] = node.getResponseBuffer(j);
    }

    // read and process each value
    // Register 0x0A - Controller voltage and Current Rating - 0
     raw_data = info_registers[0];
    renogy_info.voltage_rating = raw_data / 256;
    renogy_info.amp_rating = raw_data % 256;
    renogy_info.wattage_rating = renogy_info.voltage_rating * renogy_info.amp_rating;
 
    // Register 0x0B - Controller discharge current and productType - 1
    raw_data = info_registers[1];
    renogy_info.discharge_amp_rating = raw_data / 256; 
    renogy_info.productType = raw_data % 256;

    // Registers 0x0C to 0x13 - Product Model String - 2-9
    char* modelNo = (char *)&(info_registers[2]);
    strncpy((char*)&(renogy_info.controller_name), modelNo, 16);
    renogy_info.controller_name[16] = '\0';

    // Registers 0x014 to 0x015 - Software Version - 10-11
    itoa(info_registers[10], buffer1, 10);
    itoa(info_registers[11], buffer2, 10);
    strcat(buffer1, buffer2);
    strcpy(renogy_info.software_version, buffer1);
  
    // Registers 0x016 to 0x017 - Hardware Version - 12-13
    itoa(info_registers[12], buffer1, 10);
    itoa(info_registers[13], buffer2, 10);
    strcat(buffer1, buffer2);
    strcpy(renogy_info.hardware_version, buffer1);

    // Registers 0x018 to 0x019 - Product Serial Number - 14-15
    itoa(info_registers[14], buffer1, 10);
    itoa(info_registers[15], buffer2, 10);
    strcat(buffer1, buffer2); 
    strcpy(renogy_info.serial_number, buffer1);

    renogy_info.modbus_address = info_registers[16] % 256;
    renogy_info.last_update_time = millis();
  }
}

void GetRenogyData()
{
  static uint32_t i;
  i++;

  // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
  node.setTransmitBuffer(0, lowWord(i));
  // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
  node.setTransmitBuffer(1, highWord(i));

  renogy_read_info_registers();

  Serial.println("Info Registers");
  Serial.println("Voltage rating: " + String(renogy_info.voltage_rating));
  Serial.println("Amp rating: " + String(renogy_info.amp_rating));
  Serial.println("Voltage rating: " + String(renogy_info.discharge_amp_rating));
  Serial.println("Product type: " + String(renogy_info.productType));
  Serial.println(strcat("Controller name: ", renogy_info.controller_name));
  Serial.println(strcat("Software version: ",  renogy_info.software_version));
  Serial.println(strcat("Hardware version: ", renogy_info.hardware_version));
  Serial.println(strcat("Serial number: ", renogy_info.serial_number));
  Serial.println("Modbus address: " + String(renogy_info.modbus_address));
  Serial.println("Wattage rating: " + String(renogy_info.wattage_rating));

  renogy_read_data_registers();

  Serial.println("Battery voltage: " + String(renogy_data.battery_voltage));
  Serial.println("Battery charge amps: " + String(renogy_data.battery_charging_amps));
  Serial.println("Battery charge level: " + String(renogy_data.battery_soc) + "%");
  Serial.println("Battery temp: " + String(renogy_data.battery_temperature));
  Serial.println("Controller temp: " + String(renogy_data.controller_temperature));
  Serial.println("Panel voltage: " + String(renogy_data.solar_panel_voltage));
  Serial.println("Panel amps: " + String(renogy_data.solar_panel_amps));
  Serial.println("Panel wattage: " + String(renogy_data.solar_panel_watts));
  Serial.println("Min Battery voltage today: " + String(renogy_data.min_battery_voltage_today));
  Serial.println("Max charging amps today: " + String(renogy_data.max_charging_amps_today));
  Serial.println("Max discharging amps today: " + String(renogy_data.max_discharging_amps_today));
  Serial.println("Max charging watts today: " + String(renogy_data.max_charge_watts_today));
  Serial.println("Max discharging watts today: " + String(renogy_data.max_discharge_watts_today));
  Serial.println("Charging amphours today: " + String(renogy_data.charge_amphours_today));
  Serial.println("Discharging amphours today: " + String(renogy_data.discharge_amphours_today));
  Serial.println("Charging watthours today: " + String(renogy_data.charge_watthours_today));
  Serial.println("Discharging watthours today: " + String(renogy_data.discharge_watthours_today));
  Serial.println("Controller uptime: " + String(renogy_data.controller_uptime_days) + " days");
  Serial.println("Total Battery fullcharges: " + String(renogy_data.total_battery_fullcharges));
  Serial.println("Load State: " + String(renogy_data.loadState));
  Serial.println("Charging State: " + String(renogy_data.chargingState));
  Serial.println("Controller Faults High Word: " + String(renogy_data.controllerFaultsHi));
  Serial.println("Controller Faults Low Word: " + String(renogy_data.controllerFaultsLo));

  Serial.println("-----------------------------------------------------");
}

void setup()
{
  Serial.begin(9600);
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

  // pinMode(SOFT_RX_PIN, INPUT);
  // pinMode(SOFT_TX_PIN, OUTPUT);

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

  if ((millis() - lastTimeTelemetrySent) > mqttSendingPeriodicity)
  {
    mqtt->publish(outTopic.c_str(), renogy_data.toJSON().c_str());
    lastTimeTelemetrySent = millis();
    Serial.println("Telemetry sent");
    ReportStatus(mqttSentOK);
  }
  if ((millis() - lastTimeRenogyPolled) > modbusPollingPeriodicity)
  {
    lastTimeRenogyPolled = millis();
    GetRenogyData();
    Serial.println("Polling Renogy");
    ReportStatus(modbusReadOK);
    if (!digitalRead(MANUAL_PIN))
    {
      if (renogy_data.battery_voltage < OnMainThreshold)
      {
        ToMainMode();
      }
      else if (renogy_data.battery_voltage  > OnSolarThreshold)
      {
        ToSolarMode();
      }
    }
  }
}
