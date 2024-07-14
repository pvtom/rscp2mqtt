## InfluxDB

rscp2mqtt implements the data transfer to an InfluxDB time-series database.
InfluxDB v1.x and v2.x are supported.

### Configuration for InfluxDB v1.x

Add these lines to the .config file and adjust the values according to your environment:

```
ENABLE_INFLUXDB=true
INFLUXDB_HOST=localhost
INFLUXDB_PORT=8086
INFLUXDB_VERSION=1
INFLUXDB_MEASUREMENT=e3dc
INFLUXDB_MEASUREMENT_META=e3dc_meta
INFLUXDB_1_DB=e3dc
INFLUXDB_1_AUTH=false
INFLUXDB_1_USER=
INFLUXDB_1_PASSWORD=
INFLUXDB_TOPIC=e3dc/[a-z]+/power
INFLUXDB_TOPIC=e3dc/day/.*
```
You can configure the topics to be transferred to the InfluxDB by setting the parameter INFLUXDB_TOPIC.
INFLUXDB_TOPIC can occur several times.
If no INFLUXDB_TOPIC configuration exists, all topics will be transferred.

Topics with payload of data types integer or float will be stored to the measurement set by INFLUXDB_MEASUREMENT.
Other topics (strings) will be stored to the measurement set by INFLUXDB_MEASUREMENT_META.

Start the influx client to prepare the database.

Create a database
```
create database e3dc
```

Access data using the influx client
```
precision rfc3339
use e3dc
select * from e3dc where topic = 'e3dc/solar/power' tz('Europe/Berlin')
select * from e3dc where topic =~ /e3dc\/day/ tz('Europe/Berlin');
```

Set a retention policy to limit data collection
```
CREATE RETENTION POLICY "one_day" ON "e3dc" DURATION 24h REPLICATION 1 DEFAULT
```

### Configuration for InfluxDB v2.x

Add and adjust the following lines in .config
```
ENABLE_INFLUXDB=true
INFLUXDB_HOST=localhost
INFLUXDB_PORT=8086
INFLUXDB_VERSION=2
INFLUXDB_MEASUREMENT=e3dc
INFLUXDB_2_ORGA=<my_orga>
INFLUXDB_2_BUCKET=<my_bucket>
INFLUXDB_2_TOKEN=<my_token>
```

Please use the web admin tool of the InfluxDB v2.x to configure the bucket, the orga and the token.

### TLS Encryption

rscp2mqtt uses the library libcurl for the connection to InfluxDB. To establish a TLS connection, please use the following configuration parameters.
```
CURL_HTTPS=true
CURL_OPT_SSL_VERIFYPEER=true
CURL_OPT_SSL_VERIFYHOST=true
CURL_OPT_CAINFO=
CURL_OPT_SSLCERT=
CURL_OPT_SSLKEY=
```

For the configuration of the database look at the official InfluxDB documentation.
