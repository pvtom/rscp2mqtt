# RSCP2MQTT - Bridge between an E3/DC S10 device and a MQTT broker

This software module connects a S10 home power station from E3/DC with a MQTT broker.
It uses the RSCP interface of the S10 device.

The solution is based on the RSCP example application provided by E3/DC and it was developed and tested with a Raspberry Pi and a Linux PC (x86_64).

The tool cyclically fetches the data from the S10 and publishes it under the following topics to the MQTT broker.

Energy topics for today [kWh]:

- e3dc/battery/energy/charge
- e3dc/battery/energy/discharge
- e3dc/grid/energy/in
- e3dc/grid/energy/out
- e3dc/home/energy
- e3dc/pm_0/energy
- e3dc/pm_1/energy
- e3dc/solar/energy

Power topics - current values [W]:

- e3dc/addon/power
- e3dc/battery/power
- e3dc/grid/power
- e3dc/home/power
- e3dc/solar/power

EMS power settings [W]:
- e3dc/ems/max_charge/power
- e3dc/ems/max_discharge/power
- e3dc/ems/discharge_start/power

EMS power settings (true/false):
- e3dc/ems/discharge_start/status
- e3dc/ems/max_charge/status
- e3dc/ems/max_discharge/status
- e3dc/ems/power_limits
- e3dc/ems/power_save
- e3dc/ems/weather_regulation

Additional topics:

- e3dc/autarky
- e3dc/battery/current
- e3dc/battery/cycles
- e3dc/battery/error
- e3dc/battery/name
- e3dc/battery/rsoc
- e3dc/battery/soc
- e3dc/battery/status
- e3dc/battery/voltage
- e3dc/consumed
- e3dc/coupling/mode
- e3dc/ems/balanced_phases/L1
- e3dc/ems/balanced_phases/L2
- e3dc/ems/balanced_phases/L3
- e3dc/ems/charging_lock
- e3dc/ems/charging_throttled
- e3dc/ems/charging_time_lock
- e3dc/ems/discharging_lock
- e3dc/ems/discharging_time_lock
- e3dc/ems/emergency_power_available
- e3dc/grid_in_limit
- e3dc/system/derate_at_percent_value
- e3dc/system/derate_at_power_value
- e3dc/system/installed_peak_power
- e3dc/system/production_date
- e3dc/system/serial_number
- e3dc/system/software
- e3dc/time/zone

Only modified values will be published.

At midnight, a summary of the previous day's values is sent out:

- e3dc/yesterday/autarky
- e3dc/yesterday/battery/energy/charge
- e3dc/yesterday/battery/energy/discharge
- e3dc/yesterday/battery/soc
- e3dc/yesterday/consumed
- e3dc/yesterday/grid/energy/in
- e3dc/yesterday/grid/energy/out
- e3dc/yesterday/home/energy
- e3dc/yesterday/pm_0/energy
- e3dc/yesterday/pm_1/energy
- e3dc/yesterday/solar/energy

## Prerequisite

- A MQTT broker in your environment
- rscp2mqtt needs the library libmosquitto. To install it on a Raspberry Pi enter:

```
sudo apt-get install libmosquitto-dev
```

## Cloning the Repository

```
sudo apt-get install git # if necessary
git clone https://github.com/pvtom/rscp2mqtt.git
```

## Compilation

```
cd rscp2mqtt
make
```

## Installation

```
sudo mkdir -p /opt/rscp2mqtt
sudo chown pi:pi /opt/rscp2mqtt/
```
Adjust user and group (pi:pi) if you use another user.

Please copy rscp2mqtt and the config file into the directory /opt/rscp2mqtt

```
cp -a rscp2mqtt /opt/rscp2mqtt
cp config.template /opt/rscp2mqtt/.config
```

## Configuration & Test

Please change to the directory /opt/rscp2mqtt and edit .config to adjust to your configuration:

```
cd /opt/rscp2mqtt
nano .config
```

```
// IP address of the E3/DC S10 device
E3DC_IP=192.168.xxx.xxx
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
// Auto refresh
AUTO_REFRESH=false
```

Start the program:

```
./rscp2mqtt
```

If everything works properly, you can see something like this:

```
Connecting...
E3DC system 192.168.178.111:5033 user: <your account>
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

Stop rscp2mqtt with Crtl-C and start it in the background.

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
Be careful that the program runs only once.

## New Features in V2.0

rscp2mqtt subscribes the root topic "e3dc/set/#" and responds to incoming requests that are passed to the S10 home power station.

Start battery charging manually (payload is the energy [Wh] to charge, in steps of 100)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/manual_charge" -m 1000
```
Set weather regulation (true/false)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/weather_regulation" -m true
```
Set limits for battery charging / discharging (true/false)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/power_limits" -m true
```
Set the limit in [W] (100 to max. 30000 (depends on the system), in steps of 100)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/max_charge_power" -m 2300
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/max_discharge_power -m 4500
```
Post all topics to the MQTT broker again
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/force" -m 1
```
Log all topics and payloads to the log file
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/log" -m 1
```
Set new refresh interval (1 to 10 seconds)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/interval" -m 2
```

## New Features in V2.3

Control the power management with "e3dc/set/power_mode":

The functionality can be used to intervene in the regulation of the power management.
It should be noted here that the use of this functionality is your own responsibility.
Caution: This function can bypass a set feed-in reduction.

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

Configuration:
Turn on the functionality in the configuration file .config, add/change the following line:
```
AUTO_REFRESH=true
```

## Used Libraries and Licenses

- The RSCP example application comes from E3/DC. According to E3/DC it can be distributed under the following conditions: `The authors or copyright holders, and in special E3/DC can not be held responsible for any damage caused by the software. Usage of the software is at your own risk. It may not be issued in copyright terms as a separate work.`
- License of AES is included in the AES code files
- Eclipse Mosquitto (https://github.com/eclipse/mosquitto) with EPL-2.0
