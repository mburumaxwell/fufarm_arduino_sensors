#include "config.h"

/*
 * Need to update the firmware on the Wifi Uno Rev2 and upload the SSL certificate for INFLUXDB_SERVER
 * Getting this to work required multiple attempts and deleting the arduino.cc certificate. Instructions
 * are available at: https://github.com/xcape-io/ArduinoProps/blob/master/help/WifiNinaFirmware.md
 *
 * */
#include <SPI.h>
#if HAVE_WIFI
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-reason-code
#include <WiFiNINA.h>
#endif

#if defined(ARDUINO_AVR_LEONARDO) || defined(USE_HOME_ASSISTANT)
#include <ArduinoJson.h>
#endif

#include "sensors.h"

#define BUFFER_SIZE 256

#if USE_HOME_ASSISTANT
String HA_PREFIX = "homeassistant/sensor";
#include <PubSubClient.h>
// Using char* as might store in PROGMEM later
#define MQTT_KEEPALIVE 90
String MQTT_CLIENT_ID = "ard1";
char* MQTT_SERVER_IP = "192.168.8.100";
uint16_t MQTT_SERVER_PORT = 1883;
char* MQTT_USER = "hamqtt";
char* MQTT_PASSWORD = "UbT4Rn3oY7!S9L";
#endif // USE_HOME_ASSISTANT

extern void sen0217InterruptHandler(); // defined later in the file
FuFarmSensors sensors(sen0217InterruptHandler);
FuFarmSensorsData sensorsData;
static boolean calibrationMode = false;

// Wifi control
#if HAVE_WIFI
char ssid[] = "fumanc";
char pass[] = "FARM123!";
// char ssid[] = "PLUSNET-CFC9WG";
// char pass[] = "G7UtKycGmxGYDq";
int wifiStatus = WL_IDLE_STATUS; // the Wifi radio's status
WiFiClient wifiClient;
#else
StaticJsonDocument<200> doc;
#endif


#if USE_HOME_ASSISTANT
PubSubClient client(wifiClient);
#endif // USE_HOME_ASSISTANT

#ifdef HAVE_FLOW
void sen0217InterruptHandler() // this exists because there is no way to pass an instance method to the interrupt
{
  sensors.sen0217Interrupt();
}
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
    Serial.println(ssid);
    WiFi.disconnect(); // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5
    wifiStatus = WiFi.begin(ssid, pass);
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
#endif

#if USE_HOME_ASSISTANT
String haSensorName(String name){
  String sensor_name = MQTT_CLIENT_ID + "_" + name;
  return sensor_name;
}

String haSensorTopic(String name, String type){
  String sensor_name = haSensorName(name);
  String topic = HA_PREFIX + "/" + sensor_name + "/" + type;
  return topic;
}

void haAnnounceSensor(String name, String type, JsonDocument& payload, char buffer[]){
  String sensor_name = haSensorName(name);
  String config_topic = haSensorTopic(name, "config");
  String state_topic = haSensorTopic(name, "state");
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
#ifdef HAVE_LIGHT
  haAnnounceSensor(String("illuminance"), String("illuminance"), payload, buffer);
#endif
#ifdef HAVE_TEMP_HUMIDITY
  haAnnounceSensor(String("temperature"), String("temperature"), payload, buffer);
  haAnnounceSensor(String("humidity"), String("humidity"), payload, buffer);
#endif
#ifdef HAVE_FLOW
  haAnnounceSensor(String("volume_flow_rate"), String("volume_flow_rate"), payload, buffer);
#endif
#ifdef HAVE_TEMP_WET
  haAnnounceSensor(String("liquidtemp"), String("temperature"), payload, buffer);
#endif
#ifdef HAVE_CO2
  haAnnounceSensor(String("carbon_dioxide"), String("carbon_dioxide"), payload, buffer);
#endif
#ifdef HAVE_EC
  haAnnounceSensor(String("ec"), String("ec"), payload, buffer);
#endif
#ifdef HAVE_PH
  haAnnounceSensor(String("ph"), String("ph"), payload, buffer);
#endif
#ifdef HAVE_MOISTURE
  haAnnounceSensor(String("moisture"), String("moisture"), payload, buffer);
#endif
}

void haPublishSensor(String name, String value){
  String topic = haSensorTopic(name, "state");
  String info = "Publishing sensor: " + topic + " : " + value;
  Serial.println(info);
#ifndef MOCK
  client.publish(topic.c_str(), value.c_str(), true);
#endif // MOCK
}

  void haPublishData(FuFarmSensorsData *data) {
    String value = "";
    String sensor = "";
#ifdef HAVE_LIGHT
    sensor = "illuminance";
    value = (String)data->light;
    haPublishSensor(sensor, value);
#endif
#ifdef HAVE_TEMP_HUMIDITY
    sensor = "temperature";
    value = (String)data->temperature.air;
    haPublishSensor(sensor, value);
    sensor = "humidity";
    value = (String)data->humidity;
    haPublishSensor(sensor, value);
#endif
#ifdef HAVE_FLOW
    sensor = "volume_flow_rate";
    value = (String)data->flow;
    haPublishSensor(sensor, value);
#endif
#ifdef HAVE_TEMP_WET
  sensor = "tempwet";
  value = (String)data->temperature.wet;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_CO2
  sensor = "carbon_dioxide";
  value = (String)data->co2;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_EC
  sensor = "ec";
  value = (String)data->ec;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_PH
  sensor = "ph";
  value = (String)data->ph;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_MOISTURE
  sensor = "moisture";
  value = (String)data->moisture;
  haPublishSensor(sensor, value);
#endif
    }


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("INFO: Attempting MQTT connection...");
    // Attempt to connect
    client.setKeepAlive(MQTT_KEEPALIVE);
    if (client.connect(MQTT_CLIENT_ID.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
      Serial.println("INFO: connected");
    }
    else
    {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
#endif // USE_HOME_ASSISTANT

void setup()
{
  // pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
#if defined(ARDUINO_AVR_LEONARDO)
  analogReference(DEFAULT); // Set the default voltage of the reference voltage
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
  analogReference(VDD); // VDD: Vdd of the ATmega4809. 5V on the Uno WiFi Rev2
#else // any other board we have not validated
  #pragma message "⚠️ Unable to set analogue reference voltage. Board unknown"
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
#endif

#if USE_HOME_ASSISTANT
    // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  // client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  haRegisterSensors();
  client.disconnect();
#endif // USE_HOME_ASSISTANT
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

#if HAVE_WIFI
  connectToWifi();
#endif

  sensors.read(&sensorsData);

#if USE_HOME_ASSISTANT
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
#endif // USE_HOME_ASSISTANT


#if HAVE_WIFI
  //   // If no Wifi signal, try to reconnect it
  //  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
  //    Serial.println("Wifi connection lost");
  //  }
  // Need to shutdown wifi due to bug in Wifi: https://github.com/arduino-libraries/WiFiNINA/issues/103 | https://github.com/arduino-libraries/WiFiNINA/issues/207
  shutdownWifi();
#else
  // populate json
  doc["tempair"] = sensorsData.temperature.air;
  doc["humidity"] = sensorsData.humidity;
  doc["tempwet"] = sensorsData.temperature.wet;
  doc["co2"] = sensorsData.co2;
  doc["ec"] = sensorsData.ec;
  doc["ph"] = sensorsData.ph;
  doc["flow"] = sensorsData.flow;
  doc["light"] = sensorsData.light;
  doc["moisture"] = sensorsData.moisture;

  serializeJson(doc, Serial);
  Serial.println();
  Serial.flush();
#endif
  delay(SAMPLE_WINDOW);
} // end loop
