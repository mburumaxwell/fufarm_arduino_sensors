# Farm Urban (Arduino Sensors)

Code for Arduino Sensors

This code is built for the [Arduino UNO Wifi R2](https://store.arduino.cc/products/arduino-uno-wifi-rev2). Other boards might be added with time, if need be. However, should you need to test on another board, [supported by PlatformIO](https://docs.platformio.org/en/latest/boards/index.html), the easiest way would be to add a new environment in the [platformio.ini](./platformio.ini) file. The current code base is built to work with Arduino which means choosing boards [supported by the Arduino platform](https://docs.platformio.org/en/latest/frameworks/arduino.html#boards) is easier.

## Sensors

There are a number of sensors used in the farm for different purposes.

|Sensor|Purpose|Pin|Flag|Macro|
|--|--|--|--|--|
|Light||A0|SENSORS_LIGHT_PIN|HAVE_LIGHT|
|C02||A1|SENSORS_CO2_PIN|HAVE_CO2|
|EC||A2|SENSORS_EC_PIN|HAVE_EC|
|pH||A3|SENSORS_PH_PIN|HAVE_PH|
|Moisture||A4|SENSORS_MOISTURE_PIN|HAVE_MOISTURE|
|DHT22|Humidity and Temperature (Air)|2|SENSORS_DHT22_PIN|HAVE_TEMP_HUMIDITY|
|SEN0217|Flow Sensor|3|SENSORS_SEN0217_PIN|HAVE_FLOW|
|DS18S20|Temperature (Wet)|4|SENSORS_DS18S20_PIN|HAVE_TEMP_WET|

When you start using this, you might not have/need all of these. You can remove the ones you do not have/need by editing `build_flags` in [platformio.ini](./platformio.ini). For example, if you do not have pH sensor, you can remove the line containing `-DSENSORS_PH_PIN=A3`. By default, when a sensor is not enabled, the value is `-1` to signify invalid.

> [!IMPORTANT]
> The EC and pH probes may require calibration. The values are stored in EEPROM (otherwise referred to as KeyValue store). This code may be added to the code base at a later time but in the mean time you can have a look at [logic here](https://github.com/farm-urban/fufarm_rpi_arduino_shield).
>
> The EC and pH probes require temperature compensation. The best value to use for this is the wet temperature. However, if the wet temperature sensor is not configured, the air temperature is used which may not be as accurate. A build time warning is produced.
> The air temperature may also used if the wet temperature reading is between -1000 and -1002, limits inclusive which is usually transient.

### Calibration

The EC and pH sensor need calibration for the first time use or after not being used for an extended period of time. There are detailed guides for calibration at:

- [For pH](https://wiki.dfrobot.com/Gravity__Analog_pH_Sensor_Meter_Kit_V2_SKU_SEN0161-V2)
- [For EC (K=1)](https://wiki.dfrobot.com/Gravity__Analog_Electrical_Conductivity_Sensor___Meter_V2__K%3D1__SKU_DFR0300)

For simplicity doing calibration and to allow you recalibrate on the fly, you can use a toggle button or PCB fuse to short the calibration pin (defaults to 12) to ground. Calibration mode is exclusive and when in it, no data is collected or transmitted. When in this mode values will be printed on your Serial Monitor/Terminal every second (configurable in code), for example: `Temperature: 25.0 ^C EC: 1.41ms/cm pH: 7.12`. You can then issue calibration command via the same monitor or terminal.

|Command|Description|
|--|--|
|`enterph`|enter the calibration mode for pH|
|`calph`|calibrate with the standard buffer solution, two buffer solutions(4.0 and 7.0) will be automatically recognized|
|`exitph`|save the calibrated parameters and exit from calibration mode|
|`enterec`|enter the calibration mode|
|`calec`|calibrate with the standard buffer solution, two buffer solutions(1413us/cm and 12.88ms/cm) will be automatically recognized|
|`exitec`|save the calibrated parameters and exit from calibration mode|
|`clear`|Clear EEPROM data|

> The last two characters of the command designate which sensor is being configured e.g. `enterph` is for the pH sensor while `enterec` is for the EC sensor.

## Data Transmission

The code base supports either sending data to an InfluxDB server or Home Assistant (MQTT). These are controlled by the `USE_INFLUXDB` and `USE_HOME_ASSISTANT` constants which must be set to `1` to enable.

## Spelling

The code is checked for spelling mistakes (happens more often that one might expect). It happens automatically on push to main or when a PR is created.

If you want to run this locally:

```bash
pip install codespell
codespell                   # to check for misspellings
codespell --write-changes   # to automatically fix misspellings
```
