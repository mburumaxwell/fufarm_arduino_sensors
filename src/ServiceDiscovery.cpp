#include "ServiceDiscovery.h"

#if NETWORK_SERVICE_DISCOVERY

#include "reboot.h"

#define HOME_ASSISTANT_SERVICE_NAME "_home-assistant"

ServiceDiscovery* ServiceDiscovery::_instance = nullptr;

static void serviceFoundCallback(const char* type, MDNSServiceProtocol_t protocol, const char* name, IPAddress address, uint16_t port, const char* txt)
{
  ServiceDiscovery::instance()->serviceFound(type, protocol, name, address, port, txt);
}

ServiceDiscovery::ServiceDiscovery(UDP& udpClient) : mdns(udpClient)
{
  _instance = this;
}

ServiceDiscovery::~ServiceDiscovery()
{
  _instance = nullptr;
}

void ServiceDiscovery::begin(IPAddress ip, const char* hostname, uint8_t* mac)
{
  if (!mdns.begin(ip, hostname)) {
    Serial.println("Error setting up MDNS!");
    reboot();
  }

  // We register a service to aid with checking if the device is on the network. This aids troubleshooting
  // service discovery where network has different conditions like mDNS/Bonjour disabled, different VLAN.
  // The name must start with the description as per examples.
  // Without the TXT record, sometimes execution crashes; see https://github.com/arduino-libraries/ArduinoMDNS/issues/36
  //                                                          https://github.com/arduino-libraries/ArduinoMDNS/pull/23
  // The TXT record "\x05id=01" implies TXT record with 5 bytes 'id=01' which is just a random key value pair
  // Post 7005 is random, not using 80 to avoid confusion with HTTP.
  // The service name includes the mac address to make it easier to identify multiple devices.
  // You can list this on Linux using "avahi-browse -at" and just for this one "avahi-browse -rt _fufarm._tcp"
  // On mac "dns-sd -B _services._dns-sd._tcp local" will list all services, "dns-sd -B _fufarm" will list just the ones for farm urban,
  // and "dns-sd -L "Farm Urban <mac>" _fufarm" will print the TXT for the particular device.
  char serviceName[11 + 12 + 8 + 1]; // sizeof("Farm Urban ") = 10; sizeof(mac*2) = 12; sizeof("._fufarm") = 8; + EOF (1) = 32
  snprintf(serviceName, sizeof(serviceName), "Farm Urban %02X%02X%02X%02X%02X%02X._fufarm", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  mdns.addServiceRecord(serviceName, 7005, MDNSServiceProtocol_t::MDNSServiceTCP, "\x05id=01");

  // set callback for when service is found
  mdns.setServiceFoundCallback(serviceFoundCallback);

  discover(true);
}

void ServiceDiscovery::maintain()
{
  // only if the discovery is not already in progress
  if (!mdns.isDiscoveringService()) {
    // check if the time since the last discovery is greater than 5 min
    const auto elapsedTime = millis() - discoverTimepoint;
    if (elapsedTime > (5 * 60 * 1000))
    {
      discover(false);
      discoverTimepoint = millis(); // reset the timepoint (must be done after discovery)
    }
  }

  mdns.run();
}

void ServiceDiscovery::discover(bool initial)
{
#if USE_HOME_ASSISTANT
  // The mosquitto add-on does not advertise itself so we search for Home Assistant instead
  // On macOS, you can run "dns-sd -B _services._dns-sd._udp local" to see available services
  if (initial) {
    Serial.println(F("Searching for " HOME_ASSISTANT_SERVICE_NAME "._tcp.local service ... "));
  }
  if (!mdns.startDiscoveringService(HOME_ASSISTANT_SERVICE_NAME, _MDNSServiceProtocol_t::MDNSServiceTCP, 5000)) {
    Serial.println("Error starting mDNS discovery!");
  }
#endif // USE_HOME_ASSISTANT
}

void ServiceDiscovery::serviceFound(const char* type,
                                    MDNSServiceProtocol_t protocol,
                                    const char* name,
                                    IPAddress address,
                                    uint16_t port,
                                    const char* txt)
{
#if USE_HOME_ASSISTANT
  if (name == NULL) {
    // if the endpoint type is still unknown set it to defaults
    if (_haEndpoint.type == NetworkEndpointType::UNKNOWN) {
      Serial.println(F("No Home Assistant service found. Ensure you are on the same network."));
      Serial.println(F("Custom network configurations such as through Tailscale may affect discovery."));
    }

    return;
  }

  bool initial = _haEndpoint.type != NetworkEndpointType::IP;
  bool changed = _haEndpoint.type != NetworkEndpointType::IP
              || _haEndpoint.ip != address
              || _haEndpoint.port != HOME_ASSISTANT_MQTT_PORT;
  // We use the known port for MQTT because home assistant is on a different port (usually 8123)
  updateNetworkEndpoint(&_haEndpoint, &address, NULL, HOME_ASSISTANT_MQTT_PORT );

  if (changed) {
    Serial.print(initial ? F("Found Home Assistant service at ") : F("Found updated Home Assistant service at "));
    Serial.print(_haEndpoint.ip);
    Serial.print(F(":"));
    Serial.println(port); // Serial.println(_haEndpoint.port);

    if (haEndpointUpdatedCallback) {
      haEndpointUpdatedCallback(&_haEndpoint);
    }
  }
#endif // USE_HOME_ASSISTANT
}

void ServiceDiscovery::updateNetworkEndpoint(NetworkEndpoint *endpoint, IPAddress* ip, const char *hostname, uint16_t port)
{
  if (ip != NULL) {
    endpoint->ip = *ip;
    endpoint->type = NetworkEndpointType::IP;
  }

  if (hostname != NULL) {
    endpoint->hostname = hostname;
    endpoint->type = NetworkEndpointType::DNS;
  }

  endpoint->port = port;
}

#endif // NETWORK_SERVICE_DISCOVERY
