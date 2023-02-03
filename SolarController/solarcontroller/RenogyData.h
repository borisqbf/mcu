#ifndef RENOGYDATA_H
#define RENOGYDATA_H

#include <stdint.h>

// A struct to hold the controller data

class RenogyData
{
public:
    float batteryVoltage;          // volts
    float batteryChargingCurrent;  // amps
    float panelVoltage;            // volts
    float panelCurrent;            // amps
    uint8_t batteryCapacitySoc;    // percent
    uint8_t batteryTemperature;    // celcius
    uint8_t controllerTemperature; // celcius
    uint8_t panelPower;
    float loadVoltage;                     // volts
    float loadCurrent;                     // amps
    uint8_t loadPower;                     // watts
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

    char *toJSON();

private:
    char buffer[500];
};

#endif