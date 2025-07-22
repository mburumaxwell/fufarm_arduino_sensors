#ifndef CONFIG_H
#define CONFIG_H

// // Uncomment to skip WiFi connection for testing sensors
// #define HAVE_WIFI 0

// The amount of time to wait for a WiFi connection before rebooting
#define WIFI_CONNECTION_REBOOT_TIMEOUT_MILLIS 15000 // 15 seconds

#ifdef SENSORS_LIGHT_PIN
  #define HAVE_LIGHT
#endif

#ifdef SENSORS_CO2_PIN
  #define HAVE_CO2
#endif

#ifdef SENSORS_EC_PIN
  #define HAVE_EC
#endif

#ifdef SENSORS_PH_PIN
  #define HAVE_PH
#endif

#ifdef SENSORS_MOISTURE_PIN
  #define HAVE_MOISTURE
#endif

#ifdef SENSORS_DHT22_PIN
  #define HAVE_DHT22
#endif

#ifdef SENSORS_SEN0217_PIN
  #define HAVE_FLOW
#endif

#ifdef SENSORS_SEN0204_PIN
  #define HAVE_WATER_LEVEL_STATE
#endif

#ifdef SENSORS_DS18S20_PIN
  #define HAVE_TEMP_WET
#endif

#ifdef CALIBRATION_TOGGLE_PIN
  #define SUPPORTS_CALIBRATION
#endif

// Validation of the sensor selection
#if !defined(HAVE_TEMP_WET) && (defined(HAVE_EC) || defined(HAVE_PH))
  #pragma message "⚠️ Without DS18S20 (wet temperature), calibration of EC and PH sensors is done using air temperature which may not be as accurate!"
#endif

#if defined(HAVE_DHT22) && defined(HAVE_AHT20)
  #error "Only one temperature and humidity sensor can be configured."
#endif

#if defined(HAVE_ENS160) && (!defined(HAVE_DHT22) && !defined(HAVE_AHT20))
  #error "Air temperature and humidity are needed to ensure ENS160 works correctly! Use either DHT22 or AHT20"
#endif

#if !defined(HAVE_LIGHT) && !defined(HAVE_CO2) && \
    !defined(HAVE_EC) && !defined(HAVE_PH) && \
    !defined(HAVE_MOISTURE) && !defined(HAVE_DHT22) && \
    !defined(HAVE_FLOW) && !defined(HAVE_TEMP_WET) && \
    !defined(HAVE_WATER_LEVEL_STATE) && !defined(HAVE_AHT20) && \
    !defined(HAVE_ENS160)
  #error "At least one sensor must be configured"
#endif

// WiFi
#ifndef HAVE_WIFI
  #define HAVE_WIFI 1
  // #define HAVE_WIFI 0
#endif

#if (defined(WIFI_ENTERPRISE_USERNAME) && !defined(WIFI_ENTERPRISE_PASSWORD)) || \
    (!defined(WIFI_ENTERPRISE_USERNAME) && defined(WIFI_ENTERPRISE_PASSWORD))
  #error "Enterprise WIFI (802.1x) requires both username and password"
#else
  #ifndef WIFI_ENTERPRISE_IDENTITY
    #define WIFI_ENTERPRISE_IDENTITY "" // default in the library
  #endif
  #ifndef WIFI_ENTERPRISE_CA
    #define WIFI_ENTERPRISE_CA "" // default in the library
  #endif
#endif

// General Network
#if HAVE_WIFI // || HAVE_ETHERNET || HAVE_CELLULAR
  #define HAVE_NETWORK 1
#else
  #define HAVE_NETWORK 0
#endif

#if HAVE_NETWORK
  #define MAC_ADDRESS_LENGTH 6
#endif

// Network Service Discovery (mDNS/Bonjour)
#if HAVE_NETWORK && !defined(HOME_ASSISTANT_MQTT_HOST)
  #define NETWORK_SERVICE_DISCOVERY 1
#else
  #define NETWORK_SERVICE_DISCOVERY 0
#endif

// Home Assistant
#if HAVE_NETWORK
  #define USE_HOME_ASSISTANT 1
#else
  #define USE_HOME_ASSISTANT 0
#endif

#include <Version.h>
#ifndef DEVICE_SOFTWARE_VERSION
#define DEVICE_SOFTWARE_VERSION "0.1.0"
#endif
#define DEVICE_MANUFACTURER "Farm Urban"
#define DEVICE_MODEL "Distributed Farm"

#if USE_HOME_ASSISTANT
  #define HOME_ASSISTANT_DISCOVERY_PREFIX "homeassistant"
  #define HOME_ASSISTANT_DATA_PREFIX "homeassistant"

  // The HomeAssistant library uses PubSubClient which does not support Publishing with QoS=1 but supports subscribe with QoS=1.
  // PUBACK is only sent in response to PUBLISH when QoS=1. With the library in use, it is similar to fire-and-forget.
  // If the keep alive value is larger than the sample duration (SAMPLE_WINDOW_MILLIS), we never end up sending the PING packets.
  // Consequently, we never get to detect server disconnection.
  // If we have a lower value here, PING packets are sent every now and then meaning we can detect disconnects sooner.
  // It would be a good idea to calculate the value by halfing the sample duration (SAMPLE_WINDOW_MILLIS) but it may result in too much noise.
  #define HOME_ASSISTANT_MQTT_KEEPALIVE 30
#endif

#if HOME_ASSISTANT_MQTT_TLS
extern const char root_ca_certs[];
#endif

#endif // CONFIG_H
