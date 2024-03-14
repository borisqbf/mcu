
#include <stdint.h>
#include <Arduino.h>
#include "LEDStatusReporter.h"

void LEDStatusReporter::Blink(LedColour c)
{
    uint8_t pin = RED_LED_PIN;
    if (c == BLUE)
        pin = BLUE_LED_PIN;
    else if (c == GREEN)
        pin = GREEN_LED_PIN;

    digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
    delay(300);
    digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
    delay(500);
}

void LEDStatusReporter::LEDOnOff(LedColour c, bool on)
{
    uint8_t pin = RED_LED_PIN;
    if (c == BLUE)
        pin = BLUE_LED_PIN;
    else if (c == GREEN)
        pin = GREEN_LED_PIN;
    if (on)
        digitalWrite(pin, pin == RED_LED_PIN ? HIGH : LOW);
    else
        digitalWrite(pin, pin == RED_LED_PIN ? LOW : HIGH);
}

void LEDStatusReporter::ReportError(uint8_t errorCode)
{
    if (errorCode == 0)
    {
        LEDOnOff(RED, false);
    }
    else
    {
        for (uint8_t i = 0; i < errorCode; i++)
        {
            Blink(RED);
        }
        LEDOnOff(RED);
    }
}

void LEDStatusReporter::ReportStatus(uint8_t status)
{
    for (uint8_t i = 0; i < status; i++)
    {
        Blink(GREEN);
    }
}

void LEDStatusReporter::Setup()
{
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);

    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH);
}

void LEDStatusReporter::ReportWiFi(uint8_t status)
{
    if (status == 0)
        LEDOnOff(BLUE);
    else if (status == 1)
        LEDOnOff(BLUE, false);
    else
    {
        for (uint8_t i = 0; i < status; i++)
        {
            Blink(BLUE);
        }
    }
}
