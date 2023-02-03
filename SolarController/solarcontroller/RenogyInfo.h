#ifndef RENOGYINFO_H
#define RENOGYINFO_H

#include <stdint.h>
#include <Wstring.h>
// A struct to hold the controller info params
struct RenogyInfo
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

    String toJSON();
};

#endif