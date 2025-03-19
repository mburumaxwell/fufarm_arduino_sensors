#ifndef CONFIG_H
#define CONFIG_H

// #define MOCK ; // Uncomment to skip wifi connection for testing sensors

#ifdef MOCK
#define SAMPLE_WINDOW 5000
#else
// Time in milliseconds - 5 minutes = 1000 * 60 * 5 = 300000
#define SAMPLE_WINDOW 60000
#endif

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

  // For this flow sensor, only interrupt pins should be used. Configured on a rising edge
  // https://reference.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

  #if defined(ARDUINO_AVR_UNO_WIFI_REV2) // all digital pins
    #if SENSORS_SEN0217_PIN < 0 || SENSORS_SEN0217_PIN > 13
      #error "Pin configured for SEN0217 (flow sensor) must support interrupts on UNO WIFI REV2."
    #endif
  #elif defined(ARDUINO_AVR_LEONARDO) // only 0, 1, 2, 3, 7
    #if SENSORS_SEN0217_PIN == 0
    #elif SENSORS_SEN0217_PIN == 1
    #elif SENSORS_SEN0217_PIN == 2
    #elif SENSORS_SEN0217_PIN == 3
    #elif SENSORS_SEN0217_PIN == 7
    #else
      #error "Pin configured for SEN0217 (flow sensor) must support interrupts on LEONARDO."
    #endif
  #else // any other board we have not validated
    #pragma message "⚠️ Unable to validate if pin configured for SEN0217 (flow sensor) allows interrupts required."
  #endif
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

// Validation of the build configuration
#if !defined(HAVE_TEMP_WET) && (defined(HAVE_EC) || defined(HAVE_PH))
  #pragma message "⚠️ Without DS18S20 (wet temperature), calibration of EC and PH sensors is done using air temperature which may not be as accurate!"
#endif

#if defined(HAVE_DHT22) && defined(HAVE_AHT20)
  #error "Only one temperature and humidity sensor can be configured."
#endif

#if !defined(HAVE_LIGHT) && !defined(HAVE_CO2) && \
    !defined(HAVE_EC) && !defined(HAVE_PH) && \
    !defined(HAVE_MOISTURE) && !defined(HAVE_DHT22) && \
    !defined(HAVE_FLOW) && !defined(HAVE_TEMP_WET) && \
    !defined(HAVE_WATER_LEVEL_STATE) && !defined(HAVE_AHT20) && \
    !defined(MOCK)
  #error "At least one sensor must be configured unless mocking"
#endif

// WiFi
#if defined(ARDUINO_AVR_UNO_WIFI_REV2)
  #define HAVE_WIFI 1
#elif defined(ARDUINO_AVR_LEONARDO)
  #define HAVE_WIFI 0
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

// MQTT
#ifndef HOME_ASSISTANT_MQTT_KEEPALIVE
  #define HOME_ASSISTANT_MQTT_KEEPALIVE 90
#endif

#endif // CONFIG_H
