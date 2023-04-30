## Wallbox

This update implements functions to support an E3/DC wallbox.

Because I don't have a wallbox from E3/DC, I couldn't test the functionality.

Please give me feedback if something is not working properly or can be improved.

Configuration

Add this line to the .config file:
```
WALLBOX=true
```

The following topics are sent to the MQTT broker:

| Device / Tag | MQTT Topic | Values / [Unit] |
| --- | --- | --- |
| Wallbox power | e3dc/wallbox/total/power | [W] |
| Wallbox power | e3dc/wallbox/solar/power | [W] |
| Wallbox battery | e3dc/wallbox/battery_to_car | |
| Wallbox battery | e3dc/wallbox/battery_before_car | |
| Wallbox status | e3dc/wallbox/status | |
| Wallbox phases | e3dc/wallbox/active_phases | |
| Wallbox phases | e3dc/wallbox/number_used_phases | |
| Wallbox current | e3dc/wallbox/max_current | (true/false) |
| Wallbox plugged | e3dc/wallbox/plugged | (true/false) |
| Wallbox locked | e3dc/wallbox/locked | (true/false) |
| Wallbox charging | e3dc/wallbox/charging | (true/false) |
| Wallbox canceled | e3dc/wallbox/canceled | (true/false) |
| Wallbox mode | e3dc/wallbox/sun_mode | (true/false) |

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
