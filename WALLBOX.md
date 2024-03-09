## Wallbox

This update implements functions to support E3/DC wallboxes.
Use "e3dc/set/wallbox/index" to switch to another wallbox, if you have more than one in your environment.

Configuration

Add this line to the .config file:
```
WALLBOX=true
```

The following topics are sent to the MQTT broker:

| Device / Tag | MQTT Topic | Values / [Unit] |
| --- | --- | --- |
| Wallbox Battery | e3dc/wallbox/battery_to_car | (true/false) |
| Wallbox Battery | e3dc/wallbox/battery_before_car | (true/false) |
| Wallbox Battery | e3dc/wallbox/battery_discharge_until | [%] |
| Wallbox Battery | e3dc/wallbox/disable_battery_at_mix_mode | (true/false) |
| Wallbox Canceled | e3dc/wallbox/canceled | (true/false) |
| Wallbox Charging | e3dc/wallbox/charging | (true/false) |
| Wallbox Current | e3dc/wallbox/max_current | (true/false) |
| Wallbox Energy Total | e3dc/wallbox/energy/total | [Wh] |
| Wallbox Energy Total Last Charging | e3dc/wallbox/energy/last_charging/total | [Wh] |
| Wallbox Energy Solar | e3dc/wallbox/energy/solar | [Wh] |
| Wallbox Energy Solar Last Charging | e3dc/wallbox/energy/last_charging/solar | [Wh] |
| Wallbox Energy L1 | e3dc/wallbox/energy/L1 | [Wh] |
| Wallbox Energy L2 | e3dc/wallbox/energy/L2 | [Wh] |
| Wallbox Energy L3 | e3dc/wallbox/energy/L3 | [Wh] |
| Wallbox Index | e3dc/wallbox/index | (0..7) |
| Wallbox Key State | e3dc/wallbox/key_state | (true/false) |
| Wallbox Locked | e3dc/wallbox/locked | (true/false) |
| Wallbox Mode | e3dc/wallbox/sun_mode | (true/false) |
| Wallbox Phases | e3dc/wallbox/active_phases/L1 | (true/false) |
| Wallbox Phases | e3dc/wallbox/active_phases/L2 | (true/false) |
| Wallbox Phases | e3dc/wallbox/active_phases/L3 | (true/false) |
| Wallbox Phases | e3dc/wallbox/number_phases | |
| Wallbox Phases | e3dc/wallbox/number_used_phases | |
| Wallbox Plugged | e3dc/wallbox/plugged | (true/false) |
| Wallbox Power All | e3dc/wallbox/total/power | [W] |
| Wallbox Power Solar | e3dc/wallbox/solar/power | [W] |
| Wallbox Power L1 | e3dc/wallbox/power/L1 | [W] |
| Wallbox Power L2 | e3dc/wallbox/power/L2 | [W] |
| Wallbox Power L3 | e3dc/wallbox/power/L3 | [W] |
| Wallbox SOC | e3dc/wallbox/soc | [%] |

There is a dependency of e3dc/wallbox/battery_before_car to e3dc/wallbox/battery_to_car: e3dc/wallbox/battery_before_car can only be set if e3dc/wallbox/battery_to_car is false (issue #21).

In addition, these topics can be published to control the wallbox:

Select the wallbox (0..7) to be used
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/index" -m "0"
```

The following calls will use the set wallbox.

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
