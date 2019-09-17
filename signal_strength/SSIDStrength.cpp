#include <float.h>
#include <HardwareSerial.h>
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

bool SSIDStrength::MaxFromCWLAP(String CWLAPResponse)
{
  int startIndex = 0;
  do
  {
    const int CWLAPIndex = CWLAPResponse.indexOf("+CWLAP:", startIndex);
    if (CWLAPIndex < 0)
      break;

    const int comma1Pos = CWLAPResponse.indexOf(",", CWLAPIndex);
    if (comma1Pos < 0)
    {
      Serial.println("Corrupted CWLAP Response " + CWLAPResponse);
      return false;
    } 
    const int comma2Pos = CWLAPResponse.indexOf(",", comma1Pos + 1);
    if (comma2Pos < 0)
    {
      Serial.println("Corrupted CWLAP Response " + CWLAPResponse);
      return false;
    }
    const int comma3Pos = CWLAPResponse.indexOf(",", comma2Pos + 1);
    if (comma3Pos < 0)
    {
      Serial.println("Corrupted CWLAP Response " + CWLAPResponse);
      return false;
    }
    startIndex = comma3Pos + 1;

    float rssi = CWLAPResponse.substring(comma2Pos + 1, comma3Pos).toFloat();
    if (rssi > _strength)
    {
      _SSID = CWLAPResponse.substring(comma1Pos + 2, comma2Pos - 1);
      _SSID.trim();
      _strength = rssi;
    }
  } while (true);
}

float SSIDStrength::CalculateSignalStrengsPercentage(int strength)
{
  // signal strength is in the range from -100 dB to -15 dB
  const int range = MaxRssi - MinRssi;
  float strengthPoint =  strength - MinRssi;
  return strengthPoint / range * 100;
}
