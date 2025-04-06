# Farm Urban (Arduino Sensors)

Code for Arduino Sensors

This code is built for the following boards:

- [Arduino Leonardo](https://store.arduino.cc/products/arduino-leonardo-with-headers)
- [Arduino UNO R4 WiFi](https://store.arduino.cc/products/uno-r4-wifi)
- [Arduino UNO WiFi R2](https://store.arduino.cc/products/arduino-uno-wifi-rev2)
- [ESP32 S3 DevKitC 1 N16R8](https://www.amazon.co.uk/ESP32-DevKitC-WROOM1-Development-Bluetooth/dp/B0CLD4QKT1)

Other boards might be added with time, if need be. However, should you need to test on another board, [supported by PlatformIO](https://docs.platformio.org/en/latest/boards/index.html), the easiest way would be to add a new environment in the [platformio.ini](./platformio.ini) file. The current code base is built to work with Arduino which means choosing boards [supported by the Arduino platform](https://docs.platformio.org/en/latest/frameworks/arduino.html#boards) is easier.

You can comment/uncomment, one of the lines under `platformio` in the configuration file to target just one which is easier/faster for local use. Otherwise the commands you need are

| Name/Action             | Command Format                                        | Example                                               |
| ----------------------- | ----------------------------------------------------- | ----------------------------------------------------- |
| Build (default boards)  |                                                       | `pio run`                                             |
| Build (specific board)  | `pio run --environment {env-name}`                    | `pio run --environment uno_wifi_rev2`                 |
| Upload (specific board) | `pio run --environment {env-name} --target upload`    | `pio run --environment uno_wifi_rev2 --target upload` |
| Test                    |                                                       | `pio test --environment native`                       |
| Clean (default boards)n |                                                       | `pio run --target fullclean`                          |
| Clean (specific board)  | `pio run --environment {env-name} --target fullclean` | `pio run --environment leonardo --target fullclean`   |

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

## Setup with Arduino Shield for Raspberry Pi and Home Assistant

If using the Gravity [DFR0327 Arduino Shield for Raspberry Pi](https://www.dfrobot.com/product-1211.html) and Home Assistant, communication between the Arduino and Raspberry Pi are via the serial terminal, and the data is sent to Home Assistant via [MQTT-IO](https://github.com/flyte/mqtt-io).

There are several steps to get this working:

1. Checkout this repository (into `/opt` in this example), cd into the directory created and edit `platform.ini` to specify the sensors in use.
2. Install platformio with:

```
url -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
mkdir ~/.local/bin
ln -s ~/.platformio/penv/bin/platformio ~/.local/bin/platformio
ln -s ~/.platformio/penv/bin/pio ~/.local/bin/pio
ln -s ~/.platformio/penv/bin/piodebuggdb ~/.local/bin/piodebuggdb
```

3. Build and upload the code to the arduino with: `./upload.sh leonardo`
4. Create a python virtual environment to host mqtt-io:

```
python -m venv venv
. ./venv/bin/activate
pip install mqtt-io
```

5. Edit the `mqtt-io.yml` in this directory for your system. Ensure you add the MQTT broker and password, and that the stream_modules device matches the device used by `upload.sh` in step 2.

6. Create a systemd file to start monitoring the serial stream, for example:

```
sudo cat > /etc/systemd/system/fusensors.service <<EOF
[Unit]
After=tailscaled.service

[Service]
WorkingDirectory=/opt/fufarm_arduino_sensors
ExecStart=/opt/fufarm_arduino_sensors/venv/bin/python3 -m mqtt_io ./mqtt-io.yml
Restart=always
StandardOutput=append:/opt/fufarm_arduino_sensors/poll.log
StandardError=inherit
SyslogIdentifier=fusensors
User=fu
Group=fu

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable fusensors
sudo systemctl start fusensors

```

7. Create an `mqtt.yml` file in Home Assistant to expose the sensors from the Arduino. An example file is included in the root of this repository. This can be added to the Home Assistant `configuration.yaml` file as shown below:

```
mqtt: !include mqtt.yml
```
