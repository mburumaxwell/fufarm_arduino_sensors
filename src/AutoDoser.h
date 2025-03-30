#include "config.h"
#include "sensors.h"

#ifndef AUTO_DOSER_H
#define AUTO_DOSER_H

#ifdef HAVE_EC_DOSING

#ifdef ARDUINO_ARCH_ESP32
#include <ESP32Servo.h>
#else
#include <Servo.h>
#endif

struct FuFarmAutoDoserConfig
{
  bool enabled;
  uint16_t duration;        // seconds (range: 5-60)
  float targetEc;           // mS/cm (range: 0.1-5.0)
  uint16_t equilibriumTime; // seconds to wait before the next auto dosing (range: 5-60)
};

struct FuFarmAutoDoserState
{
  uint16_t count;         // number of times the auto dosing was performed
  tm lastTime;      // last time the auto dosing was performed
  uint32_t totalDuration; // total time the auto dosing was performed
};

class FuFarmAutoDoser
{
public:
  FuFarmAutoDoser(const FuFarmAutoDoserConfig *defaultConfig);
  ~FuFarmAutoDoser();

  void begin();
  void maintain();
  void update(FuFarmSensorsData *sensorsData);
  void setConfig(FuFarmAutoDoserConfig *config);

  /**
   * Registers callback that will be called each the current time is needed.
   *
   * @param callback
   */
  inline void onCurrentTimeRequested(void (*callback)(tm *time)) { currentTimeRequestedCallback = *callback; }

  /**
   * Registers callback that will be called each time the auto dosing state is updated.
   *
   * @param callback
   */
  inline void onStateUpdated(void (*callback)(FuFarmAutoDoserState *state)) { stateUpdatedCallback = callback; }

  /**
   * Returns the current state of the auto dosing.
   */
  inline FuFarmAutoDoserState *getState() { return &state; }

  /**
   * Returns the current configuration of the auto dosing.
   */
  inline FuFarmAutoDoserConfig *getConfig() { return &config; }

private:
  void dose();

private:
  FuFarmAutoDoserConfig config;
  FuFarmAutoDoserState state;
  void (*currentTimeRequestedCallback)(tm *time);
  void (*stateUpdatedCallback)(FuFarmAutoDoserState *state);
  Servo pump;
  unsigned long timepoint;

  float currentEC;
  bool currentValuesConsumed;
};

#endif // HAVE_EC_DOSING

#endif // AUTO_DOSER_H
