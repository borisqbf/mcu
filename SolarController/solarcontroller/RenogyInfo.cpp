#include <stdio.h>
#include "RenogyInfo.h"

String RenogyInfo::toJSON()
{
    char buffer[300];
    sprintf(buffer, "{\n\"MaxSupportedVoltage\":\"%u\",\n\"ChargingCurrentRating\":\"%u\",\n\"DischargingCurrentRating\":\"%u\",\n\"ProductType\":\"%u\",\n\"SoftwareVersion\":\"%s\",\n\"HardwareVersion\":\"%s\",\n\"SerialNumber\":\"%s\",\n\"ModbusAddress\":\"%u\"\n }\n",
            maxSupportedVltage, chargingCurrentRating, dischargingCurrentRating, productType, softwareVersion, hardwareVersion, serialNumber, modbusAddress);
    return (String(buffer));
}