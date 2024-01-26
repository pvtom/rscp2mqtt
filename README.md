# RSCP2MQTT - Bridge between an E3/DC device and an MQTT broker
[![GitHub sourcecode](https://img.shields.io/badge/Source-GitHub-green)](https://github.com/pvtom/rscp2mqtt/)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/pvtom/rscp2mqtt)](https://github.com/pvtom/rscp2mqtt/releases/latest)
[![GitHub last commit](https://img.shields.io/github/last-commit/pvtom/rscp2mqtt)](https://github.com/pvtom/rscp2mqtt/commits)
[![GitHub issues](https://img.shields.io/github/issues/pvtom/rscp2mqtt)](https://github.com/pvtom/rscp2mqtt/issues)
[![GitHub pull requests](https://img.shields.io/github/issues-pr/pvtom/rscp2mqtt)](https://github.com/pvtom/rscp2mqtt/pulls)
[![GitHub](https://img.shields.io/github/license/pvtom/rscp2mqtt)](https://github.com/pvtom/rscp2mqtt/blob/main/LICENSE)

This software module connects a home power station from E3/DC to an MQTT broker.

It uses the RSCP interface of the device. The solution is based on the RSCP sample application provided by E3/DC and was developed and tested with a Raspberry Pi and a Linux PC (x86_64).

The tool cyclically queries data from the home power station and publishes it to an MQTT broker using these [topics](TOPICS.md).

Supported topic areas are:

- Energy topics for today
- Current power values
- Autarky and self-consumption
- Battery status
- Energy management (EMS) power settings
- Data from yesterday and the current week, month and year
- Values of the power meter (PM)
- Values of the photovoltaic inverter (PVI)
- Values of the emergency power supply (EP)
- Values of the wallbox (WB)

For continuous provision of values, you can configure several topics that are published in each cycle. Default: Only modified values will be published.

## Features

- Topic prefix is configurable
- E3/DC [wallbox](WALLBOX.md) topics
- [InfluxDB](INFLUXDB.md) support
- Topics for temperatures (battery, PVI)
- Idle periods
- System error messages
- Details of the battery modules (DCB)
- Additional topics for PVI
- Units as InfluxDB tags
- Historical data for past years
- "set/force" for specific topics
- Battery SOC limiter
- Docker images at https://hub.docker.com/r/pvtom/rscp2mqtt
- [Dashboard](https://github.com/pvtom/rscp2mqtt-dashboard) is available
- Configuration of the topics that will be published to InfluxDB (INFLUXDB_TOPIC)
- MQTT Client ID can be configured
- Multiple battery strings are supported (BATTERY_STRINGS parameter)
- Automatic detection of the number of PVI trackers
- Query of historical daily values

## Docker

Instead of installing the package you can use a [Docker image](DOCKER.md).

## Prerequisites

- An MQTT broker in your environment
- rscp2mqtt needs the library libmosquitto. For installation please enter:

```
sudo apt-get install libmosquitto-dev
```

If you like to transfer data to InfluxDB install the libcurl library:
```
sudo apt-get install curl libcurl4-openssl-dev
```

## Cloning the Repository

```
sudo apt-get install git # if necessary
git clone https://github.com/pvtom/rscp2mqtt.git
cd rscp2mqtt
```

## Compilation

To build a program version without InfluxDB use:
```
make
```

To build a program version including InfluxDB support use:
```
make WITH_INFLUXDB=yes
```

## Installation

```
sudo mkdir -p /opt/rscp2mqtt
sudo chown pi:pi /opt/rscp2mqtt/
```
Adjust user and group (pi:pi) if you use another user.

Copy `rscp2mqtt` into the directory `/opt/rscp2mqtt`

```
cp -a rscp2mqtt /opt/rscp2mqtt
```

## Configuration

Copy the config template file into the directory `/opt/rscp2mqtt`
```
cp config.template /opt/rscp2mqtt/.config
```

Please change to the directory `/opt/rscp2mqtt` and edit `.config` to adjust to your configuration:

```
cd /opt/rscp2mqtt
nano .config
```
The configuration parameters are described in the file.

The prefix of the topics can be configured by the attribute PREFIX. By default all topics start with "e3dc". This can be changed to any other string that MQTT accepts as a topic, max. 24 characters. E.g. "s10" or "s10/1".

If your system has more than one battery string (e.g. S10 Pro), you have to configure the parameter BATTERY_STRINGS accordingly.
Battery topics that belong to a battery string are extended by the number of the battery string.
Battery modules (DCB topics) are numbered consecutively.

Find InfluxDB configurations in [InfluxDB](INFLUXDB.md).

The parameter FORCE_PUB can occur several times. You can use it to define topics that will be published in each cycle, even if the values do not change. To check the definition, look at the log output after the program start.

## Program start

Start the program:
```
./rscp2mqtt
```
or in verbose mode
```
./rscp2mqtt -v
```

If everything works properly, you will see something like this:

```
rscp2mqtt [v3.13]
E3DC system >192.168.178.111:5033< user: >your E3DC user<
MQTT broker >localhost:1883< qos = >0< retain = >false< client id >✗< prefix >e3dc<
Fetching data every second.
Requesting PVI ✓ | PM ✓ | DCB (1 battery string) ✓ | Wallbox ✗ | Autorefresh ✓
Log level = 0
Stdout to terminal

[2024-01-26 16:45:00] pid=30130 ppid=1 RscpMqttMain.cpp(2317) Connecting to server 192.168.178.111:5033
[2024-01-26 16:45:00] pid=30130 ppid=1 RscpMqttMain.cpp(2324) Success: E3DC connected.
[2024-01-26 16:45:00] pid=30130 ppid=1 RscpMqttMain.cpp(1391) RSCP authentication level 10
[2024-01-26 16:45:00] pid=30130 ppid=1 RscpMqttMain.cpp(1918) Connecting to broker localhost:1883
[2024-01-26 16:45:00] pid=30130 ppid=1 RscpMqttMain.cpp(1927) Success: MQTT broker connected.
```

Check the configuration if the connections are not established.

Notes: In the following examples I assume that the MQTT broker runs on the same machine as rscp2mqtt under port 1883. If the MQTT broker runs on another server, please use its name instead of `localhost` and change the port if it is different.
The default prefix `e3dc` is used for the topics. Please adjust the calls if you use a different prefix.

If you use the Mosquitto tools you can subscribe the topics with (here without user / password)

```
mosquitto_sub -h localhost -p 1883 -t 'e3dc/#' -v
```

## Daemon Mode

Start the program in daemon mode:
```
./rscp2mqtt -d
```
The daemon can be terminated with
```
pkill rscp2mqtt
```
Be careful that the program runs only once.

## Systemd

rscp2mqtt can be managed by systemd. For this purpose, copy the file `rscp2mqtt.service` to the systemd directory:
```
sudo cp -a rscp2mqtt.service /etc/systemd/system/
```
Configure the service `sudo nano rscp2mqtt.service` (adjust user 'User=pi'), if needed.

Register the service and start it with:
```
sudo systemctl enable rscp2mqtt
sudo systemctl start rscp2mqtt
```

## Device Control

rscp2mqtt subscribes to the root topic "e3dc/set/#" and forwards incoming requests to the home power station. In this way, the device can be controlled, additional data can be queried and changes can be made to its configuration.

### Battery Charging

Start battery charging manually (payload is the energy [Wh] to charge)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/manual_charge" -m 1000
```

### Weather Regulation

Set weather regulation (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/weather_regulation" -m true
```

### Power Limits

Set limits for battery charging and discharging (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_limits" -m true
```
Set the charging and discharging power limits in [W]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/max_charge_power" -m 2300
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/max_discharge_power" -m 4500
```

### Idle Periods

Set idle periods to lock battery charging or discharging

Note: The set operations will work only if the idle period functionality is turned on (via the S10 display).

Parameters:
- Day: "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday" or "today"
- Mode: "charge" or "discharge"
- Active: "true" or "false"
- Period: "hh:mi-hh:mi"
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/idle_period" -m "today:charge:true:00:00-23:59"
```

With the topics "e3dc/ems/charging_lock" and "e3dc/ems/discharging_lock" you can check whether a lock is currently active or not.

### Battery SOC Limiter

The SOC limiter can be used to conveniently control the charging or discharging of the house battery. It uses the Idle Periods described previously.

#### Maximum SOC
When the maximum SOC value is reached, the battery charging process is stopped.

#### Minimum SOC
The battery charge level does not fall below the minimum SOC value. When the SOC is reached, discharging stops.

#### Home Power
When the house power is above the set value, discharging will stop. This can be used, for example, to prevent charging the electric vehicle from the house battery (when using a non-E3/DC wallbox that cannot be controlled directly by the device).

Set the minimum SOC to limit discharging the battery [%]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/discharge/soc" -m 30
```
Turn off the limiter for discharging the battery
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/discharge/soc" -m 0
```
Set the home power value which stops discharging the battery [W]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/discharge/by_home_power" -m 7000
```
Reset the home power value which stops discharging the battery
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/discharge/by_home_power" -m 0
```
Reset the limiter at the day change
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/discharge/durable" -m false
```
Keep the limiter setting even after the day change
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/discharge/durable" -m true
```
Set the maximum SOC to limit charging the battery [%]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/charge/soc" -m 80
```
Turn off the limiter for charging the battery
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/charge/soc" -m 0
```
Reset the limiter at the day change
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/charge/durable" -m false
```
Keep the limiter setting even after the day change
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/limit/charge/durable" -m true
```

### Emergency Power

Set battery reserve for emergency power
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/reserve/energy" -m 1500 # in [Wh]
# or
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/reserve/percent" -m 10 # in [%]
```

### Power Management

Control the power management with "e3dc/set/power_mode":

The functionality can be used to intervene in the regulation of the power management.
Caution: This function can be used to bypass a set feed-in reduction. Use this functionality at your own risk.

Automatic / normal mode
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_mode" -m "auto"
```
Idle mode (number of cycles)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_mode" -m "idle:60"
```
Discharge mode (power in [W], number of cycles)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_mode" -m "discharge:2000:60"
```
Charge mode (power in [W], number of cycles)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_mode" -m "charge:2000:60"
```
Grid charge mode (power in [W], number of cycles)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_mode" -m "grid_charge:2000:60"
```

After the time has elapsed (number of cycles multiplied by the configured interval) plus a few seconds, the system automatically returns to normal mode.

Turn on the functionality in the configuration file .config, add/change the following line:
```
AUTO_REFRESH=true
```

## Wallbox Control

Set solar or mix mode with the current in [A] (6..32 Ampere)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/control" -m "solar:16"
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/control" -m "mix:8"
```

Stop charging
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/control" -m "stop"
```

Set battery to car mode (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/battery_to_car" -m true
```

Set battery before car mode (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/battery_before_car" -m true
```

Set battery discharge until [%]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/battery_discharge_until" -m 80
```

Set disable charging battery at mix mode (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/disable_battery_at_mix_mode" -m true
```

Set number of phases (1/3)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/number_phases" -m 1
```

## Historical daily data

Historical data for a specific day (format "YYYY-MM-DD") can be queried by
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/request/day" -m "2024-01-01"
```

Please use the script `request_days.sh` to request data for a time span.
```
./request_days.sh localhost 1883 e3dc 2024-01-01 2024-01-13
```

## System Commands

Refresh all topics
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/force" -m 1
```
Refresh specific topics
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/force" -m "e3dc/history/2021.*"
```
Log all topics and payloads to the log file
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/log" -m 1
```
Set a new refresh interval (1..300 seconds)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/interval" -m 2
```
Turn PM requests on or off (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/requests/pm" -m true
```
Turn PVI requests on or off (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/requests/pvi" -m true
```
Turn DCB requests on or off (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/requests/dcb" -m true
```
Turn SOC limiter on or off (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/soc_limiter" -m true
```

## Used Libraries and Licenses

- The RSCP example application comes from E3/DC. According to E3/DC it can be distributed under the following conditions: `The authors or copyright holders, and in special E3/DC can not be held responsible for any damage caused by the software. Usage of the software is at your own risk. It may not be issued in copyright terms as a separate work.`
- License of AES is included in the AES code files
- Eclipse Mosquitto (https://github.com/eclipse/mosquitto) with EPL-2.0
- libcurl (https://github.com/curl/curl/blob/master/COPYING)
