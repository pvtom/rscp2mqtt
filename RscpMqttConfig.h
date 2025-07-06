#ifndef RSCP_MQTT_CONFIG_H_
#define RSCP_MQTT_CONFIG_H_

#define MAX_DCB_COUNT 64
#define MAX_PM_COUNT  8
#define MAX_WB_COUNT  8

#define ENV_STRING(NAME, VAR)                         \
    do {                                              \
        const char *env = getenv(NAME);               \
        if (env) {                                    \
            strncpy(VAR, env, sizeof(VAR) - 1);       \
            VAR[sizeof(VAR) - 1] = '\0';              \
        }                                             \
    } while (0)

#define ENV_STRING_N(NAME, VAR, N)                    \
    do {                                              \
        const char *env = getenv(NAME);               \
        if (env) {                                    \
            strncpy(VAR, env, N);                     \
            VAR[N] = '\0';                            \
        }                                             \
    } while (0)

#define ENV_INT(NAME, VAR)                            \
    do {                                              \
        const char *env = getenv(NAME);               \
        if (env) {                                    \
            char *endptr;                             \
            long val = strtol(env, &endptr, 10);      \
            if (*endptr == '\0')                      \
                VAR = (int)val;                       \
        }                                             \
    } while (0)

#define ENV_INT_RANGE(NAME, VAR, FROM, TO)            \
    do {                                              \
        const char *env = getenv(NAME);               \
        if (env) {                                    \
            char *endptr;                             \
            long val = strtol(env, &endptr, 10);      \
            if ((*endptr == '\0') && (val >= FROM) && (val <= TO)) \
                VAR = (int)val;                       \
        }                                             \
    } while (0)

#define ENV_BOOL(NAME, VAR)                                            \
    do {                                                               \
        const char *env = getenv(NAME);                                \
        if (env) {                                                     \
            if (strcasecmp(env, "true") == 0 || strcmp(env, "1") == 0) \
                VAR = true;                                            \
            else                                                       \
                VAR = false;                                           \
        }                                                              \
    } while (0)

typedef struct _wb_t {
    int day_add_total[MAX_WB_COUNT];
    int day_total[MAX_WB_COUNT];
    int day_add_solar[MAX_WB_COUNT];
    int day_solar[MAX_WB_COUNT];
    int last_wallbox_energy_total_start[MAX_WB_COUNT];
    int last_wallbox_energy_solar_start[MAX_WB_COUNT];
    int last_wallbox_plugged_last[MAX_WB_COUNT];
    int last_diff_total[MAX_WB_COUNT];
    int last_diff_solar[MAX_WB_COUNT];
    int last_add_total[MAX_WB_COUNT];
    int last_add_solar[MAX_WB_COUNT];
} wb_t;

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
    char *mqtt_tls_cafile;
    char *mqtt_tls_capath;
    char *mqtt_tls_certfile;
    char *mqtt_tls_keyfile;
    char *mqtt_tls_password;
    bool mqtt_tls;
    int mqtt_qos;
    bool mqtt_retain;
    char mqtt_client_id[128];
#ifdef INFLUXDB
    uint32_t influxdb_version;
    char influxdb_host[128];
    uint32_t influxdb_port;
    char influxdb_db[128];
    char influxdb_measurement[128];
    char influxdb_measurement_meta[128];
    bool influxdb_auth;
    char influxdb_user[128];
    char influxdb_password[128];
    char influxdb_orga[128];
    char influxdb_bucket[128];
    char influxdb_token[128];
    bool curl_https;
    char *curl_protocol;
    bool curl_opt_ssl_verifypeer;
    bool curl_opt_ssl_verifyhost;
    char *curl_opt_cainfo;
    char *curl_opt_sslcert;
    char *curl_opt_sslkey;
#endif
    char prefix[25];
    int history_start_year;
    char *logfile;
    char *historyfile;
    bool verbose;
    bool once;
    int interval;
    int log_level;
    int battery_strings;
    bool pvi_requests;
    int pvi_tracker;
    int pvi_temp_count;
    int bat_dcb_count[MAX_DCB_COUNT];
    int bat_dcb_start[MAX_DCB_COUNT];
    int bat_dcb_cell_voltages;
    int bat_dcb_cell_temperatures;
    bool pm_extern;
    int pm_number;
    int pm_indexes[MAX_PM_COUNT];
    bool pm_requests;
    bool ems_requests;
    bool hst_requests;
    bool dcb_requests;
    bool soc_limiter;
    bool daily_values;
    bool statistic_values;
    bool wallbox;
    int wb_number;
    int wb_indexes[MAX_WB_COUNT];
    bool daemon;
    bool mqtt_pub;
#ifdef INFLUXDB
    bool influxdb_on;
#endif
    bool auto_refresh;
    bool store_setup;
    char true_value[5];
    char false_value[6];
    bool raw_mode;
    char *raw_topic_regex;    
} config_t;

#endif
