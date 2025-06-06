#include "config.h"

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#if HAVE_WIFI

#include <WiFi.h>

/**
 * This class is a wrapper for the WiFi logic.
 * It is where all the WiFi related code is located.
 */
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
   * Scan for networks and connect to the one configured.
   */
  void begin();

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

  /**
   * Get the station interface IP address.
   */
  inline IPAddress localIP() { return WiFi.localIP(); }

  /**
   * Get the station interface IP address.
   */
  inline uint8_t * macAddress() { return _macAddress; }

  /**
   * Get the station interface IP address.
   */
  inline const char* hostname() { return _hostname; }

private:
  uint8_t _status;
  char _hostname[20]; // e.g. "fufarm-012345678901" plus EOF
  uint8_t _macAddress[MAC_ADDRESS_LENGTH];

private:
void printMacAddress(uint8_t mac[]);
#if !WIFI_SKIP_LIST_NETWORKS
  void listNetworks();
#endif
  void connect(bool initial);
};

#endif // HAVE_WIFI

#endif // WIFI_MANAGER_H
