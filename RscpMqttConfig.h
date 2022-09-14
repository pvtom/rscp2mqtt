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
    char logfile[128];
    int interval;
    bool daemon;
} config_t;

#endif
