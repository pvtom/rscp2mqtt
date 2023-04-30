# RSCP2MQTT - Bridge between an E3/DC S10 device and an MQTT broker

This software module connects a home power station from E3/DC to an MQTT broker.
It uses the RSCP interface of the S10 device.

The solution is based on the RSCP sample application provided by E3/DC and was developed and tested with a Raspberry Pi and a Linux PC (x86_64).

The tool fetches the data cyclically from the S10 and publishes it to the MQTT broker under certain [topics](TOPICS.md).

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

For continuous provision of values, you can configure several topics that are published in each cycle. Default: Only modified values will be published.

## New Features

- E3/DC [wallbox](WALLBOX.md) topics
- [InfluxDB](INFLUXDB.md) support

## Prerequisite

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

Please copy `rscp2mqtt` and the config file into the directory `/opt/rscp2mqtt`

```
cp -a rscp2mqtt /opt/rscp2mqtt
cp config.template /opt/rscp2mqtt/.config
```

## Configuration & Test

Please change to the directory `/opt/rscp2mqtt` and edit `.config` to adjust to your configuration:

```
cd /opt/rscp2mqtt
nano .config
```

```
// IP address of the E3/DC S10 device
E3DC_IP=xxx.xxx.xxx.xxx
// Port of the E3/DC S10 device, default is 5033
E3DC_PORT=5033
// E3/DC account
E3DC_USER=your_e3dc_user
E3DC_PASSWORD=your_e3dc_password
// AES password
E3DC_AES_PASSWORD=your_aes_password
// Target MQTT broker
MQTT_HOST=localhost
// Default port is 1883
MQTT_PORT=1883
// MQTT user / password authentication necessary? Depends on the MQTT broker configuration.
MQTT_AUTH=false
// if true, then enter here
MQTT_USER=
MQTT_PASSWORD=
// MQTT parameters
MQTT_QOS=0
MQTT_RETAIN=false
// log file
LOGFILE=/tmp/rscp2mqtt.log
// Interval requesting the E3/DC S10 device in seconds (1..10)
INTERVAL=1
// enable PVI requests, default is true
PVI_REQUESTS=true
// number of available PV strings(trackers), default is 2
PVI_TRACKER=2
// enable PM requests, default is true
PM_REQUESTS=true
// Auto refresh, default is false
AUTO_REFRESH=false
// Disable MQTT publish support (dryrun mode)
DISABLE_MQTT_PUBLISH=false
// Wallbox, default is false
WALLBOX=true
// topics to be published in each cycle (regular expressions)
FORCE_PUB=e3dc/[a-z]+/power
FORCE_PUB=e3dc/battery/rsoc
```

Find InfluxDB configurations in [InfluxDB](INFLUXDB.md).

The parameter FORCE_PUB can occur several times. You can use it to define topics that will be published in each cycle, even if the values do not change. To check the definition, look at the log output after the program start.

Start the program:

```
./rscp2mqtt
```

If everything works properly, you will see something like this:

```
Connecting...
E3DC system 192.168.178.111:5033 user: <your E3DC user>
MQTT broker localhost:1883 qos = 0 retain = false
Fetching data every second.

Connecting to server 192.168.178.111:5033
Connected successfully

Request authentication at 2022-01-08 09:59:55 (1641632395)
RSCP authentitication level 10

Connecting to broker localhost:1883
Connected successfully

Request cyclic data at 2022-01-08 09:59:56 (1641632396)
MQTT: publish topic >e3dc/solar/power< payload >2663<
MQTT: publish topic >e3dc/battery/power< payload >1953<
MQTT: publish topic >e3dc/home/power< payload >665<
MQTT: publish topic >e3dc/grid/power< payload >-45<
MQTT: publish topic >e3dc/addon/power< payload >0<
MQTT: publish topic >e3dc/coupling/mode< payload >3<
...
```

Check the configuration if the connections are not established.

If you use the Mosquitto tools you can subscribe the topics with (here without user / password)

```
mosquitto_sub -h localhost -p 1883 -t 'e3dc/#' -v
```

Stop `rscp2mqtt` with Crtl-C and start it in the background.

## Daemon Mode

Start the program in daemon mode:
```
./rscp2mqtt -d
```
If you like to start `rscp2mqtt` during the system start, use `/etc/rc.local`. Add the following line before `exit 0`.
```
(cd /opt/rscp2mqtt ; /usr/bin/sudo -H -u pi /opt/rscp2mqtt/rscp2mqtt -d)
```
Adjust the user (pi) if you use another user.

The daemon can be terminated with
```
pkill rscp2mqtt
```
Alternatively, `rscp2mqtt` can be managed by systemd. To do this, copy the file `rscp2mqtt.service` to the systemd directory:
```
sudo cp -a rscp2mqtt.service /etc/systemd/system/
```
Configure the service `sudo nano rscp2mqtt.service` (adjust user 'User=pi'), if needed.

Register the service and start it with:
```
sudo systemctl start rscp2mqtt
sudo systemctl enable rscp2mqtt
```
Be careful that the program runs only once.

## Logging

If stdout is redirected to another process or rscp2mqtt is started with option -s (silent mode), only the log information is passed (issue #10).

## Device Control

rscp2mqtt subscribes to the root topic "e3dc/set/#" and forwards incoming requests to the S10 home power station. In this way, the unit can be controlled and changes made to its configuration.

### Battery Charging

Start battery charging manually (payload is the energy [Wh] to charge)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/manual_charge" -m 1000
```

### Weather Regulation

Set weather regulation (true/false)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/weather_regulation" -m true
```

### Power Limits

Set limits for battery charging and discharging (true/false)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_limits" -m true
```
Set the charging and discharging power limits in [W]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/max_charge_power" -m 2300
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/max_discharge_power" -m 4500
```
### Emergency Power

Set battery reserve for emergency power
```
mosquitto_pub -h localhost -t "e3dc/set/reserve/energy" -m 1500 # in [Wh]
# or
mosquitto_pub -h localhost -t "e3dc/set/reserve/percent" -m 10 # in [%]
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

## System Commands

Post all topics and payloads to the MQTT broker again
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/force" -m 1
```
Log all topics and payloads to the log file
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/log" -m 1
```
Set a new refresh interval (1..10 seconds)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/interval" -m 2
```
Set PM requests on or off (true/false)
```
mosquitto_pub -h localhost -t "e3dc/set/requests/pm" -m true
```
Set PVI requests on or off (true/false)
```
mosquitto_pub -h localhost -t "e3dc/set/requests/pvi" -m true
```

## Used Libraries and Licenses

- The RSCP example application comes from E3/DC. According to E3/DC it can be distributed under the following conditions: `The authors or copyright holders, and in special E3/DC can not be held responsible for any damage caused by the software. Usage of the software is at your own risk. It may not be issued in copyright terms as a separate work.`
- License of AES is included in the AES code files
- Eclipse Mosquitto (https://github.com/eclipse/mosquitto) with EPL-2.0
- libcurl (https://github.com/curl/curl/blob/master/COPYING)
