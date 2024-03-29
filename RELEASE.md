## Release Notes

### Release v3.18 (23.03.2024)

- Issue #53: Power meter summaries
- Program option --config to set the configuration file instead of .config
- Program option --help
- Minor optimisations for accessing the data cache

### Release v3.17 (16.03.2024)

- Issue #51: Some wallbox topics have been renamed!
  Renamed to: discharge_battery_to_car, charge_battery_before_car, discharge_battery_until, disable_battery_at_mix_mode
  New: phases/L1, phases/L2, phases/L3 replace active_phases
- Issue #52: "e3dc/set/interval" not considered

### Release v3.16 (01.03.2024)

- Issue #49: Add wallbox data (fixes and add-ons)
- Issue #50: Power meter energy values with float precision

### Release v3.15 (17.02.2024)

- Issue #47: Rename the topic e3dc/grid_in_duration to e3dc/grid_in_limit_duration
- Issue #48: Historical Day Data for the Start Year not working
- Issue #49: Add wallbox data
- Option to configure daily historical (#40) and statistic values

### Release v3.14 (10.02.2024)

- Issue #46: Multiple power meters support
- Issue #45: Configuration of the external power meter
- Switch for the wallbox to be used
- Statistic topics for daily max and min power and SOC values, daily grid-in-limit and sunshine durations
- Minor bug fixes and improvements

### Release v3.13 (26.01.2024)

- Issue #44: Cleanup rsoc / soc topics. Prevention of SOC toggling.

### Release v3.12 (21.01.2024)

- Issue #43: Disable auto detection of the number of PVI trackers by setting PVI_TRACKER in .config

### Release v3.11 (13.01.2024)

- Query of historical daily values (Issue #40)
- Issue #41: Enables the reserve to be set to 100%

### Release v3.10 (03.01.2024)

- Issue #30: Multiple battery strings are supported (e.g. S10 Pro)
- Automatic detection of the number of PVI trackers
- Additional topics

### Release v3.9 (23.12.2023)

- Note: New [Dashboard](https://github.com/pvtom/rscp2mqtt-dashboard) is available
- Issue #34: Configuration of the topics that will be published to InfluxDB
- Issue #35: MQTT Client ID can be configured
- Log Level to suppress "received errors"
- More detailled [TOPIC](TOPICS.md) overview

### Release v3.8 (26.11.2023)

- Pull requests #23, #24, #25, #26, #27 and #28 (build workflow)
- Historical data for past years
- "set/force" for specific topics
- Battery SOC limiter
- Issue #29: get/set wallbox number of phases
- Issue #32: syntax check for device IP address
