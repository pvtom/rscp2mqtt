## Docker

A Docker image is available at https://hub.docker.com/r/pvtom/rscp2mqtt

### Configuration

- Create a config file as described in the [Readme](README.md) or
- Set the parameters by -e KEY=VALUE ([Config](CONFIG.md))

### Start the docker container

```
docker run --rm -e TZ=Europe/Berlin -e E3DC_IP=<IP> -e E3DC_USER=<your user> -e E3DC_PASSWORD=<your password -e E3DC_AES_PASSWORD=<your AES password> -e MQTT_HOST=<MQTT broker> pvtom/rscp2mqtt:latest

# or with .config
docker run -e TZ=Europe/Berlin --rm -v /path/to/your/.config:/opt/rscp2mqtt/.config pvtom/rscp2mqtt:latest
```

### Start the docker container including InfluxDB support 
 
```
docker run --rm -e TZ=Europe/Berlin -e E3DC_IP=<IP> -e E3DC_USER=<your user> -e E3DC_PASSWORD=<your password -e E3DC_AES_PASSWORD=<your AES password> -e MQTT_HOST=<MQTT broker> -e ENABLE_INFLUXDB=true -e INFLUXDB_HOST=<host name> -e INFLUXDB_VERSION=1 -e INFLUXDB_1_DB=<db name> pvtom/rscp2mqtt:latest-with-influxdb

# or with .config
docker run -e TZ=Europe/Berlin --rm -v /path/to/your/.config:/opt/rscp2mqtt/.config pvtom/rscp2mqtt:latest-with-influxdb
```

### Start the docker container with TLS to connect the MQTT broker

Depending on the configuration of your TLS environment, adopt your .config file.

Example:
```
MQTT_TLS=true
MQTT_TLS_CAFILE=tls/ca.crt
MQTT_TLS_CERTFILE=tls/client.crt
MQTT_TLS_KEYFILE=tls/client.key
```

Start with
```
docker run -e TZ=Europe/Berlin --rm -v /path/to/your/.config:/opt/rscp2mqtt/.config -v /path/to/your/tls:/opt/rscp2mqtt/tls pvtom/rscp2mqtt:latest
```
or with InfluxDB support
```
docker run -e TZ=Europe/Berlin --rm -v /path/to/your/.config:/opt/rscp2mqtt/.config -v /path/to/your/tls:/opt/rscp2mqtt/tls pvtom/rscp2mqtt:latest-with-influxdb
```
