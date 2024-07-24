## Wallbox

rscp2mqtt provides the following functions to support E3/DC wallboxes.

### Configuration

Add these lines to the .config file:
```
WALLBOX=true
WB_INDEX=0
```

If you have more than one wallbox in your environment, you can configure the index numbers of the wallboxes:
```
WB_INDEX=1
WB_INDEX=2
...
WB_INDEX=7
```

### Topics

The following topics are sent to the MQTT broker:

| Device / Tag | MQTT Topic | Values / [Unit] |
| --- | --- | --- |
| Wallbox Battery | e3dc/wallbox/charge_battery_before_car | (true/false) |
| Wallbox Battery | e3dc/wallbox/discharge_battery_to_car | (true/false) |
| Wallbox Battery | e3dc/wallbox/discharge_battery_until | [%] |
| Wallbox Battery | e3dc/wallbox/disable_battery_at_mix_mode | (true/false) |
| Wallbox Suspended * | e3dc/wallbox/suspended | (true/false) |
| Wallbox Charging *| e3dc/wallbox/charging | (true/false) |
| Wallbox Current * | e3dc/wallbox/max_current | [A] |
| Wallbox Energy L1 * | e3dc/wallbox/energy/L1 | [Wh] |
| Wallbox Energy L2 * | e3dc/wallbox/energy/L2 | [Wh] |
| Wallbox Energy L3 * | e3dc/wallbox/energy/L3 | [Wh] |
| Wallbox Energy Solar * | e3dc/wallbox/energy/solar | [Wh] |
| Wallbox Energy Solar (midnight) ** | e3dc/wallbox/energy_start/solar | [Wh] |
| Wallbox Energy Solar (today) * | e3dc/wallbox/day/solar | [Wh] |
| Wallbox Energy Solar Last Charging * | e3dc/wallbox/energy/last_charging/solar | [Wh] |
| Wallbox Energy Total * | e3dc/wallbox/energy/total | [Wh] |
| Wallbox Energy Total (midnight) ** | e3dc/wallbox/energy_start/total | [Wh] |
| Wallbox Energy Total (today) * | e3dc/wallbox/energy/day/total | [Wh] |
| Wallbox Energy Total Last Charging * | e3dc/wallbox/energy/last_charging/total | [Wh] |
| Wallbox Index * | e3dc/wallbox/index | (0..7) |
| Wallbox Key State * | e3dc/wallbox/key_state | (true/false) |
| Wallbox Locked * | e3dc/wallbox/locked | (true/false) |
| Wallbox Mode * | e3dc/wallbox/sun_mode | (true/false) |
| Wallbox Phases * | e3dc/wallbox/phases/L1 | (true/false) |
| Wallbox Phases * | e3dc/wallbox/phases/L2 | (true/false) |
| Wallbox Phases * | e3dc/wallbox/phases/L3 | (true/false) |
| Wallbox Phases * | e3dc/wallbox/number_phases | |
| Wallbox Plugged * | e3dc/wallbox/plugged | (true/false) |
| Wallbox Power All | e3dc/wallbox/solar/power | [W] |
| Wallbox Power Solar | e3dc/wallbox/total/power | [W] |
| Wallbox Power L1 * | e3dc/wallbox/power/L1 | [W] |
| Wallbox Power L2 * | e3dc/wallbox/power/L2 | [W] |
| Wallbox Power L3 * | e3dc/wallbox/power/L3 | [W] |
| Wallbox SOC * | e3dc/wallbox/soc | [%] |

*) If more than one wallbox exists (maximum 8), topics are extended by the number of the wallbox (1..8), e.g. e3dc/wallbox/1/charging, e3dc/wallbox/2/charging, ... e3dc/wallbox/8/charging

**) The value is required to be able to calculate the daily value.

### Wallbox Control

In addition, the following topics can be published to control the wallboxes:

Sun Mode (true/1/false/0)
```
# if one wallbox is available
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/sun_mode" -m true
#
# or if more than one wallbox is available
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/1/sun_mode" -m true # for the first wallbox
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/2/sun_mode" -m true # for the second wallbox
# ...
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/8/sun_mode" -m true # for the eighth wallbox
```

Suspend charging (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/suspended" -m true
#
# or if more than one wallbox is available
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/1/suspended" -m true # for the first wallbox
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/2/suspended" -m true # for the second wallbox
# ...
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/8/suspended" -m true # for the eighth wallbox
```

Toggle suspend charging
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/toggle" -m 1
#
# or if more than one wallbox is available
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/1/toggle" -m 1 # for the first wallbox
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/2/toggle" -m 1 # for the second wallbox
# ...
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/8/toggle" -m 1 # for the eighth wallbox
```

Set max current (1..32 A)
```
mosquitto_pub -h localhost -p 1883 -t"e3dc/set/wallbox/max_current" -m 16
#
# or if more than one wallbox is available
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/1/max_current" -m 16 # for the first wallbox
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/2/max_current" -m 16 # for the second wallbox
# ...
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/8/max_current" -m 16 # for the eighth wallbox
```

Set number of phases (1/3)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/number_phases" -m 1
#
# or if more than one wallbox is available
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/1/number_phases" -m 1 # for the first wallbox
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/2/number_phases" -m 1 # for the second wallbox
# ...
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/8/number_phases" -m 1 # for the eighth wallbox
```

The following settings apply to all available wallboxes:

Set battery to car mode (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/discharge_battery_to_car" -m true
```

Set battery before car mode (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/charge_battery_before_car" -m true
```
There is a dependency of e3dc/set/wallbox/charge_battery_before_car to e3dc/set/wallbox/discharge_battery_to_car: e3dc/set/wallbox/charge_battery_before_car can only be set if e3dc/set/wallbox/discharge_battery_to_car is false (issue #21).

Set discharge battery until [%]
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/discharge_battery_until" -m 80
```

Set disable charging battery at mix mode (true/1/false/0)
```
mosquitto_pub -h localhost -p 1883 -t "e3dc/set/wallbox/disable_battery_at_mix_mode" -m true
```

If the MQTT broker runs on another server, please use its name instead of `localhost` and change the port if it is different.
Please adjust the calls if you use a different prefix.
