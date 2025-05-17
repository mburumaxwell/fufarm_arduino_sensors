#include "reboot.h"
#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#elif defined(ARDUINO_UNOR4_WIFI)
#else
#include <avr/wdt.h>
#endif

__attribute__((noreturn)) void reboot()
{
  beforeReset();  // do housekeeping before rebooting

#if defined(ARDUINO_ARCH_ESP32)
  // For ESP32: restart the CPU
  esp_restart();
#elif defined(ARDUINO_UNOR4_WIFI)
  // For the Uno R4 WiFi: perform a system reset using the NVIC
  NVIC_SystemReset();
#elif defined(ARDUINO_ARCH_MEGAAVR)
  // ATmega4809 (e.g., Uno WiFi Rev2) WDT reset using direct register access
  // From the section 11.5 – Configuration Change Protection (CCP).
  // Unlocks protected writes for the next 4 CPU cycles.
  // CCP must be set before WDT because setting WDT re-engages protection.
  CPU_CCP = 0xD8; // D8 is a magic value not combination of individual flags
  WDT.CTRLA = WDT_PERIOD_64CLK_gc; // Set timeout and enable watchdog
  while (true) { }
#else
  // Classic AVR (e.g., Leonardo)
  wdt_enable(WDTO_15MS);
  while (true) { } // Wait for the watchdog to reset the board
#endif
}

__attribute__((weak)) void beforeReset() {
  // Default empty implementation.
  // Can be overridden elsewhere to save data to EEPROM or flash.
}
