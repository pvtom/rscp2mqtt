#ifndef RSCP_MQTT_CONFIG_H_
#define RSCP_MQTT_CONFIG_H_

#define MAX_DCB_COUNT 64
#define MAX_PM_COUNT  8
#define MAX_WB_COUNT  8

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
    char mqtt_client_id[128];
#ifdef INFLUXDB
    uint32_t influxdb_version;
    char influxdb_host[128];
    uint32_t influxdb_port;
    char influxdb_db[128];
    char influxdb_measurement[128];
    bool influxdb_auth;
    char influxdb_user[128];
    char influxdb_password[128];
    char influxdb_orga[128];
    char influxdb_bucket[128];
    char influxdb_token[128];
#endif
    char prefix[25];
    int history_start_year;
    char *logfile;
    char *historyfile;
    bool verbose;
    int interval;
    int log_level;
    int battery_strings;
    bool pvi_requests;
    int pvi_tracker;
    int pvi_temp_count;
    int bat_dcb_count[MAX_DCB_COUNT];
    int bat_dcb_start[MAX_DCB_COUNT];
    bool pm_extern;
    int pm_number;
    int pm_index[MAX_PM_COUNT];
    bool pm_requests;
    bool dcb_requests;
    bool soc_limiter;
    bool daily_values;
    bool statistic_values;
    bool wallbox;
    int wb_index;
    bool daemon;
    bool mqtt_pub;
#ifdef INFLUXDB
    bool influxdb_on;
#endif
    bool auto_refresh;
} config_t;

#endif
