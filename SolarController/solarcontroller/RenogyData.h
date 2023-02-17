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
    uint16_t loadPower;                     // watts
    float minBatteryVoltageToday;          // volts
    float maxBatteryVoltageToday;          // volts
    float maxChargingCurrentToday;         // amps
    float maxDischargingCurrentToday;      // amps
    uint16_t maxChargePowerToday;           // watts
    uint16_t maxDischargePowerToday;        // watts
    uint16_t chargeAmphoursToday;           // amp hours
    uint16_t dischargeAmphoursToday;        // amp hours
    uint16_t powerGenerationToday;          // watt hours
    uint16_t powerConsumptionToday;         // watt hours
    uint16_t totalNumOperatingDays;         // days
    uint16_t totalNumBatteryOverDischarges; // count
    uint16_t totalNumBatteryFullCharges;    // count
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