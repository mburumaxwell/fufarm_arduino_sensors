#include "config.h"

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#if HAVE_WIFI

#include <WiFi.h>

class WiFiManager
{
public:
  /**
   * Creates a new instance of the WiFiManager class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  WiFiManager();

  /**
   * Cleanup resources created and managed by the WiFiManager class.
   */
  ~WiFiManager();

  /**
   * Scan for networks and connect to the one known
   */
  void begin();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

private:
  uint8_t _status;

private:
  void printMacAddress(uint8_t mac[]);
  void printEncryptionType(uint8_t type);
  void listNetworks();
  void connect();
};

#endif // HAVE_WIFI

#endif // WIFI_MANAGER_H
