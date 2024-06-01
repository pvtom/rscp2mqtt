## Docker

A Docker image is available at https://hub.docker.com/r/pvtom/rscp2mqtt

### Configuration

Create a config file as described in the [Readme](README.md).

### Start the docker container

```
docker run -e TZ=Europe/Berlin --rm -v /path/to/your/.config:/opt/rscp2mqtt/.config pvtom/rscp2mqtt:latest
```

### Start the docker container including InfluxDB support 
 
```
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
