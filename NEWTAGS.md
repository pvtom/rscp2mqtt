## New Tags and Topics

The following options are available to support the clarification and integration of further topics:

- Raw data mode
- Configuration of tags that can be queried
- Configuration of new topics
- One-time execution

### Raw Data Mode

The raw data mode helps to find out which data is output when configuring new query tags.

It can be configured in the .config with
```
RAW_DATA=true
```
rscp2mqtt will generate additional output such as
```
e3dc/raw/TAG_EMS_POWER_PV
e3dc/raw/TAG_EMS_POWER_BAT
e3dc/raw/TAG_EMS_POWER_HOME
e3dc/raw/TAG_EMS_POWER_GRID
e3dc/raw/TAG_BAT_DATA/TAG_BAT_RSOC
```

The tag names are used to build the topic names. If there are several results for a tag, a counter is added to the end of the topic.
Raw data is only published via MQTT.

To limit the amount of raw data, a configuration entry can be created with a regular expression that describes the topics to be published:
```
RAW_TOPIC_REGEX=raw/(TAG_DCDC_DATA|TAG_BAT_DATA)/.*
```

### Configuration of Tags

All tags that are listed in RscpTags.h and have "REQ" in their name can be used.
ADD_NEW_REQUEST and ADD_NEW_REQUEST_AT_START (for one-time execution at program start) are usable for configuration in the .config.

```
ADD_NEW_REQUEST=<container>:<tag>[:value]-<sequence>
ADD_NEW_REQUEST_AT_START=<container>:<tag>[:value]-<sequence>
```

Containers are substructured tags. But containers are not labeled as such and therefore cannot be distinguished from other tags. The root container has the value 0.
If several tags are queried for a container, a query sequence can be specified, otherwise use "0".
In some cases it is necessary to set a "value" first, e.g. to specify a battery pack, a wallbox or a photovoltaic inverter.

#### Example "IP address"
```
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_IP_ADDRESS-0
# root container, tag for querying the IP address
```

#### Example "Battery info for the battery package with index 0"
```
ADD_NEW_REQUEST=TAG_BAT_REQ_DATA:TAG_BAT_INDEX:0-0
ADD_NEW_REQUEST=TAG_BAT_REQ_DATA:TAG_BAT_REQ_INFO-1
# battery data container, index value 0 as sequence 0 and request info as sequence 1
```

### Configuration of New Topics

If you have successfully configured a new tag and were able to find the output in the raw data, you can configure a new topic for it.

```
ADD_NEW_TOPIC=<container>:<tag>:<unit>:<divisor>:<bit_to_bool>:<topic>
```
Float values are divided by the divisor. In this way power values in Watt can be converted into kW.
Integer values can be converted to a boolean value (true/false) with "bit_to_bool" using a boolean AND.

unit = "_" will be handled as "no unit".

Please enter the topic without the prefix.

#### Example "IP address"
```
ADD_NEW_TOPIC=0:TAG_INFO_IP_ADDRESS:_:1:0:system/ip_address
```

rscp2mqtt always requests container and tag, no further levels. If further levels are output in the raw data, please specify the last two levels with ADD_NEW_TOPIC.

#### Example "Max DC Power"
```
ADD_NEW_REQUEST=0:TAG_EMS_REQ_MAX_DC_POWER-0
ADD_NEW_TOPIC=TAG_EMS_MAX_DC_POWER:TAG_EMS_PARAM_POWER_VALUE_L1:W:1:0:system/max_dc_power_L1
ADD_NEW_TOPIC=TAG_EMS_MAX_DC_POWER:TAG_EMS_PARAM_POWER_VALUE_L2:W:1:0:system/max_dc_power_L2
ADD_NEW_TOPIC=TAG_EMS_MAX_DC_POWER:TAG_EMS_PARAM_POWER_VALUE_L3:W:1:0:system/max_dc_power_L3
```

### Configuration of New Set Topics

```
ADD_NEW_SET_TOPIC=<container>:<tag>:<index>:<topic>:<regex>#<datatype<
# or
ADD_NEW_SET_TOPIC=<container>:<tag>:<index>:<topic>:<true_regex>:<true_value>:<false_regex>:<false_value>#<datatype<
```
You can use "regex" to define a regular expression that checks the input payload, e.g. for permitted numbers. Don't use ":" or "#" in the regex!

To check boolean values use the fields "true_regex" and "false_regex" to check the input payload for true and false. "true_value" and "false_value" define the values that are to be sent to the home power station.

"index" can be the index of a wallbox. Set "0" if unknown.

Possible data types are "Bool", "Char8", "UChar8", "Int32", "UInt32" and "Float32".

See various examples in RscpMqttMapping.h

#### Examples
```
ADD_NEW_SET_TOPIC=TAG_EMS_REQ_SET_POWER_SETTINGS:TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED:0:set/weather:^true|on|1$:1:^false|off|0$:0#UChar8
ADD_NEW_SET_TOPIC=TAG_WB_REQ_DATA:TAG_WB_REQ_SET_MIN_CHARGE_CURRENT:0:set/wallbox/min_current:^[0-9]{1,2}$#UChar8
```

### One-time Execution

The program can be started that it executes only one entire interval.
```
./rscp2mqtt -1
```

### Remarks

The procedure for integrating new tags is always trial and error. The structure of the tags is not standardized. There is no guarantee that you will get the expected values. Sometimes you receive multiple tags or tag structures in response to a single query. A look at the list of generated errors can be very helpful.

RscpTags.h is based on the old portal, so new features may not be available.

If you have made a big catch and it makes sense to integrate what you have found directly into the tool, let the community know.

### Further Examples
```
ADD_NEW_REQUEST=0:TAG_EMS_REQ_IS_PV_DERATING-0
ADD_NEW_TOPIC=0:TAG_EMS_IS_PV_DERATING:_:1:0:system/is_pv_derating

# Network
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_IP_ADDRESS-0
ADD_NEW_TOPIC=0:TAG_INFO_IP_ADDRESS:_:1:0:system/ip_address
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_SUBNET_MASK-0
ADD_NEW_TOPIC=0:TAG_INFO_SUBNET_MASK:_:1:0:system/subnet_mask
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_GATEWAY-0
ADD_NEW_TOPIC=0:TAG_INFO_GATEWAY:_:1:0:system/gateway
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_DHCP_STATUS-0
ADD_NEW_TOPIC=0:TAG_INFO_DHCP_STATUS:_:1:0:system/dhcp_status
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_DNS-0
ADD_NEW_TOPIC=0:TAG_INFO_DNS:_:1:0:system/dns
ADD_NEW_REQUEST_AT_START=0:TAG_INFO_REQ_MAC_ADDRESS-0
ADD_NEW_TOPIC=0:TAG_INFO_MAC_ADDRESS:_:1:0:system/mac_address

# Info Tag
ADD_NEW_REQUEST=0:TAG_INFO_REQ_INFO-0
ADD_NEW_REQUEST=TAG_NETWORK_REQ_INFO:TAG_NETWORK_PARAM_IP-0

# DC/DC
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_INDEX:0-0
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_STATUS-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_STATUS_AS_STRING-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_GET_BAT_CONNECTED-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_I_BAT-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_U_BAT-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_P_BAT-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_I_DCL-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_U_DCL-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_P_DCL-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_ON_GRID-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_DERATING-1
ADD_NEW_REQUEST=TAG_DCDC_REQ_DATA:TAG_DCDC_REQ_DERATING_VALUE-1

# Wallbox & Emergency Power
ADD_NEW_REQUEST=0:TAG_EMS_REQ_GET_EP_WALLBOX_ALLOW-0
ADD_NEW_REQUEST=0:TAG_EMS_REQ_GET_MAX_EP_WALLBOX_POWER_W-0
ADD_NEW_REQUEST=0:TAG_EMS_REQ_GET_MIN_EP_WALLBOX_POWER_W-0
ADD_NEW_REQUEST=0:TAG_EMS_REQ_GET_EP_WALLBOX_ENERGY-0
ADD_NEW_REQUEST=0:TAG_EMS_REQ_EP_DELAY-0

# System Specifications
ADD_NEW_REQUEST_AT_START=0:TAG_EMS_REQ_GET_SYS_SPECS-0

# SG Ready
ADD_NEW_REQUEST_AT_START=0:TAG_SGR_REQ_HW_PROVIDER_LIST-0
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_INDEX:_:1:0:sg_ready/1/provider/index
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_NAME:_:1:0:sg_ready/1/provider/name
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_INDEX:_:1:0:sg_ready/2/provider/index
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_NAME:_:1:0:sg_ready/2/provider/name
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_INDEX:_:1:0:sg_ready/3/provider/index
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_NAME:_:1:0:sg_ready/3/provider/name
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_INDEX:_:1:0:sg_ready/4/provider/index
ADD_NEW_TOPIC=TAG_SGR_HW_PROVIDER:TAG_SGR_NAME:_:1:0:sg_ready/4/provider/name
ADD_NEW_REQUEST=0:TAG_SGR_REQ_READY_TO_USE-0
ADD_NEW_TOPIC=0:TAG_SGR_READY_TO_USE:W:1:0:sg_ready/ready_to_use
ADD_NEW_REQUEST=0:TAG_SGR_REQ_COOLDOWN_END-0
ADD_NEW_TOPIC=0:TAG_SGR_COOLDOWN_END:_:1:0:sg_ready/cooldown_end
ADD_NEW_REQUEST=0:TAG_SGR_REQ_USED_POWER-0
ADD_NEW_TOPIC=0:TAG_SGR_USED_POWER:W:1:0:sg_ready/used_power
ADD_NEW_REQUEST=0:TAG_SGR_REQ_GLOBAL_OFF-0
ADD_NEW_TOPIC=0:TAG_SGR_GLOBAL_OFF:_:1:0:sg_ready/global_off
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_INDEX:0-0
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_REQ_STATE-1
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_INDEX:_:1:0:sg_ready/1/index
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_STATE:_:1:0:sg_ready/1/status
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_AKTIV:_:1:0:sg_ready/1/active
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_INDEX:1-2
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_REQ_STATE-3
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_INDEX:_:1:0:sg_ready/2/index
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_STATE:_:1:0:sg_ready/2/status
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_AKTIV:_:1:0:sg_ready/2/active
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_INDEX:2-4
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_REQ_STATE-5
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_INDEX:_:1:0:sg_ready/3/index
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_STATE:_:1:0:sg_ready/3/status
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_AKTIV:_:1:0:sg_ready/3/active
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_INDEX:3-6
ADD_NEW_REQUEST=TAG_SGR_REQ_DATA:TAG_SGR_REQ_STATE-7
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_INDEX:_:1:0:sg_ready/4/index
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_STATE:_:1:0:sg_ready/4/status
ADD_NEW_TOPIC=TAG_SGR_DATA:TAG_SGR_AKTIV:_:1:0:sg_ready/4/active
```
