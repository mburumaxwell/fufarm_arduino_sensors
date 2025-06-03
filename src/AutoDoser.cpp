#include "AutoDoser.h"

#ifdef HAVE_EC_DOSING

FuFarmAutoDoser::FuFarmAutoDoser(const FuFarmAutoDoserConfig *defaultConfig)
{
  // we do not have a way to pull the defaults from any non-volatile storage
  // so we use those passed or initialize our own

  memcpy(&this->config, defaultConfig, sizeof(FuFarmAutoDoserConfig));
  memset(&this->state, 0, sizeof(FuFarmAutoDoserState));
}

FuFarmAutoDoser::~FuFarmAutoDoser()
{
  pump.detach();
  stateUpdatedCallback = nullptr;
}

void FuFarmAutoDoser::begin()
{
  pump.attach(PUMP_EC_DOSING_PIN);
  timepoint = millis();
}

void FuFarmAutoDoser::update(FuFarmSensorsData *sensorsData)
{
  currentEC = sensorsData->ec;
  currentValuesConsumed = false;
}

void FuFarmAutoDoser::maintain()
{
  // if dosing is disabled return
  if (!config.enabled)
  {
    return;
  }

  // if we have not consumed current values, the current EC is less than the target EC,
  // and the time since the last dosing is greater than the equilibrium time then dose

  // check if the current EC is less than the target EC
  if (!currentValuesConsumed && currentEC < config.targetEc)
  {
    // check if the time since the last dosing is greater than the equilibrium time
    const auto elapsedTime = (millis() - timepoint) / 1000;
    if (elapsedTime > config.equilibriumTime)
    {
      dose();
      timepoint = millis(); // reset the timepoint (must be done after dosing)
      currentValuesConsumed = true;
    }
  }
}

void FuFarmAutoDoser::setConfig(FuFarmAutoDoserConfig *config)
{
  // copy the config, logic will update in the maintain() function
  memcpy(&this->config, config, sizeof(FuFarmAutoDoserConfig));
}

void FuFarmAutoDoser::dose()
{
  // Dose for a given duration
  // 0 is full speed forward
  // 90 is stopped
  // 180 is full speed reverse
  const uint32_t duration = config.duration * 1000;
  Serial.print(F("Dosing for "));
  Serial.print(duration);
  Serial.println(F(" ms."));
  pump.write(0); // full speed forward
  delay(duration);
  pump.write(90); // stop

  // update the state
  state.count++;
  if (currentTimeRequestedCallback)
  {
    currentTimeRequestedCallback(&(state.lastTime));
  }
  else
  {
    time_t now = 0;
    localtime_r(&now, &(state.lastTime));
  }
  state.totalDuration += duration;
  if (stateUpdatedCallback)
  {
    stateUpdatedCallback(&state);
  }
  Serial.println(F("Dosing completed"));
}

#endif // AutoDoser
