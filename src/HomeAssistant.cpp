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
  waterLevel.setDeviceClass("moisture");
  waterLevel.setIcon("mdi:cup-water");
  waterLevel.setName("Water Tank Level");
  waterLevel.setExpireAfter(EXPIRE_AFTER_SECONDS);
#endif
#ifdef HAVE_LIGHT
  // TODO: the sensor we are using does not support lux, we need to consider using a custom class
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
  ec.setUnitOfMeasurement("ms/cm");
  ec.setExpireAfter(EXPIRE_AFTER_SECONDS);
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

#endif // USE_HOME_ASSISTANT
