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
    const char *errorStr;
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

uint32_t RenogyController::Combined(uint16_t low, uint16_t high)
{
    return (static_cast<uint32_t>(high) << 16) | ((static_cast<uint32_t>(low)) & 0xFFFF);
}

bool RenogyController::RenogyReadDataRegisters()
{
    uint8_t result;
    uint16_t dataRegisters[numDataRegisters];
    uint16_t rawData;

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
        renogyData.controllerFaults = 0;
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

        uint16_t rawData = dataRegisters[3]; 
        renogyData.controllerTemperature = rawData / 256; // high byte
        renogyData.batteryTemperature = rawData % 256; // low byte

        renogyData.loadVoltage = dataRegisters[4] * 0.1;
        renogyData.loadCurrent = dataRegisters[5] * 0.01;
        renogyData.loadPower = dataRegisters[6];
        renogyData.panelVoltage = dataRegisters[7] * 0.1;
        renogyData.panelCurrent = dataRegisters[8] * 0.01;
        renogyData.panelPower = dataRegisters[9];

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

        renogyData.totalChargingAmphours = Combined(dataRegisters[25], dataRegisters[24]);
        renogyData.totalDischargingAmphours = Combined(dataRegisters[27], dataRegisters[26]);
        renogyData.cumulativePowerGeneration = Combined(dataRegisters[29], dataRegisters[28]);
        renogyData.cumulativePowerConsumption = Combined(dataRegisters[31], dataRegisters[30]);

        rawData = dataRegisters[32];
        renogyData.loadState = rawData / 256; // high byte
        renogyData.chargingState = rawData % 256; // low byte
        renogyData.controllerFaults = Combined(dataRegisters[34], dataRegisters[33]);
        return true;
    }
}

bool RenogyController::RenogyReadInfoRegisters()
{
    uint8_t result;
    uint16_t infoRegisters[numInfoRegisters];
    uint16_t rawData;

    result = node.readHoldingRegisters(0x00A, numInfoRegisters);

    if (result != node.ku8MBSuccess)
    {
        HandleModbusError(result);
        return false;
    }
    else
    {
        memset(renogyInfo.productModel, '\0', sizeof(renogyInfo.productModel));
        memset(renogyInfo.softwareVersion, '\0', sizeof(renogyInfo.softwareVersion));
        memset(renogyInfo.hardwareVersion, '\0', sizeof(renogyInfo.hardwareVersion));
        memset(renogyInfo.serialNumber, '\0', sizeof(renogyInfo.serialNumber));

        for (uint8_t i = 0; i < numInfoRegisters; i++)
        {
            infoRegisters[i] = node.getResponseBuffer(i);
        }

        rawData = infoRegisters[0];
        renogyInfo.maxSupportedVltage = rawData / 256;    // high byte
        renogyInfo.chargingCurrentRating = rawData % 256; // low byte
        rawData = infoRegisters[1];
        renogyInfo.dischargingCurrentRating = rawData / 256; // high byte
        renogyInfo.productType = rawData % 256; // low byte
        char *modelNo = (char *)&(infoRegisters[2]);
        strncpy(renogyInfo.productModel, modelNo, 16);
        renogyInfo.productModel[16] = '\0';

        sprintf(renogyInfo.softwareVersion, "%d%d", infoRegisters[10], infoRegisters[11]);
        renogyInfo.softwareVersion[6] = '\0';
        sprintf(renogyInfo.hardwareVersion, "%d%d", infoRegisters[12], infoRegisters[13]);
        renogyInfo.hardwareVersion[6] = '\0';
        sprintf(renogyInfo.serialNumber, "%d%d", infoRegisters[14], infoRegisters[15]);
        renogyInfo.serialNumber[6] = '\0';

        renogyInfo.modbusAddress = infoRegisters[16] % 256; // low byte
        return true;
    }
}

bool RenogyController::GetRenogyData()
{
    bool infoRegistersOK = false;
    bool dataRegistersOk = false;
    Serial1.println("Polling Renogy");
    /*
    static uint32_t i;
    i++;

    // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
    node.setTransmitBuffer(0, lowWord(i));
    // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
    node.setTransmitBuffer(1, highWord(i));
    */
    if (RenogyReadInfoRegisters())
    {
        Serial1.println("Info Registers");
        Serial1.print("Voltage rating: ");
        Serial1.println(renogyInfo.maxSupportedVltage);
        Serial1.print("Amp rating: ");
        Serial1.println(renogyInfo.chargingCurrentRating);
        Serial1.print("Discharge Current rating: ");
        Serial1.println(renogyInfo.dischargingCurrentRating);
        Serial1.print("Product type: ");
        Serial1.println(renogyInfo.productType);
        Serial1.print("Controller name: ");
        Serial1.println(renogyInfo.productModel);
        Serial1.print("Software version: ");
        Serial1.println(renogyInfo.softwareVersion);
        Serial1.print("Hardware version: ");
        Serial1.println(renogyInfo.hardwareVersion);
        Serial1.print("Serial number: ");
        Serial1.println(renogyInfo.serialNumber);
        Serial1.print("Modbus address: ");
        Serial1.println(renogyInfo.modbusAddress);
        infoRegistersOK = true;
    }
    if (RenogyReadDataRegisters())
    {
        Serial1.print("Battery voltage: ");
        Serial1.println(renogyData.batteryVoltage);
        Serial1.print("Battery charge current: ");
        Serial1.println(renogyData.batteryChargingCurrent);
        Serial1.print("Percent battery charge level : ");
        Serial1.println(renogyData.batteryCapacitySoc);
        Serial1.print("Battery temp: ");
        Serial1.println(renogyData.batteryTemperature);
        Serial1.print("Controller temp: ");
        Serial1.println(renogyData.controllerTemperature);
        Serial1.print("Panel voltage: ");
        Serial1.println(renogyData.panelVoltage);
        Serial1.print("Panel current: ");
        Serial1.println(renogyData.panelCurrent);
        Serial1.print("Panel power: ");
        Serial1.println(renogyData.panelPower);
        Serial1.print("Min Battery voltage today: ");
        Serial1.println(renogyData.minBatteryVoltageToday);
        Serial1.print("Maxn Battery voltage today: ");
        Serial1.println(renogyData.maxBatteryVoltageToday);
        Serial1.print("Max charging current today: ");
        Serial1.println(renogyData.maxChargingCurrentToday);
        Serial1.print("Max discharging current today: ");
        Serial1.println(renogyData.maxDischargingCurrentToday);
        Serial1.print("Max charging power today: ");
        Serial1.println(renogyData.maxChargePowerToday);
        Serial1.print("Max discharging power today: ");
        Serial1.println(renogyData.maxDischargePowerToday);
        Serial1.print("Charging amphours today: ");
        Serial1.println(renogyData.chargeAmphoursToday);
        Serial1.print("Discharging amphours today: ");
        Serial1.println(renogyData.dischargeAmphoursToday);
        Serial1.print("Power generated today: ");
        Serial1.println(renogyData.powerGenerationToday);
        Serial1.print("Power consumption today: ");
        Serial1.println(renogyData.powerConsumptionToday);
        Serial1.print("Controller uptime (days): ");
        Serial1.println(renogyData.totalNumOperatingDays);
        Serial1.print("Total Battery fullcharges: ");
        Serial1.println(renogyData.totalNumBatteryFullCharges);
        Serial1.print("Total Battery overDischarges: ");
        Serial1.println(renogyData.totalNumBatteryOverDischarges);
        Serial1.print("Total charging amphours: ");
        Serial1.println(renogyData.totalChargingAmphours);
        Serial1.print("Total discharging amphours: ");
        Serial1.println(renogyData.totalDischargingAmphours);
        Serial1.print("Cumulative Power Generation: ");
        Serial1.println(renogyData.cumulativePowerGeneration);
        Serial1.print("Cumulative Power Consumption: ");
        Serial1.println(renogyData.cumulativePowerConsumption);
        Serial1.print("Load State: ");
        Serial1.println(renogyData.loadState);
        Serial1.print("Charging State: ");
        Serial1.println(renogyData.chargingState);
        Serial1.print("Controller Faults: ");
        Serial1.println(renogyData.controllerFaults);
        dataRegistersOk = true;
    }
    return dataRegistersOk && infoRegistersOK;
}

void RenogyController::Setup()
{
    node.begin(modbusAddress, Serial);
}
