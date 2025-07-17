# Developer Documentation

## Testing MQTT without a Home Assistant setup

Sometimes, you may not have direct access to a Home Assistant setup and you want to test the MQTT communication, you can setup a local broker using [mosquitto](https://mosquitto.org/documentation/) and then test the status of the broker with [MQTT Explorer](https://mqtt-explorer.com/).

### Docker

1. Install [docker](https://docs.docker.com/engine/install/). If on OSX, make sure the root code directory is shared from the host: **Docker -> Preferences... -> Resources -> File Sharing**

2. Start the broker with:

   `docker run -it -p 1883:1883 -v "$PWD/mosquitto.conf:/mosquitto/config/mosquitto.conf" eclipse-mosquitto`

### Local install

1. Install mosquitto:

   `brew install mosquitto`

2. Run using:

   `mosquitto -v -c mosquitto.conf`

### Ngrok to troubleshoot internet connection issues

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

### TLS

Mosquitto supports TLS and there may be a reason to use it in your Home Assistant setup such borrowing your neighbour's WiFi (just an example but seriously be careful). You may also have a domain name for your home setup for easy access.
In these cases, once you have setup TLS on your Mosquitto/HomeAssistant, you can enable TLS in this code by setting `-DHOME_ASSISTANT_MQTT_TLS=1`, changing the host e.g. `-DHOME_ASSISTANT_MQTT_HOST=\"home.yens.io\"`, changing the port e.g. `-DHOME_ASSISTANT_MQTT_PORT=8883`, and adding the relevant certificates (self-signed certs) if any in `certs.cpp`. Once you have setup TLS, you might want to disable connection on non-authenticated port (usually 1883).

> Ultimately, the best security is isolation of your IoT network at work or home down to a different VLAN if possible. TLS is the alternative when isolation is not possible or practical. If you manage both, you're golden.

## Spelling

The code is checked for spelling mistakes (happens more often that one might expect). It happens automatically on push to main or when a PR is created.

If you want to run this locally:

```bash
pip install codespell
codespell                   # to check for misspellings
codespell --write-changes   # to automatically fix misspellings
```
