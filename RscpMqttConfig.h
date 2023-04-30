#ifndef RSCP_MQTT_CONFIG_H_
#define RSCP_MQTT_CONFIG_H_

typedef struct _config_t {
    char e3dc_ip[20];
    uint32_t e3dc_port;
    char e3dc_user[128];
    char e3dc_password[128];
    char aes_password[128];
    char mqtt_host[128];
    uint32_t mqtt_port;
    char mqtt_user[128];
    char mqtt_password[128];
    bool mqtt_auth;
    int mqtt_qos;
    bool mqtt_retain;
#ifdef INFLUXDB
    uint32_t influxdb_version;
    char influxdb_host[128];
    uint32_t influxdb_port;
    char influxdb_db[128];
    char influxdb_measurement[128];
    char influxdb_orga[128];
    char influxdb_bucket[128];
    char influxdb_token[128];
#endif
    char *logfile;
    bool verbose;
    int interval;
    bool pvi_requests;
    int pvi_tracker;
    bool pm_requests;
    bool wallbox;
    bool daemon;
    bool mqtt_pub;
#ifdef INFLUXDB
    bool influxdb_on;
#endif
    bool auto_refresh;
} config_t;

#endif
