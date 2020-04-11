#define UndefinedSSID "Undefined"

#include <WString.h>

class SSIDStrength
{
  private:
    float _strength;
    String _SSID;

  public:
    SSIDStrength(String SSID, float strenth);
    SSIDStrength();
    void MaxFromCWLAP(int numberOfAvailableNetworks, String ssid = "");
    static float CalculateSignalStrengsPercentage(int strength);
    void Reset();
    inline String getSSID() {
      return _SSID;
    }
    inline float getStrength() {
      return _strength;
    }
    inline bool IsUndefined() {
      return (_SSID == UndefinedSSID) || (_strength <= SSIDStrength::MinRssi);
    }
    static const int MinRssi = -100;
    static const int MaxRssi = -15;
};
