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

FuFarmHomeAssistant::FuFarmHomeAssistant(Client &client) :
// The strings being passed in constructors are the sensor identifiers.
// They should be unique for the device and are required by the library.
// We can probably find a better source but this works for the moment.
#ifdef HAVE_WATER_LEVEL_STATE
                                                           waterLevel("waterLevel"),
#endif
#ifdef HAVE_LIGHT
                                                           light("light"),
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
                                                           temperature("temperature"), humidity("humidity"),
#endif
#ifdef HAVE_ENS160
                                                           aqi("aqi"), tvoc("tvoc"), eco2("eco2"),
#endif
#ifdef HAVE_FLOW
                                                           flow("flow"),
#endif
#ifdef HAVE_TEMP_WET
                                                           liquidtemp("liquidtemp"),
#endif
#ifdef HAVE_CO2
                                                           co2("co2"),
#endif
#ifdef HAVE_EC
                                                           ec("ec"),
#endif
#ifdef HAVE_PH
                                                           ph("ph"),
#endif
#ifdef HAVE_MOISTURE
                                                           moisture("moisture"),
#endif
                                                           mqtt(client, device)
{
  // set device's details
  device.setName(HOME_ASSISTANT_DEVICE_NAME);
  device.setSoftwareVersion(HOME_ASSISTANT_DEVICE_SOFTWARE_VERSION);
#ifdef HOME_ASSISTANT_DEVICE_MANUFACTURER
  device.setManufacturer(HOME_ASSISTANT_DEVICE_MANUFACTURER);
#endif
#ifdef HOME_ASSISTANT_DEVICE_MODEL
  device.setModel(HOME_ASSISTANT_DEVICE_MODEL);
#endif

  // set MQTT properties
  mqtt.setDiscoveryPrefix(HOME_ASSISTANT_DISCOVERY_PREFIX);
  mqtt.setDataPrefix(HOME_ASSISTANT_DATA_PREFIX);
  mqtt.setKeepAlive(HOME_ASSISTANT_MQTT_KEEPALIVE);

  // Setup sensors with the relevant information
  // Device classes are listed at https://www.home-assistant.io/integrations/sensor/#device-class
#ifdef HAVE_WATER_LEVEL_STATE
  // TODO: find a better device class for this
  waterLevel.setDeviceClass("moisture");
  waterLevel.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_LIGHT
  light.setDeviceClass("illuminance");
  light.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  temperature.setDeviceClass("temperature");
  temperature.setExpireAfter(EXPIRE_AFTER_SECONDS);

  humidity.setDeviceClass("humidity");
  humidity.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_ENS160
  aqi.setDeviceClass("aqi");
  aqi.setExpireAfter(EXPIRE_AFTER_SECONDS);

  tvoc.setDeviceClass("volatile_organic_compounds_parts");
  tvoc.setExpireAfter(EXPIRE_AFTER_SECONDS);

  eco2.setDeviceClass("carbon_dioxide");
  eco2.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_FLOW
  flow.setDeviceClass("volume_flow_rate");
  flow.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_TEMP_WET
  liquidtemp.setDeviceClass("temperature");
  liquidtemp.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_CO2
  co2.setDeviceClass("carbon_dioxide");
  co2.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_EC
  ec.setDeviceClass("ec");
  ec.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_PH
  ph.setDeviceClass("ph");
  ph.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_MOISTURE
  moisture.setDeviceClass("moisture");
  moisture.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
}

FuFarmHomeAssistant::~FuFarmHomeAssistant()
{
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
  mqtt.begin(host, port, username, password);
}

void FuFarmHomeAssistant::maintain()
{
  mqtt.loop();
}

void FuFarmHomeAssistant::setValues(FuFarmSensorsData *source, const bool force)
{
#ifdef HAVE_WATER_LEVEL_STATE
  if (source->waterLevelState.has_value())
  {
    waterLevel.setState(source->waterLevelState.value(), force);
  }
#endif
#ifdef HAVE_LIGHT
  if (source->light.has_value())
  {
    light.setValue(source->light.value(), force);
  }
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  if (source->temperatureAir.has_value())
  {
    temperature.setValue(source->temperatureAir.value(), force);
  }
  if (source->humidity.has_value())
  {
    humidity.setValue(source->humidity.value(), force);
  }
#endif
#ifdef HAVE_ENS160
  if (source->aqi.has_value())
  {
    aqi.setValue(source->aqi.value(), force);
  }
  if (source->tvoc.has_value())
  {
    tvoc.setValue(source->tvoc.value(), force);
  }
  if (source->eco2.has_value())
  {
    eco2.setValue(source->eco2.value(), force);
  }
#endif
#ifdef HAVE_FLOW
  if (source->flow.has_value())
  {
    flow.setValue(source->flow.value(), force);
  }
#endif
#ifdef HAVE_TEMP_WET
  if (source->temperatureWet.has_value())
  {
    liquidtemp.setValue(source->temperatureWet.value(), force);
  }
#endif
#ifdef HAVE_CO2
  if (source->co2.has_value())
  {
    co2.setValue(source->co2.value(), force);
  }
#endif
#ifdef HAVE_EC
  if (source->ec.has_value())
  {
    ec.setValue(source->ec.value(), force);
  }
#endif
#ifdef HAVE_PH
  if (source->ph.has_value())
  {
    ph.setValue(source->ph.value(), force);
  }
#endif
#ifdef HAVE_MOISTURE
  if (source->moisture.has_value())
  {
    moisture.setValue(source->moisture.value(), force);
  }
#endif
}

#endif // USE_HOME_ASSISTANT
