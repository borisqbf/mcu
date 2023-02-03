#include <stdio.h>
#include "RenogyData.h"

String RenogyData::toJSON()
{
    char buffer[500];
    sprintf(buffer, "{\n\"BatteryVoltage\":\"%.2f\",\n"
                    "\"BatteryChargingCurrent\":\"%.2f\",\n"
                    "\"PanelVoltage\":\"%.2f\",\n"
                    "\"PanelCurrent\":\"%.2f\",\n"
                    /*
                    "\"BatteryCapacitySoc\":\"%u\",\n"
                    "\"BatteryTemperature\":\"%u\",\n"
                    "\"ControllerTemperature\":\"%u\",\n"*/
                    "\"PanelPower\":\"%u\",\n"
                    /*"\"LoadVoltage\":\"%.2f\",\n"
                     "\"LoadCurrent\":\"%.2f\",\n"
                     "\"LoadPower\":\"%u\",\n"
                     "\"MinBatteryVoltageToday\":\"%.2f\",\n"
                     "\"MaxBatteryVoltageToday\":\"%.2f\",\n"
                     "\"MaxChargingCurrentToday\":\"%.2f\",\n"
                     "\"MaxDischargingCurrentToday\":\"%.2f\",\n"
                      "\"MaxChargePowerToday\":\"%u\",\n"
                     "\"MaxDischargePowerToday\":\"%u\",\n"
                     "\"ChargeAmphoursToday\":\"%u\",\n"
                     "\"DischargeAmphoursToday\":\"%u\",\n"
                     "\"PowerGenerationToday\":\"%u\",\n"
                     "\"PowerConsumptionToday\":\"%u\",\n"
                     "\"TotalNumOperatingDays\":\"%u\",\n"
                     "\"TotalNumBatteryOverDischarges\":\"%u\",\n"
                     "\"TotalNumBatteryFullCharges\":\"%u\",\n"
                     "\"TotalChargingAmphours\":\"%u\",\n"
                     "\"TotalDischargingAmphours\":\"%u\",\n"
                     "\"CumulativePowerGeneration\":\"%u\",\n"
                     "\"CumulativePowerConsumption\":\"%u\",\n"
                     "\"LoadState\":\"%u\",\n"*/
                    "\"ChargingState\":\"%u\",\n"
                    "\"ControllerFaultsHi\":\"%u\",\n"
                    "\"ControllerFaultsLo\":\"%u\"\n}\n",
            batteryVoltage,
            batteryChargingCurrent,
            panelVoltage,
            panelCurrent,
            /*batteryCapacitySoc,
            batteryTemperature,
            controllerTemperature,*/
            panelPower,
            /*loadVoltage,
            loadCurrent,
            loadPower,
            minBatteryVoltageToday,
            maxBatteryVoltageToday,
            maxChargingCurrentToday,
            maxDischargingCurrentToday,
            maxChargePowerToday,
            maxDischargePowerToday,
            chargeAmphoursToday,
            dischargeAmphoursToday,
            powerGenerationToday,
            powerConsumptionToday,
            totalNumOperatingDays,
            totalNumBatteryOverDischarges,
            totalNumBatteryFullCharges,
            totalChargingAmphours,
            totalDischargingAmphours,
            cumulativePowerGeneration,
            cumulativePowerConsumption,
            loadState,*/
            chargingState,
            controllerFaultsHi,
            controllerFaultsLo);
    return (String(buffer));
}