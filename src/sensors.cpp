#include <Arduino.h>
#include <EEPROM.h>
#include "sensors.h"

// Will be different depending on the reference voltage.
// We use float to avoid integer overflow.
#if defined(ARDUINO_ESP32S3_DEV)
// nothing to set there because it supports reading in millivolts
#elif defined(ARDUINO_UNOR4_WIFI)
#define ANALOG_REFERENCE_MILLI_VOLTS 5000.0f
#define ANALOG_MAX_VALUE 16384 // 14 bit ADC
#else
#define ANALOG_REFERENCE_MILLI_VOLTS 5000.0f
#define ANALOG_MAX_VALUE 1024 // 10 bit ADC
#endif

// to avoid possible loss of precision, multiply before dividing
#if defined(ARDUINO_ESP32S3_DEV)
#define ANALOG_READ_MILLI_VOLTS(pin) (analogReadMilliVolts(pin))
#else
#define ANALOG_READ_MILLI_VOLTS(pin) ((analogRead(pin) * ANALOG_REFERENCE_MILLI_VOLTS) / ANALOG_MAX_VALUE)
#endif

#define KVALUEADDR 0x00

FuFarmSensors* FuFarmSensors::_instance = nullptr;

#ifdef HAVE_FLOW
static void sen0217InterruptHandler()
{
  // An instance will always exist because the interrupt is attached when an instance method is called.
  // There is therefore no need to check if it is null.
  // The function is also static meaning it cannot be referenced outside this file.
  FuFarmSensors::instance()->sen0217Interrupt();
}
#endif

#ifdef HAVE_ENS160
FuFarmSensors::FuFarmSensors() : ens160(&Wire, /* I2C Address */ 0x53)
#else
FuFarmSensors::FuFarmSensors()
#endif
{
  _instance = this;
}

FuFarmSensors::~FuFarmSensors()
{
  _instance = nullptr;
}

void FuFarmSensors::begin()
{
#if defined(HAVE_AHT20) || defined(HAVE_ENS160)
  uint8_t status = -1, count = 0;
#endif

#ifdef HAVE_WATER_LEVEL_STATE
  pinMode(SENSORS_SEN0204_PIN, INPUT);
#endif

#ifdef HAVE_DHT22
  dht.setup(SENSORS_DHT22_PIN, DHTesp::DHT22);
#elif HAVE_AHT20
  count = 0;
  while ((status = aht20.begin()) != 0)
  {
    Serial.print("AHT20 sensor initialisation failed. Error status: ");
    Serial.println(status);
    count++;
    if (count > 5)
    {
      Serial.println("Could not initialise AHT20 sensor - continuing without it");
      break;
    }
    delay(1000);
  }
#endif

#ifdef HAVE_ENS160
  count = 0;
  while ((status = ens160.begin()) != NO_ERR)
  {
    Serial.print("ENS160 sensor initialisation failed. Error status: ");
    Serial.println(status);
    count++;
    if (count > 5)
    {
      Serial.println("Could not initialise ENS160 sensor - continuing without it");
      break;
    }
    delay(1000);
  }
#endif

#ifdef HAVE_FLOW
  pulseCount = 0;
  pinMode(SENSORS_SEN0217_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSORS_SEN0217_PIN), sen0217InterruptHandler, RISING);
#endif

#ifdef HAVE_EC
  ecProbe.begin();
#endif

#ifdef HAVE_PH
  phProbe.begin();
#endif

#ifdef HAVE_TEMP_WET
  ds.begin(SENSORS_DS18S20_PIN); // Wet temperature chip i/o
#endif
}

void FuFarmSensors::calibration(unsigned long readIntervalMs)
{
  // References:
  // - https://wiki.dfrobot.com/Gravity__Analog_pH_Sensor_Meter_Kit_V2_SKU_SEN0161-V2
  // - https://wiki.dfrobot.com/Gravity__Analog_Electrical_Conductivity_Sensor___Meter_V2__K%3D1__SKU_DFR0300

  static float temperature = 25; // assumed room temperature (used when there is no wet temp sensor)
  static unsigned long timepoint = millis();
#ifdef HAVE_EC
  static float voltageEC, ecValue;
#endif
#ifdef HAVE_PH
  static float voltagePH, phValue;
#endif
  if ((millis() - timepoint) > readIntervalMs)
  {
    timepoint = millis();
#ifdef HAVE_TEMP_WET
    temperature = readTempWet();
#endif

#ifdef HAVE_EC
    voltageEC = ANALOG_READ_MILLI_VOLTS(SENSORS_EC_PIN);
    ecValue = ecProbe.readEC(voltageEC, temperature);
#endif

#ifdef HAVE_PH
    voltagePH = ANALOG_READ_MILLI_VOLTS(SENSORS_PH_PIN);
    phValue = phProbe.readPH(voltagePH, temperature);
#endif

    Serial.print(F("Temperature: "));
    Serial.print(temperature, 1);
#ifdef HAVE_EC
    Serial.print(F("^C  EC: "));
    Serial.print(ecValue, 2);
#endif
#ifdef HAVE_PH
    Serial.print(F(" pH: "));
    Serial.print(phValue, 2);
#endif
    Serial.println();
  }

  // Check if we have received a command via Serial
  // This logic is adapted to support all sensors and EEPROM without logging errors
  if (cmdSerialDataAvailable() > 0)
  {
    // if received Serial CMD from the serial monitor, enter into the respective calibration mode
    // EEPROM: clear
    // EC: enterec, calec, exitec
    // pH: enterph, calph, exitph

    if (strstr(buffer, "CLEAR"))
    {
      Serial.println();
      Serial.println(F(">>>Clearing EEPROM<<<"));

      for (uint8_t i = 0; i < 8; i++)
      {
        EEPROM.write(KVALUEADDR + i, 0xFF);
      }

      Serial.println(F(">>>EEPROM cleared!<<<"));
      Serial.println();
    }
#ifdef HAVE_EC
    else if (strstr(buffer, "ENTEREC") || strstr(buffer, "CALEC") || strstr(buffer, "EXITEC"))
    {
      ecProbe.calibration(voltageEC, temperature, buffer); // calibration process by Serial CMD
    }
#endif
#ifdef HAVE_PH
    else if (strstr(buffer, "ENTERPH") || strstr(buffer, "CALPH") || strstr(buffer, "EXITPH"))
    {
      phProbe.calibration(voltagePH, temperature, buffer); // calibration process by Serial CMD
    }
#endif

    // clear buffer
    bufferIndex = 0;
    memset(buffer, 0, (sizeof(buffer)));
  }
}

void FuFarmSensors::read(FuFarmSensorsData *dest)
{
  dest->light = readLight();

  float airTemperature = -1;
#ifdef HAVE_DHT22
  TempAndHumidity th = dht.getTempAndHumidity();
  airTemperature = dest->temperature.air = th.temperature;
  dest->humidity = th.humidity;
#elif HAVE_AHT20
  if (aht20.startMeasurementReady(true))
  {
    airTemperature = dest->temperature.air = aht20.getTemperature_C();
    dest->humidity = aht20.getHumidity_RH();
  }
  else
  {
    Serial.println("AHT20 sensor not ready");
    airTemperature = dest->temperature.air = -1;
    dest->humidity = -1;
  }
#else
  airTemperature = dest->temperature.air = -1;
  dest->humidity = -1;
#endif

#ifdef HAVE_ENS160
  if (dest->temperature.air != -1 && dest->humidity != -1)
  {
    ens160.setTempAndHum(dest->temperature.air, dest->humidity);
  }
  else
  {
    Serial.println("Could not set air temperature and humidity for ENS160. Its values might not be accurate.");
  }

  uint8_t status = ens160.getENS160Status();
  if (status != DFRobot_ENS160::eSensorStatus_t::eNormalOperation)
  {
    Serial.print("ENS160 is not yet in normal operation mode (initial phase can take upto 1 hour). Current mode: ");
    Serial.println(status);
  }
  else
  {
    dest->airQuality.index = convertENS160AQItoHA(ens160.getAQI());
    dest->airQuality.tvoc = ens160.getTVOC();
    dest->airQuality.eco2 = ens160.getECO2();
  }
#endif

  dest->flow = readFlow();
  dest->co2 = readCO2();

  float calibrationTemperature = airTemperature;
#ifdef HAVE_TEMP_WET
  float wetTemperature = dest->temperature.wet = readTempWet();
  calibrationTemperature = wetTemperature;
  if (wetTemperature == -1000 || wetTemperature == -1001 || wetTemperature == -1002)
  {
    calibrationTemperature = airTemperature;
  }
#endif

  dest->ec = readEC(calibrationTemperature);
  dest->ph = readPH(calibrationTemperature);
  dest->moisture = readMoisture();
  dest->waterLevelState = readWaterLevelState();
}

void FuFarmSensors::sen0217Interrupt()
{
#ifdef HAVE_FLOW
  pulseCount += 1;
#endif
}

bool FuFarmSensors::readWaterLevelState()
{
#ifdef HAVE_WATER_LEVEL_STATE
  return (bool)digitalRead(SENSORS_SEN0204_PIN);
#else
  return false;
#endif
}

int32_t FuFarmSensors::readLight()
{
#ifdef HAVE_LIGHT
  float voltage = ANALOG_READ_MILLI_VOLTS(SENSORS_LIGHT_PIN);
  return (int32_t)(voltage / 10.0);
#else
  return -1;
#endif
}

int32_t FuFarmSensors::readCO2()
{
#ifdef HAVE_CO2
  // Calculate CO2 concentration in ppm
  float voltage = ANALOG_READ_MILLI_VOLTS(SENSORS_CO2_PIN);
  if (voltage == 0.0)
    return -1.0; // Error
  else if (voltage < 400.0)
    return -2.0; // Preheating
  else
  {
    float voltage_difference = voltage - 400.0;
    return (int32_t)(voltage_difference * 50.0 / 16.0);
  }
#else
  return -1;
#endif
}

float FuFarmSensors::readEC(float temperature)
{
#ifdef HAVE_EC
  float voltage = ANALOG_READ_MILLI_VOLTS(SENSORS_EC_PIN);
  return ecProbe.readEC(voltage, temperature);
#else
  return -1;
#endif
}

float FuFarmSensors::readPH(float temperature)
{
#ifdef HAVE_PH
  float voltage = ANALOG_READ_MILLI_VOLTS(SENSORS_PH_PIN);
  return phProbe.readPH(voltage, temperature);
#else
  return -1;
#endif
}

float FuFarmSensors::readFlow()
{
#ifdef HAVE_FLOW
  /*
   * From YF-S201 manual:
   * Pulse Characteristic:F=7Q(L/MIN).
   * 2L/MIN=16HZ 4L/MIN=32.5HZ 6L/MIN=49.3HZ 8L/MIN=65.5HZ 10L/MIN=82HZ
   * sample_window is in milli seconds, so hz is pulseCount * 1000 / SAMPLE_WINDOW_MILLIS
   * */
  float hertz = (float)(pulseCount * 1000.0) / SAMPLE_WINDOW_MILLIS;
  pulseCount = 0; // reset flow counter
  return hertz / 7.0;
#else
  return -1;
#endif
}

int32_t FuFarmSensors::readMoisture()
{
#ifdef HAVE_MOISTURE
  // Need to calibrate this
  int dry = 587;
  int wet = 84;
  int reading = analogRead(SENSORS_MOISTURE_PIN);
  return (int32_t)(100.0 * (dry - reading) / (dry - wet));
#else
  return -1;
#endif
}

float FuFarmSensors::readTempWet()
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
  if (!present)
  {
    // TODO: decide if we should return invalid
  }
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

// inspired by
// - .pio/libdeps/leonardo/DFRobot_EC/DFRobot_EC.cpp
// - .pio/libdeps/leonardo/DFRobot_PH/DFRobot_PH.cpp
bool FuFarmSensors::cmdSerialDataAvailable()
{
  char received;
  while (Serial.available() > 0)
  {
    received = Serial.read();
    if (received == '\n' || received == '\r' || bufferIndex == sizeof(buffer) - 1)
    {
      bufferIndex = 0;
      strupr(buffer);
      return true;
    }
    else
    {
      buffer[bufferIndex] = received;
      bufferIndex++;
    }
  }
  return false;
}

char *FuFarmSensors::strupr(char *str)
{
  if (str == NULL)
    return NULL;
  char *ptr = str;
  while (*ptr != ' ')
  {
    *ptr = toupper((unsigned char)*ptr);
    ptr++;
  }
  return str;
}

// https://www.home-assistant.io/integrations/waqi/
uint16_t FuFarmSensors::convertENS160AQItoHA(uint8_t indexRaw) {
  switch (indexRaw) {
    case 1: return 25;   // Excellent -> US AQI 0-50 (mapped to midpoint)
    case 2: return 75;   // Good      -> US AQI 51-100
    case 3: return 125;  // Moderate  -> US AQI 101-150
    case 4: return 175;  // Poor      -> US AQI 151-200
    case 5: return 250;  // Unhealthy -> US AQI 201-300+
    default: return -1;  // Invalid value
  }
}
