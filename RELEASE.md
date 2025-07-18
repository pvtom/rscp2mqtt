## Release Notes

### Release v3.38 (06.07.2025)

Features:
- Configuration via environment variables, e.g. to simplify the start of a Docker container

### Release v3.37 (12.04.2025)

Bug fixes:
- Issue #106: Unexpected program crash when using raw mode
- Issue #107: Memory leak in v3.36

### Release v3.36 (29.03.2025)

Use not recommended due to existing errors!

Features:
- Issue #103: Battery module cell information (voltage and temperature)

### Release v3.35 (14.03.2025)

Bug fixes:
- Issue #102: Topic e3dc/ems/emergency_power_available is missing

### Release v3.34 (01.03.2025)

Features:
- Improvements for raw data mode and additional topics (discovered during the investigation of issue #98)

### Release v3.33 (15.02.2025)

Bug fixes:
- Issue #9, #96: External PM Energy Data (error with day and history topics)

### Release v3.32 (02.02.2025)

Features:
- Issue #9, #96: External PM Energy Data

### Release v3.31 (26.10.2024)

Bug fixes:
- Issue #90: Cycle Time is not the Intervall given in cfg
- Issue #91: Setting e3dc/set/discharge_start_power is not working

### Release v3.30 (14.09.2024)

Bug fixes:
- Issue #87: Socket receive error. errno 104

### Release v3.29 (04.08.2024)

Bug fixes:
- Issue #85: Error in "New Set Topics"

Features:
- Issue #84: Wallbox - min current
- MacOS support

### Release v3.28 (27.07.2024)

Bug fixes:
- Issue #83: Raw data mode: Problem assembling the topic names

### Release v3.27 (24.07.2024)

Bug fixes:
- Issue #81: Raw data mode: Possible segmentation fault due to a memory leak

### Release v3.26 (14.07.2024)

Use not recommended due to existing errors!

Features:
- Issue #76: Multiple wallboxes
- Additional tags and topics by configuration
- Raw data mode
- Deletion of deprecated wallbox commands (e3dc/set/wallbox/control)

Pull request:
- #78 (build workflow)

### Release v3.25 (08.06.2024)

Features:
- Issue #74: Min. Charge/Discharge Limit

### Release v3.24 (01.06.2024)

Bug fixes:
- Prevention of a program crash when reconnecting to the MQTT broker

Features:
- MQTT with TLS support
- InfluxDB with https

### Release v3.23 (12.05.2024)

Bug fixes:
- Issue #62: Wallbox daily energy values are negative in some cases
- Issue #67: Program crash after several hours

Features:
- Issue #64: Provide program version as topic e3dc/rscp2mqtt/version
- Issue #68: 1/0 instead of true/false
- Buffered logging (LOG_MODE)

### Release v3.22 (29.04.2024)

- Issue #63: e3dc/set/wallbox/charge is renamed to e3dc/set/wallbox/suspended

### Release v3.21 (28.04.2024)

Features:
- Issue #62: Daily wallbox energy values
- Issue #63: New wallbox control commands for charging, sun/mix mode and max_current
- Updated RscpTags.h
- Logging with tag names instead of hex numbers

### Release v3.20 (20.04.2024)

Bug fixes:
- Issue #55, #61: Improvements to the timeout behavior
- Issue #57: Energy per string (today's values)

### Release v3.19 (14.04.2024)

Bug fixes:
- Issue #55: Improvements to the timeout behavior
- Issue #58: INFLUX_TOPIC wird nicht geschrieben (new measurement for string data)
- Issue #59: Error with e3dc/grid_in_limit_duration

Features:
- Issue #57: Energy per string (today's values)

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
