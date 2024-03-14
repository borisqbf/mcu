#ifndef RENOGYINFO_H
#define RENOGYINFO_H

#include <stdint.h>

// A struct to hold the controller info params
class RenogyInfo
{

public:
    uint8_t maxSupportedVltage;       // volts
    uint8_t chargingCurrentRating;    // amps
    uint8_t dischargingCurrentRating; // amps
    uint8_t productType;
    char productModel[17];
    char softwareVersion[11];
    char hardwareVersion[11];
    char serialNumber[11];
    uint8_t modbusAddress;

    char *toJSON();

private:
    char buffer[300];
};

#endif