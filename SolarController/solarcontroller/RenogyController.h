#ifndef RENOGYCONTROLLER_H
#define RENOGYCONTROLLER_H
#include <stdint.h>
#include <ModbusMaster.h>

#include "RenogyData.h"
#include "RenogyInfo.h"
#include "MqttController.h"

const uint32_t numDataRegisters = 35;
const uint32_t numInfoRegisters = 17;

const uint8_t modbusAddress = 255;
const int modbusBaudRate = 9600;

const int modbusPollingPeriodicity = 1 * 60 * 1000; // report state every xx minutes


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

class RenogyController
{
public:
    RenogyController();
    void Setup();
    static RenogyController *GetInstance();
    void HandleModbusError(uint8_t errorCode);
    float GetBatteryVoltage() { return renogyData.batteryVoltage; };
    bool PublishRenogyData();

    bool GetRenogyData();
    bool IsUpdateRequired();

private:
    bool RenogyReadInfoRegisters();
    bool RenogyReadDataRegisters();
    unsigned long lastTimeRenogyPolled;

    RenogyInfo renogyInfo;
    RenogyData renogyData;
    ModbusMaster node;
    MqttController *mqttController;
};

#endif