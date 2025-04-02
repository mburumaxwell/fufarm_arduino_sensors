#include "reboot.h"
#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#elif defined(ARDUINO_UNOR4_WIFI)
#else
#include <avr/wdt.h>
#endif

void reboot()
{
  beforeReset();  // do housekeeping before rebooting

#if defined(ARDUINO_ARCH_ESP32)
  // For ESP32: restart the CPU
  esp_restart();
#elif defined(ARDUINO_UNOR4_WIFI)
  // For the Uno R4 WiFi: perform a system reset using the NVIC
  NVIC_SystemReset();
#else
  // For AVR boards like the Uno WiFi Rev2: enable watchdog with a short timeout
  wdt_enable(WDTO_15MS);
  while (true)
  {
    // Wait for the watchdog to reset the board
  }
#endif
}

__attribute__((weak)) void beforeReset() {
  // Default empty implementation.
  // Can be overridden elsewhere to save data to EEPROM or flash.
}
