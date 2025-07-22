#include <esp_system.h>

#include "reboot.h"

__attribute__((noreturn)) void reboot()
{
  beforeReset(); // do housekeeping before rebooting
  esp_restart();
}

__attribute__((weak)) void beforeReset() {
  // Default empty implementation.
  // Can be overridden elsewhere to save data to EEPROM or flash.
}
