#include "config.h"
#include "sensors.h"

#if HAVE_WIFI
#include "HomeAssistant.h"
#include "WiFiManager.h"
#else
#include <ArduinoJson.h>
#endif

FuFarmSensors sensors;
FuFarmSensorsData sensorsData;
static boolean calibrationMode = false;

// Wifi control
#if HAVE_WIFI
WiFiManager wifiManager;
WiFiClient wifiClient;
FuFarmHomeAssistant ha(wifiClient);
#else
StaticJsonDocument<200> doc;
#endif

void setup()
{
  Serial.begin(9600);

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
#if defined(ARDUINO_AVR_LEONARDO)
  analogReference(DEFAULT); // Set the default voltage of the reference voltage
#elif defined(ARDUINO_UNOR4_WIFI)
  analogReference(AR_DEFAULT); // AR_DEFAULT: 5V on the Uno R4 WiFi
  analogReadResolution(14);    // change to 14-bit resolution
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
  analogReference(VDD); // VDD: Vdd of the ATmega4809. 5V on the Uno WiFi Rev2
#else // any other board we have not validated
#pragma message "⚠️ Unable to set analogue reference voltage. Board not supported."
#endif

  sensors.begin();

  // if calibration toggle is shorted then we are in calibration mode
#ifdef SUPPORTS_CALIBRATION
  pinMode(CALIBRATION_TOGGLE_PIN, INPUT_PULLUP);
  calibrationMode = digitalRead(CALIBRATION_TOGGLE_PIN) == 0;
  if (calibrationMode)
  {
    Serial.println(F("Calibration toggle has been shorted hence enter calibration mode. To exit, remove the short and reset."));
    Serial.println(F("EC and PH sensors take calibration commands via Serial Monitor/Terminal."));
    Serial.println(F("Available commands for PH sensor:"));
    Serial.println(F("enterph -> enter the calibration mode for PH"));
    Serial.println(F("calph   -> calibrate with the standard buffer solution, two buffer solutions(4.0 and 7.0) will be automatically recognized"));
    Serial.println(F("exitph  -> save the calibrated parameters and exit from calibration mode"));
    Serial.println(F("Available commands for EC sensor:"));
    Serial.println(F("enterec -> enter the calibration mode"));
    Serial.println(F("calec   -> calibrate with the standard buffer solution, two buffer solutions(1413us/cm and 12.88ms/cm) will be automatically recognized"));
    Serial.println(F("exitec  -> save the calibrated parameters and exit from calibration mode"));
    return; // no further initialization can happen when in calibration mode
  }
  else
  {
    Serial.println(F("Assuming sensors have been calibrated. To enter calibration mode, short the calibration toggle and reset."));
  }
#endif

#if HAVE_WIFI
  wifiManager.begin();

  uint8_t mac[6];
  WiFi.macAddress(mac);
  ha.setUniqueDeviceId(mac, sizeof(mac));
  Serial.print("Home Assistant Unique Device ID: ");
  Serial.println(ha.getUniqueId());
  ha.begin();

#endif // HAVE_WIFI
} // end setup

static unsigned long timepoint = millis();

void loop()
{
  if (calibrationMode)
  {
    sensors.calibration();
    return; // nothing else can happen when in calibration mode
  }

  if ((millis() - timepoint) >= SAMPLE_WINDOW_MILLIS)
  {
    timepoint = millis();
    sensors.read(&sensorsData);
#if HAVE_WIFI
    ha.setValues(&sensorsData);
#else
    // populate json
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
    if (sensorsData.temperatureAir.has_value())
    {
      doc["tempair"] = sensorsData.temperatureAir.value();
    }
    if (sensorsData.humidity.has_value())
    {
      doc["humidity"] = sensorsData.humidity.value();
    }
#endif
#ifdef HAVE_ENS160
    if (sensorsData.aqi.has_value())
    {
      doc["aqi"] = sensorsData.aqi.value();
    }
    if (sensorsData.tvoc.has_value())
    {
      doc["tvoc"] = sensorsData.tvoc.value();
    }
    if (sensorsData.eco2.has_value())
    {
      doc["eco2"] = sensorsData.eco2.value();
    }
#endif
#ifdef HAVE_TEMP_WET
    if (sensorsData.temperatureWet.has_value())
    {
      doc["tempwet"] = sensorsData.temperatureWet.value();
    }
#endif
#ifdef HAVE_CO2
    if (sensorsData.co2.has_value())
    {
      doc["co2"] = sensorsData.co2.value();
    }
#endif
#ifdef HAVE_EC
    if (sensorsData.ec.has_value())
    {
      doc["ec"] = sensorsData.ec.value();
    }
#endif
#ifdef HAVE_PH
    if (sensorsData.ph.has_value())
    {
      doc["ph"] = sensorsData.ph.value();
    }
#endif
#ifdef HAVE_FLOW
    if (sensorsData.flow.has_value())
    {
      doc["flow"] = sensorsData.flow.value();
    }
#endif
#ifdef HAVE_LIGHT
    if (sensorsData.light.has_value())
    {
      doc["light"] = sensorsData.light.value();
    }
#endif
#ifdef HAVE_MOISTURE
    if (sensorsData.moisture.has_value())
    {
      doc["moisture"] = sensorsData.moisture.value();
    }
#endif
#ifdef HAVE_WATER_LEVEL_STATE
    if (sensorsData.waterLevelState.has_value())
    {
      doc["water_level"] = sensorsData.waterLevelState.value();
    }
#endif

    serializeJson(doc, Serial);
    Serial.println();
    Serial.flush();
#endif
  }

#if HAVE_WIFI
  wifiManager.maintain();
  ha.maintain();
#endif // HAVE_WIFI

  // This delay should be short so that the networking stuff is maintained correctly.
  // Network maintenance includes checking for WiFi connection, server connection, and sending PINGs.
  // Updating of sensor values happens over a longer delay by checking elapsed time above.
  delay(500);
}
