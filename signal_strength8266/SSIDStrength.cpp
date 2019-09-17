#include <float.h>
#include "ESP8266WiFi.h"
#include "SSIDStrength.h"

SSIDStrength::SSIDStrength()
{
  Reset();
}

SSIDStrength::SSIDStrength(String SSID, float strenth)
{
  _strength = strenth;
  _SSID = SSID;
}

void SSIDStrength::Reset()
{
  _strength = MinRssi;
  _SSID = UndefinedSSID;
}

void SSIDStrength::MaxFromCWLAP(int numberOfAvailableNetworks, String ssid)
{
  for (int i = 0; i < numberOfAvailableNetworks; ++i)
  {
    float rssi = WiFi.RSSI(i);
    String tmpSsid = WiFi.SSID(i);
    tmpSsid.trim();
    if (ssid != "" && ssid != tmpSsid)
    {
      continue;
    }
    if (rssi > _strength)
    {

      _strength = rssi;
      _SSID = tmpSsid;
    }
    delay(10);
  }
}

float SSIDStrength::CalculateSignalStrengsPercentage(int strength)
{
  // signal strength is in the range from -100 dB to -15 dB
  const int range = MaxRssi - MinRssi;
  float strengthPoint =  strength - MinRssi;
  return strengthPoint / range * 100;
}
