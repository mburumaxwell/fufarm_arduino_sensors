#include "HomeAssistant.h"

#if USE_HOME_ASSISTANT

#ifndef HOME_ASSISTANT_MQTT_USERNAME
#define HOME_ASSISTANT_MQTT_USERNAME nullptr
#endif

#ifndef HOME_ASSISTANT_MQTT_PASSWORD
#define HOME_ASSISTANT_MQTT_PASSWORD nullptr
#endif

#define EXPIRE_AFTER_SECONDS ((SAMPLE_WINDOW_MILLIS * 2) / 1000)
#define SET_EXPIRE_AFTER(sensor) sensor.setExpireAfter(EXPIRE_AFTER_SECONDS)

FuFarmHomeAssistant *FuFarmHomeAssistant::_instance = nullptr;

FuFarmHomeAssistant::FuFarmHomeAssistant(Client &client) :
// The strings being passed in constructors are the sensor identifiers.
// They should be unique for the device and are required by the library.
// We can probably find a better source but this works for the moment.
#ifdef HAVE_WATER_LEVEL_STATE
  waterLevel(HOME_ASSISTANT_DEVICE_NAME"_waterLevel"),
#endif
#ifdef HAVE_LIGHT
  light(HOME_ASSISTANT_DEVICE_NAME"_light"),
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  temperature(HOME_ASSISTANT_DEVICE_NAME"_temperature", HASensorNumber::PrecisionP2),
  humidity(HOME_ASSISTANT_DEVICE_NAME"_humidity", HASensorNumber::PrecisionP2),
#endif
#ifdef HAVE_ENS160
  aqi(HOME_ASSISTANT_DEVICE_NAME"_aqi"),
  tvoc(HOME_ASSISTANT_DEVICE_NAME"_tvoc"),
  eco2(HOME_ASSISTANT_DEVICE_NAME"_eco2"),
#endif
#ifdef HAVE_FLOW
  flow(HOME_ASSISTANT_DEVICE_NAME"_flow", HASensorNumber::PrecisionP2),
#endif
#ifdef HAVE_TEMP_WET
  liquidtemp(HOME_ASSISTANT_DEVICE_NAME"_liquidtemp", HASensorNumber::PrecisionP2),
#endif
#ifdef HAVE_CO2
  co2(HOME_ASSISTANT_DEVICE_NAME"_co2"),
#endif
#ifdef HAVE_EC
  ec(HOME_ASSISTANT_DEVICE_NAME"_ec", HASensorNumber::PrecisionP2),
#ifdef HAVE_EC_DOSING
  ecAutoDosingEnabled(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_enabled"),
  ecAutoDosingDuration(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_duration"),
  ecAutoDosingTarget(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_target", HASensorNumber::PrecisionP2),
  ecAutoDosingEquilibriumTime(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_equilibrium_time"),
  ecAutoDosingCount(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_count"),
  ecAutoDosingLastTime(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_last_time"),
  ecAutoDosingTotalDuration(HOME_ASSISTANT_DEVICE_NAME"_ec_auto_dosing_total_duration"),
#endif
#endif
#ifdef HAVE_PH
  ph(HOME_ASSISTANT_DEVICE_NAME"_ph", HASensorNumber::PrecisionP2),
#endif
#ifdef HAVE_MOISTURE
  moisture(HOME_ASSISTANT_DEVICE_NAME"_moisture"),
#endif
  mqtt(client, device)
{
  _instance = this;

  // set device's details
  device.setName(HOME_ASSISTANT_DEVICE_NAME);
  device.setSoftwareVersion(DEVICE_SOFTWARE_VERSION);
#ifdef DEVICE_MANUFACTURER
  device.setManufacturer(DEVICE_MANUFACTURER);
#endif
#ifdef DEVICE_MODEL
  device.setModel(DEVICE_MODEL);
#endif

  // set MQTT properties
  mqtt.setDiscoveryPrefix(HOME_ASSISTANT_DISCOVERY_PREFIX);
  mqtt.setDataPrefix(HOME_ASSISTANT_DATA_PREFIX);
  mqtt.setKeepAlive(HOME_ASSISTANT_MQTT_KEEPALIVE);

  // Setup sensors with the relevant information
  // Device classes are listed at https://www.home-assistant.io/integrations/sensor/#device-class
#ifdef HAVE_WATER_LEVEL_STATE
  waterLevel.setDeviceClass("moisture");
  waterLevel.setIcon("mdi:cup-water");
  waterLevel.setName("Sump Level Indicator");
  waterLevel.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_LIGHT
  // TODO: the sensor we are using does not support lux, we may want to consider using a custom class
  light.setDeviceClass("illuminance");
  light.setUnitOfMeasurement("lx");
  light.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  temperature.setDeviceClass("temperature");
  temperature.setUnitOfMeasurement("°C");
  temperature.setExpireAfter(EXPIRE_AFTER_SECONDS);

  humidity.setDeviceClass("humidity");
  humidity.setUnitOfMeasurement("%");
  humidity.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_ENS160
  aqi.setDeviceClass("aqi");
  aqi.setExpireAfter(EXPIRE_AFTER_SECONDS);
  // no need to set unit for AQI, it is documented as unitless

  tvoc.setDeviceClass("volatile_organic_compounds_parts");
  tvoc.setUnitOfMeasurement("ppb");
  tvoc.setExpireAfter(EXPIRE_AFTER_SECONDS);

  eco2.setDeviceClass("carbon_dioxide");
  eco2.setName("Carbon Dioxide (Equivalent)");
  eco2.setUnitOfMeasurement("ppm");
  eco2.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_FLOW
  flow.setDeviceClass("volume_flow_rate");
  flow.setIcon("mdi:waves-arrow-right"); // default icon is unknown so we set ours
  flow.setUnitOfMeasurement("L/min");
  flow.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_TEMP_WET
  liquidtemp.setDeviceClass("temperature");
  liquidtemp.setName("Liquid Temperature");
  liquidtemp.setUnitOfMeasurement("°C");
  liquidtemp.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_CO2
  co2.setDeviceClass("carbon_dioxide");
  co2.setUnitOfMeasurement("ppm");
  co2.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_EC
  // As of 2025-Mar-28, Home Assistant does not have a device class for EC, we create a custom one
  ec.setIcon("mdi:waveform");
  ec.setName("Electrical Conductivity");
  ec.setUnitOfMeasurement("mS/cm");
  ec.setExpireAfter(EXPIRE_AFTER_SECONDS);
#ifdef HAVE_EC_DOSING
  ecAutoDosingEnabled.setName("EC Auto Dosing Enabled");
  ecAutoDosingEnabled.setIcon("mdi:pipe-valve");
  ecAutoDosingEnabled.onCommand([](bool state, HASwitch *sender)
                                { FuFarmHomeAssistant::instance()->onECAutoDosingEnabledCommand(state, sender); });

  ecAutoDosingDuration.setIcon("mdi:timeline-clock-outline");
  ecAutoDosingDuration.setName("EC Auto Dosing Duration");
  ecAutoDosingDuration.onCommand([](HANumeric number, HANumber *sender)
                                 { FuFarmHomeAssistant::instance()->onECAutoDosingDurationCommand(number, sender); });
  ecAutoDosingDuration.setUnitOfMeasurement("seconds");
  ecAutoDosingDuration.setMin(5);
  ecAutoDosingDuration.setMax(60);
  ecAutoDosingDuration.setStep(1);

  ecAutoDosingTarget.setIcon("mdi:waveform");
  ecAutoDosingTarget.setName("EC Auto Dosing Target");
  ecAutoDosingTarget.onCommand([](HANumeric number, HANumber *sender)
                               { FuFarmHomeAssistant::instance()->onECAutoDosingTargetCommand(number, sender); });
  ecAutoDosingTarget.setUnitOfMeasurement("mS/cm");
  ecAutoDosingTarget.setMin(0.1);
  ecAutoDosingTarget.setMax(5.0);
  ecAutoDosingTarget.setStep(0.1); // will give about 50 steps

  ecAutoDosingEquilibriumTime.setIcon("mdi:timeline-clock-outline");
  ecAutoDosingEquilibriumTime.setName("EC Auto Dosing Equilibrium Time");
  ecAutoDosingEquilibriumTime.onCommand([](HANumeric number, HANumber *sender)
                                        { FuFarmHomeAssistant::instance()->onECAutoDosingEquilibriumTimeCommand(number, sender); });
  ecAutoDosingEquilibriumTime.setUnitOfMeasurement("seconds");
  ecAutoDosingEquilibriumTime.setMin(5);
  ecAutoDosingEquilibriumTime.setMax(60);
  ecAutoDosingEquilibriumTime.setStep(1);

  ecAutoDosingCount.setName("EC Auto Dosing Count");
  ecAutoDosingCount.setUnitOfMeasurement("times");
  ecAutoDosingCount.setIcon("mdi:counter");

  ecAutoDosingLastTime.setDeviceClass("timestamp");
  ecAutoDosingLastTime.setName("EC Auto Dosing Last Time");
  ecAutoDosingLastTime.setIcon("mdi:clock-time-four-outline");

  ecAutoDosingTotalDuration.setName("EC Auto Dosing Total Duration");
  ecAutoDosingTotalDuration.setUnitOfMeasurement("seconds");
  ecAutoDosingTotalDuration.setIcon("mdi:clock-time-four-outline");
#endif
#endif
#ifdef HAVE_PH
  ph.setDeviceClass("ph");
  ph.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_MOISTURE
  moisture.setDeviceClass("moisture");
  moisture.setUnitOfMeasurement("%");
  moisture.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
}

FuFarmHomeAssistant::~FuFarmHomeAssistant()
{
  _instance = nullptr;
  mqtt.disconnect();
#ifdef HAVE_EC_DOSING
  ecAutoDosingConfigUpdatedCallback = nullptr;
#endif
}

void FuFarmHomeAssistant::setUniqueDeviceId(const uint8_t *value, uint8_t length)
{
  device.setUniqueId(value, length);
}

void FuFarmHomeAssistant::begin()
{
  begin(HOME_ASSISTANT_MQTT_HOST,
        HOME_ASSISTANT_MQTT_PORT,
        HOME_ASSISTANT_MQTT_USERNAME,
        HOME_ASSISTANT_MQTT_PASSWORD);
}

void FuFarmHomeAssistant::begin(const char *host, const uint16_t port, const char *username, const char *password)
{
  // TODO: bring sensor initialization here so that errors can be caught in setup instead of in the constructor

#ifdef HAVE_EC_DOSING
  // set values before connecting to the broker (these values will be displayed from the start)
  ecAutoDosingEnabled.setCurrentState(ecAutoDosingConfig.enabled);
  ecAutoDosingDuration.setCurrentState(ecAutoDosingConfig.duration);
  ecAutoDosingTarget.setCurrentState(ecAutoDosingConfig.targetEc);
  ecAutoDosingEquilibriumTime.setCurrentState(ecAutoDosingConfig.equilibriumTime);
  ecAutoDosingCount.setCurrentValue(ecAutoDosingState.count);
  // ecAutoDosingLastTime.setCurrentValue("1970-01-01T00:00:00+00:00");
  ecAutoDosingTotalDuration.setCurrentValue(ecAutoDosingState.totalDuration);
#endif

  mqtt.begin(host, port, username, password);
}

void FuFarmHomeAssistant::maintain()
{
  mqtt.loop();
}

void FuFarmHomeAssistant::update(FuFarmSensorsData *source, const bool force)
{
  if (!connected())
  {
    Serial.println(F("MQTT not connected, skipping update"));
    return;
  }

#ifdef HAVE_WATER_LEVEL_STATE
  waterLevel.setState(source->waterLevelState, force);
#endif
#ifdef HAVE_LIGHT
  light.setValue(source->light, force);
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  temperature.setValue(source->temperature.air, force);
  humidity.setValue(source->humidity, force);
#endif
#ifdef HAVE_ENS160
  aqi.setValue(source->airQuality.index, force);
  tvoc.setValue(source->airQuality.tvoc, force);
  eco2.setValue(source->airQuality.eco2, force);
#endif
#ifdef HAVE_FLOW
  flow.setValue(source->flow, force);
#endif
#ifdef HAVE_TEMP_WET
  liquidtemp.setValue(source->temperature.wet, force);
#endif
#ifdef HAVE_CO2
  co2.setValue(source->co2, force);
#endif
#ifdef HAVE_EC
  ec.setValue(source->ec, force);
#endif
#ifdef HAVE_PH
  ph.setValue(source->ph, force);
#endif
#ifdef HAVE_MOISTURE
  moisture.setValue(source->moisture, force);
#endif
}

#ifdef HAVE_EC_DOSING
void FuFarmHomeAssistant::update(FuFarmAutoDoserState *source, const bool force)
{
  // cache the state
  memcpy(&this->ecAutoDosingState, source, sizeof(FuFarmAutoDoserState));

  if (!connected())
  {
    Serial.println(F("MQTT not connected, skipping update"));
    return;
  }

  // update the state
  ecAutoDosingCount.setValue(ecAutoDosingState.count, force);
  char lastTimeIso[21];
  formatISO8601(lastTimeIso, sizeof(lastTimeIso), &ecAutoDosingState.lastTime);
  ecAutoDosingLastTime.setValue(lastTimeIso);
  ecAutoDosingTotalDuration.setValue(ecAutoDosingState.totalDuration, force);
}

void FuFarmHomeAssistant::setInitialECAutoDosingValues(const FuFarmAutoDoserConfig *config, const FuFarmAutoDoserState *state)
{
  // copy the config & state, logic will update in the maintain() function
  memcpy(&this->ecAutoDosingConfig, config, sizeof(FuFarmAutoDoserConfig));
  memcpy(&this->ecAutoDosingState, state, sizeof(FuFarmAutoDoserState));
}

void FuFarmHomeAssistant::onECAutoDosingEnabledCommand(bool state, HASwitch *sender)
{
  Serial.print(F("EC Auto Dosing was updated to "));
  Serial.println(state ? F("ON") : F("OFF"));

  // update the config and send it to the callback
  ecAutoDosingConfig.enabled = state;
  updateECAutoDosingConfig();

  // report the selected option back to the HA panel
  sender->setState(state);
}

void FuFarmHomeAssistant::onECAutoDosingDurationCommand(HANumeric number, HANumber *sender)
{
  // noting to do when the value is not set
  if (!number.isSet())
  {
    Serial.println(F("EC Auto Dosing Duration was updated but the value was not set"));
    return;
  }

  // get the value from the number
  uint16_t value = number.toUInt16();
  Serial.print(F("EC Auto Dosing Duration was updated to "));
  Serial.print(value);
  Serial.println(F(" seconds."));

  // ensure the value is within the range
  if (value < 5 || value > 60)
  {
    Serial.println(F("EC Auto Dosing Duration is out of range (5-60 seconds)"));
    return;
  }

  // update the config and send it to the callback
  ecAutoDosingConfig.duration = value;
  updateECAutoDosingConfig();

  // report the selected option back to the HA panel
  sender->setState(number);
}

void FuFarmHomeAssistant::onECAutoDosingTargetCommand(HANumeric number, HANumber *sender)
{
  // noting to do when the value is not set
  if (!number.isSet())
  {
    Serial.println(F("EC Auto Dosing Target EC was updated but the value was not set"));
    return;
  }

  // get the value from the number
  float value = number.toFloat();
  Serial.print(F("EC Auto Dosing Target was updated to "));
  Serial.print(value);
  Serial.println(F(" mS/cm."));

  // ensure the value is within the range
  if (value < 0.1 || value > 5.0)
  {
    Serial.println(F("EC Auto Dosing Target is out of range (0.1-5.0 mS/cm)"));
    return;
  }

  // update the config and send it to the callback
  ecAutoDosingConfig.targetEc = value;
  updateECAutoDosingConfig();

  // report the selected option back to the HA panel
  sender->setState(number);
}

void FuFarmHomeAssistant::onECAutoDosingEquilibriumTimeCommand(HANumeric number, HANumber *sender)
{
  // noting to do when the value is not set
  if (!number.isSet())
  {
    Serial.println(F("EC Auto Dosing Equilibrium Time was updated but the value was not set"));
    return;
  }

  // get the value from the number
  uint16_t value = number.toUInt16();
  Serial.print(F("EC Auto Dosing Equilibrium Time was updated to "));
  Serial.print(value);
  Serial.println(F(" seconds."));

  // ensure the value is within the range
  if (value < 5 || value > 60)
  {
    Serial.println(F("EC Auto Dosing Equilibrium Time is out of range (5-60 seconds)"));
    return;
  }

  // update the config and send it to the callback
  ecAutoDosingConfig.equilibriumTime = value;
  updateECAutoDosingConfig();

  // report the selected option back to the HA panel
  sender->setState(number);
}

void FuFarmHomeAssistant::updateECAutoDosingConfig()
{
  if (!ecAutoDosingConfigUpdatedCallback)
  {
    Serial.print(F("EC Auto Dosing Config was updated but no callback was set."));
  }
  else
  {
    ecAutoDosingConfigUpdatedCallback(&ecAutoDosingConfig);
  }
}
#endif

void FuFarmHomeAssistant::formatISO8601(char *buffer, size_t size, tm *info)
{
  // format the time in ISO 8601 format e.g. 2023-10-01T12:00:00Z
  strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", info);
  strcat(buffer, "Z");
}

#endif // USE_HOME_ASSISTANT
