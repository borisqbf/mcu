#include <WString.h>
#include "RenogyController.h"
#include "LEDStatusReporter.h"

static RenogyController theInstance;

RenogyController *RenogyController::GetInstance()
{
    return &theInstance;
};

RenogyController::RenogyController()
{
    lastTimeRenogyPolled = 0;
    mqttController = MqttController::GetInstance();
};

bool RenogyController::PublishRenogyData()
{ 
    return mqttController->PublishMessage(renogyData.toJSON()); 
}

bool RenogyController::IsUpdateRequired()
{
    if ((millis() - lastTimeRenogyPolled) > modbusPollingPeriodicity)
    {
        lastTimeRenogyPolled = millis();
        return true;
    }
    else
        return false;
    
}

void RenogyController::HandleModbusError(uint8_t errorCode)
{
    String errorStr;
    switch (errorCode)
    {
    case node.ku8MBIllegalDataAddress:
        errorStr = "Illegal data address";
        LEDStatusReporter::ReportError(modbusIllegalDataAddressError);
        break;
    case node.ku8MBIllegalDataValue:
        errorStr = "Illegal data value";
        LEDStatusReporter::ReportError(modbusIllegalDataValueError);
        break;
    case node.ku8MBIllegalFunction:
        errorStr = "Illegal function";
        LEDStatusReporter::ReportError(modbusIllegalFunctionError);
        break;
    case node.ku8MBSlaveDeviceFailure:
        errorStr = "Slave device failure";
        LEDStatusReporter::ReportError(modbusSlaveDeviceFailure);
        break;
    case node.ku8MBSuccess:
        errorStr = "Success";
        break;
    case node.ku8MBInvalidSlaveID:
        errorStr = "Invalid slave ID: The slave ID in the response does not match that of the request.";
        LEDStatusReporter::ReportError(modbusnvalidSlaveIDError);
        break;
    case node.ku8MBInvalidFunction:
        errorStr = "Invalid function: The function code in the response does not match that of the request.";
        LEDStatusReporter::LEDStatusReporter::ReportError(modbusInvalidFunction);
        break;
    case node.ku8MBResponseTimedOut:
        errorStr = "Response timed out";
        LEDStatusReporter::ReportError(modbusResponseTimedOutError);
        break;
    case node.ku8MBInvalidCRC:
        errorStr = "InvalidCRC";
        LEDStatusReporter::ReportError(modbusInvalidCRCError);
        break;
    default:
        errorStr = "Unknown error";
        LEDStatusReporter::ReportError(modbusUnknownError);
        break;
    }
    Serial1.println(errorStr);
    mqttController->PublishMessage(errorStr);
}

bool RenogyController::RenogyReadDataRegisters()
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
        return false;
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
        return true;
    }
}

bool RenogyController::RenogyReadInfoRegisters()
{
    uint8_t result;
    uint16_t infoRegisters[numInfoRegisters];
    char buffer1[40], buffer2[40];
    uint8_t rawData;

    result = node.readHoldingRegisters(0x00A, numInfoRegisters);

    if (result != node.ku8MBSuccess)
    {
        HandleModbusError(result);
        return false;
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
        renogyInfo.softwareVersion[4] = '\0';

        // Registers 0x016 to 0x017 - Hardware Version - 12-13
        itoa(infoRegisters[12], buffer1, 10);
        itoa(infoRegisters[13], buffer2, 10);
        strcat(buffer1, buffer2);
        strcpy(renogyInfo.hardwareVersion, buffer1);
        renogyInfo.hardwareVersion[4] = '\0';

        // Registers 0x018 to 0x019 - Product Serial Number - 14-15
        itoa(infoRegisters[14], buffer1, 10);
        itoa(infoRegisters[15], buffer2, 10);
        strcat(buffer1, buffer2);
        strcpy(renogyInfo.serialNumber, buffer1);
        renogyInfo.serialNumber[4] = '\0';

        renogyInfo.modbusAddress = infoRegisters[16] % 256;
        return true;
    }
}

bool RenogyController::GetRenogyData()
{
    bool infoRegistersOK = false;
    bool dataRegistersOk = false;
    Serial1.println("Polling Renogy");
    static uint32_t i;
    i++;

    // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
    node.setTransmitBuffer(0, lowWord(i));
    // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
    node.setTransmitBuffer(1, highWord(i));

    if (RenogyReadInfoRegisters())
    {
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
        infoRegistersOK = true;
    }
    if (RenogyReadDataRegisters())
    {
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
        dataRegistersOk = true;
    }
    return dataRegistersOk && infoRegistersOK;
}

void RenogyController::Setup()
{
    node.begin(modbusAddress, Serial);
}
