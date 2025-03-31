#include "config.h"

#ifndef HOME_ASSISTANT_H
#define HOME_ASSISTANT_H

#if USE_HOME_ASSISTANT

#include <ArduinoHA.h>
#include "sensors.h"

/**
 * This class is a wrapper for the Home Assistant logic.
 * It is where all the Home Assistant related code is located.
 */
class FuFarmHomeAssistant
{
public:
  /**
   * Creates a new instance of the FuFarmHomeAssistant class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param client The Etherclient or WiFiClient that's going to be used for the network communication.
   */
  FuFarmHomeAssistant(Client &client);

  /**
   * Cleanup resources created and managed by the FuFarmHomeAssistant class.
   */
  ~FuFarmHomeAssistant();

  /**
   * Returns pointer to the unique ID. It can be nullptr if the device has no ID assigned.
   */
  inline const char *getUniqueId() const { return device.getUniqueId(); }

  /**
   * Sets unique ID of the device based on the given byte array.
   * Each byte is converted into a hex string representation, so the final length of the unique ID will be twice as given.
   *
   * @param uniqueId Bytes array that's going to be converted into the string.
   * @param length Number of bytes in the array.
   */
  void setUniqueDeviceId(const uint8_t *value, uint8_t length);

  /**
   * Sets parameters of the MQTT connection using the IP address and port.
   * Also sets device_class, units, expiry, and other properties of each sensor.
   * Connection to the broker will be done in the first loop cycle.
   * This class automatically reconnects to the broker if connection is lost.
   *
   * Connection parameters (host, port, username, password, etc) are pulled from the build flags.
   */
  void begin();

  /**
   * Sets parameters of the MQTT connection using the IP address and port.
   * Also sets device_class, units, expiry, and other properties of each sensor.
   * Connection to the broker will be done in the first loop cycle.
   * This class automatically reconnects to the broker if connection is lost.
   *
   * @param host Domain or IP address of the MQTT broker.
   * @param port Port of the MQTT broker.
   * @param username Username for authentication. It can be nullptr if the anonymous connection needs to be performed.
   * @param password Password for authentication. It can be nullptr if the anonymous connection needs to be performed.
   */
  void begin(const char *host, const uint16_t port = 1883, const char *username = nullptr, const char *password = nullptr);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

  /**
   * Publishes MQTT messages for configured sensors.
   * In some cases, if a sensor value is the same as the previous one the MQTT message won't be published.
   * It means that MQTT messages will produced each time the setValues method is called but they might not
   * be the same from time to time.
   *
   * @param source All values for configured sensors.
   * @param force Forces to update the state without comparing it to a previous known state.
   */
  void setValues(FuFarmSensorsData *source, const bool force = true);

private:
  HADevice device;
  HAMqtt mqtt;

private:
#ifdef HAVE_WATER_LEVEL_STATE
  HABinarySensor waterLevel;
#endif
#ifdef HAVE_LIGHT
  HASensorNumber light;
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  HASensorNumber temperature, humidity;
#endif
#ifdef HAVE_ENS160
  HASensorNumber aqi, tvoc, eco2;
#endif
#ifdef HAVE_FLOW
  HASensorNumber flow;
#endif
#ifdef HAVE_TEMP_WET
  HASensorNumber liquidtemp;
#endif
#ifdef HAVE_CO2
  HASensorNumber co2;
#endif
#ifdef HAVE_EC
  HASensorNumber ec;
#endif
#ifdef HAVE_PH
  HASensorNumber ph;
#endif
#ifdef HAVE_MOISTURE
  HASensorNumber moisture;
#endif
};

#endif // USE_HOME_ASSISTANT

#endif // HOME_ASSISTANT_H
