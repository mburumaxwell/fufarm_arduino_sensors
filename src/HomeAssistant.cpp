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
#endif
#ifdef HAVE_PH
  ph(HOME_ASSISTANT_DEVICE_NAME"_ph", HASensorNumber::PrecisionP2),
#endif
#ifdef HAVE_MOISTURE
  moisture(HOME_ASSISTANT_DEVICE_NAME"_moisture"),
#endif
  mqtt(client, device)
{
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
  mqtt.disconnect();
}

void FuFarmHomeAssistant::setUniqueDeviceId(const uint8_t *value, uint8_t length)
{
  device.setUniqueId(value, length);
}

void FuFarmHomeAssistant::connect(const NetworkEndpoint *endpoint)
{
  // if already connected, disconnect (assumes this method is only called when the endpoint is acquired or has changed)
  if (connected()) {
    mqtt.disconnect();
  }

  const char *username = HOME_ASSISTANT_MQTT_USERNAME;
  const char *password = HOME_ASSISTANT_MQTT_PASSWORD;

  if (endpoint->type == NetworkEndpointType::DNS) {
    mqtt.begin(endpoint->hostname, endpoint->port, username, password);
  }
  else if (endpoint->type == NetworkEndpointType::IP) {
    mqtt.begin(endpoint->ip, endpoint->port, username, password);
  }
  else {
    Serial.print("Unknown or unhandled network endpoint type: ");
    Serial.println(endpoint->type);
  }
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

#endif // USE_HOME_ASSISTANT
