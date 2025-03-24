#include "config.h"

#if HAVE_WIFI
#include <WiFi.h>
#else
#include <ArduinoJson.h>
#endif

#include "sensors.h"

FuFarmSensors sensors;
FuFarmSensorsData sensorsData;
static boolean calibrationMode = false;

// Wifi control
#if HAVE_WIFI
#include "HomeAssistant.h"

int wifiStatus = WL_IDLE_STATUS; // the Wifi radio's status
WiFiClient wifiClient;
FuFarmHomeAssistant ha(wifiClient);
#else
StaticJsonDocument<200> doc;
#endif

#if HAVE_WIFI
void printMacAddress(byte mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if (mac[i] < 16)
    {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0)
    {
      Serial.print(":");
    }
  }
  Serial.println();
}

void printCurrentNet()
{
  Serial.println("Currently connected to network");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case ENC_TYPE_WEP:
      Serial.println("WEP");
      break;
    case ENC_TYPE_TKIP:
      Serial.println("WPA");
      break;
    case ENC_TYPE_CCMP:
      Serial.println("WPA2");
      break;
    case ENC_TYPE_NONE:
      Serial.println("None");
      break;
    case ENC_TYPE_AUTO:
      Serial.println("Auto");
      break;
    case ENC_TYPE_UNKNOWN:
    default:
      Serial.println("Unknown");
      break;
  }
}

void listNetworks() {
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a WiFi connection");
    while (true);
  }
  Serial.print("Number of available networks: ");
  Serial.println(numSsid);
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
  }
}

void connectToWifi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("connectToWifi - already connected");
    return;
  }
  wifiStatus = WL_IDLE_STATUS;
  int attempts = 0;
  while (wifiStatus != WL_CONNECTED)
  {
    listNetworks(); // Printing this after connecting to the Wifi causes the client connection to fail. WTAF.
    Serial.println();
    if (attempts > 0){
      Serial.print("Failed to connect on attempt:");
      Serial.print(attempts);
      Serial.print(" - reason:");
      Serial.println(WiFi.reasonCode()); // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-reason-code
      Serial.println();
    }
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_SSID);
    WiFi.disconnect(); // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5
#if defined(WIFI_ENTERPRISE_USERNAME) && defined(WIFI_ENTERPRISE_PASSWORD)
    wifiStatus = WiFi.beginEnterprise(WIFI_SSID,
                                      WIFI_ENTERPRISE_USERNAME,
                                      WIFI_ENTERPRISE_PASSWORD,
                                      WIFI_ENTERPRISE_IDENTITY,
                                      WIFI_ENTERPRISE_CA);
#elif defined(WIFI_PASSPHRASE)
    wifiStatus = WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
#else
    wifiStatus = WiFi.begin(WIFI_SSID);
#endif
    delay(10000);
    attempts++;
  }
  printCurrentNet();
}

void shutdownWifi()
{
  Serial.println("*** shutdownWifi ***");
  WiFi.disconnect();
  WiFi.end();
}
#endif // HAVE_WIFI

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
  if (calibrationMode) {
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
  } else {
    Serial.println(F("Assuming sensors have been calibrated. To enter calibration mode, short the calibration toggle and reset."));
  }
#endif

#if HAVE_WIFI
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ; // don't continue
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }
  connectToWifi();

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
  if (calibrationMode) {
    sensors.calibration();
    return; // nothing else can happen when in calibration mode
  }

  if ((millis() - timepoint) >= SAMPLE_WINDOW_MILLIS) {
    timepoint = millis();
    sensors.read(&sensorsData);
#if HAVE_WIFI
    ha.setValues(&sensorsData);
    // TODO: this logic keeps disconnecting so we need to improve it (fix: only scan in setup to avoid different status)
    connectToWifi();
#else
    // populate json
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
    doc["tempair"] = sensorsData.temperature.air;
    doc["humidity"] = sensorsData.humidity;
#endif
#ifdef HAVE_ENS160
    doc["aqi"] = sensorsData.airQuality.index;
    doc["tvoc"] = sensorsData.airQuality.tvoc;
    doc["eco2"] = sensorsData.airQuality.eco2;
#endif
#ifdef HAVE_TEMP_WET
    doc["tempwet"] = sensorsData.temperature.wet;
#endif
#ifdef HAVE_CO2
    doc["co2"] = sensorsData.co2;
#endif
#ifdef HAVE_EC
    doc["ec"] = sensorsData.ec;
#endif
#ifdef HAVE_PH
    doc["ph"] = sensorsData.ph;
#endif
#ifdef HAVE_FLOW
    doc["flow"] = sensorsData.flow;
#endif
#ifdef HAVE_LIGHT
    doc["light"] = sensorsData.light;
#endif
#ifdef HAVE_MOISTURE
    doc["moisture"] = sensorsData.moisture;
#endif
#ifdef HAVE_WATER_LEVEL_STATE
    doc["water_level"] = sensorsData.waterLevelState;
#endif

    serializeJson(doc, Serial);
    Serial.println();
    Serial.flush();
#endif
  }

#if HAVE_WIFI
  ha.maintain();
#endif // HAVE_WIFI

  // This delay should be short so that the networking stuff is maintained correctly.
  // Network maintenance includes checking for WiFi connection, server connection, and sending PINGs.
  // Updating of sensor values happens over a longer delay by checking elapsed time above.
  delay(500);
}
