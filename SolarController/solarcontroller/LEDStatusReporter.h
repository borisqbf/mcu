#ifndef LEDSTATUSREPORTER_H
#define LEDSTATUSREPORTER_H


const uint8_t RED_LED_PIN = 15;
const uint8_t BLUE_LED_PIN = 5;
const uint8_t GREEN_LED_PIN = 0;

enum LedColour
{
    RED,
    BLUE,
    GREEN
};

class LEDStatusReporter
{
private:
    static void Blink(LedColour c);
    static void LEDOnOff(LedColour c, bool on = true);
    
public:
    static void ReportError(uint8_t errorCode);
    static void ReportStatus(uint8_t status);
    static void ReportWiFi(uint8_t status);
    static void Setup();
};

#endif