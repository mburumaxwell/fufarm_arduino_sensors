#include "WiFiManager.h"

#if HAVE_WIFI

#include "reboot.h"

#ifdef ARDUINO_ARCH_ESP32
#include "esp_eap_client.h"
#endif

WiFiManager::WiFiManager() : _status(WL_IDLE_STATUS)
{
}

WiFiManager::~WiFiManager()
{
}

void WiFiManager::begin()
{
#ifndef ARDUINO_ARCH_ESP32
  // check if the WiFi module is present
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ; // don't continue
  }

  // check firmware version of the module
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }
#endif

#ifdef ARDUINO_ARCH_ESP32
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
#endif

  WiFi.disconnect(); // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5

  listNetworks();
  connect();
}

void WiFiManager::maintain()
{
  connect();
}

#ifndef ARDUINO_ARCH_ESP32
void WiFiManager::printMacAddress(uint8_t mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if (mac[i] < 16)
    {
      Serial.print(F("0"));
    }
    Serial.print(mac[i], HEX);
    if (i > 0)
    {
      Serial.print(":");
    }
  }
}
#endif

#ifdef ARDUINO_ARCH_ESP32
const __FlashStringHelper *encryptionTypeToString(wifi_auth_mode_t mode)
{
  switch (mode)
  {
  case WIFI_AUTH_OPEN:
    return F("open");
  case WIFI_AUTH_WEP:
    return F("WEP");
  case WIFI_AUTH_WPA_PSK:
    return F("WPA");
  case WIFI_AUTH_WPA2_PSK:
    return F("WPA2");
  case WIFI_AUTH_WPA_WPA2_PSK:
    return F("WPA+WPA2");
  case WIFI_AUTH_WPA2_ENTERPRISE:
    return F("WPA2-EAP");
  case WIFI_AUTH_WPA3_PSK:
    return F("WPA3");
  case WIFI_AUTH_WPA2_WPA3_PSK:
    return F("WPA2+WPA3");
  case WIFI_AUTH_WAPI_PSK:
    return F("WAPI");
  default:
    return F("unknown");
  }
}
#else
const __FlashStringHelper *encryptionTypeToString(uint8_t type)
{
  // read the encryption type and print out the name:
  switch (type)
  {
  case ENC_TYPE_WEP:
    return F("WEP");
  case ENC_TYPE_TKIP:
    return F("WPA");
  case ENC_TYPE_CCMP:
    return F("WPA2");
  case ENC_TYPE_NONE:
    return F("None");
  case ENC_TYPE_AUTO:
    return F("Auto");
  case ENC_TYPE_UNKNOWN:
  default:
    return F("Unknown");
  }
}
#endif

void WiFiManager::listNetworks()
{
  Serial.println("** Scan Networks **");
  int8_t count = WiFi.scanNetworks();
  if (count == -1)
  {
    Serial.println("Couldn't scan for WiFi networks");
    return; // nothing more to do here
  }

  Serial.print("Number of available networks: ");
  Serial.println(count);
  for (int8_t i = 0; i < count; i++)
  {
    Serial.print(i);
    Serial.print(") ");
    Serial.print(WiFi.SSID(i));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    Serial.print(encryptionTypeToString(WiFi.encryptionType(i)));
    Serial.println();
  }
  Serial.println();

  // Delete the scan result to free memory for code below.
#ifdef ARDUINO_ARCH_ESP32
  WiFi.scanDelete();
#endif
}

void WiFiManager::connect()
{
  // if already connected, there is nothing more to do
  uint8_t status = WiFi.status();
  if (status == WL_CONNECTED)
  {
    return;
  }

  // detect disconnection
  if (_status == WL_CONNECTED && _status != status)
  {
    Serial.println("WiFi disconnected");
  }

  Serial.print("Attempting to connect to WiFi SSID: ");
  Serial.println(WIFI_SSID);

#ifdef ARDUINO_ARCH_ESP32
#if defined(WIFI_ENTERPRISE_IDENTITY)
  esp_eap_client_set_identity((uint8_t *)WIFI_ENTERPRISE_IDENTITY, strlen(WIFI_ENTERPRISE_IDENTITY));
#endif
#if defined(WIFI_ENTERPRISE_USERNAME)
  esp_eap_client_set_username((uint8_t *)WIFI_ENTERPRISE_USERNAME, strlen(WIFI_ENTERPRISE_USERNAME));
#endif
#if defined(WIFI_ENTERPRISE_PASSWORD)
  esp_eap_client_set_password((uint8_t *)WIFI_ENTERPRISE_PASSWORD, strlen(WIFI_ENTERPRISE_PASSWORD));
#endif
#ifdef WIFI_ENTERPRISE_PASSWORD
  esp_wifi_sta_enterprise_enable();
#endif
#endif

#if defined(WIFI_ENTERPRISE_PASSWORD) && !defined(ARDUINO_ARCH_ESP32)
  status = WiFi.beginEnterprise(WIFI_SSID,
                                WIFI_ENTERPRISE_USERNAME,
                                WIFI_ENTERPRISE_PASSWORD,
                                WIFI_ENTERPRISE_IDENTITY,
                                WIFI_ENTERPRISE_CA);
#elif defined(WIFI_PASSPHRASE)
  status = WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
#else
  status = WiFi.begin(WIFI_SSID);
#endif

  unsigned long started = millis();
  while (status != WL_CONNECTED)
  {
    // Timeout reached – perform a reset
    if ((millis() - started) > WIFI_CONNECTION_REBOOT_TIMEOUT_MILLIS) {
      Serial.println(" taken too long. Rebooting ....");
      reboot();
    }

    delay(500);
    Serial.print(".");
    status = WiFi.status();
  }
  _status = WL_CONNECTED;

  Serial.println();
  Serial.println("WiFi connected successfully!");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

#ifdef ARDUINO_ARCH_ESP32
  Serial.print("BSSID: ");
  Serial.println(WiFi.BSSIDstr());
#else
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.BSSID(mac);
  Serial.print("BSSID: ");
  printMacAddress(mac);
  Serial.println();
#endif

  Serial.print("Signal strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

#ifdef ARDUINO_ARCH_ESP32
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
#else
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
  Serial.println();
#endif
}

#endif // HAVE_WIFI
