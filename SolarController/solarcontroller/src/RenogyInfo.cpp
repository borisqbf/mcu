#include <stdio.h>
#include <cstring>
#include "RenogyInfo.h"

char* RenogyInfo::toJSON()
{
    memset(buffer, '\0', sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "{\n\"MaxSupportedVoltage\":\"%u\",\n\"ChargingCurrentRating\":\"%u\",\n\"DischargingCurrentRating\":\"%u\",\n\"ProductType\":\"%u\",\n\"SoftwareVersion\":\"%s\",\n\"HardwareVersion\":\"%s\",\n\"SerialNumber\":\"%s\",\n\"ModbusAddress\":\"%u\"\n }\n",
            maxSupportedVltage, chargingCurrentRating, dischargingCurrentRating, productType, softwareVersion, hardwareVersion, serialNumber, modbusAddress);
    return buffer;
}