## Wallbox

This update implements functions to support an E3/DC wallbox.

Configuration

Add this line to the .config file:
```
WALLBOX=true
```

The following topics are sent to the MQTT broker:

| Device / Tag | MQTT Topic | Values / [Unit] |
| --- | --- | --- |
| Wallbox battery | e3dc/wallbox/battery_to_car | (true/false) |
| Wallbox battery | e3dc/wallbox/battery_before_car | (true/false) |
| New: Wallbox battery | e3dc/wallbox/battery_discharge_until | [%] |
| New: Wallbox battery | e3dc/wallbox/disable_battery_at_mix_mode | (true/false) |
| Wallbox canceled | e3dc/wallbox/canceled | (true/false) |
| Wallbox charging | e3dc/wallbox/charging | (true/false) |
| Wallbox current | e3dc/wallbox/max_current | (true/false) |
| Wallbox locked | e3dc/wallbox/locked | (true/false) |
| Wallbox mode | e3dc/wallbox/sun_mode | (true/false) |
| Wallbox phases | e3dc/wallbox/active_phases | |
| Wallbox phases | e3dc/wallbox/number_phases | |
| Wallbox phases | e3dc/wallbox/number_used_phases | |
| Wallbox plugged | e3dc/wallbox/plugged | (true/false) |
| Wallbox power | e3dc/wallbox/total/power | [W] |
| Wallbox power | e3dc/wallbox/solar/power | [W] |
| Wallbox status | e3dc/wallbox/status | |

The new topics are based on additional tags introduced by https://github.com/nischram/E3dcGui

There is a dependency of e3dc/wallbox/battery_before_car to e3dc/wallbox/battery_to_car: e3dc/wallbox/battery_before_car can only be set if e3dc/wallbox/battery_to_car is false (issue #21).

In addition, these topics can be published to control the wallbox:

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

Set number of phases (1-3)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/number_phases" -m 1
```

If the MQTT broker runs on another server, please use its name instead of `localhost` and change the port if it is different.
Please adjust the calls if you use a different prefix.
