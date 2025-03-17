/*
 * Need to update the firmware on the Wifi Uno Rev2 and upload the SSL certificate for INFLUXDB_SERVER
 * Getting this to work required multiple attempts and deleting the arduino.cc certificate. Instructions
 * are available at: https://github.com/xcape-io/ArduinoProps/blob/master/help/WifiNinaFirmware.md
 *
 * */
#include <SPI.h>
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-reason-code
#include <WiFiNINA.h>

// Sensors
#include <DFRobot_EC.h>
#include <DFRobot_PH.h>
#include <DHTesp.h>
#include <OneWire.h>

#include "config.h"

#define BUFFER_SIZE 256

#if USE_HOME_ASSISTANT
String HA_PREFIX = "homeassistant/sensor";
#include <PubSubClient.h>
#include <ArduinoJson.h>
// Using char* as might store in PROGMEM later
#define MQTT_KEEPALIVE 90
String MQTT_CLIENT_ID = "ard1";
char* MQTT_SERVER_IP = "192.168.8.100";
uint16_t MQTT_SERVER_PORT = 1883;
char* MQTT_USER = "hamqtt";
char* MQTT_PASSWORD = "UbT4Rn3oY7!S9L";
#endif // USE_HOME_ASSISTANT

char ssid[] = "fumanc";
char pass[] = "FARM123!";
// char ssid[] = "PLUSNET-CFC9WG";
// char pass[] = "G7UtKycGmxGYDq";

// #define MOCK ; // Uncomment to skip wifi connection for testing sensors

#if USE_INFLUXDB
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_SSL // Uncomment to connect via SSL on port 443
// #define INFLUXDB_PORT 8086
#define INFLUXDB_SERVER "europe-west1-1.gcp.cloud2.influxdata.com"
// #define INFLUXDB_SERVER "eu-central-1-1.aws.cloud2.influxdata.com"
// #define INFLUXDB_SERVER "farmuaa1.farmurban.co.uk"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> API Tokens -> <select token>)
#define INFLUXDB_TOKEN "jmhtqC2iVoUIVgHmfr6nL6kLXeinh3PpC_duaoqbPO7HtSGW8RUyumq6X4v35nZz-73qco3f66P8pbTTRJ20DKsEoQ=="
// #define INFLUXDB_TOKEN "jmhtTmGBbzIfpaAmkO5bX2X8YHaZja5FHeplIZGjivGyXKYXExOTv75h6ByIJHH695LwEwUl1g1CHqTADITxkmzTdA=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "s.bannon@liverpool.ac.uk"
// #define INFLUXDB_ORG "Farm Urban"
#define INFLUXDB_BUCKET "UTC Experiment 1"
// #define INFLUXDB_BUCKET "Jens Home"
#define INFLUXDB_MEASUREMENT "sensors"
#define INFLUXDB_STATION_ID "sys1"
#endif


#ifdef MOCK
#define SAMPLE_WINDOW 5000
#else
// Time in milliseconds - 5 minutes = 1000 * 60 * 5 = 300000
#define SAMPLE_WINDOW 60000
#endif

#ifdef HAVE_TEMP_HUMIDITY
DHTesp dht;              // Temperature and Humidity
#endif

#ifdef HAVE_TEMP_WET
OneWire ds(SENSORS_DS18S20_PIN); // Wet temperature chip i/o
#endif

#ifdef HAVE_EC
DFRobot_EC ecProbe;      // EC probe
#endif

#ifdef HAVE_PH
DFRobot_PH phProbe;      // pH probe
#endif

#ifdef HAVE_FLOW
volatile int pulseCount; // Flow Sensor
#endif

// Wifi control
int wifiStatus = WL_IDLE_STATUS; // the Wifi radio's status
WiFiClient wifiClient;

#if USE_HOME_ASSISTANT
PubSubClient client(wifiClient);
#endif // USE_HOME_ASSISTANT

// Will be different depending on the reference voltage
#define VOLTAGE_CONVERSION 5000;

int getLight(int pin)
{
#ifdef HAVE_LIGHT
  float voltage = analogRead(pin) / 1024.0 * VOLTAGE_CONVERSION;
  return (int)(voltage / 10.0);
#else
  return -1;
#endif
}

int getCO2(int pin)
{
#ifdef HAVE_CO2
  // Calculate CO2 concentration in ppm
  float voltage = analogRead(pin) / 1024.0 * VOLTAGE_CONVERSION;
  if (voltage == 0.0)
  {
    // Error
    return -1.0;
  }
  else if (voltage < 400.0)
  {
    // Preheating
    return -2.0;
  }
  else
  {
    float voltage_difference = voltage - 400.0;
    return (int)(voltage_difference * 50.0 / 16.0);
  }
#else
  return -1;
#endif
}

float getEC(int pin, float temperature)
{
#ifdef HAVE_EC
  float voltage = analogRead(pin) / 1024.0 * VOLTAGE_CONVERSION;
  return ecProbe.readEC(voltage, temperature);
#else
  return -1;
#endif
}

float getPH(int pin, float temperature)
{
#ifdef HAVE_PH
  float voltage = analogRead(pin) / 1024.0 * VOLTAGE_CONVERSION;
  return phProbe.readPH(voltage, temperature);
#else
  return -1;
#endif
}

float getTempWet()
{
#ifdef HAVE_TEMP_WET
  // returns the temperature from one DS18S20 in DEG Celsius
  byte data[12];
  byte addr[8];

  if (!ds.search(addr))
  {
    // no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if (OneWire::crc8(addr, 7) != addr[7])
  {
    // Serial.println("CRC is not valid!");
    return -1001;
  }

  if (addr[0] != 0x10 && addr[0] != 0x28)
  {
    // Serial.print("Device is not recognized");
    return -1002;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++)
  { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); // using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;
#else
  return -1;
#endif
}

float getFlow()
/* From YF-S201 manual:
   Pulse Characteristic:F=7Q(L/MIN).
   2L/MIN=16HZ 4L/MIN=32.5HZ 6L/MIN=49.3HZ 8L/MIN=65.5HZ 10L/MIN=82HZ
   sample_window is in milli seconds, so hz is pulseCount * 1000 / SAMPLE_WINDOW
 */
{
#ifdef HAVE_FLOW
  float hertz = (float)(pulseCount * 1000.0) / SAMPLE_WINDOW;
  pulseCount = 0; // reset flow counter
  return hertz / 7.0;
#else
  return -1;
#endif
}

void flowPulse()
{
  pulseCount += 1;
}

int getMoisture(int pin)
{
#ifdef HAVE_MOISTURE
  // Need to calibrate this
  int dry = 587;
  int wet = 84;
  int reading = analogRead(pin);
  return (int)(100.0 * (dry - reading) / (dry - wet));
#else
  return -1;
#endif
}

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

#if USE_INFLUXDB
// From: https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino/blob/master/src/util/helpers.cpp
static char invalidChars[] = "$&+,/:;=?@ <>#%{}|\\^~[]`";
static char hex_digit(char c)
{
  return "0123456789ABCDEF"[c & 0x0F];
}

String urlEncode(const char *src)
{
  int n = 0;
  char c, *s = (char *)src;
  while ((c = *s++))
  {
    if (strchr(invalidChars, c))
    {
      n++;
    }
  }
  String ret;
  ret.reserve(strlen(src) + 2 * n + 1);
  s = (char *)src;
  while ((c = *s++))
  {
    if (strchr(invalidChars, c))
    {
      ret += '%';
      ret += hex_digit(c >> 4);
      ret += hex_digit(c);
    }
    else
      ret += c;
  }
  return ret;
}

String createLineProtocol(int light, float tempair, float humidity, float flow, int co2, float tempwet, float ec, float ph, int moisture)
{
  String lineProtocol = INFLUXDB_MEASUREMENT;
  // Tags
  lineProtocol += ",station_id=";
  lineProtocol += INFLUXDB_STATION_ID;
  // Fields
  // Temperature and humidity are always configured when using InfluxDB
  lineProtocol += " tempair=";
  lineProtocol += String(tempair, 2);
  lineProtocol += ",humidity=";
  lineProtocol += String(humidity, 2);
#ifdef HAVE_LIGHT
  lineProtocol += ",light=";
  lineProtocol += light;
#endif
#ifdef HAVE_FLOW
  lineProtocol += ",flow=";
  lineProtocol += String(flow, 1);
#endif
#ifdef HAVE_CO2
  lineProtocol += ",co2=";
  lineProtocol += co2;
#endif
#ifdef HAVE_TEMP_WET
  lineProtocol += ",tempwet=";
  lineProtocol += String(tempwet, 2);
#endif
#ifdef HAVE_EC
  lineProtocol += ",cond=";
  lineProtocol += String(ec, 2);
#endif
#ifdef HAVE_PH
  lineProtocol += ",ph=";
  lineProtocol += String(ph, 2);
#endif
#ifdef HAVE_MOISTURE
  lineProtocol += ",moisture=";
  lineProtocol += moisture;
#endif
  return lineProtocol;
}

int postData(String lineProtocol){
  String url = "/api/v2/write?org=" + urlEncode(INFLUXDB_ORG);
  url += "&bucket=";
  url += urlEncode(INFLUXDB_BUCKET);
  wifiClient.println("POST " + url + " HTTP/1.1");
  wifiClient.println("Host: " + String(INFLUXDB_SERVER));
  wifiClient.println("Content-Type: text/plain");
  wifiClient.println("Authorization: Token " + String(INFLUXDB_TOKEN));
  wifiClient.println("Connection: close");
  wifiClient.print("Content-Length: ");
  wifiClient.println(lineProtocol.length());
  wifiClient.println();   // end HTTP header
  wifiClient.print(lineProtocol); // send HTTP body

  // Debug return values
  delay(2000); // Need to wait for response to come back - not sure of optimal time
  Serial.println("<Http Response>");
  while (wifiClient.available())
  {
    // read an incoming byte from the server and print it to serial monitor:
    char c = wifiClient.read();
    Serial.print(c);
  }
  Serial.println("</Http Response>");
  return 0;
}

int postDataToInfluxDB(int light, float tempair, float humidity, float flow, int co2, float tempwet, float ec, float ph, int moisture)
{
  String lineProtocol = createLineProtocol(light, tempair, humidity, flow, co2, tempwet, ec, ph, moisture);
  Serial.print("Created line protocol: ");
  Serial.println(lineProtocol);
#ifdef MOCK
  Serial.println("Skipping postDataToInfluxDB");
  return;
#endif
  Serial.print("Attempting to connect to: ");
  Serial.println(INFLUXDB_SERVER);
#ifdef INFLUXDB_SSL
  if (wifiClient.connectSSL(INFLUXDB_SERVER, 443))
#else
  if (wifiClient.connect(INFLUXDB_SERVER, INFLUXDB_PORT))
#endif
  {
    Serial.println("connected");
    postData(lineProtocol);
    if (wifiClient.connected())
    {
      wifiClient.stop();
    }
  Serial.println("disconnected");
  } else { // if not connected:
    Serial.println("connection failed");
    wifiClient.stop();
    return -1;
  }
}
#endif // USE_INFLUXDB

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
  client.publish(topic.c_str(), value.c_str(), true);
}

  void haPublishData(int light, float tempair, float humidity, float flow, int co2, float tempwet, float ec, float ph, int moisture) {
    String value = "";
    String sensor = "";
#ifdef HAVE_LIGHT
    sensor = "illuminance";
    value = (String)light;
    haPublishSensor(sensor, value);
#endif
#ifdef HAVE_TEMP_HUMIDITY
    sensor = "temperature";
    value = (String)tempair;
    haPublishSensor(sensor, value);
    sensor = "humidity";
    value = (String)humidity;
    haPublishSensor(sensor, value);
#endif
#ifdef HAVE_FLOW
    sensor = "volume_flow_rate";
    value = (String)flow;
    haPublishSensor(sensor, value);
#endif
#ifdef HAVE_TEMP_WET
  sensor = "tempwet";
  value = (String)tempwet;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_CO2
  sensor = "carbon_dioxide";
  value = (String)co2;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_EC
  sensor = "ec";
  value = (String)ec;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_PH
  sensor = "ph";
  value = (String)ph;
  haPublishSensor(sensor, value);
#endif
#ifdef HAVE_MOISTURE
  sensor = "moisture";
  value = (String)moisture;
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
  //    pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  dht.setup(SENSORS_DHT22_PIN, DHTesp::DHT22);

  // https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  // analogReference(DEFAULT); // Set the default voltage of the reference voltage
  analogReference(VDD); // VDD: Vdd of the ATmega4809. 5V on the Uno WiFi Rev2

#ifdef HAVE_FLOW
  attachInterrupt(digitalPinToInterrupt(SENSORS_SEN0217_PIN), flowPulse, RISING);
  pulseCount = 0;
#endif

#ifdef HAVE_EC
  ecProbe.begin();
#endif

#ifdef HAVE_PH
  phProbe.begin();
#endif

#ifndef MOCK
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
  // Serial.println("Starting main loop");
  // digitalWrite(LED_BUILTIN, HIGH);
  connectToWifi();

  int light = getLight(SENSORS_LIGHT_PIN);
  TempAndHumidity th = dht.getTempAndHumidity();
  float tempair = th.temperature;
  float humidity = th.humidity;
  float flow = getFlow();
  int co2 = getCO2(SENSORS_CO2_PIN);
#ifdef HAVE_TEMP_WET
  float tempwet = getTempWet();
  float calibrationTemperature = tempwet;
  if (tempwet == -1000 || tempwet == -1001 || tempwet == -1002) {
    calibrationTemperature = tempair;
  }
#else
  float calibrationTemperature = tempair;
#endif
  float ec = getEC(SENSORS_EC_PIN, calibrationTemperature);
  float ph = getPH(SENSORS_PH_PIN, calibrationTemperature);
  int moisture = getMoisture(SENSORS_MOISTURE_PIN);
#if USE_INFLUXDB
  postDataToInfluxDB(light, tempair, humidity, flow, co2, tempwet, ec, ph, moisture);
#endif // USE_INFLUXDB

#if USE_HOME_ASSISTANT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  haPublishData(light, tempair, humidity, flow, co2, tempwet, ec, ph, moisture);
  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();
#endif // USE_HOME_ASSISTANT
  //   // If no Wifi signal, try to reconnect it
  //  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
  //    Serial.println("Wifi connection lost");
  //  }
  // Need to shutdown wifi due to bug in Wifi: https://github.com/arduino-libraries/WiFiNINA/issues/103 | https://github.com/arduino-libraries/WiFiNINA/issues/207
  shutdownWifi();
  delay(SAMPLE_WINDOW);
} // end loop
