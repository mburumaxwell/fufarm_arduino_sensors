#include "config.h"

#ifndef HOME_ASSISTANT_H
#define HOME_ASSISTANT_H

#if USE_HOME_ASSISTANT

#include <ArduinoHA.h>
#include "sensors.h"

#ifdef HAVE_EC_DOSING
#include "AutoDoser.h"
#endif

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
   * If the connection has not been established, nothing is published.
   * In some cases, if a sensor value is the same as the previous one the MQTT message won't be published.
   * This means MQTT messages are not always produced when the update method is called.
   *
   * @param source All values for configured sensors.
   * @param force Forces to update the state without comparing it to a previous known state.
   */
  void update(FuFarmSensorsData *source, const bool force = true);

#ifdef HAVE_EC_DOSING
  /**
   * Publishes MQTT messages for auto doser state.
   * In some cases, if a value is the same as the previous one the MQTT message won't be published.
   * It means that MQTT messages will produced each time the setValues method is called but they might not
   * be the same from time to time.
   *
   * @param source Auto doser config.
   * @param force Forces to update the state without comparing it to a previous known state.
   */
  void update(FuFarmAutoDoserState *source, const bool force = false);

  /**
   * Sets initial values of the auto dosing.
   * This method should be called only once, when the auto dosing is initialized.
   * It should be called before the begin() method.
   *
   * @param config Initial configuration of the auto dosing.
   * @param state Initial state of the auto dosing.
   */
  void setInitialECAutoDosingValues(const FuFarmAutoDoserConfig *config, const FuFarmAutoDoserState *state);

  /**
   * Registers callback that will be called each time the EC auto dosing configuration is updated in the HA panel.
   *
   * @param callback
   */
  inline void onECAutoDosingConfigUpdated(void (*callback)(FuFarmAutoDoserConfig *config)) { ecAutoDosingConfigUpdatedCallback = callback; }
#endif

  /**
   * Returns the current state of the MQTT connection.
   * @return true if connected, false otherwise.
   */
  inline bool connected() { return mqtt.isConnected(); }

  /**
   * Returns existing instance (singleton) of the FuFarmHomeAssistant class.
   * It may be a null pointer if the FuFarmHomeAssistant object was never constructed or it was destroyed.
   */
  inline static FuFarmHomeAssistant *instance() { return _instance; }

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
#ifdef HAVE_EC_DOSING
  HASwitch ecAutoDosingEnabled;
  HANumber ecAutoDosingDuration, ecAutoDosingTarget, ecAutoDosingEquilibriumTime;
  HASensorNumber ecAutoDosingCount;
  HASensor ecAutoDosingLastTime;
  HASensorNumber ecAutoDosingTotalDuration;

  void onECAutoDosingEnabledCommand(bool state, HASwitch *sender);
  void onECAutoDosingDurationCommand(HANumeric number, HANumber *sender);
  void onECAutoDosingTargetCommand(HANumeric number, HANumber *sender);
  void onECAutoDosingEquilibriumTimeCommand(HANumeric number, HANumber *sender);

  FuFarmAutoDoserConfig ecAutoDosingConfig;
  FuFarmAutoDoserState ecAutoDosingState;
  void (*ecAutoDosingConfigUpdatedCallback)(FuFarmAutoDoserConfig *config);
  void updateECAutoDosingConfig();
#endif
#endif
#ifdef HAVE_PH
  HASensorNumber ph;
#endif
#ifdef HAVE_MOISTURE
  HASensorNumber moisture;
#endif

  /// Living instance of the FuFarmSensors class. It can be nullptr.
  static FuFarmHomeAssistant *_instance;

private:
  /**
   * Formats the given time in ISO 8601 format (`YYYY-MM-DDTHH:MM:SSZ`) e.g. `2023-10-01T12:00:00Z`.
   * The time is assumed to be UTC.
   * @param buffer Buffer to store the formatted time.
   * @param size Size of the buffer.
   * @param info Pointer to the tm structure containing the time information.
   * @note The buffer should be large enough (at least 21 bytes) to hold the formatted string.
   */
  void formatISO8601(char *buffer, size_t size, tm *info);
};

#endif // USE_HOME_ASSISTANT

#endif // HOME_ASSISTANT_H
