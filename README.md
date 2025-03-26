# Farm Urban (Arduino Sensors)

Code for Arduino Sensors

This code is built for the following boards:

- [Arduino Leonardo](https://store.arduino.cc/products/arduino-leonardo-with-headers)
- [Arduino UNO R4 WiFi](https://store.arduino.cc/products/uno-r4-wifi)
- [Arduino UNO WiFi R2](https://store.arduino.cc/products/arduino-uno-wifi-rev2)
- [ESP32 S3 DevKitC 1 N16R8](https://www.amazon.co.uk/ESP32-DevKitC-WROOM1-Development-Bluetooth/dp/B0CLD4QKT1)

Other boards might be added with time, if need be. However, should you need to test on another board, [supported by PlatformIO](https://docs.platformio.org/en/latest/boards/index.html), the easiest way would be to add a new environment in the [platformio.ini](./platformio.ini) file. The current code base is built to work with Arduino which means choosing boards [supported by the Arduino platform](https://docs.platformio.org/en/latest/frameworks/arduino.html#boards) is easier.

You can comment/uncomment, one of the lines under `platformio` in the configuration file to target just one which is easier/faster for local use. Otherwise the commands you need are

| Name/Action                   | Command Format                                        | Example                                               |
| ----------------------------- | ----------------------------------------------------- | ----------------------------------------------------- |
| Build (default environments)  |                                                       | `pio run`                                             |
| Build (specific environment)  | `pio run --environment {env-name}`                    | `pio run --environment uno_wifi_rev2`                 |
| Upload (specific environment) | `pio run --environment {env-name} --target upload`    | `pio run --environment uno_wifi_rev2 --target upload` |
| Test                          |                                                       | `pio test --environment native`                       |
| Clean (default environments)n |                                                       | `pio run --target fullclean`                          |
| Clean (specific environment)  | `pio run --environment {env-name} --target fullclean` | `pio run --environment leonardo --target fullclean`   |

## WiFi

Boards that support WiFi are generally easier to work with because they transmit directly to Home Assistant. The WiFi network you connect to is controlled by the common `build_flags_wifi` in `platformio.ini`.

- To connect to a public network, you only need to configure `-DWIFI_SSID=\"<your-network>\"` for example, `-DWIFI_SSID=\"Airport_Free_WiFi\"
- To connect to a simple WPA2 network, you need to set both `-DWIFI_SSID=\"<your-network>\"` and `-DWIFI_PASSPHRASE=\"<your-value>\"`
- To connect to an 802.1x or enterprise network, you need to set `; -DWIFI_PASSPHRASE=\"<your-value>\"`, `-DWIFI_ENTERPRISE_USERNAME=\"<your-value>\"`, and `-DWIFI_ENTERPRISE_PASSWORD=\"<your-value>\"`. Optionally, you may need `-DWIFI_ENTERPRISE_IDENTITY=\"<your-value>\"` or `-DWIFI_ENTERPRISE_CA=\"<your-value>\"`. This is for networks such as those used at hospitals for staff members, universities, shared accommodation or maybe your workplace.

> [!NOTE]
>
> 1. Networks with captive portals do not work.
> 2. Networks with spaces need to be escaped appropriately. For example, to connect to a network named `ASK4 Wireless`, you should use `-DWIFI_SSID=\"ASK4\ Wireless\"`. To connect to a network named `Jen's iPhone`, you should use `-DWIFI_SSID=\"Jen\'s\ iPhone\"`. At times, the device is unable to connect when there are special characters, renaming your phone should fix it, e.g `Jen's iphone` -> `JensiPhone`.
> 3. Arduino UNO R4 WiFi, does not (yet) support enterprise WiFi.

### WiFi Firmware

The WiFi modules may need firmware updates to work properly such as when using an older board or you have not used a particular board for a while. In some cases, multiple attempts may be required. See:

- [Arduino UNO R4 WiFi](https://support.arduino.cc/hc/en-us/articles/9670986058780-Update-the-connectivity-module-firmware-on-UNO-R4-WiFi)
- [Arduino UNO WiFi Rev2](https://github.com/xcape-io/ArduinoProps/blob/master/help/WifiNinaFirmware.md)

## Sensors

There are a number of sensors used in the farm for different purposes.

| Sensor   | Purpose                        | Pin | Flag                 | Macro                  |
| -------- | ------------------------------ | --- | -------------------- | ---------------------- |
| Light    |                                | A0  | SENSORS_LIGHT_PIN    | HAVE_LIGHT             |
| CO2      |                                | A1  | SENSORS_CO2_PIN      | HAVE_CO2               |
| EC       |                                | A2  | SENSORS_EC_PIN       | HAVE_EC                |
| pH       |                                | A3  | SENSORS_PH_PIN       | HAVE_PH                |
| Moisture |                                | A4  | SENSORS_MOISTURE_PIN | HAVE_MOISTURE          |
| DHT22    | Humidity and Temperature (Air) | 2   | SENSORS_DHT22_PIN    | HAVE_DHT22             |
| SEN0217  | Flow Sensor                    | 3   | SENSORS_SEN0217_PIN  | HAVE_FLOW              |
| DS18S20  | Temperature (Wet)              | 4   | SENSORS_DS18S20_PIN  | HAVE_TEMP_WET          |
| SEN0204  | Water Level Sensor             | 5   | SENSORS_SEN0204_PIN  | HAVE_WATER_LEVEL_STATE |
| AHT20    | Humidity and Temperature (Air) | I2C | HAVE_AHT20           | HAVE_AHT20             |
| ENS160   | Air Quality & Multi Gas        | I2C | HAVE_ENS160          | HAVE_ENS160            |

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

| Command   | Description                                                                                                                  |
| --------- | ---------------------------------------------------------------------------------------------------------------------------- |
| `enterph` | enter the calibration mode for pH                                                                                            |
| `calph`   | calibrate with the standard buffer solution, two buffer solutions(4.0 and 7.0) will be automatically recognized              |
| `exitph`  | save the calibrated parameters and exit from calibration mode                                                                |
| `enterec` | enter the calibration mode                                                                                                   |
| `calec`   | calibrate with the standard buffer solution, two buffer solutions(1413us/cm and 12.88ms/cm) will be automatically recognized |
| `exitec`  | save the calibrated parameters and exit from calibration mode                                                                |
| `clear`   | Clear EEPROM data                                                                                                            |

> The last two characters of the command designate which sensor is being configured e.g. `enterph` is for the pH sensor while `enterec` is for the EC sensor.

## Data Transmission

The code base supports either sending data to Home Assistant (MQTT) or printing out JSON via Serial. Data is sent to HomeAssistant, if the board has network support (e.g. WiFi). HomeAssistant is capable enough to replay the information to any other destination including InfluxDB (which we used to support in this repository).

## Testing MQTT without a Home Assistant setup

Sometimes, you may not have direct access to a Home Assistant setup and you want to test the MQTT communication, you can setup a local broker using [mosquitto](https://mosquitto.org/documentation/) and the test the status of the broker with [MQTT Explorer](https://mqtt-explorer.com/).

### Docker

1. Install [docker](https://docs.docker.com/engine/install/). If on OSX, make sure the root code directory is shared from the host: **Docker -> Preferences... -> Resources -> File Sharing**

2. Start the broker with:

   `docker run -it -p 1883:1883 -v "$PWD/mosquitto.conf:/mosquitto/config/mosquitto.conf" eclipse-mosquitto`

### Local install

1. Install mosquitto:

   `brew install mosquitto`

2. Run using:

   `mosquitto -v -c mosquitto.conf`

### Ngrok to resolve connection issues

<details>
<summary><strong>Details</strong></summary>
If your machine is connected to a different network it does not allow incoming traffic for security reasons, you can tunnel the connection via [ngrok](https://ngrok.com). In another terminal window/tab install using `brew install ngrok`, setup auth using instructions on your account e.g. `ngrok config add-authtoken <some-token>` then run `ngrok tcp 1883`. You will then see an output such as:

```txt
Forwarding                    tcp://6.tcp.eu.ngrok.io:14333 -> localhost:1883
```

You can then change your `platformio.ini` settings to match this. In this case, `-DHOME_ASSISTANT_MQTT_HOST=\"6.tcp.eu.ngrok.io\"` and `-DHOME_ASSISTANT_MQTT_PORT=14333`.

</details>
<br/>
Once you upload, the code and the connection happens, in the mosquitto window you will see output that looks something like:

<details>
<summary>Console output</summary>
```txt
1742759640: mosquitto version 2.0.21 starting
1742759640: Config loaded from mosquitto.conf.
1742759640: Opening ipv6 listen socket on port 1883.
1742759640: Opening ipv4 listen socket on port 1883.
1742759640: mosquitto version 2.0.21 running
1742759655: New connection from ::1:56091 on port 1883.
1742759655: New client connected from ::1:56091 as 48ca435e0080 (p2, c1, k90).
1742759655: No will message specified.
1742759655: Sending CONNACK to 48ca435e0080 (0, 0)
1742759655: Received PUBLISH from 48ca435e0080 (d0, q0, r1, m0, 'homeassistant/sensor/48ca435e0080/light/config', ... (193 bytes))
1742759655: Received PUBLISH from 48ca435e0080 (d0, q0, r1, m0, 'homeassistant/48ca435e0080/light/stat_t', ... (3 bytes))
```
</details>

## Spelling

The code is checked for spelling mistakes (happens more often that one might expect). It happens automatically on push to main or when a PR is created.

If you want to run this locally:

```bash
pip install codespell
codespell                   # to check for misspellings
codespell --write-changes   # to automatically fix misspellings
```
