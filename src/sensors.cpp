#include "sensors.h"
#include <Arduino.h>

// Will be different depending on the reference voltage
#define ANALOG_REFERENCE_MILLI_VOLTS 5000
#define ANALOG_MAX_VALUE 1024 // 10 bit ADC
// to avoid possible loss of precision, multiply before dividing
#define ANALOG_READ_MILLI_VOLTS(pin) ((analogRead(pin) * ANALOG_REFERENCE_MILLI_VOLTS) / ANALOG_MAX_VALUE)

FuFarmSensors::FuFarmSensors(void (*sen0217InterruptHandler)() = nullptr)
{
}

FuFarmSensors::~FuFarmSensors()
{
}

void FuFarmSensors::begin()
{
  dht.setup(SENSORS_DHT22_PIN, DHTesp::DHT22);

#ifdef HAVE_FLOW
  pulseCount = 0;
  if (sen0217InterruptHandler != nullptr)
  {
    attachInterrupt(digitalPinToInterrupt(SENSORS_SEN0217_PIN), sen0217InterruptHandler, RISING);
  }
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

void FuFarmSensors::calibration()
{
  // TODO: implement calibration (maybe based on a button?)
  // phProbe.calibration(...);
  // ecProbe.calibration(...);
}

void FuFarmSensors::read(FuFarmSensorsData *dest)
{
  dest->light = readLight();

  TempAndHumidity th = dht.getTempAndHumidity();
  float airTemperature = dest->temperature.air = th.temperature;
  dest->humidity = th.humidity;

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
}

void FuFarmSensors::sen0217Interrupt()
{
#ifdef HAVE_FLOW
  pulseCount += 1;
#endif
}

int FuFarmSensors::readLight()
{
#ifdef HAVE_LIGHT
  float voltage = ANALOG_READ_MILLI_VOLTS(SENSORS_LIGHT_PIN);
  return (int)(voltage / 10.0);
#else
  return -1;
#endif
}

int FuFarmSensors::readCO2()
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
    return (int)(voltage_difference * 50.0 / 16.0);
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
   * sample_window is in milli seconds, so hz is pulseCount * 1000 / SAMPLE_WINDOW
   * */
  float hertz = (float)(pulseCount * 1000.0) / SAMPLE_WINDOW;
  pulseCount = 0; // reset flow counter
  return hertz / 7.0;
#else
  return -1;
#endif
}

int FuFarmSensors::readMoisture()
{
#ifdef HAVE_MOISTURE
  // Need to calibrate this
  int dry = 587;
  int wet = 84;
  int reading = analogRead(SENSORS_MOISTURE_PIN);
  return (int)(100.0 * (dry - reading) / (dry - wet));
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
