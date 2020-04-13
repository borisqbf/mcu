extern "C" {
#include <user_interface.h>
}

#define ets_wdt_disable ((void (*)(void))0x400030f0)
#define ets_delay_us ((void (*)(int))0x40002ecc)

#define _R (uint32_t *)0x60000700

void nk_deep_sleep(uint64_t time)
{
  ets_wdt_disable();
  *(_R + 4) = 0;
  *(_R + 17) = 4;
  *(_R + 1) = *(_R + 7) + 5;
  *(_R + 6) = 8;
  *(_R + 2) = 1 << 20;
  ets_delay_us(10);
  *(_R + 39) = 0x11;
  *(_R + 40) = 3;
  *(_R) &= 0xFCF;
  *(_R + 1) = *(_R + 7) + (45*(time >> 8));
  *(_R + 16) = 0x7F;
  *(_R + 2) = 1 << 20;
  __asm volatile ("waiti 0");
}
void setup() {

  delay(1000);
  Serial.begin(74880);

  rst_info *rinfo = ESP.getResetInfoPtr();

  Serial.print(String("\nResetInfo.reason = ") + (*rinfo).reason + ": " + ESP.getResetReason() + "\n");

  Serial.println(".");  
  Serial.println("I'm awake.");

  delay(5000);
  Serial.println("Going to sleep.");
  nk_deep_sleep(10e6);
}

void loop() { }
