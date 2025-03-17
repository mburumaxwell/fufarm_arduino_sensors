#include <DFRobot_EC.h>
#include <DFRobot_PH.h>
#include <DHTesp.h>
#include <OneWire.h>

#include "config.h"

struct FuFarmSensorsData {
  int light;
  float humidity;
  float flow;
  int co2;
  struct FuFarmSensorsTemperature {
    float air;
    float wet;
  } temperature;
  float ec;
  float ph;
  int moisture;
};

class FuFarmSensors
{
public:
  FuFarmSensors(void (*sen0217InterruptHandler)() = nullptr);
  ~FuFarmSensors();
  void begin();                       // initialization
  void calibration();                 // calibration
  void read(FuFarmSensorsData* dest); // read all sensor data
  void sen0217Interrupt();            // should be called from the interrupt handler for SEN0217 passed in the constructor

private:
#ifdef HAVE_TEMP_HUMIDITY
  DHTesp dht; // Temperature and Humidity
#endif

#ifdef HAVE_TEMP_WET
  OneWire ds; // Wet temperature chip i/o
#endif

#ifdef HAVE_EC
  DFRobot_EC ecProbe; // EC probe
#endif

#ifdef HAVE_PH
  DFRobot_PH phProbe; // pH probe
#endif

#ifdef HAVE_FLOW
  volatile int pulseCount; // Flow Sensor
  void (*sen0217InterruptHandler)();
#endif

private:
  int readLight();
  int readCO2();
  float readEC(float temperature);
  float readPH(float temperature);
  float readFlow();
  int readMoisture();
  float readTempWet();
};
