## Configuration

rscp2mqtt offers the option for setting configuration parameters by a config file named .config.

Starting with release v3.38 configuration can also be set via environment variables. This simplifies the start of a Docker container, as no .config file is required.

If you use both, the environment variables overwrite the parameters set with .config! (Exception: parameters that can be set multiple times).

### Configuration E3/DC and MQTT Broker

| Key | Value/Range | Example |
| --- | --- | --- |
| E3DC_IP | string | 192.168.178.11 |
| E3DC_PORT | 0 - 65535 | 5033 |
| E3DC_USER | string | user@provider.de |
| E3DC_PASSWORD | string | mypwd |
| E3DC_AES_PASSWORD | string | myaespwd |
| MQTT_HOST | string | localhost |
| MQTT_PORT | 0 - 65535 | 1883 |
| MQTT_USER | string | |
| MQTT_PASSWORD | string | |
| MQTT_AUTH | true/false | false |
| MQTT_TLS_CAFILE | string | /home/pi/ca.crt |
| MQTT_TLS_CAPATH | string | |
| MQTT_TLS_CERTFILE | string | /home/pi/client.crt |
| MQTT_TLS_KEYFILE | string | /home/pi/client.key |
| MQTT_TLS_PASSWORD | string | |
| MQTT_TLS | true/false | false |
| MQTT_RETAIN | true/false | false |
| MQTT_QOS | 0 - 2 | 0 |
| PREFIX | string | e3dc |

### Configuration Output Values

| Key | Value/Range | Example |
| --- | --- | --- |
| AUTO_REFRESH | true/false | false |
| BATTERY_STRINGS | integer | 1 |
| DAILY_VALUES | true/false | false |
| DCB_REQUESTS | true/false | true |
| EMS_REQUESTS | true/false | true |
| FORCE_PUB * | string | e3dc/[a-z]+/power |
| HISTORY_START_YEAR | integer | 2024 |
| HST_REQUESTS | true/false | true |
| INTERVAL | integer | 4 |
| LIMIT_CHARGE_SOC | 0 - 100 | 0 |
| LIMIT_DISCHARGE_SOC | 0 - 100 | 0 |
| LIMIT_CHARGE_DURABLE | true/false | false |
| LIMIT_DISCHARGE_DURABLE | true/false | false |
| LIMIT_DISCHARGE_BY_HOME_POWER | 0 - 99999 | 0 |
| PM_EXTERN | true/false | false |
| PM_INDEX | integer | 0 |
| PM_REQUESTS | true/false | true |
| PVI_TRACKER | integer | 0 |
| RAW_MODE | true/false | true |
| RAW_TOPIC_REGEX | string | |
| RETAIN_FOR_SETUP | true/false | true |
| SOC_LIMITER | true/false | false |
| STATISTIC_VALUES | true/false | true |
| USE_TRUE_FALSE | true/false | false |
| VERBOSE | true/false | false |
| WALLBOX | true/false | false |
| WB_INDEX | integer | 0 |

*) Multiple FORCE_PUB and INFLUXDB_TOPIC

The parameters FORCE_PUB and INFLUXDB_TOPIC can be set multiple times in the configuration file.
This is not possible via environment variables, as the values would be overwritten. Therefore, please use FORCE_PUB_01, FORCE_PUB_02, INFLUXDB_TOPIC_01, INFLUXDB_TOPIC_02, INFLUXDB_TOPIC_03 etc.

### InfluxDB

| Key | Value/Range | Example |
| --- | --- | --- |
| ENABLE_INFLUXDB | true/false | false |
| INFLUXDB_TOPIC * | string | e3dc/system/.* |

### Configuration for InfluxDB v1.x

| Key | Value/Range | Example |
| --- | --- | --- |
| INFLUXDB_HOST | string | localhost |
| INFLUXDB_PORT | 0 - 65535 | 8086 |
| INFLUXDB_VERSION | integer | 1 |
| INFLUXDB_1_DB | string | e3dc |
| INFLUXDB_1_USER | string | |
| INFLUXDB_1_PASSWORD | string | |
| INFLUXDB_1_AUTH | true/false | false |

### Configuration for InfluxDB v2.x

| Key | Value/Range | Example |
| --- | --- | --- |
| INFLUXDB_2_ORGA | string | |
| INFLUXDB_2_BUCKET | string | |
| INFLUXDB_2_TOKEN | string | |
| INFLUXDB_MEASUREMENT | string | e3dc |
| INFLUXDB_MEASUREMENT_META | string | e3dc_meta |

### Configuration for InfluxDB SSL

| Key | Value/Range | Example |
| --- | --- | --- |
| CURL_HTTPS | true/false | false |
| CURL_OPT_SSL_VERIFYPEER | true/false | true |
| CURL_OPT_SSL_VERIFYHOST | true/false | true |
| CURL_OPT_CAINFO | string | |
| CURL_OPT_SSLCERT | string | |
| CURL_OPT_SSLKEY | string | |

### Hints

The prefix of the topics can be configured by the attribute PREFIX. By default all topics start with "e3dc". This can be changed to any other string that MQTT accepts as a topic, max. 24 characters. E.g. "s10" or "s10/1".

If your system has more than one battery string (e.g. S10 Pro), you have to configure the parameter BATTERY_STRINGS accordingly.
Battery topics that belong to a battery string are extended by the number of the battery string.
Battery modules (DCB topics) are numbered consecutively.

Logging can be configured for messages that the home power station output in response to a request.
Messages are collected including a counter for the number of occurrences.
The errors will be logged in a bundle at midnight, at the end of the program or by querying with e3dc/set/log/errors.
This reduces the number of error messages.
To do this, set in the .config file: `LOG_MODE=BUFFERED`.
You can also switch off the logging of such messages completely with `LOG_MODE=OFF`.
If every event is to be logged: `LOG_MODE=ON`.
