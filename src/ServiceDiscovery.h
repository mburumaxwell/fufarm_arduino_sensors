#include "config.h"

#ifndef SERVICE_DISCOVERY_H
#define SERVICE_DISCOVERY_H

#if HAVE_NETWORK

#include <Arduino.h>

enum NetworkEndpointType
{
    UNKNOWN,
    IP,
    DNS,
};

struct NetworkEndpoint
{
    NetworkEndpointType type;
    IPAddress ip;
    const char *hostname;
    uint16_t port;
};

#endif

#if NETWORK_SERVICE_DISCOVERY

#include <ArduinoMDNS.h>

/**
 * This class is a wrapper for the service discovery via mDNS/Bonjour.
 */
class ServiceDiscovery
{
public:
  /**
   * Creates a new instance of the ServiceDiscovery class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  ServiceDiscovery(UDP& udpClient);

  /**
   * Cleanup resources created and managed by the ServiceDiscovery class.
   */
  ~ServiceDiscovery();

  /**
   * Scan for networks and connect to the one configured.
   *
   * @param ip IP address of the network interface (WiFi, Ethernet, etc).
   * @param hostname Hostname set on the network interface.
   * @param mac MAC address of the network interface.
   */
  void begin(IPAddress ip, const char* hostname, uint8_t* mac);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

#if USE_HOME_ASSISTANT
  /**
   * Returns the network endpoint to use for home assistant
   */
  inline NetworkEndpoint* haEndpoint() { return &_haEndpoint; }

  /**
   * Registers callback that will be called each time the ha endpoint is updated.
   *
   * @param callback
   */
  inline void onHaEndpointUpdated(void (*callback)(NetworkEndpoint *endpoint)) { haEndpointUpdatedCallback = callback; }
#endif

  /**
   * Returns existing instance (singleton) of the ServiceDiscovery class.
   * It may be a null pointer if the ServiceDiscovery object was never constructed or it was destroyed.
   */
  inline static ServiceDiscovery *instance() { return _instance; }

  void serviceFound(const char* type,
                    MDNSServiceProtocol_t protocol,
                    const char* name,
                    IPAddress address,
                    uint16_t port,
                    const char* txt);

private:
    MDNS mdns;
#if USE_HOME_ASSISTANT
  NetworkEndpoint _haEndpoint;
#endif

private:
  void (*haEndpointUpdatedCallback)(NetworkEndpoint *endpoint);
  unsigned long discoverTimepoint;

  /// Living instance of the ServiceDiscovery class. It can be nullptr.
  static ServiceDiscovery *_instance;

private:
  void discover(bool initial);
  void updateNetworkEndpoint(NetworkEndpoint *endpoint, IPAddress* ip, const char *hostname, uint16_t port);
};

#endif // NETWORK_SERVICE_DISCOVERY

#endif // SERVICE_DISCOVERY_H
