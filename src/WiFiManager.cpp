#include "WiFiManager.h"

#if HAVE_WIFI

WiFiManager::WiFiManager()
{
}

WiFiManager::~WiFiManager()
{
}

void WiFiManager::begin()
{
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

  WiFi.disconnect(); // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5

  listNetworks();

  connect();
}

void WiFiManager::maintain()
{
  connect();
}

void WiFiManager::printMacAddress(uint8_t mac[])
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
}

void WiFiManager::printEncryptionType(uint8_t type)
{
  // read the encryption type and print out the name:
  switch (type)
  {
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
    printEncryptionType(WiFi.encryptionType(i));
  }
  Serial.println();
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
  if (_status != status)
  {
    Serial.println("WiFi disconnected");
  }

  Serial.print("Attempting to connect to WiFi SSID: ");
  Serial.println(WIFI_SSID);

#if defined(WIFI_ENTERPRISE_USERNAME) && defined(WIFI_ENTERPRISE_PASSWORD)
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

  while (status != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
  }
  _status = WL_CONNECTED;

  Serial.println();
  Serial.println("WiFi connected successfully!");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.BSSID(mac);
  Serial.print("BSSID: ");
  printMacAddress(mac);
  Serial.println();

  Serial.print("Signal strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
  Serial.println();
}

#endif // HAVE_WIFI
