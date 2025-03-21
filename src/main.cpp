#include "config.h"

#if defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
#include <WiFiNINA.h>
#endif

#include <ArduinoJson.h>
#include "sensors.h"

#define BUFFER_SIZE 256

extern void sen0217InterruptHandler(); // defined later in the file
FuFarmSensors sensors(sen0217InterruptHandler);
FuFarmSensorsData sensorsData;
static boolean calibrationMode = false;

// Wifi control
#if HAVE_WIFI
#include <PubSubClient.h>

String HA_DISCOVERY_PREFIX = "homeassistant";
String MQTT_CLIENT_ID(HOME_ASSISTANT_MQTT_CLIENT_ID);
int wifiStatus = WL_IDLE_STATUS; // the Wifi radio's status
WiFiClient wifiClient;
PubSubClient client(wifiClient);
#else
StaticJsonDocument<200> doc;
#endif

void sen0217InterruptHandler() // this exists because there is no way to pass an instance method to the interrupt
{
#ifdef HAVE_FLOW
  sensors.sen0217Interrupt();
#endif
}

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
#ifdef MOCK
  Serial.println("Skipping connectToWifi");
  return;
#endif
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
#ifdef MOCK
  Serial.println("Skipping shutdownWifi");
  return;
#endif
  Serial.println("*** shutdownWifi ***");
  WiFi.disconnect();
  WiFi.end();
}

String haSensorName(String name){
  String sensor_name = MQTT_CLIENT_ID + "_" + name;
  return sensor_name;
}

String haSensorTopic(String name, String type, bool isBinary){
  String sensor_name = haSensorName(name);
  String sensor_prefix = isBinary ? "binary_sensor" : "sensor";
  String topic = HA_DISCOVERY_PREFIX + "/" + sensor_prefix + "/" + sensor_name + "/" + type;
  return topic;
}

void haAnnounceSensor(String name, String type, bool isBinary, JsonDocument& payload, char buffer[]){
  String sensor_name = haSensorName(name);
  String config_topic = haSensorTopic(name, "config", isBinary);
  String state_topic = haSensorTopic(name, "state", isBinary);
  payload["name"] = sensor_name;
  payload["device_class"] = type; // https://www.home-assistant.io/integrations/sensor/#device-class
  payload["state_topic"] = state_topic;
  payload["unique_id"] = sensor_name;
  // payload["unit_of_measurement"] = measurement;
  payload["expire_after"] = (String)(SAMPLE_WINDOW * 2);
  serializeJson(payload, buffer, BUFFER_SIZE);
  String info = "Announcing sensor: " + config_topic + "\n" + buffer;
  Serial.println(info);
  client.publish(config_topic.c_str(), buffer, true);
  payload.clear();
}

void haRegisterSensors() {
  StaticJsonDocument<200> payload;
  char buffer[BUFFER_SIZE];
#ifdef HAVE_WATER_LEVEL_STATE
  // https://www.home-assistant.io/integrations/binary_sensor/
  haAnnounceSensor(String("water_level"), String("moisture"), true, payload, buffer);
#endif
#ifdef HAVE_LIGHT
  haAnnounceSensor(String("illuminance"), String("illuminance"), false, payload, buffer);
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  haAnnounceSensor(String("temperature"), String("temperature"), false, payload, buffer);
  haAnnounceSensor(String("humidity"), String("humidity"), false, payload, buffer);
#endif
#if defined(HAVE_ENS160)
  haAnnounceSensor(String("aqi"), String("aqi"), false, payload, buffer);
  haAnnounceSensor(String("tvoc"), String("volatile_organic_compounds_parts"), false, payload, buffer);
  haAnnounceSensor(String("ec02"), String("carbon_dioxide"), false, payload, buffer);
#endif
#ifdef HAVE_FLOW
  haAnnounceSensor(String("volume_flow_rate"), String("volume_flow_rate"), false, payload, buffer);
#endif
#ifdef HAVE_TEMP_WET
  haAnnounceSensor(String("liquidtemp"), String("temperature"), false, payload, buffer);
#endif
#ifdef HAVE_CO2
  haAnnounceSensor(String("carbon_dioxide"), String("carbon_dioxide"), false, payload, buffer);
#endif
#ifdef HAVE_EC
  haAnnounceSensor(String("ec"), String("ec"), false, payload, buffer);
#endif
#ifdef HAVE_PH
  haAnnounceSensor(String("ph"), String("ph"), false, payload, buffer);
#endif
#ifdef HAVE_MOISTURE
  haAnnounceSensor(String("moisture"), String("moisture"), false, payload, buffer);
#endif
}

void haPublishSensor(String name, bool isBinary, String value){
  String topic = haSensorTopic(name, "state", isBinary);
  String info = "Publishing sensor: " + topic + " : " + value;
  Serial.println(info);
#ifndef MOCK
  client.publish(topic.c_str(), value.c_str(), true);
#endif // MOCK
}

  void haPublishData(FuFarmSensorsData *data) {
    String value = "";
    String sensor = "";
#ifdef HAVE_WATER_LEVEL_STATE
    sensor = "water_level";
    value = data->waterLevelState ? "ON" : "OFF";
    haPublishSensor(sensor, true, value);
#endif
#ifdef HAVE_LIGHT
    sensor = "illuminance";
    value = (String)data->light;
    haPublishSensor(sensor, false, value);
#endif
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
    sensor = "temperature";
    value = (String)data->temperature.air;
    haPublishSensor(sensor, false, value);
    sensor = "humidity";
    value = (String)data->humidity;
    haPublishSensor(sensor, false, value);
#endif
#if defined(HAVE_ENS160)
    sensor = "aqi";
    value = (String)sensorsData.airQuality.index;
    haPublishSensor(sensor, false, value);
    sensor = "tvoc";
    value = (String)data->airQuality.tvoc;
    haPublishSensor(sensor, false, value);
    sensor = "ec02";
    value = (String)data->airQuality.eco2;
    haPublishSensor(sensor, false, value);
#endif

#ifdef HAVE_FLOW
    sensor = "volume_flow_rate";
    value = (String)data->flow;
    haPublishSensor(sensor, false, value);
#endif
#ifdef HAVE_TEMP_WET
  sensor = "tempwet";
  value = (String)data->temperature.wet;
  haPublishSensor(sensor, false, value);
#endif
#ifdef HAVE_CO2
  sensor = "carbon_dioxide";
  value = (String)data->co2;
  haPublishSensor(sensor, false, value);
#endif
#ifdef HAVE_EC
  sensor = "ec";
  value = (String)data->ec;
  haPublishSensor(sensor, false, value);
#endif
#ifdef HAVE_PH
  sensor = "ph";
  value = (String)data->ph;
  haPublishSensor(sensor, false, value);
#endif
#ifdef HAVE_MOISTURE
  sensor = "moisture";
  value = (String)data->moisture;
  haPublishSensor(sensor, false, value);
#endif
    }


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("INFO: Attempting MQTT connection...");
    // Attempt to connect
    client.setKeepAlive(HOME_ASSISTANT_MQTT_KEEPALIVE);
#ifdef HOME_ASSISTANT_MQTT_USER
    bool connected = client.connect(HOME_ASSISTANT_MQTT_CLIENT_ID, HOME_ASSISTANT_MQTT_USER, HOME_ASSISTANT_MQTT_PASSWORD);
#else
    bool connected = client.connect(HOME_ASSISTANT_MQTT_CLIENT_ID);
#endif
    if (connected)
    {
      Serial.println("INFO: connected");
    }
    else
    {
      Serial.print("ERROR: failed, rc=");
      Serial.println(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
#endif // HAVE_WIFI

void setup()
{
  // pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
#if defined(ARDUINO_AVR_LEONARDO)
  analogReference(DEFAULT); // Set the default voltage of the reference voltage
#elif defined(ARDUINO_UNOR4_WIFI)
  analogReference(AR_DEFAULT); // AR_DEFAULT: 5V on the Uno R4 WiFi
  analogReadResolution(14); //change to 14-bit resolution
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
  } else {
    Serial.println(F("Assuming sensors have been calibrated. To enter calibration mode, short the calibration toggle and reset."));
  }
#endif

#ifndef MOCK
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

  // init the MQTT connection
  client.setServer(HOME_ASSISTANT_MQTT_SERVER_IP, HOME_ASSISTANT_MQTT_SERVER_PORT);
  // client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  haRegisterSensors();
  client.disconnect();
#endif // HAVE_WIFI
#endif // MOCK
} // end setup

void loop()
{
  if (calibrationMode) {
    sensors.calibration();
    return; // nothing else can happen when in calibration mode
  }

  // Serial.println("Starting main loop");
  // digitalWrite(LED_BUILTIN, HIGH);

#ifdef ARDUINO_AVR_UNO_WIFI_REV2
  connectToWifi();
#endif

  sensors.read(&sensorsData);

#if HAVE_WIFI
#ifndef MOCK
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
#endif // MOCK
  haPublishData(&sensorsData);
#ifndef MOCK
  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();
#endif // MOCK
#endif // HAVE_WIFI


#ifdef ARDUINO_AVR_UNO_WIFI_REV2
  //   // If no Wifi signal, try to reconnect it
  //  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
  //    Serial.println("Wifi connection lost");
  //  }
  // Need to shutdown wifi due to bug in Wifi: https://github.com/arduino-libraries/WiFiNINA/issues/103 | https://github.com/arduino-libraries/WiFiNINA/issues/207
  shutdownWifi();
#endif
#if HAVE_WIFI
#else
  // populate json
#if defined(HAVE_DHT22) || defined(HAVE_AHT20)
  doc["tempair"] = sensorsData.temperature.air;
  doc["humidity"] = sensorsData.humidity;
#endif
#ifdef HAVE_ENS160
  doc["aqi"] = sensorsData.airQuality.index;
  doc["tvoc"] = sensorsData.airQuality.tvoc;
  doc["ec02"] = sensorsData.airQuality.eco2;
#endif
#ifdef HAVE_TEMP_WET
  doc["tempwet"] = sensorsData.temperature.wet;
#endif
#ifdef HAVE_C02
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
  delay(SAMPLE_WINDOW);
} // end loop
