## Release Notes

### Release v3.14 (10.02.2024)

- Issue #46: Multiple power meters support
- Issue #45: Configuration of the external power meter
- Switch for the wallbox to be used
- Statistic topics for daily max and min power and SOC values, daily grid-in and sunshine durations
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
