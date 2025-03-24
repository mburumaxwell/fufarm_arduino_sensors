#include "HomeAssistant.h"

#if USE_HOME_ASSISTANT

#ifndef HOME_ASSISTANT_MQTT_USERNAME
#define HOME_ASSISTANT_MQTT_USERNAME nullptr
#endif

#ifndef HOME_ASSISTANT_MQTT_PASSWORD
#define HOME_ASSISTANT_MQTT_PASSWORD nullptr
#endif

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
#endif
#ifdef HAVE_LIGHT
  light.setDeviceClass("illuminance");
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  temperature.setDeviceClass("temperature");
  humidity.setDeviceClass("humidity");
#endif
#ifdef HAVE_ENS160
  aqi.setDeviceClass("aqi");
  tvoc.setDeviceClass("volatile_organic_compounds_parts");
  eco2.setDeviceClass("carbon_dioxide");
#endif
#ifdef HAVE_FLOW
  flow.setDeviceClass("volume_flow_rate");
#endif
#ifdef HAVE_TEMP_WET
  liquidtemp.setDeviceClass("temperature");
#endif
#ifdef HAVE_CO2
  co2.setDeviceClass("carbon_dioxide");
#endif
#ifdef HAVE_EC
  ec.setDeviceClass("ec");
#endif
#ifdef HAVE_PH
  ph.setDeviceClass("ph");
#endif
#ifdef HAVE_MOISTURE
  moisture.setDeviceClass("moisture");
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

void FuFarmHomeAssistant::setValues(FuFarmSensorsData *source)
{
#ifdef HAVE_WATER_LEVEL_STATE
  waterLevel.setState(source->waterLevelState);
#endif
#ifdef HAVE_LIGHT
  light.setValue(source->light);
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  temperature.setValue(source->temperature.air);
  humidity.setValue(source->humidity);
#endif
#ifdef HAVE_ENS160
  aqi.setValue(source->airQuality.index);
  tvoc.setValue(source->airQuality.tvoc);
  eco2.setValue(source->airQuality.eco2);
#endif
#ifdef HAVE_TEMP_WET
  liquidtemp.setValue(source->temperature.wet);
#endif
#ifdef HAVE_CO2
  co2.setValue(source->co2);
#endif
#ifdef HAVE_EC
  ec.setValue(source->ec);
#endif
#ifdef HAVE_PH
  ph.setValue(source->ph);
#endif
#ifdef HAVE_MOISTURE
  moisture.setValue(source->moisture);
#endif
}

#endif // USE_HOME_ASSISTANT
