#define RX 8
#define TX 9

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <LcdBarGraph.h>
#include "SSIDStrength.h"

SoftwareSerial esp8266(RX, TX);

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LcdBarGraph lbg(&lcd, 16, 0, 1);

SSIDStrength strongestWiFi;
SSIDStrength currentStrength;

enum ProcessingState
{
  SendingResetCommand,
  SendingListAllNetworksCommand,
  ProcessingTopSSID,
  SendingListSelectedNetworkCommand,
  ProcessMeasurementResult,
  Testing
};

ProcessingState currentState;

const char* GenerateCurrentSwlapCommand(const char *swlap, const char *ssid)
{
  const int ssidLen = strlen(ssid);
  const int bufferLen = strlen(swlap) + ssidLen + 3;
  const char *currentSwlap = new char[bufferLen];
  memset(currentSwlap, '\0', bufferLen);
  strncpy(currentSwlap, swlap, strlen(swlap) - 2);
  strcat(currentSwlap, "=\"");
  strcat(currentSwlap, ssid);
  strcat(currentSwlap, "\"\r\n");
  Serial.println(currentSwlap);
  return currentSwlap;
}

void setup()
{
  Serial.begin(9600);
  esp8266.begin(9600);
  lcd.begin(16, 2);
  currentState = SendingResetCommand;
}

int i = 0;
int j = 0;

void SetProcessingState(ProcessingState st)
{
  currentState = st;
  i = 0;
  j = 0;
}

const char swlap[] = "AT+CWLAP\r\n";
const char rst[] = "AT+RST\r\n";
char *currentSwlap = '\0';

void ProcessEspResponce()
{
  String response = esp8266.readString();
  if (response.indexOf("+CWLAP:") < 0)
  {
    Serial.print(response);
    if (response.indexOf("ERROR") >= 0)
    {
      SetProcessingState(SendingListAllNetworksCommand);
    }
    else if (response.indexOf("ready") >= 0)
    {
      SetProcessingState(SendingListAllNetworksCommand);
    }
    return;
  }
  if (currentState == ProcessingTopSSID)
  {
    strongestWiFi.MaxFromCWLAP(response);
    int ssidLen = strongestWiFi.getSSID().length() + 1;
    const char *ssid = new char[ssidLen];
    strongestWiFi.getSSID().toCharArray(ssid, ssidLen);
    int strength = strongestWiFi.getStrength();
    Serial.print("SSID ");
    Serial.println();
    Serial.print("Signal ");
    Serial.println(strength);

    if (strongestWiFi.IsUndefined())
    {
      lcd.print("No WiFi signal");
      lcd.setCursor(0, 0);
      SetProcessingState(SendingListAllNetworksCommand);
      return;
    }
    else
    {
      lcd.print(ssid);
      for (int k = ssidLen; k <= 10; k++)
      {
        lcd.print(" ");
      }

      lcd.print(strength);
      lcd.print("dBm");
      currentSwlap = GenerateCurrentSwlapCommand(swlap, ssid);
      SetProcessingState(SendingListSelectedNetworkCommand);
    }
  }
  else if (currentState == ProcessMeasurementResult)
  {
    currentStrength.Reset();
    currentStrength.MaxFromCWLAP(response);
    const int strength = currentStrength.getStrength();

    lcd.setCursor(10, 0);
    if (strength > SSIDStrength::MinRssi)
    {
      lcd.print(strength);
      lbg.drawValue(SSIDStrength::CalculateSignalStrengsPercentage(strength), 100);
    }
    else if (strongestWiFi.getSSID() != UndefinedSSID)
    {
      lcd.print("---");
    }
  }
  SetProcessingState(SendingListSelectedNetworkCommand);
}

void loop()
{
  if (esp8266.available())
  {
    ProcessEspResponce();
  }
  if (Serial.available())
  {
    esp8266.write(Serial.read());
  }
  else
  {
    switch (currentState)
    {
      case SendingResetCommand:
        if (j < sizeof(rst))
        {
          esp8266.write(rst[j++]);
        }
        break;
        
      case SendingListAllNetworksCommand:
        if (i < sizeof(swlap))
        {
          esp8266.write(swlap[i++]);
        }
        else
        {
          SetProcessingState(ProcessingTopSSID);
        }
        break;

      case SendingListSelectedNetworkCommand:
        if (i < strlen(currentSwlap))
        {
          esp8266.write(currentSwlap[i++]);
        }
        else
        {
          SetProcessingState(ProcessMeasurementResult);
        }
        break;
    }
  }
}
