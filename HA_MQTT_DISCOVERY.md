## Home Assistant - MQTT Discovery

Here are a few examples to connect rscp2mqtt with the tool Home Assistant. The Home Assistant offers "MQTT Discovery" for this purpose. Please adapt to your own needs. 
For the Home Assistant, the MQTT integration must be configured with discovery prefix "homeassistent". Follow the manual on https://www.home-assistant.io/integrations/mqtt/

Power
```
mosquitto_pub -h localhost -p 1883 -r -t "homeassistant/sensor/rscp2mqtt/solar_power/config" -m '{ "state_topic": "e3dc/solar/power", "device": { "identifiers": "e3dc", "name": "e3dc", "model": "S10", "manufacturer": "E3/DC", "sa": "S10" }, "unique_id": "e3dc_solar_power", "name": "Solar Power", "unit_of_measurement": "W", "device_class": "power" }'
```

Energy
```
mosquitto_pub -h localhost -p 1883 -r -t "homeassistant/sensor/rscp2mqtt/solar_energy/config" -m '{ "state_topic": "e3dc/solar/energy", "device": { "identifiers": "e3dc", "name": "e3dc", "model": "S10", "manufacturer": "E3/DC", "sa": "S10" }, "unique_id": "e3dc_solar_energy", "name": "Solar Energy", "unit_of_measurement": "kWh", "device_class": "energy" }'
```

Switch
```
mosquitto_pub -h localhost -p 1883 -r -t "homeassistant/switch/rscp2mqtt/set_weather_regulation/config" -m '{ "state_topic": "e3dc/ems/weather_regulation", "command_topic": "e3dc/set/weather_regulation", "payload_on": "true", "payload_off": "false", "state_on": "true", "state_off": "false", "device": { "identifiers": "e3dc", "name": "e3dc", "model": "S10", "manufacturer": "E3/DC", "sa": "S10" }, "unique_id": "set_e3dc_weather_regulation", "name": "Weather Regulation", "device_class": "switch" }'
```

If the MQTT broker runs on another server, please use its name instead of `localhost` and change the port if it is different.
Please adjust the calls if you use a different prefix.
