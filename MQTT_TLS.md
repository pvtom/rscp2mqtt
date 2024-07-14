## MQTT with TLS

rscp2mqtt can connect the MQTT broker using TLS.

### Configuration

Add these lines to the .config file and adjust the values according to your environment:

```
MQTT_TLS=true
MQTT_TLS_CAFILE=/home/pi/ca.crt
MQTT_TLS_CERTFILE=/home/pi/client.crt
MQTT_TLS_KEYFILE=/home/pi/client.key
```

### Certificates and Broker Configuration

Please follow these commands to create an example environment on your computer with a running Mosquitto broker:

Switch to root
```
sudo -i
```

Create server key file and certificate
```
cd /etc/mosquitto/ca_certificates

openssl genrsa -des3 -out ca.key 2048
openssl req -new -x509 -days 1826 -key ca.key -out ca.crt

cd /etc/mosquitto/certs

openssl genrsa -out mosquitto.key 2048
openssl req -new -out mosquitto.csr -key mosquitto.key

# Common Name = ip address of the server

openssl x509 -req -in mosquitto.csr -CA /etc/mosquitto/ca_certificates/ca.crt -CAkey /etc/mosquitto/ca_certificates/ca.key -CAcreateserial -out mosquitto.crt
```

Create client key file and certificate
```
cd /etc/mosquitto/certs

openssl genrsa -out client.key 2048
openssl req -new -out client.csr -key client.key

# Common Name = ip address of the server

openssl x509 -req -in client.csr -CA /etc/mosquitto/ca_certificates/ca.crt -CAkey /etc/mosquitto/ca_certificates/ca.key -CAcreateserial -out client.crt

chmod a+r *
```

### Broker Configuration
```
cd /etc/mosquitto/conf.d

nano 010-listener-with-tls.conf
```

Please insert the following lines into 010-listener-with-tls.conf
```
listener 8883
certfile /etc/mosquitto/certs/mosquitto.crt
keyfile /etc/mosquitto/certs/mosquitto.key
cafile /etc/mosquitto/ca_certificates/ca.crt
require_certificate true
```

Restart the MQTT broker
```
systemctl restart mosquitto.service
```

### Prepare Client 

Copy client key and certificate
```
cd /home/pi
sudo mv /etc/mosquitto/certs/client.* .
sudo chown pi.pi client.*
cp /etc/mosquitto/ca_certificates/ca.crt .
```

Adjust .config to the ip address (according to the certificate definition) and the new port number
```
MQTT_HOST=192.168.178.123
MQTT_PORT=8883
```

Start rscp2mqtt

Subscribe to the MQTT broker
```
# use the ip address of the server
mosquitto_sub -h 192.168.178.123 -p 8883 -t "#" --cafile /home/pi/ca.crt --cert /home/pi/client.crt --key /home/pi/client.key
```
