#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef INFLUXDB
    #include <curl/curl.h>
#endif
#include "RscpProtocol.h"
#include "RscpTags.h"
#include "RscpMqttMapping.h"
#include "RscpMqttConfig.h"
#include "SocketConnection.h"
#include "AES.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <mosquitto.h>
#include <thread>
#include <regex>
#include <mutex>

#define RSCP2MQTT               "v3.11"

#define AES_KEY_SIZE            32
#define AES_BLOCK_SIZE          32

#define E3DC_FOUNDED            2010
#define CONFIG_FILE             ".config"
#define DEFAULT_INTERVAL_MIN    1
#define DEFAULT_INTERVAL_MAX    300
#define IDLE_PERIOD_CACHE_SIZE  14
#define DEFAULT_PREFIX          "e3dc"
#define SUBSCRIBE_TOPIC         "set/#"

#define MQTT_PORT_DEFAULT       1883

#ifdef INFLUXDB
    #define INFLUXDB_PORT_DEFAULT   8086
    #define CURL_BUFFER_SIZE        1024
#endif

#define CHARGE_LOCK_TRUE        "today:charge:true:00:00-23:59"
#define CHARGE_LOCK_FALSE       "today:charge:false:00:00-23:59"
#define DISCHARGE_LOCK_TRUE     "today:discharge:true:00:00-23:59"
#define DISCHARGE_LOCK_FALSE    "today:discharge:false:00:00-23:59"
#define PV_SOLAR_MIN            200
#define MAX_DAYS_PER_ITERATION  12

static int iSocket = -1;
static int iAuthenticated = 0;
static AES aesEncrypter;
static AES aesDecrypter;
static uint8_t ucEncryptionIV[AES_BLOCK_SIZE];
static uint8_t ucDecryptionIV[AES_BLOCK_SIZE];

static struct mosquitto *mosq = NULL;
static time_t e3dc_ts = 0;
static time_t e3dc_diff = 0;
static int gmt_diff = 0;
static config_t cfg;
static int day, leap_day, year, curr_year, battery_nr;
static uint8_t period_change_nr = 0;
static bool period_trigger = false;
static bool history_init = true;

std::mutex mtx;

static bool mqttRcvd = false;

#ifdef INFLUXDB
    CURL *curl = NULL;
    struct curl_slist *headers = NULL;
#endif

void logMessage(char *file, char *srcfile, int line, char *format, ...);

void wsleep(int sec) {
    for (uint8_t i = 0; i < sec; i++) {
        mtx.lock();
        if (mqttRcvd) {
            mtx.unlock();
            return;
        }
        mtx.unlock();
        sleep(1);
    }
    return;
}

int storeMQTTReceivedValue(std::vector<RSCP_MQTT::rec_cache_t> & c, char *topic_in, char *payload) {
    char topic[TOPIC_SIZE];
    mtx.lock();
    mqttRcvd = true;
    for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        snprintf(topic, TOPIC_SIZE, "%s/%s", cfg.prefix, it->topic);
        if (!strcmp(topic, topic_in) && it->done) {
            //logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"MQTT: received topic >%s< payload >%s<\n", it->topic, payload);
            if (std::regex_match(payload, std::regex(it->regex_true))) {
                if (strcmp(it->value_true, "")) strncpy(it->payload, it->value_true, PAYLOAD_SIZE);
                else strncpy(it->payload, payload, PAYLOAD_SIZE);
                if (it->queue) {
                    if (!strcmp(it->topic, "set/request/day")) {
                        int year, month, day;
                        if (sscanf(it->payload, "%d-%d-%d", &year, &month, &day)) {
                            RSCP_MQTT::requestQ.push({day, month, year});
                        }
                    }
                } else it->done = false;
                break;
            } else if (std::regex_match(payload, std::regex(it->regex_false))) {
                if (strcmp(it->value_false, "")) strncpy(it->payload, it->value_false, PAYLOAD_SIZE);
                else strncpy(it->payload, payload, PAYLOAD_SIZE);
                it->done = false;
                break;
            } else {
                it->done = true;
                break;
            }
        }
    }
    mtx.unlock();
    return(0);
}

void hotfixSetEntry(std::vector<RSCP_MQTT::rec_cache_t> & c, uint32_t tag, int type, char *regex_true) {
    printf("hotfixSetEntry was called with tag >0x%08X< type >%d< regex_true >%s<\n", tag, type, regex_true);
    for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if (it->tag == tag) {
            if (type) it->type = type;
            if (regex_true && (strcmp(regex_true, ""))) strcpy(it->regex_true, regex_true);
            printf("hotfixSetEntry: tag >0x%08X< topic >%s< type >%d< regex_true >%s<\n", it->tag, it->topic, it->type, it->regex_true);
        }
    }
    return;
}

// topic: <prefix>/set/power_mode payload: auto / idle:60 (cycles) / discharge:2000:60 (Watt:cycles) / charge:2000:60 (Watt und sec) / grid_charge:2000:60 (Watt:cycles)
int handleSetPower(std::vector<RSCP_MQTT::rec_cache_t> & c, uint32_t container, char *payload) {
    char cycles[12];
    char cmd[12];
    char power[12];
    char modus[2];

    if (!strcmp(payload, "auto")) {
        strcpy(cycles, "0");
        strcpy(modus, "0");
        strcpy(power, "0");
    } else if ((sscanf(payload, "%12[^:]:%12[^:]", cmd, cycles) == 2) && (!strcmp(cmd, "idle"))) {
        strcpy(modus, "1");
        strcpy(power, "0");
    } else if (sscanf(payload, "%12[^:]:%12[^:]:%12[^:]", cmd, power, cycles) == 3) {
        if (!strcmp(cmd, "discharge")) strcpy(modus, "2");
        else if (!strcmp(cmd, "charge")) strcpy(modus, "3");
        else strcpy(modus, "4");
    } else return(0);

    for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if (it->container == container) {
            switch (it->tag) {
                case TAG_EMS_REQ_SET_POWER_MODE: {
                    strcpy(it->payload, modus);
                    it->refresh_count = abs(atoi(cycles));
                    break;
                }
                case TAG_EMS_REQ_SET_POWER_VALUE: {
                    strcpy(it->payload, power);
                    it->refresh_count = abs(atoi(cycles));
                    break;
                }
            }
        }
    }
    return(0);
}

int handleSetIdlePeriod(RscpProtocol *protocol, SRscpValue *rootContainer, char *payload) {
    int day, starthour, startminute, endhour, endminute;
    char daystring[12];
    char typestring[12];
    char activestring[12];

    memset(daystring, 0, sizeof(daystring));
    memset(typestring, 0, sizeof(typestring));
    memset(activestring, 0, sizeof(activestring));

    if (sscanf(payload, "%12[^:]:%12[^:]:%12[^:]:%d:%d-%d:%d", daystring, typestring, activestring, &starthour, &startminute, &endhour, &endminute) != 7) {
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"handleSetIdlePeriod: payload=>%s< not enough attributes.\n", payload);
        return(1);
    }

    for (day = 0; day < 8; day++) {
        if (!strcmp(RSCP_MQTT::days[day].c_str(), daystring)) break;
    }
    if (day == 7) { // today
        time_t rawtime;
        time(&rawtime);
        struct tm *l = localtime(&rawtime);
        day = l->tm_wday?(l->tm_wday-1):6;
    }

    if (starthour*60+startminute >= endhour*60+endminute) {
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"handleSetIdlePeriod: payload=>%s< time range %02d:%02d-%02d:%02d not allowed.\n", payload, starthour, startminute, endhour, endminute);
        return(1);
    }

    SRscpValue setTimeContainer;
    protocol->createContainerValue(&setTimeContainer, TAG_EMS_REQ_SET_IDLE_PERIODS);

    SRscpValue setDayContainer;
    protocol->createContainerValue(&setDayContainer, TAG_EMS_IDLE_PERIOD);
    protocol->appendValue(&setDayContainer, TAG_EMS_IDLE_PERIOD_TYPE, (uint8_t)strcmp(typestring, "discharge")?0:1); // 0 = charge 1 = discharge
    protocol->appendValue(&setDayContainer, TAG_EMS_IDLE_PERIOD_DAY, (uint8_t)day); // 0 = Monday -> 6 = Sunday
    protocol->appendValue(&setDayContainer, TAG_EMS_IDLE_PERIOD_ACTIVE, strcmp(activestring, "true")?false:true);

    SRscpValue setStartContainer;
    protocol->createContainerValue(&setStartContainer, TAG_EMS_IDLE_PERIOD_START);
    protocol->appendValue(&setStartContainer, TAG_EMS_IDLE_PERIOD_HOUR, (uint8_t)starthour);
    protocol->appendValue(&setStartContainer, TAG_EMS_IDLE_PERIOD_MINUTE, (uint8_t)startminute);
    protocol->appendValue(&setDayContainer, setStartContainer);
    protocol->destroyValueData(setStartContainer);

    SRscpValue setStopContainer;
    protocol->createContainerValue(&setStopContainer, TAG_EMS_IDLE_PERIOD_END);
    protocol->appendValue(&setStopContainer, TAG_EMS_IDLE_PERIOD_HOUR, (uint8_t)endhour);
    protocol->appendValue(&setStopContainer, TAG_EMS_IDLE_PERIOD_MINUTE, (uint8_t)endminute);
    protocol->appendValue(&setDayContainer, setStopContainer);
    protocol->destroyValueData(setStopContainer);

    protocol->appendValue(&setTimeContainer, setDayContainer);
    protocol->destroyValueData(setDayContainer);

    protocol->appendValue(rootContainer, setTimeContainer);
    protocol->destroyValueData(setTimeContainer);

    return(0);
}

void addTemplTopics(uint32_t container, int index, char *seg, int start, int n, int inc) {
    for (int c = start; c < n; c++) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCacheTempl.begin(); it != RSCP_MQTT::RscpMqttCacheTempl.end(); ++it) {
            if (it->container == container) {
                RSCP_MQTT::cache_t cache = { it->container, it->tag, c, "", "", it->format, "", it->divisor, it->bit_to_bool, it->history_log, it->changed, it->influx, it->forced };
                if ((seg == NULL) && (index == 0)) { // no args
                    strcpy(cache.topic, it->topic);
                } else if ((seg == NULL) && (index == 1)) { // "%d"
                    snprintf(cache.topic, TOPIC_SIZE, it->topic, c + inc);
                } else if ((seg != NULL) && (index == 0)) { // "%s"
                    snprintf(cache.topic, TOPIC_SIZE, it->topic, seg);
                } else { // "%s" with "%s/%d"
                    char buffer[TOPIC_SIZE];
                    snprintf(buffer, TOPIC_SIZE, "%s/%d", seg, c + inc);
                    snprintf(cache.topic, TOPIC_SIZE, it->topic, buffer);
                }
                strcpy(cache.unit, it->unit);
                for (std::vector<RSCP_MQTT::topic_store_t>::iterator it2 = RSCP_MQTT::TopicStore.begin(); it2 != RSCP_MQTT::TopicStore.end(); ++it2) {
                    switch (it2->type) {
                        case FORCED_TOPIC: {
                            if (std::regex_match(cache.topic, std::regex(it2->topic))) {
                               cache.forced = true;
                               if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Topic >%s< forced\n", cache.topic);
                            }
                            break;
                        }
                        case LOG_TOPIC: {
                            if (std::regex_match(cache.topic, std::regex(it2->topic))) {
                               cache.history_log = true;
                               if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Topic >%s< marked for logging\n", cache.topic);
                            }
                            break;
                        }
#ifdef INFLUXDB
                        case INFLUXDB_TOPIC: {
                            if (std::regex_match(cache.topic, std::regex(it2->topic))) {
                               cache.influx = true;
                               if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Topic >%s< marked influx relevant\n", cache.topic);
                            }
                            break;
                        }
#endif
                    }
                }
                RSCP_MQTT::RscpMqttCache.push_back(cache);
            }
        }
    }
    return;
}

void mqttCallbackOnConnect(struct mosquitto *mosq, void *obj, int result) {
    char topic[TOPIC_SIZE];
    snprintf(topic, TOPIC_SIZE, "%s/%s", cfg.prefix, SUBSCRIBE_TOPIC); 
    if (!result) {
        mosquitto_subscribe(mosq, NULL, topic, cfg.mqtt_qos);
    } else {
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: subscribing topic >%s< failed\n", topic);
    }
}

void mqttCallbackOnMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    if (mosq && msg && msg->topic && msg->payloadlen) {
        storeMQTTReceivedValue(RSCP_MQTT::RscpMqttReceiveCache, msg->topic, (char *)msg->payload);
    }
}

void mqttListener(struct mosquitto *m) {
    if (m) {
         logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"MQTT: starting listener loop\n");
         mosquitto_loop_forever(m, -1, 1);
    }
}

#ifdef INFLUXDB
int insertInfluxDb(char *buffer) {
    CURLcode res = CURLE_OK;

    if (strlen(buffer)) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"%s\n", curl_easy_strerror(res));
    }
    return(res);
}

void handleInfluxDb(char *buffer, char *topic, char *payload, char *unit, char *timestamp) {
    char line[CURL_BUFFER_SIZE];

    if (topic == NULL) {
        insertInfluxDb(buffer);
        return;
    }

    if (!strcmp(payload, "true")) sprintf(line, "%s,topic=%s value=1", cfg.influxdb_measurement, topic);
    else if (!strcmp(payload, "false")) sprintf(line, "%s,topic=%s value=0", cfg.influxdb_measurement, topic);
    else if (strcmp(unit, "") && timestamp) sprintf(line, "%s,topic=%s,unit=%s value=%s %s", cfg.influxdb_measurement, topic, unit, payload, timestamp);
    else if (strcmp(unit, "")) sprintf(line, "%s,topic=%s,unit=%s value=%s", cfg.influxdb_measurement, topic, unit, payload);
    else return;

    if (strlen(buffer) + strlen(line) + 1 < CURL_BUFFER_SIZE) {
        strcat(buffer, "\n");
        strcat(buffer, line);
    } else {
        insertInfluxDb(buffer);
        strcpy(buffer, "");
        strcat(buffer, line);
    }
    return;
}
#endif

int handleMQTT(std::vector<RSCP_MQTT::cache_t> & v, int qos, bool retain) {
#ifdef INFLUXDB
    char buffer[CURL_BUFFER_SIZE];
    strcpy(buffer, "");
#endif
    int rc = 0;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if ((it->changed || it->forced) && strcmp(it->topic, "") && strcmp(it->payload, "")) {
            if (cfg.verbose) {
               if (cfg.mqtt_pub) printf("MQTT ");
#ifdef INFLUXDB
               if (cfg.influxdb_on && it->influx) printf("INFLUX ");
#endif
               printf("publish topic >%s< payload >%s< unit >%s<\n", it->topic, it->payload, it->unit);
            }
            if (it->history_log) logMessage(cfg.historyfile, (char *)__FILE__, __LINE__, (char *)"topic >%s< payload >%s< unit >%s<\n", it->topic, it->payload, it->unit);
            if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, it->topic, strlen(it->payload), it->payload, qos, retain);
#ifdef INFLUXDB
            if (cfg.influxdb_on && curl && it->influx) handleInfluxDb(buffer, it->topic, it->payload, it->unit, NULL);
#endif
            it->changed = false;
        }
    }
#ifdef INFLUXDB
    if (cfg.influxdb_on && curl) handleInfluxDb(buffer, NULL, NULL, NULL, NULL);
#endif
    return(rc);
}

int handleMQTTIdlePeriods(std::vector<RSCP_MQTT::idle_period_t> & v, int qos, bool retain) {
    int rc = 0;
    int i = 0;
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    RSCP_MQTT::idle_period_t e;

    while (!v.empty()) {
        e = v.back();
        snprintf(topic, TOPIC_SIZE, "%s/idle_period/%d/%d", cfg.prefix, e.marker, ++i);
        sprintf(payload, "%s:%s:%s:%02d:%02d-%02d:%02d", RSCP_MQTT::days[e.day].c_str(), e.type?"discharge":"charge", e.active?"true":"false", e.starthour, e.startminute, e.endhour, e.endminute);
        if (cfg.verbose && cfg.mqtt_pub) printf("MQTT publish topic >%s< payload >%s<\n", topic, payload);
        if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, qos, retain);
        v.pop_back();
    }
    return(rc);
}

int handleMQTTErrorMessages(std::vector<RSCP_MQTT::error_t> & v, int qos, bool retain) {
    int rc = 0;
    int i = 0;
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    RSCP_MQTT::error_t e;

    while (!v.empty()) {
        e = v.back();
        snprintf(topic, TOPIC_SIZE, "%s/error_message/%d/meta", cfg.prefix, ++i);
        snprintf(payload, PAYLOAD_SIZE, "type=%d code=%d source=%s", e.type, e.code, e.source);
        if (cfg.verbose && cfg.mqtt_pub) printf("MQTT publish topic >%s< payload >%s<\n", topic, e.message);
        if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, qos, retain);
        snprintf(topic, TOPIC_SIZE, "%s/error_message/%d", cfg.prefix, i);
        if (cfg.verbose && cfg.mqtt_pub) printf("MQTT publish topic >%s< payload >%s<\n", topic, e.message);
        if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(e.message), e.message, qos, retain);
        v.pop_back();
    }
    return(rc);
}

void logCache(std::vector<RSCP_MQTT::cache_t> & v, char *file, char *prefix) {
    if (file) {
        FILE *fp;
        fp = fopen(file, "a");
        if (!fp) return;
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            fprintf(fp, "%s: topic >%s< payload >%s< unit >%s<%s\n", prefix, it->topic, it->payload, it->unit, (it->forced?" forced":""));
        }
        fflush(fp);
        fclose(fp);
    } else {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            printf("%s: topic >%s< payload >%s< unit >%s<%s\n", prefix, it->topic, it->payload, it->unit, (it->forced?" forced":""));
        }
        fflush(NULL);
    }
    return;
}

void logTopics(std::vector<RSCP_MQTT::cache_t> & v, char *file) {
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (it->forced) logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< forced\n", it->topic);
        if (it->influx) logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< marked influx relevant\n", it->topic);
        if (it->history_log) logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< marked for logging\n", it->topic);
    }   
    return;
}

void logHealth(char *file) {
    logTopics(RSCP_MQTT::RscpMqttCache, file);
    for (std::vector<RSCP_MQTT::not_supported_tags_t>::iterator it = RSCP_MQTT::NotSupportedTags.begin(); it != RSCP_MQTT::NotSupportedTags.end(); ++it) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"Container 0x%08X Tag 0x%08X not supported.\n", it->container, it->tag);
    }
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCacheTempl.begin(); it != RSCP_MQTT::RscpMqttCacheTempl.end(); ++it) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"Container 0x%08X Tag 0x%08X in temporary cache.\n", it->container, it->tag);
    }
}

void logMessage(char *file, char *srcfile, int line, char *format, ...)
{
    FILE * fp;

    if (file) {
        fp = fopen(file, "a");
        if (!fp) return;
    }

    pid_t pid, ppid;
    va_list args;
    char timestamp[64];

    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);
    strftime(timestamp, 26, "%Y-%m-%d %H:%M:%S", l);
    pid = getpid();
    ppid = getppid();

    va_start(args, format);
    if (file) {
        fprintf(fp, "[%s] pid=%d ppid=%d %s(%d) ", timestamp, pid, ppid, srcfile, line);
        vfprintf(fp, format, args);
        fflush(fp);
        fclose(fp);
    } else {
        printf("[%s] pid=%d ppid=%d %s(%d) ", timestamp, pid, ppid, srcfile, line);
        vprintf(format, args);
        fflush(NULL);
    }
    va_end(args);
    return;
}

void copyCache(std::vector<RSCP_MQTT::cache_t> & to, std::vector<RSCP_MQTT::cache_t> & from, uint32_t container) {
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = from.begin(); it != from.end(); ++it) {
        if (it->container == container) {
            RSCP_MQTT::cache_t cache = { it->container, it->tag, it->index, "", "", it->format, "", it->divisor, it->bit_to_bool, it->history_log, it->changed, it->influx, it->forced };
            strcpy(cache.topic, it->topic);
            strcpy(cache.payload, it->payload);
            strcpy(cache.unit, it->unit);
            to.push_back(cache);
        }
    }
    return;
}

void cleanupCache(std::vector<RSCP_MQTT::cache_t> & v) {
    std::vector<RSCP_MQTT::cache_t> TempCache({});
    copyCache(TempCache, v, TAG_DB_HISTORY_DATA_YEAR);
    copyCache(TempCache, v, TAG_DB_HISTORY_DATA_DAY);
    RSCP_MQTT::RscpMqttCacheTempl.clear();
    copyCache(v, TempCache, TAG_DB_HISTORY_DATA_DAY);
    copyCache(v, TempCache, TAG_DB_HISTORY_DATA_YEAR);
    return;
}

bool existsNotSupportedTag(uint32_t container, uint32_t tag) {
    for (std::vector<RSCP_MQTT::not_supported_tags_t>::iterator it = RSCP_MQTT::NotSupportedTags.begin(); it != RSCP_MQTT::NotSupportedTags.end(); ++it) {
        if ((it->container == container) && (it->tag == tag)) return(true);
    }
    return(false);
}

void pushNotSupportedTag(uint32_t container, uint32_t tag) {
    if (existsNotSupportedTag(container, tag)) return;
    RSCP_MQTT::not_supported_tags_t v;
    v.container = container;
    v.tag = tag;
    RSCP_MQTT::NotSupportedTags.push_back(v);
    return;
}

void refreshCache(std::vector<RSCP_MQTT::cache_t> & v, char *payload) {
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if ((it->changed == false) && strcmp(it->payload, "") && (std::regex_match(it->topic, std::regex(payload)) || std::regex_match(payload, std::regex("^true|on|1$")))) it->changed = true;
    }
    if (std::regex_match(payload, std::regex("^true|on|1$")) || std::regex_match(payload, std::regex(".*(idle|period).*"))) period_trigger = true;
    return;
}

void addPrefix(std::vector<RSCP_MQTT::cache_t> & v, char *prefix) {
    char topic[TOPIC_SIZE];
    if (prefix) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            snprintf(topic, TOPIC_SIZE, "%s/%s", cfg.prefix, it->topic);
            strcpy(it->topic, topic);
        }
    }
    return;
}

void setTopicsForce(std::vector<RSCP_MQTT::cache_t> & v, char *regex) {
    if (regex) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            if (std::regex_match(it->topic, std::regex(regex))) it->forced = true;
        }
    }
    return;
}

void setTopicHistory(std::vector<RSCP_MQTT::cache_t> & v, char *regex) {
    if (regex) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            if (std::regex_match(it->topic, std::regex(regex))) it->history_log = true;
        }
    }
    return;
}

#ifdef INFLUXDB
void setTopicsInflux(std::vector<RSCP_MQTT::cache_t> & v, char *regex) {
    if (regex) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            if (std::regex_match(it->topic, std::regex(regex))) it->influx = true;
        }
    }
    return;
}
#endif

int handleImmediately(RscpProtocol *protocol, SRscpValue *response, uint32_t container, int year, int month, int day) {
#ifdef INFLUXDB
    char buffer[CURL_BUFFER_SIZE];
    char ts[24];
    strcpy(buffer, "");
    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);
    l->tm_sec = 0;
    l->tm_min = 0;
    l->tm_hour = 0;
    l->tm_isdst = -1;
#endif
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    int rc = 0;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCacheTempl.begin(); it != RSCP_MQTT::RscpMqttCacheTempl.end(); ++it) {
        if ((it->container == container) && (it->tag == response->tag)) {
            snprintf(topic, TOPIC_SIZE, it->topic, year, month, day);
            snprintf(payload, PAYLOAD_SIZE, "%0.2f", protocol->getValueAsFloat32(response) / it->divisor);
            if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, cfg.mqtt_qos, cfg.mqtt_retain);
#ifdef INFLUXDB
            l->tm_mday = day;
            l->tm_mon = month - 1;
            l->tm_year = year - 1900;
            sprintf(ts, "%llu000000000", timegm(l) - gmt_diff);
            if (cfg.influxdb_on && curl) {
                for (std::vector<RSCP_MQTT::topic_store_t>::iterator store = RSCP_MQTT::TopicStore.begin(); store != RSCP_MQTT::TopicStore.end(); ++store) {
                    if ((store->type == INFLUXDB_TOPIC) && std::regex_match(topic, std::regex(store->topic))) {
                        handleInfluxDb(buffer, topic, payload, it->unit, ts);
                    }
                }
            }
#endif
        }
    }
#ifdef INFLUXDB
    if (cfg.influxdb_on && curl) handleInfluxDb(buffer, NULL, NULL, NULL, NULL);
#endif
    return(rc);
}

int storeIntegerValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, int value, int index) {
    char buf[PAYLOAD_SIZE];
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
            if (it->bit_to_bool) {
                if (value & it->bit_to_bool) strcpy(buf, "true");
                else strcpy(buf, "false");
            } else {
                snprintf(buf, PAYLOAD_SIZE, "%d", value);
            }
            if (strcmp(it->payload, buf)) {
                strcpy(it->payload, buf);
                it->changed = true;
            }
        }
    }
    return(1);
}

int storeStringValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, char *value, int index) {
    char buf[PAYLOAD_SIZE];
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
                snprintf(buf, PAYLOAD_SIZE, "%s", value);
            if (strcmp(it->payload, buf)) {
                strcpy(it->payload, buf);
                it->changed = true;
                it->influx = false;
            }
            break;
        }
    }
    return(1);
}

int storeResponseValue(std::vector<RSCP_MQTT::cache_t> & c, RscpProtocol *protocol, SRscpValue *response, uint32_t container, int index) {
    char buf[PAYLOAD_SIZE];
    int rc = -1;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((!it->container || (it->container == container)) && (it->tag == response->tag) && (it->index == index)) {
            switch (response->dataType) {
                case RSCP::eTypeBool: {
                    if (protocol->getValueAsBool(response)) strcpy(buf, "true");
                    else strcpy(buf, "false");
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeInt16: {
                    snprintf(buf, PAYLOAD_SIZE, "%i", protocol->getValueAsInt16(response));
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeTimestamp:
                case RSCP::eTypeInt32: {
                    snprintf(buf, PAYLOAD_SIZE, "%i", protocol->getValueAsInt32(response));
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeUInt16: {
                    if (it->bit_to_bool) {
                        if (protocol->getValueAsUInt16(response) & it->bit_to_bool) strcpy(buf, "true");
                        else strcpy(buf, "false");
                    } else {
                        snprintf(buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUInt16(response));
                    }
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeUInt32: {
                    if (it->bit_to_bool) {
                        if (protocol->getValueAsUInt32(response) & it->bit_to_bool) strcpy(buf, "true");
                        else strcpy(buf, "false");
                    } else {
                        snprintf(buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUInt32(response));
                    }
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeUInt64: {
                    snprintf(buf, PAYLOAD_SIZE, "%lu", protocol->getValueAsUInt64(response));
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeChar8: {
                    snprintf(buf, PAYLOAD_SIZE, "%i", protocol->getValueAsChar8(response));
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeUChar8: {
                    if (it->bit_to_bool) {
                        if (protocol->getValueAsUChar8(response) & it->bit_to_bool) strcpy(buf, "true");
                        else strcpy(buf, "false");
                    } else {
                        snprintf(buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUChar8(response));
                    }
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeFloat32: {
                    switch (it->format) {
                        case F_FLOAT_0 : {
                            snprintf(buf, PAYLOAD_SIZE, "%.0f", protocol->getValueAsFloat32(response) / it->divisor);
                            break;
                        }
                        case F_FLOAT_1 : {
                            snprintf(buf, PAYLOAD_SIZE, "%0.1f", protocol->getValueAsFloat32(response) / it->divisor);
                            break;
                        }
                        case F_FLOAT_2 : {
                            snprintf(buf, PAYLOAD_SIZE, "%0.2f", protocol->getValueAsFloat32(response) / it->divisor);
                            break;
                        }
                    }
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeDouble64: {
                    snprintf(buf, PAYLOAD_SIZE, "%.0lf", protocol->getValueAsDouble64(response) / it->divisor);
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                    }
                    break;
                }
                case RSCP::eTypeString: {
                    snprintf(buf, PAYLOAD_SIZE, "%s", protocol->getValueAsString(response).c_str());
                    if (strcmp(it->payload, buf)) {
                        strcpy(it->payload, buf);
                        it->changed = true;
                        it->influx = false;
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            rc = response->dataType;
            if ((container == TAG_DB_HISTORY_DATA_DAY) && (index == 0) && (it->changed)) {
                time_t rawtime; 
                time(&rawtime);
                struct tm *l = localtime(&rawtime);
                handleImmediately(protocol, response, TAG_DB_HISTORY_DATA_DAY, l->tm_year + 1900, l->tm_mon + 1, l->tm_mday);
            }
        }
    }
    return(rc);
}

int getIntegerValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, int index) {
    int value = 0;
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
            value = atoi(it->payload);
            break;
        }
    }
    return(value);
}

void socLimiter(std::vector<RSCP_MQTT::cache_t> & c, RscpProtocol *protocol, SRscpValue *rootContainer, bool day_switch) {
    static int charge_locked = 0;
    static int discharge_locked = 0;
    int solar_power = getIntegerValue(c, 0, TAG_EMS_POWER_PV, 0);
    int battery_soc = getIntegerValue(c, 0, TAG_EMS_BAT_SOC, 0);
    int home_power = getIntegerValue(c, 0, TAG_EMS_POWER_HOME, 0);
    int limit_charge_soc = getIntegerValue(c, 0, 0, 1);
    int limit_discharge_soc = getIntegerValue(c, 0, 0, 2);
    int limit_discharge_by_home_power = getIntegerValue(c, 0, 0, 5);

    // reset for the next day if durable is false
    if (day_switch) {
        if (!getIntegerValue(c, 0, 0, 3)) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, 1); // reset charge soc limit
        if (!getIntegerValue(c, 0, 0, 4)) {
            storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, 2); // reset discharge soc limit
            storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, 5); // reset discharge by_home_power limit
        }
    }

    // control charge limit
    if (!day_switch && limit_charge_soc && (solar_power >= PV_SOLAR_MIN) && (battery_soc >= limit_charge_soc) && (battery_soc != 100) && !charge_locked) {
        charge_locked = 1;
        handleSetIdlePeriod(protocol, rootContainer, (char *)CHARGE_LOCK_TRUE);
    } else if ((day_switch || !solar_power || (battery_soc < limit_charge_soc) || (battery_soc == 100) || !limit_charge_soc) && charge_locked) {
        charge_locked = 0;
        handleSetIdlePeriod(protocol, rootContainer, (char *)CHARGE_LOCK_FALSE);
    }

    // control discharge limit
    if ((!day_switch && limit_discharge_soc && (battery_soc <= limit_discharge_soc) && (battery_soc != 0) && !discharge_locked)
      || (!day_switch && limit_discharge_by_home_power && (home_power >= limit_discharge_by_home_power) && !discharge_locked)) {
        discharge_locked = 1;
        handleSetIdlePeriod(protocol, rootContainer, (char *)DISCHARGE_LOCK_TRUE);
    } else if (discharge_locked && (day_switch || (!limit_discharge_soc && !limit_discharge_by_home_power) || (limit_discharge_soc && (battery_soc > limit_discharge_soc)) || (battery_soc == 0) || (limit_discharge_by_home_power && (home_power < (limit_discharge_by_home_power * 9 / 10))))) {
        discharge_locked = 0;
        handleSetIdlePeriod(protocol, rootContainer, (char *)DISCHARGE_LOCK_FALSE);
    }

    return;
}

void classifyValues(std::vector<RSCP_MQTT::cache_t> & c) {
    int battery_power = getIntegerValue(c, 0, TAG_EMS_POWER_BAT, 0);
    int battery_soc = getIntegerValue(c, 0, TAG_EMS_BAT_SOC, 0);
    int grid_power = getIntegerValue(c, 0, TAG_EMS_POWER_GRID, 0);

    if (battery_power < 0) storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"DISCHARGING", 6);
    else if ((battery_power == 0) && (!battery_soc)) storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"EMPTY", 6);
    else if ((battery_power == 0) && (battery_soc == 100)) storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"FULL", 6);
    else if (battery_power == 0) storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"PENDING", 6);
    else storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"CHARGING", 6);

    if (grid_power <= 0) storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"IN", 7);
    else storeStringValue(RSCP_MQTT::RscpMqttCache, 0, 0, (char *)"OUT", 7);

    return;
}

int createRequest(SRscpFrameBuffer * frameBuffer) {
    RscpProtocol protocol;
    SRscpValue rootValue;

    // The root container is create with the TAG ID 0 which is not used by any device.
    protocol.createContainerValue(&rootValue, 0);

    SRscpTimestamp start, interval, span;

    char buffer[64];
    bool day_switch_phase = false;
    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);
    int day_iteration;

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", l);

    if ((l->tm_hour == 23) && (l->tm_min == 59)) day_switch_phase = true;

    l->tm_sec = 0;
    l->tm_min = 0;
    l->tm_hour = 0;
    l->tm_isdst = -1;

    if (curr_year < l->tm_year + 1900) {
        addTemplTopics(TAG_DB_HISTORY_DATA_YEAR, 1, NULL, curr_year, l->tm_year + 1900, 0);
        curr_year = l->tm_year + 1900;
    }

    //---------------------------------------------------------------------------------------------------------
    // Create a request frame
    //---------------------------------------------------------------------------------------------------------
    if (iAuthenticated == 0) {
        //logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Request authentication (%li)\n", rawtime);
        // authentication request
        SRscpValue authenContainer;
        protocol.createContainerValue(&authenContainer, TAG_RSCP_REQ_AUTHENTICATION);
        protocol.appendValue(&authenContainer, TAG_RSCP_AUTHENTICATION_USER, cfg.e3dc_user);
        protocol.appendValue(&authenContainer, TAG_RSCP_AUTHENTICATION_PASSWORD, cfg.e3dc_password);
        // append sub-container to root container
        protocol.appendValue(&rootValue, authenContainer);
        // free memory of sub-container as it is now copied to rootValue
        protocol.destroyValueData(authenContainer);
    } else {
        //if (!cfg.daemon) printf("\nRequest cyclic data at %s (%li)\n", buffer, rawtime);
        //if (!cfg.daemon && e3dc_ts) printf("Difference to E3DC time is %li hour(s)\n", e3dc_diff);
        // request power data information
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_PV);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_BAT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_HOME);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_GRID);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_ADD);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_COUPLING_MODE);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_MODE);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_STATUS);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_BALANCED_PHASES);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_BAT_SOC);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_STORED_ERRORS);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_USED_CHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_BAT_CHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_DCDC_CHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_USER_CHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_USED_DISCHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_BAT_DISCHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_DCDC_DISCHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_USER_DISCHARGE_LIMIT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_REMAINING_BAT_CHARGE_POWER);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_REMAINING_BAT_DISCHARGE_POWER);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_EMERGENCY_POWER_STATUS);

        // Wallbox
        if (cfg.wallbox) {
            protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_WB_ALL);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_WB_SOLAR);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_BATTERY_TO_CAR_MODE);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_BATTERY_BEFORE_CAR_MODE);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_GET_WB_DISCHARGE_BAT_UNTIL);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_GET_WALLBOX_ENFORCE_POWER_ASSIGNMENT);
        }

        if (!e3dc_ts) {
            protocol.appendValue(&rootValue, TAG_INFO_REQ_SERIAL_NUMBER);
            protocol.appendValue(&rootValue, TAG_INFO_REQ_PRODUCTION_DATE);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_INSTALLED_PEAK_POWER);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_DERATE_AT_PERCENT_VALUE);
            protocol.appendValue(&rootValue, TAG_EMS_REQ_DERATE_AT_POWER_VALUE);
            protocol.appendValue(&rootValue, TAG_PVI_REQ_INVERTER_COUNT);
            protocol.appendValue(&rootValue, TAG_BAT_REQ_AVAILABLE_BATTERIES);
        }
        protocol.appendValue(&rootValue, TAG_INFO_REQ_SW_RELEASE);

        // request e3dc timestamp
        protocol.appendValue(&rootValue, TAG_INFO_REQ_TIME);
        protocol.appendValue(&rootValue, TAG_INFO_REQ_TIME_ZONE);
        //protocol.appendValue(&rootValue, TAG_INFO_REQ_UTC_TIME);

        // request idle_periods
        if (period_trigger) protocol.appendValue(&rootValue, TAG_EMS_REQ_GET_IDLE_PERIODS);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_IDLE_PERIOD_CHANGE_MARKER);

        // request battery information
        SRscpValue batteryContainer;
        for (uint8_t i = 0; i < cfg.battery_strings; i++) {
            protocol.createContainerValue(&batteryContainer, TAG_BAT_REQ_DATA);
            protocol.appendValue(&batteryContainer, TAG_BAT_INDEX, i);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_ASOC);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_RSOC);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_MODULE_VOLTAGE);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_CURRENT);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_CHARGE_CYCLES);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_STATUS_CODE);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_ERROR_CODE);
            if (!e3dc_ts) protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DEVICE_NAME);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_TRAINING_MODE);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_MAX_DCB_CELL_TEMPERATURE);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_MIN_DCB_CELL_TEMPERATURE);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_USABLE_CAPACITY);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_USABLE_REMAINING_CAPACITY);
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DESIGN_CAPACITY);
            if (!cfg.bat_dcb_count[i]) {
                protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DCB_COUNT);
            }
            if (cfg.dcb_requests) {
                for (uint8_t j = 0; j < cfg.bat_dcb_count[i]; j++) {
                    protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DCB_INFO, j);
                }
            }
            if (!e3dc_ts) {
                SRscpValue BattSpecContainer;
                protocol.createContainerValue(&BattSpecContainer, TAG_BAT_REQ_SPECIFICATION);
                protocol.appendValue(&batteryContainer, BattSpecContainer);
                protocol.destroyValueData(BattSpecContainer);
            }
            protocol.appendValue(&rootValue, batteryContainer);
            protocol.destroyValueData(batteryContainer);
        }

        // request EMS information
        SRscpValue PowerContainer;
        protocol.createContainerValue(&PowerContainer, TAG_EMS_REQ_GET_POWER_SETTINGS);
        protocol.appendValue(&rootValue, PowerContainer);
        protocol.destroyValueData(PowerContainer);

        // request PM information
        if (cfg.pm_requests) {
            SRscpValue PMContainer;
            protocol.createContainerValue(&PMContainer, TAG_PM_REQ_DATA);
            if (!cfg.pm_extern)
                protocol.appendValue(&PMContainer, TAG_PM_INDEX, (uint8_t)0);
            else
                protocol.appendValue(&PMContainer, TAG_PM_INDEX, (uint8_t)6);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_POWER_L1);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_POWER_L2);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_POWER_L3);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_VOLTAGE_L1);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_VOLTAGE_L2);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_VOLTAGE_L3);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_ENERGY_L1);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_ENERGY_L2);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_ENERGY_L3);
            protocol.appendValue(&PMContainer, TAG_PM_REQ_ACTIVE_PHASES);
            protocol.appendValue(&rootValue, PMContainer);
            protocol.destroyValueData(PMContainer);
        }

        // request PVI information
        if (cfg.pvi_requests) {
            SRscpValue PVIContainer;
            protocol.createContainerValue(&PVIContainer, TAG_PVI_REQ_DATA);
            protocol.appendValue(&PVIContainer, TAG_PVI_INDEX, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_USED_STRING_COUNT);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_ON_GRID);
            if (!existsNotSupportedTag(TAG_PVI_DATA, TAG_PVI_REQ_COS_PHI)) protocol.appendValue(&PVIContainer, TAG_PVI_REQ_COS_PHI);
            if (!existsNotSupportedTag(TAG_PVI_DATA, TAG_PVI_REQ_VOLTAGE_MONITORING)) protocol.appendValue(&PVIContainer, TAG_PVI_REQ_VOLTAGE_MONITORING);
            if (!existsNotSupportedTag(TAG_PVI_DATA, TAG_PVI_REQ_FREQUENCY_UNDER_OVER)) protocol.appendValue(&PVIContainer, TAG_PVI_REQ_FREQUENCY_UNDER_OVER);
            for (uint8_t i = 0; i < cfg.pvi_tracker; i++) {
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_POWER, i);
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_VOLTAGE, i);
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_CURRENT, i);
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_STRING_ENERGY_ALL, i);
            }
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_STRING_ENERGY_ALL, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_POWER, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_POWER, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_POWER, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_VOLTAGE, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_VOLTAGE, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_VOLTAGE, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_CURRENT, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_CURRENT, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_CURRENT, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_FREQUENCY, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_APPARENTPOWER, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_APPARENTPOWER, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_APPARENTPOWER, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_REACTIVEPOWER, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_REACTIVEPOWER, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_REACTIVEPOWER, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_ALL, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_ALL, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_ALL, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_MAX_APPARENTPOWER, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_MAX_APPARENTPOWER, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_MAX_APPARENTPOWER, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_DAY, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_DAY, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_DAY, (uint8_t)2);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_GRID_CONSUMPTION, (uint8_t)0);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_GRID_CONSUMPTION, (uint8_t)1);
            protocol.appendValue(&PVIContainer, TAG_PVI_REQ_AC_ENERGY_GRID_CONSUMPTION, (uint8_t)2);
            if (!cfg.pvi_temp_count) {
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_TEMPERATURE_COUNT);
            }
            for (uint8_t i = 0; i < cfg.pvi_temp_count; i++) {
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_TEMPERATURE, i);
            }
            protocol.appendValue(&rootValue, PVIContainer);
            protocol.destroyValueData(PVIContainer);
        }

        // request EP information
        protocol.appendValue(&rootValue, TAG_SE_REQ_EP_RESERVE);

        // Wallbox
        if (cfg.wallbox) {
            SRscpValue WBContainer;
            protocol.createContainerValue(&WBContainer, TAG_WB_REQ_DATA) ;
            protocol.appendValue(&WBContainer, TAG_WB_INDEX, 0); // first wallbox
            protocol.appendValue(&WBContainer, TAG_WB_REQ_DEVICE_STATE);
            protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_ACTIVE_PHASES);
            protocol.appendValue(&WBContainer, TAG_WB_REQ_EXTERN_DATA_ALG);
            protocol.appendValue(&WBContainer, TAG_WB_REQ_NUMBER_PHASES);
            protocol.appendValue(&WBContainer, TAG_WB_REQ_KEY_STATE);
            protocol.appendValue(&rootValue, WBContainer);
            protocol.destroyValueData(WBContainer);
        }

        // request db history information
        if (e3dc_ts) {
            SRscpValue dbContainer;
            start.nanoseconds = 0;
            interval.nanoseconds = 0;
            span.nanoseconds = 0;
            // TODAY
            start.seconds = (e3dc_diff * 3600) + mktime(l) - 1;
            interval.seconds = 24 * 3600;
            span.seconds = (24 * 3600) - 1;
            gmt_diff = 3600 * (e3dc_diff - l->tm_isdst);
            protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_DAY);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_START, start);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
            protocol.appendValue(&rootValue, dbContainer);
            protocol.destroyValueData(dbContainer);
            // YESTERDAY
            start.seconds = (e3dc_diff * 3600) + mktime(l) - (24 * 3600) - 1;
            interval.seconds = 24 * 3600;
            span.seconds = (24 * 3600) - 1;
            protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_DAY);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_START, start);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
            protocol.appendValue(&rootValue, dbContainer);
            protocol.destroyValueData(dbContainer);
            // WEEK
            start.seconds = (e3dc_diff * 3600) + mktime(l) - (((l->tm_wday?l->tm_wday:7) - 1) * 24 * 3600) - 1;
            interval.seconds = 7 * 24 * 3600;
            span.seconds = (7 * 24 * 3600) - 1;
            protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_WEEK);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_START, start);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
            protocol.appendValue(&rootValue, dbContainer);
            protocol.destroyValueData(dbContainer);
            // MONTH
            l->tm_mday = 1;
            start.seconds = (e3dc_diff * 3600) + mktime(l) - 1;
            interval.seconds = 31 * 24 * 3600;
            span.seconds = (31 * 24 * 3600) - 1;
            protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_MONTH);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_START, start);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
            protocol.appendValue(&rootValue, dbContainer);
            protocol.destroyValueData(dbContainer);
            // YEAR
            for (year = curr_year; year >= cfg.history_start_year; year--) {
                l->tm_hour = 0;
                l->tm_min = 0;
                l->tm_mday = 1;
                l->tm_mon = 0;
                l->tm_year = year - 1900;
                l->tm_isdst = 0;
                start.seconds = timegm(l) - gmt_diff;
                span.seconds = (((!(year%4) && ((year%100) || !(year%400)))?366:365) * 24 * 3600);
                interval.seconds = span.seconds;
                protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_YEAR);
                protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_START, start);
                protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
                protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
                protocol.appendValue(&rootValue, dbContainer);
                protocol.destroyValueData(dbContainer);
            }
        }
        mtx.lock();
        day_iteration = 0;
        while ((day_iteration++ < MAX_DAYS_PER_ITERATION) && (!RSCP_MQTT::requestQ.empty())) {
            if (!RSCP_MQTT::requestQ.empty()) {
                RSCP_MQTT::date_t x;

                x = RSCP_MQTT::requestQ.front();
                RSCP_MQTT::requestQ.pop();
                leap_day = ((x.month == 2) && (!(x.year%4) && ((x.year%100) || !(x.year%400))))?1:0;
                if ((x.year > cfg.history_start_year) && (x.year <= curr_year) && (x.month > 0) && (x.month <= 12) && (x.day > 0) && (x.day <= (RSCP_MQTT::nr_of_days[x.month - 1] + leap_day))) {
                    RSCP_MQTT::paramQ.push({x.day, x.month, x.year});
                    SRscpValue dbContainer;
                    l->tm_mday = x.day;
                    l->tm_mon = x.month - 1;
                    l->tm_year = x.year - 1900;
                    start.seconds = timegm(l) - gmt_diff - 15 * 60;
                    interval.seconds = 24 * 3600;
                    span.seconds = 24 * 3600;
                    protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_DAY);
                    protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_UTC_TIME_START, start);
                    protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
                    protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
                    protocol.appendValue(&rootValue, dbContainer);
                    protocol.destroyValueData(dbContainer);
                }
            }
        }
        // handle incoming MQTT requests
        if (mqttRcvd || cfg.auto_refresh) {
            mqttRcvd = false;
            uint32_t container = 0;
            SRscpValue ReqContainer;

            for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = RSCP_MQTT::RscpMqttReceiveCache.begin(); it != RSCP_MQTT::RscpMqttReceiveCache.end(); ++it) {
                if ((it->done == false) || (it->refresh_count > 0)) {
                    if (it->refresh_count > 0) it->refresh_count = it->refresh_count - 1;
                    if (!it->container && !it->tag) { //system call
                        if (!strcmp(it->topic, "set/log")) logCache(RSCP_MQTT::RscpMqttCache, cfg.logfile, buffer);
                        if (!strcmp(it->topic, "set/health")) logHealth(cfg.logfile);
                        if (!strcmp(it->topic, "set/force")) refreshCache(RSCP_MQTT::RscpMqttCache, it->payload);
                        if ((!strcmp(it->topic, "set/interval")) && (atoi(it->payload) >= DEFAULT_INTERVAL_MIN) && (atoi(it->payload) <= DEFAULT_INTERVAL_MAX)) cfg.interval = atoi(it->payload);
                        if ((!strcmp(it->topic, "set/power_mode")) && cfg.auto_refresh) handleSetPower(RSCP_MQTT::RscpMqttReceiveCache, TAG_EMS_REQ_SET_POWER, it->payload);
                        if (!strcmp(it->topic, "set/limit/charge/soc")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), 1);
                        if (!strcmp(it->topic, "set/limit/discharge/soc")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), 2);
                        if (!strcmp(it->topic, "set/limit/charge/durable")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), 3);
                        if (!strcmp(it->topic, "set/limit/discharge/durable")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), 4);
                        if (!strcmp(it->topic, "set/limit/discharge/by_home_power")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), 5);
                        if (!strcmp(it->topic, "set/idle_period")) handleSetIdlePeriod(&protocol, &rootValue, it->payload);
                        if (!strcmp(it->topic, "set/requests/pm")) {
                            if (!strcmp(it->payload, "true")) cfg.pm_requests = true; else cfg.pm_requests = false;
                        }
                        if (!strcmp(it->topic, "set/requests/pvi")) {
                            if (!strcmp(it->payload, "true")) cfg.pvi_requests = true; else cfg.pvi_requests = false;
                        }
                        if (!strcmp(it->topic, "set/requests/dcb")) {
                            if (!strcmp(it->payload, "true")) cfg.dcb_requests = true; else cfg.dcb_requests = false;
                        }
                        if (!strcmp(it->topic, "set/soc_limiter")) {
                            if (!strcmp(it->payload, "true")) cfg.soc_limiter = true; else cfg.soc_limiter = false;
                        }
                        it->done = true;
                        continue;
                    }
                    if (container && (it->container != container)) {
                        protocol.appendValue(&rootValue, ReqContainer);
                        protocol.destroyValueData(ReqContainer);
                    }
                    if (it->container && (it->container != container)) {
                        protocol.createContainerValue(&ReqContainer, it->container);
                        if (it->container == TAG_SE_REQ_SET_EP_RESERVE) protocol.appendValue(&ReqContainer, TAG_SE_PARAM_INDEX, 0);
                        if (it->container == TAG_WB_REQ_DATA) protocol.appendValue(&ReqContainer, TAG_WB_INDEX, 0); // first wallbox
                        container = it->container;
                    }

                    // Wallbox
                    if (cfg.wallbox && (container == TAG_WB_REQ_DATA) && (it->tag == TAG_WB_EXTERN_DATA)) {
                        uint8_t wallboxExt[6];
                        for (size_t i = 0; i < sizeof(wallboxExt); ++i) wallboxExt[i] = 0;
                        if (sscanf(it->payload, "solar:%d", (int*)&wallboxExt[1]) == 1) {
                            wallboxExt[0] = 1;
                            //logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Wallbox: solar mode.\n");
                        } else if (sscanf(it->payload, "mix:%d", (int*)&wallboxExt[1]) == 1) {
                            wallboxExt[0] = 2;
                            //logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Wallbox: mix mode.\n");
                        } else if (!strcmp(it->payload, "stop")) {
                            wallboxExt[0] = 1;
                            wallboxExt[4] = 1;
                            //logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Wallbox: Stop charging.\n");
                        } else {
                            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: wallbox format incorrect. >%<\n", it->payload);
                        }
                        if (wallboxExt[0] > 0) {
                            if (wallboxExt[1] < 6) wallboxExt[1] = 6; // min current 6A
                            else if (wallboxExt[1] > 32) wallboxExt[1] = 32; // max current 32A
                            SRscpValue WBExtContainer;
                            protocol.createContainerValue(&WBExtContainer, TAG_WB_REQ_SET_EXTERN);
                            protocol.appendValue(&WBExtContainer, TAG_WB_EXTERN_DATA_LEN, sizeof(wallboxExt));
                            protocol.appendValue(&WBExtContainer, TAG_WB_EXTERN_DATA, wallboxExt, sizeof(wallboxExt));
                            protocol.appendValue(&ReqContainer, WBExtContainer);
                            protocol.destroyValueData(WBExtContainer);
                            //logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Wallbox: container created. Current: %d A.\n", wallboxExt[1]);
                        }
                    }

                    switch (it->type) {
                        case RSCP::eTypeBool: {
                            bool value = false;
                            if (!strcmp(it->payload, "true")) value = true;
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, value);
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, value);
                            }
                            break;
                        }
                        case RSCP::eTypeChar8: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (int8_t) atoi(it->payload));
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, (int8_t) atoi(it->payload));
                            }
                            break;
                        }
                        case RSCP::eTypeUChar8: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (uint8_t) atoi(it->payload));
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, (uint8_t) atoi(it->payload));
                            }
                            break;
                        }
                        case RSCP::eTypeInt32: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (int32_t) atoi(it->payload));
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, (int32_t) atoi(it->payload));
                            }
                            break;
                        }
                        case RSCP::eTypeUInt32: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (uint32_t) atoi(it->payload));
                            } else { 
                                protocol.appendValue(&ReqContainer, it->tag, (uint32_t) atoi(it->payload));
                            }
                            break;
                        }
                        case RSCP::eTypeFloat32: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (float) atof(it->payload));
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, (float) atof(it->payload));
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    it->done = true;
                }
            }
            if (container) {
                protocol.appendValue(&rootValue, ReqContainer);
                protocol.destroyValueData(ReqContainer);
            }
        }
        mtx.unlock();

        if (cfg.soc_limiter) socLimiter(RSCP_MQTT::RscpMqttCache, &protocol, &rootValue, day_switch_phase);
    }

    // create buffer frame to send data to the S10
    protocol.createFrameAsBuffer(frameBuffer, rootValue.data, rootValue.length, true); // true to calculate CRC on for transfer
    // the root value object should be destroyed after the data is copied into the frameBuffer and is not needed anymore
    protocol.destroyValueData(rootValue);

    return(0);
}

int handleResponseValue(RscpProtocol *protocol, SRscpValue *response) {
    RSCP_MQTT::date_t x;

    // check if any of the response has the error flag set and react accordingly
    if (response->dataType == RSCP::eTypeError) {
        // handle error for example access denied errors
        uint32_t uiErrorCode = protocol->getValueAsUInt32(response);
        if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Container/Tag 0x%08X received error code %u.\n", response->tag, uiErrorCode);
        if ((response->tag == TAG_DB_HISTORY_DATA_YEAR) && (cfg.history_start_year < curr_year) && (year < curr_year)) {
            cfg.history_start_year++;
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"History start year has been corrected >%d<\n", cfg.history_start_year);
        } else if ((response->tag == TAG_BAT_DATA) && (uiErrorCode == 6) && (cfg.battery_strings > 1)) { // Battery string not available
            cfg.battery_strings = cfg.battery_strings - 1;
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Number of battery strings has been corrected >%d<\n", cfg.battery_strings);
        } else if (uiErrorCode == 6) pushNotSupportedTag(response->tag, 0);
        return(-1);
    }

    // check the SRscpValue TAG to detect which response it is
    switch (response->tag) {
    case TAG_RSCP_AUTHENTICATION: {
        // It is possible to check the response->dataType value to detect correct data type
        // and call the correct function. If data type is known,
        // the correct function can be called directly like in this case.
        uint8_t ucAccessLevel = protocol->getValueAsUChar8(response);
        if (ucAccessLevel > 0)
            iAuthenticated = 1;
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"RSCP authentication level %i\n", ucAccessLevel);
        break;
    }
    case TAG_INFO_TIME: {
        time_t now;
        time(&now);
        e3dc_ts = protocol->getValueAsInt32(response);
        e3dc_diff = (e3dc_ts - now + 60) / 3600;
        //storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, response, 0, 0);
        break;
    }
    case TAG_EMS_IDLE_PERIOD_CHANGE_MARKER: {
        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, response, 0, 0);
        if (period_change_nr != protocol->getValueAsUChar8(response)) {
            period_change_nr = protocol->getValueAsUChar8(response);
            period_trigger = true;
        }
        break;
    }
    case TAG_EMS_SET_POWER_SETTINGS:
    case TAG_EMS_GET_POWER_SETTINGS:
    case TAG_SE_EP_RESERVE:
    case TAG_PM_DATA:
    case TAG_BAT_DATA: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Container 0x%08X Tag 0x%08X received error code %u.\n", response->tag, containerData[i].tag, uiErrorCode);
                if ((response->tag == TAG_PM_DATA) && (uiErrorCode == 6)) { // Power Meter not found (intern/extern)
                    if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Power Meter not found. Switch intern/extern.\n");
                    cfg.pm_extern = !cfg.pm_extern;
                } else if (uiErrorCode == 6) pushNotSupportedTag(response->tag, containerData[i].tag);
            } else {
                switch (containerData[i].tag) {
                    case TAG_BAT_DCB_INFO: {
                        int dcb_nr;
                        std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                        for (size_t j = 0; j < container.size(); j++) {
                            if (container[j].tag == TAG_BAT_DCB_INDEX) {
                                dcb_nr = protocol->getValueAsUInt16(&container[j]) + cfg.bat_dcb_start[battery_nr];
                            } else {
                                storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(container[j]), containerData[i].tag, dcb_nr);
                            }
                        }
                        protocol->destroyValueData(container);
                        break;
                    }
                    case TAG_BAT_DCB_COUNT: {
                        int dcb_nr;
                        cfg.bat_dcb_count[battery_nr] = protocol->getValueAsUChar8(&containerData[i]);
                        cfg.bat_dcb_start[battery_nr] = battery_nr?cfg.bat_dcb_count[battery_nr - 1]:0;
                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                        addTemplTopics(TAG_BAT_DCB_INFO, 1, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1);
                        if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"TAG_BAT_DCB_COUNT battery_nr = %d bat_dcb_count = %d bat_dcb_start = %d\n", battery_nr, cfg.bat_dcb_count[battery_nr], cfg.bat_dcb_start[battery_nr]);
                        break;
                    }
                    case TAG_BAT_SPECIFICATION: {
                        std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                        for (size_t j = 0; j < container.size(); j++) {
                            storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(container[j]), containerData[i].tag, battery_nr);
                        }
                        protocol->destroyValueData(container);
                        break;
                    }
                    default: {
                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, (response->tag == TAG_BAT_DATA)?battery_nr:0);
                    }
                }
            }
        }
        if (response->tag == TAG_BAT_DATA) battery_nr++;
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_WB_DATA: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Container 0x%08X Tag 0x%08X received error code %u.\n", response->tag, containerData[i].tag, uiErrorCode);
                if (uiErrorCode == 6) { // Wallbox not available
                    if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Wallbox not available.\n");
                    //cfg.wallbox = false;
                    pushNotSupportedTag(response->tag, containerData[i].tag);
                }
            } else if (containerData[i].tag == TAG_WB_EXTERN_DATA_ALG) {
                std::vector<SRscpValue> wallboxContent = protocol->getValueAsContainer(&containerData[i]);
                for (size_t j = 0; j < wallboxContent.size(); ++j) {
                    if (wallboxContent[j].dataType == RSCP::eTypeError) {
                         uint32_t uiErrorCode = protocol->getValueAsUInt32(&wallboxContent[j]);
                         if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Tag 0x%08X received error code %u.\n", wallboxContent[j].tag, uiErrorCode);
                         if (uiErrorCode == 6) pushNotSupportedTag(response->tag, wallboxContent[i].tag);
                    } else {
                        switch (wallboxContent[j].tag) {
                            case TAG_WB_EXTERN_DATA: {
                                uint8_t wallboxExt[8];
                                memcpy(&wallboxExt, &wallboxContent[j].data[0], sizeof(wallboxExt));
                                for (size_t b = 0; b < sizeof(wallboxExt); ++b) {
                                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, wallboxExt[b], b);
                                }
                            }
                         }
                     }
                 }
            } else {
                storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
            }
        }
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_PVI_DATA: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Container 0x%08X Tag 0x%08X received error code %u.\n", response->tag, containerData[i].tag, uiErrorCode);
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, containerData[i].tag);
            } else {
            switch (containerData[i].tag) {
                case TAG_PVI_TEMPERATURE:
                case TAG_PVI_AC_VOLTAGE:
                case TAG_PVI_AC_CURRENT:
                case TAG_PVI_AC_POWER:
                case TAG_PVI_AC_APPARENTPOWER:
                case TAG_PVI_AC_REACTIVEPOWER:
                case TAG_PVI_AC_ENERGY_ALL:
                case TAG_PVI_AC_MAX_APPARENTPOWER:
                case TAG_PVI_AC_ENERGY_DAY:
                case TAG_PVI_AC_ENERGY_GRID_CONSUMPTION:
                case TAG_PVI_AC_FREQUENCY:
                case TAG_PVI_DC_STRING_ENERGY_ALL:
                case TAG_PVI_DC_VOLTAGE:
                case TAG_PVI_DC_CURRENT:
                case TAG_PVI_DC_POWER:
                case TAG_PVI_COS_PHI:
                case TAG_PVI_VOLTAGE_MONITORING:
                case TAG_PVI_FREQUENCY_UNDER_OVER: {
                    int tracker;
                    std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                    for (size_t j = 0; j < container.size(); j++) {
                        if (container[j].tag == TAG_PVI_INDEX) {
                            tracker = protocol->getValueAsUInt16(&container[j]);
                        }
                        else if ((container[j].tag == TAG_PVI_VALUE)) {
                            storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(container[j]), containerData[i].tag, tracker);
                        }
                    }
                    protocol->destroyValueData(container);
                    break;
                }
                case TAG_PVI_TEMPERATURE_COUNT: {
                    cfg.pvi_temp_count = protocol->getValueAsUChar8(&containerData[i]);
                    storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                    addTemplTopics(TAG_PVI_TEMPERATURE, 1, NULL, 0, cfg.pvi_temp_count, 1);
                    break;
                }
                case TAG_PVI_USED_STRING_COUNT: {
                    cfg.pvi_tracker = protocol->getValueAsUChar8(&containerData[i]);
                    addTemplTopics(TAG_PVI_DC_POWER, 1, NULL, 0, cfg.pvi_tracker, 1);
                    addTemplTopics(TAG_PVI_DC_VOLTAGE, 1, NULL, 0, cfg.pvi_tracker, 1);
                    addTemplTopics(TAG_PVI_DC_CURRENT, 1, NULL, 0, cfg.pvi_tracker, 1);
                    addTemplTopics(TAG_PVI_DC_STRING_ENERGY_ALL, 1, NULL, 0, cfg.pvi_tracker, 1);
                    storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                    break;
                }
                case TAG_PVI_ON_GRID: {
                    storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                    break;
                }
                default:
                    //if (!cfg.daemon) printf("Unknown PVI tag %08X\n", response->tag);
                    break;
                }
            }
        }
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_EMS_GET_IDLE_PERIODS:
    case TAG_EMS_STORED_ERRORS: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Container 0x%08X Tag 0x%08X received error code %u.\n", response->tag, containerData[i].tag, uiErrorCode);
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, containerData[i].tag);
            } else {
                switch (containerData[i].tag) {
                    case TAG_EMS_IDLE_PERIOD: {
                        uint8_t period, type, active, starthour, startminute, endhour, endminute;
                        period_trigger = false;
                        std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                        for (size_t j = 0; j < container.size(); j++) {
                            switch (container[j].tag) {
                                case TAG_EMS_IDLE_PERIOD_DAY: {
                                    period = protocol->getValueAsUChar8(&container[j]);
                                    break;
                                }
                                case TAG_EMS_IDLE_PERIOD_TYPE: {
                                    type = protocol->getValueAsUChar8(&container[j]);
                                    break;
                                }
                                case TAG_EMS_IDLE_PERIOD_ACTIVE: {
                                    active = protocol->getValueAsUChar8(&container[j]);
                                    break;
                                }
                                case TAG_EMS_IDLE_PERIOD_START: {
                                    std::vector<SRscpValue> start = protocol->getValueAsContainer(&container[j]);
                                    for (size_t k = 0; k < start.size(); k++) {
                                        switch (start[k].tag) {
                                            case TAG_EMS_IDLE_PERIOD_HOUR: {
                                                starthour = protocol->getValueAsUChar8(&start[k]);
                                                break;
                                            }
                                            case TAG_EMS_IDLE_PERIOD_MINUTE: {
                                                startminute = protocol->getValueAsUChar8(&start[k]);
                                                break;
                                            }
                                        }
                                    }
                                    protocol->destroyValueData(start);
                                    break;
                                }
                                case TAG_EMS_IDLE_PERIOD_END: {
                                    std::vector<SRscpValue> stop = protocol->getValueAsContainer(&container[j]);
                                    for (size_t k = 0; k < stop.size(); k++) {
                                        switch (stop[k].tag) {
                                            case TAG_EMS_IDLE_PERIOD_HOUR: {
                                                endhour = protocol->getValueAsUChar8(&stop[k]);
                                                break;
                                            }
                                            case TAG_EMS_IDLE_PERIOD_MINUTE: {
                                                endminute = protocol->getValueAsUChar8(&stop[k]);
                                                break;
                                            }
                                        }
                                    }
                                    protocol->destroyValueData(stop);
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        RSCP_MQTT::idle_period_t ip = {period_change_nr, type, period, starthour, startminute, endhour, endminute, (bool)active};
                        RSCP_MQTT::IdlePeriodCache.push_back(ip);
                        protocol->destroyValueData(container);
                        break;
                    }
                    case TAG_EMS_ERROR_CONTAINER: {
                        RSCP_MQTT::error_t error;
                        std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                        for (size_t j = 0; j < container.size(); j++) {
                            switch (container[j].tag) {
                                case TAG_EMS_ERROR_TYPE: {
                                    error.type = protocol->getValueAsUChar8(&container[j]);
                                }
                                case TAG_EMS_ERROR_SOURCE: {
                                    snprintf(error.source, SOURCE_SIZE, "%s", protocol->getValueAsString(&container[j]).c_str());
                                }
                                case TAG_EMS_ERROR_MESSAGE: {
                                    snprintf(error.message, PAYLOAD_SIZE, "%s", protocol->getValueAsString(&container[j]).c_str());
                                }
                                case TAG_EMS_ERROR_CODE: {
                                    error.code = protocol->getValueAsInt32(&container[j]);
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        RSCP_MQTT::ErrorCache.push_back(error);
                        protocol->destroyValueData(container);
                        break;
                    }
                }
            }
        }
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_DB_HISTORY_DATA_DAY: {
        if (day >= 2) {
            if (!RSCP_MQTT::paramQ.empty()) {
                x = RSCP_MQTT::paramQ.front();
                RSCP_MQTT::paramQ.pop();
            }
        }
    }
    case TAG_DB_HISTORY_DATA_WEEK:
    case TAG_DB_HISTORY_DATA_MONTH:
    case TAG_DB_HISTORY_DATA_YEAR: {
        std::vector<SRscpValue> historyData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < historyData.size(); ++i) {
            if (historyData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&historyData[i]);
                if (cfg.log_level) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Container 0x%08X Tag 0x%08X received error code %u.\n", response->tag, historyData[i].tag, uiErrorCode);
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, 0);
            } else {
                switch (historyData[i].tag) {
                case TAG_DB_SUM_CONTAINER: {
                    std::vector<SRscpValue> dbData = protocol->getValueAsContainer(&(historyData[i]));
                    for (size_t j = 0; j < dbData.size(); ++j) {
                        if (response->tag == TAG_DB_HISTORY_DATA_DAY) {
                            if (day >= 2) {
                                handleImmediately(protocol, &(dbData[j]), TAG_DB_HISTORY_DATA_DAY, x.year, x.month, x.day);
                            } else storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(dbData[j]), response->tag, day);
                        } else if (response->tag == TAG_DB_HISTORY_DATA_MONTH) {
                            storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(dbData[j]), response->tag, 0);
                        } else if (response->tag == TAG_DB_HISTORY_DATA_YEAR) {
                            if (year == curr_year) storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(dbData[j]), response->tag, 0);
                            else storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(dbData[j]), response->tag, year);
                        } else storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(dbData[j]), response->tag, 0);
                    }
                    if (response->tag == TAG_DB_HISTORY_DATA_DAY) day++;
                    else if (response->tag == TAG_DB_HISTORY_DATA_YEAR) year--;
                    protocol->destroyValueData(dbData);
                    break;
                }
                case TAG_DB_VALUE_CONTAINER: {
                    break;
                }
                default:
                    break;
                }
            }
        }
        protocol->destroyValueData(historyData);
        break;
    }
    default:
        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, response, 0, 0);
        break;
    }
    return(0);
}

static int processReceiveBuffer(const unsigned char * ucBuffer, int iLength){
    RscpProtocol protocol;
    SRscpFrame frame;

    int iResult = protocol.parseFrame(ucBuffer, iLength, &frame);

    if (iResult < 0) {
        // check if frame length error occured
        // in that case the full frame length was not received yet
        // and the receive function must get more data
        if (iResult == RSCP::ERR_INVALID_FRAME_LENGTH)
            return(0);
        // otherwise a not recoverable error occured and the connection can be closed
        else
            return(iResult);
    }

    // process each SRscpValue struct seperately
    day = 0;
    year = curr_year;
    battery_nr = 0;
    for (size_t i = 0; i < frame.data.size(); i++)
        handleResponseValue(&protocol, &frame.data[i]);

    // destroy frame data and free memory
    protocol.destroyFrameData(frame);

    // returned processed amount of bytes
    return(iResult);
}

static void receiveLoop(bool & bStopExecution){
    //--------------------------------------------------------------------------------------------------------------
    // RSCP Receive Frame Block Data
    //--------------------------------------------------------------------------------------------------------------
    // setup a static dynamic buffer which is dynamically expanded (re-allocated) on demand
    // the data inside this buffer is not released when this function is left
    static int iReceivedBytes = 0;
    static std::vector<uint8_t> vecDynamicBuffer;

    // check how many RSCP frames are received, must be at least 1
    // multiple frames can only occur in this example if one or more frames are received with a big time delay
    // this should usually not occur but handling this is shown in this example
    int iReceivedRscpFrames = 0;

    while (!bStopExecution && ((iReceivedBytes > 0) || iReceivedRscpFrames == 0)) {
        // check and expand buffer
        if ((vecDynamicBuffer.size() - iReceivedBytes) < 4096) {
            // check maximum size
            if (vecDynamicBuffer.size() > RSCP_MAX_FRAME_LENGTH) {
                // something went wrong and the size is more than possible by the RSCP protocol
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Maximum buffer size exceeded %zu\n", vecDynamicBuffer.size());
                bStopExecution = true;
                break;
            }
            // increase buffer size by 4096 bytes each time the remaining size is smaller than 4096
            vecDynamicBuffer.resize(vecDynamicBuffer.size() + 4096);
        }
        // receive data
        int iResult = SocketRecvData(iSocket, &vecDynamicBuffer[0] + iReceivedBytes, vecDynamicBuffer.size() - iReceivedBytes);
        if (iResult < 0) {
            // check errno for the error code to detect if this is a timeout or a socket error
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                // receive timed out -> continue with re-sending the initial block
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Response receive timeout (retry)\n");
                break;
            }
            // socket error -> check errno for failure code if needed
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Socket receive error. errno %i\n", errno);
            bStopExecution = true;
            break;
        } else if (iResult == 0) {
            // connection was closed regularly by peer
            // if this happens on startup each time the possible reason is
            // wrong AES password or wrong network subnet (adapt hosts.allow file required)
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Connection closed by peer\n");
            bStopExecution = true;
            break;
        }
        // increment amount of received bytes
        iReceivedBytes += iResult;

        // process all received frames
        while (!bStopExecution) {
            // round down to a multiple of AES_BLOCK_SIZE
            int iLength = ROUNDDOWN(iReceivedBytes, AES_BLOCK_SIZE);
            // if not even 32 bytes were received then the frame is still incomplete
            if (iLength == 0)
                break;
            // resize temporary decryption buffer
            std::vector<uint8_t> decryptionBuffer;
            decryptionBuffer.resize(iLength);
            // initialize encryption sequence IV value with value of previous block
            aesDecrypter.SetIV(ucDecryptionIV, AES_BLOCK_SIZE);
            // decrypt data from vecDynamicBuffer to temporary decryptionBuffer
            aesDecrypter.Decrypt(&vecDynamicBuffer[0], &decryptionBuffer[0], iLength / AES_BLOCK_SIZE);

            // data was received, check if we received all data
            int iProcessedBytes = processReceiveBuffer(&decryptionBuffer[0], iLength);
            if (iProcessedBytes < 0) {
                // an error occured;
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: parsing RSCP frame: %i\n", iProcessedBytes);
                // stop execution as the data received is not RSCP data
                bStopExecution = true;
                break;

            } else if (iProcessedBytes > 0) {
                // round up the processed bytes as iProcessedBytes does not include the zero padding bytes
                iProcessedBytes = ROUNDUP(iProcessedBytes, AES_BLOCK_SIZE);
                // store the IV value from encrypted buffer for next block decryption
                memcpy(ucDecryptionIV, &vecDynamicBuffer[0] + iProcessedBytes - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
                // move the encrypted data behind the current frame data (if any received) to the front
                memcpy(&vecDynamicBuffer[0], &vecDynamicBuffer[0] + iProcessedBytes, vecDynamicBuffer.size() - iProcessedBytes);
                // decrement the total received bytes by the amount of processed bytes
                iReceivedBytes -= iProcessedBytes;
                // increment a counter that a valid frame was received and
                // continue parsing process in case a 2nd valid frame is in the buffer as well
                iReceivedRscpFrames++;
            } else {
                // iProcessedBytes is 0
                // not enough data of the next frame received, go back to receive mode if iReceivedRscpFrames == 0
                // or transmit mode if iReceivedRscpFrames > 0
                break;
            }
        }
    }
}

static void mainLoop(void){
    RscpProtocol protocol;
    bool bStopExecution = false;
    int countdown = 3;
    int interval = 1;

    while (!bStopExecution) {
        //--------------------------------------------------------------------------------------------------------------
        // RSCP Transmit Frame Block Data
        //--------------------------------------------------------------------------------------------------------------
        SRscpFrameBuffer frameBuffer;
        memset(&frameBuffer, 0, sizeof(frameBuffer));

        // create an RSCP frame with requests to some example data
        createRequest(&frameBuffer);

        // check that frame data was created
        if (frameBuffer.dataLength > 0) {
            // resize temporary encryption buffer to a multiple of AES_BLOCK_SIZE
            std::vector<uint8_t> encryptionBuffer;
            encryptionBuffer.resize(ROUNDUP(frameBuffer.dataLength, AES_BLOCK_SIZE));
            // zero padding for data above the desired length
            memset(&encryptionBuffer[0] + frameBuffer.dataLength, 0, encryptionBuffer.size() - frameBuffer.dataLength);
            // copy desired data length
            memcpy(&encryptionBuffer[0], frameBuffer.data, frameBuffer.dataLength);
            // set continues encryption IV
            aesEncrypter.SetIV(ucEncryptionIV, AES_BLOCK_SIZE);
            // start encryption from encryptionBuffer to encryptionBuffer, blocks = encryptionBuffer.size() / AES_BLOCK_SIZE
            aesEncrypter.Encrypt(&encryptionBuffer[0], &encryptionBuffer[0], encryptionBuffer.size() / AES_BLOCK_SIZE);
            // save new IV for next encryption block
            memcpy(ucEncryptionIV, &encryptionBuffer[0] + encryptionBuffer.size() - AES_BLOCK_SIZE, AES_BLOCK_SIZE);

            // send data on socket
            int iResult = SocketSendData(iSocket, &encryptionBuffer[0], encryptionBuffer.size());
            if (iResult < 0) {
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Socket send error %i. errno %i\n", iResult, errno);
                bStopExecution = true;
            } else
                // go into receive loop and wait for response
                receiveLoop(bStopExecution);
        }
        // free frame buffer memory
        protocol.destroyFrameData(&frameBuffer);

        classifyValues(RSCP_MQTT::RscpMqttCache);

        // MQTT connection
        if (!mosq) {
            if (strcmp(cfg.mqtt_client_id, ""))
                mosq = mosquitto_new(cfg.mqtt_client_id, true, NULL);
            else
                mosq = mosquitto_new(NULL, true, NULL);
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Connecting to broker %s:%u\n", cfg.mqtt_host, cfg.mqtt_port);
            if (mosq) {
                mosquitto_threaded_set(mosq, true);
                mosquitto_connect_callback_set(mosq, (void (*)(mosquitto*, void*, int))mqttCallbackOnConnect);
                mosquitto_message_callback_set(mosq, (void (*)(mosquitto*, void*, const mosquitto_message*))mqttCallbackOnMessage);
                if (cfg.mqtt_auth && strcmp(cfg.mqtt_user, "") && strcmp(cfg.mqtt_password, "")) mosquitto_username_pw_set(mosq, cfg.mqtt_user, cfg.mqtt_password);
                if (!mosquitto_connect(mosq, cfg.mqtt_host, cfg.mqtt_port, 10)) {
                    std::thread th(mqttListener, mosq);
                    th.detach();
                    logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Success: MQTT broker connected.\n");
                } else {
                    logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Connection failed.\n");
                    mosquitto_destroy(mosq);
                    mosq = NULL;
                }
            }
        }

        // MQTT publish
        if (mosq) {
            if (handleMQTT(RSCP_MQTT::RscpMqttCache, cfg.mqtt_qos, cfg.mqtt_retain)) {
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"MQTT connection lost\n");
                mosquitto_disconnect(mosq);
                mosquitto_destroy(mosq);
                mosq = NULL;
            }
            if (handleMQTTIdlePeriods(RSCP_MQTT::IdlePeriodCache, cfg.mqtt_qos, false)) {
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"MQTT connection lost\n");
                mosquitto_disconnect(mosq);
                mosquitto_destroy(mosq);
                mosq = NULL;
            }
            if (handleMQTTErrorMessages(RSCP_MQTT::ErrorCache, cfg.mqtt_qos, false)) {
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"MQTT connection lost\n");
                mosquitto_disconnect(mosq);
                mosquitto_destroy(mosq);
                mosq = NULL;
            }
        }

        if (countdown >= 0) {
            countdown--;
            if (countdown == 0) {
                cleanupCache(RSCP_MQTT::RscpMqttCacheTempl);
                interval = cfg.interval;
            }
        }

        // main loop sleep / cycle time before next request
        wsleep(interval);
    }
}

int main(int argc, char *argv[]){
    char key[128], value[128], line[256];
    int i = 0;

    cfg.daemon = false;
    cfg.verbose = false;

    while (i < argc) {
        if (!strcmp(argv[i], "-d")) cfg.daemon = true;
        if (!strcmp(argv[i], "-v")) cfg.verbose = true;
        i++;
    }

    FILE *fp;
    fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        printf("Config file %s not found. Program terminated.\n", CONFIG_FILE);
        exit(EXIT_FAILURE);
    }

    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);
    curr_year = l->tm_year + 1900;

    strcpy(cfg.mqtt_host, "");
    cfg.mqtt_port = MQTT_PORT_DEFAULT;
    strcpy(cfg.mqtt_client_id, "");
    cfg.mqtt_auth = false;
    cfg.mqtt_qos = 0;
    cfg.mqtt_retain = false;
    cfg.interval = 1;
    cfg.log_level = 0;
    cfg.battery_strings = 1;
    cfg.pvi_requests = true;
    cfg.pvi_tracker = 0;
    cfg.pvi_temp_count = 0;
    for (uint8_t i = 0; i < MAX_DCB_COUNT; i++) {
        cfg.bat_dcb_count[i] = 0;
        cfg.bat_dcb_start[i] = 0;
    }
    cfg.pm_extern = false;
    cfg.pm_requests = true;
    cfg.dcb_requests = true;
    cfg.soc_limiter = false;
    cfg.wallbox = false;
    cfg.auto_refresh = false;
    strcpy(cfg.prefix, DEFAULT_PREFIX);
    cfg.history_start_year = curr_year - 1;
#ifdef INFLUXDB
    bool influx_reduced = false;
    strcpy(cfg.influxdb_host, "");
    cfg.influxdb_port = INFLUXDB_PORT_DEFAULT;
    cfg.influxdb_on = false;
    cfg.influxdb_auth = false;
    strcpy(cfg.influxdb_user, "");
    strcpy(cfg.influxdb_password, "");
#endif
    cfg.mqtt_pub = true;
    cfg.logfile = NULL;
    cfg.historyfile = NULL;

    uint32_t hotfix_tag = 0;

    RSCP_MQTT::topic_store_t store = { 0, "" };

    while (fgets(line, sizeof(line), fp)) {
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        if (sscanf(line, "%127[^ \t=]=%127[^\n]", key, value) == 2) {
            if (strcasecmp(key, "E3DC_IP") == 0)
                strcpy(cfg.e3dc_ip, value);
            else if (strcasecmp(key, "E3DC_PORT") == 0)
                cfg.e3dc_port = atoi(value);
            else if (strcasecmp(key, "E3DC_USER") == 0)
                strcpy(cfg.e3dc_user, value);
            else if (strcasecmp(key, "E3DC_PASSWORD") == 0)
                strcpy(cfg.e3dc_password, value);
            else if (strcasecmp(key, "E3DC_AES_PASSWORD") == 0)
                strcpy(cfg.aes_password, value);
            else if (strcasecmp(key, "MQTT_HOST") == 0)
                strcpy(cfg.mqtt_host, value);
            else if (strcasecmp(key, "MQTT_PORT") == 0)
                cfg.mqtt_port = atoi(value);
            else if (strcasecmp(key, "MQTT_USER") == 0)
                strcpy(cfg.mqtt_user, value);
            else if (strcasecmp(key, "MQTT_PASSWORD") == 0)
                strcpy(cfg.mqtt_password, value);
            else if (strcasecmp(key, "MQTT_CLIENT_ID") == 0) {
                strcpy(cfg.mqtt_client_id, value);
            } else if ((strcasecmp(key, "MQTT_AUTH") == 0) && (strcasecmp(value, "true") == 0))
                cfg.mqtt_auth = true;
            else if ((strcasecmp(key, "MQTT_RETAIN") == 0) && (strcasecmp(value, "true") == 0))
                cfg.mqtt_retain = true;
            else if (strcasecmp(key, "MQTT_QOS") == 0)
                cfg.mqtt_qos = atoi(value);
            else if (strcasecmp(key, "PREFIX") == 0)
                strncpy(cfg.prefix, value, 24);
            else if (strcasecmp(key, "HISTORY_START_YEAR") == 0)
                cfg.history_start_year = atoi(value);
#ifdef INFLUXDB
            else if (strcasecmp(key, "INFLUXDB_HOST") == 0)
                strcpy(cfg.influxdb_host, value);
            else if (strcasecmp(key, "INFLUXDB_PORT") == 0)
                cfg.influxdb_port = atoi(value);
            else if (strcasecmp(key, "INFLUXDB_VERSION") == 0)
                cfg.influxdb_version = atoi(value);
            else if (strcasecmp(key, "INFLUXDB_1_DB") == 0)
                strcpy(cfg.influxdb_db, value);
            else if (strcasecmp(key, "INFLUXDB_1_USER") == 0)
                strcpy(cfg.influxdb_user, value);
            else if (strcasecmp(key, "INFLUXDB_1_PASSWORD") == 0)
                strcpy(cfg.influxdb_password, value);
            else if ((strcasecmp(key, "INFLUXDB_1_AUTH") == 0) && (strcasecmp(value, "true") == 0))
                cfg.influxdb_auth = true;
            else if (strcasecmp(key, "INFLUXDB_2_ORGA") == 0)
                strcpy(cfg.influxdb_orga, value);
            else if (strcasecmp(key, "INFLUXDB_2_BUCKET") == 0)
                strcpy(cfg.influxdb_bucket, value);
            else if (strcasecmp(key, "INFLUXDB_2_TOKEN") == 0)
                strcpy(cfg.influxdb_token, value);
            else if (strcasecmp(key, "INFLUXDB_MEASUREMENT") == 0)
                strcpy(cfg.influxdb_measurement, value);
            else if ((strcasecmp(key, "ENABLE_INFLUXDB") == 0) && (strcasecmp(value, "true") == 0))
                cfg.influxdb_on = true;
#endif
            else if (strcasecmp(key, "LOGFILE") == 0) {
                cfg.logfile = (char *)malloc(sizeof(char) * strlen(value) + 1);
                strcpy(cfg.logfile, value);
            } else if (strcasecmp(key, "TOPIC_LOG_FILE") == 0) {
                cfg.historyfile = (char *)malloc(sizeof(char) * strlen(value) + 1);
                strcpy(cfg.historyfile, value);
            } else if (strcasecmp(key, "INTERVAL") == 0)
                cfg.interval = atoi(value);
            else if (strcasecmp(key, "LOG_LEVEL") == 0)
                cfg.log_level = atoi(value);
            else if ((strcasecmp(key, "PVI_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                cfg.pvi_requests = false;
            else if (strcasecmp(key, "BATTERY_STRINGS") == 0)
                cfg.battery_strings = atoi(value);
            else if ((strcasecmp(key, "PM_EXTERN") == 0) && (strcasecmp(value, "true") == 0))
                cfg.pm_extern = true;
            else if ((strcasecmp(key, "PM_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                cfg.pm_requests = false;
            else if ((strcasecmp(key, "DCB_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                cfg.dcb_requests = false;
            else if ((strcasecmp(key, "SOC_LIMITER") == 0) && (strcasecmp(value, "true") == 0))
                cfg.soc_limiter = true;
            else if ((strcasecmp(key, "WALLBOX") == 0) && (strcasecmp(value, "true") == 0))
                cfg.wallbox = true;
            else if (((strcasecmp(key, "DISABLE_MQTT_PUBLISH") == 0) || (strcasecmp(key, "DRYRUN") == 0)) && (strcasecmp(value, "true") == 0))
                cfg.mqtt_pub = false;
            else if ((strcasecmp(key, "AUTO_REFRESH") == 0) && (strcasecmp(value, "true") == 0))
                cfg.auto_refresh = true;
            else if (strcasecmp(key, "FORCE_PUB") == 0) {
                store.type = FORCED_TOPIC;
                strcpy(store.topic, value);
                RSCP_MQTT::TopicStore.push_back(store);
            }
            else if (strcasecmp(key, "TOPIC_LOG") == 0) {
                store.type = LOG_TOPIC;
                strcpy(store.topic, value);
                RSCP_MQTT::TopicStore.push_back(store);
            }
#ifdef INFLUXDB
            else if (strcasecmp(key, "INFLUXDB_TOPIC") == 0) {
                store.type = INFLUXDB_TOPIC;
                strcpy(store.topic, value);
                RSCP_MQTT::TopicStore.push_back(store);
                influx_reduced = true;
            }
#endif
            else if (strcasecmp(key, "HOTFIX_SET_TAG") == 0) {
                printf("HOTFIX_SET_TAG tag >0x%08X<\n", (uint32_t)atol(value));
                hotfix_tag = (uint32_t)atol(value);
                hotfixSetEntry(RSCP_MQTT::RscpMqttReceiveCache, hotfix_tag, 0, NULL);
            }
            else if (strcasecmp(key, "HOTFIX_TO_TYPE") == 0) {
                if (hotfix_tag) hotfixSetEntry(RSCP_MQTT::RscpMqttReceiveCache, hotfix_tag, atoi(value), NULL);
            }
            else if (strcasecmp(key, "HOTFIX_TO_REGEX") == 0) {
                if (hotfix_tag) hotfixSetEntry(RSCP_MQTT::RscpMqttReceiveCache, hotfix_tag, 0, value);
            }
        }
    }
    fclose(fp);

#ifdef INFLUXDB
    if (!influx_reduced) {
        store.type = INFLUXDB_TOPIC;
        strcpy(store.topic, ".*");
        RSCP_MQTT::TopicStore.push_back(store);
    }
#endif

    if (cfg.interval < DEFAULT_INTERVAL_MIN) cfg.interval = DEFAULT_INTERVAL_MIN;
    if (cfg.interval > DEFAULT_INTERVAL_MAX) cfg.interval = DEFAULT_INTERVAL_MAX;
    if ((cfg.mqtt_qos < 0) || (cfg.mqtt_qos > 2)) cfg.mqtt_qos = 0;
#ifdef INFLUXDB
    if ((cfg.influxdb_version < 1) || (cfg.influxdb_version > 2)) cfg.influxdb_version = 1;
#endif
    if ((cfg.history_start_year < E3DC_FOUNDED) || (cfg.history_start_year > (curr_year - 1))) cfg.history_start_year = curr_year - 1;

    // prepare RscpMqttReceiveCache
    for (uint8_t c = 0; c < IDLE_PERIOD_CACHE_SIZE; c++) {
        RSCP_MQTT::rec_cache_t cache = { 0, 0, "set/idle_period", SET_IDLE_PERIOD_REGEX, "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true };
        RSCP_MQTT::RscpMqttReceiveCache.push_back(cache);
    }
    sort(RSCP_MQTT::RscpMqttReceiveCache.begin(), RSCP_MQTT::RscpMqttReceiveCache.end(), RSCP_MQTT::compareRecCache);

    printf("rscp2mqtt [%s]\n", RSCP2MQTT);
    printf("E3DC system >%s:%u< user: >%s<\n", cfg.e3dc_ip, cfg.e3dc_port, cfg.e3dc_user);
    if (!std::regex_match(cfg.e3dc_ip, std::regex("^(?:\\b\\.?(?:25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){4}$"))) {
        printf("Error: >%s< is not a valid IP address.\n", cfg.e3dc_ip);
        exit(EXIT_FAILURE);
    }
    if (mosquitto_sub_topic_check(cfg.prefix) != MOSQ_ERR_SUCCESS) {
        printf("Error: MQTT prefix >%s< is not accepted.\n", cfg.prefix);
        exit(EXIT_FAILURE);
    }
    printf("MQTT broker >%s:%u< qos = >%i< retain = >%s< client id >%s< prefix >%s<\n", cfg.mqtt_host, cfg.mqtt_port, cfg.mqtt_qos, cfg.mqtt_retain ? "true" : "false", strcmp(cfg.mqtt_client_id, "")?cfg.mqtt_client_id:"✗", cfg.prefix);

    addPrefix(RSCP_MQTT::RscpMqttCache, cfg.prefix);
    addPrefix(RSCP_MQTT::RscpMqttCacheTempl, cfg.prefix);

    for (std::vector<RSCP_MQTT::topic_store_t>::iterator it = RSCP_MQTT::TopicStore.begin(); it != RSCP_MQTT::TopicStore.end(); ++it) {
        switch (it->type) {
            case FORCED_TOPIC: {
                setTopicsForce(RSCP_MQTT::RscpMqttCache, it->topic);
                break;
            }
            case LOG_TOPIC: {
                setTopicHistory(RSCP_MQTT::RscpMqttCache, it->topic);
                break;
            }
#ifdef INFLUXDB
            case INFLUXDB_TOPIC: {
                setTopicsInflux(RSCP_MQTT::RscpMqttCache, it->topic);
                break;
            }
#endif
        }
    }

    // History year
    if (cfg.verbose) printf("History year range from %d to %d\n", cfg.history_start_year, curr_year);
    if (cfg.history_start_year < curr_year) addTemplTopics(TAG_DB_HISTORY_DATA_YEAR, 1, NULL, cfg.history_start_year, curr_year, 0);

    // Battery Strings
    addTemplTopics(TAG_BAT_DATA, (cfg.battery_strings == 1)?0:1, (char *)"battery", 0, cfg.battery_strings, 1);
    addTemplTopics(TAG_BAT_SPECIFICATION, (cfg.battery_strings == 1)?0:1, (char *)"battery", 0, cfg.battery_strings, 1);

#ifdef INFLUXDB
    if (cfg.influxdb_on && (cfg.influxdb_version == 1)) {
        printf("INFLUXDB v1 >%s:%u< db = >%s< measurement = >%s<\n", cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_db, cfg.influxdb_measurement);
        if (!strcmp(cfg.influxdb_host, "") || !strcmp(cfg.influxdb_db, "") || !strcmp(cfg.influxdb_measurement, "")) printf("Error: INFLUXDB not configured correctly\n");
    } else if (cfg.influxdb_on && (cfg.influxdb_version == 2)) {
        printf("INFLUXDB v2 >%s:%u< orga = >%s< bucket = >%s< measurement = >%s<\n", cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_orga, cfg.influxdb_bucket, cfg.influxdb_measurement);
        if (!strcmp(cfg.influxdb_host, "") || !strcmp(cfg.influxdb_orga, "") || !strcmp(cfg.influxdb_bucket, "") || !strcmp(cfg.influxdb_measurement, "")) printf("Error: INFLUXDB not configured correctly\n");
    }
#endif
    printf("Fetching data every ");
    if (cfg.interval == 1) printf("second.\n"); else printf("%i seconds.\n", cfg.interval);
    printf("Requesting PVI %s | PM %s | DCB %s (%d battery string%s) | Wallbox %s | Autorefresh %s\n", cfg.pvi_requests?"✓":"✗", cfg.pm_requests?"✓":"✗", cfg.dcb_requests?"✓":"✗", cfg.battery_strings, (cfg.battery_strings > 1)?"s":"", cfg.wallbox?"✓":"✗", cfg.auto_refresh?"✓":"✗");

    if (!cfg.mqtt_pub) printf("DISABLE_MQTT_PUBLISH / DRYRUN mode\n");
    printf("Log level = %d\n", cfg.log_level);

    if (isatty(STDOUT_FILENO)) {
      printf("Stdout to terminal\n");
    } else {
      printf("Stdout to pipe/file\n");
      cfg.daemon = false;
      cfg.verbose = false;
    }
    printf("\n");

    if (cfg.daemon) {
        printf("...working as a daemon.\n");
        pid_t pid, sid;
        pid = fork();
        if (pid < 0)
            exit(EXIT_FAILURE);
        if (pid > 0)
            exit(EXIT_SUCCESS);
        umask(0);
        sid = setsid();
        if (sid < 0)
            exit(EXIT_FAILURE);
        if ((chdir("/")) < 0)
            exit(EXIT_FAILURE);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        cfg.verbose = false;
    } else {
        if (cfg.logfile) free(cfg.logfile);
        cfg.logfile = NULL;
    }

    if (cfg.verbose) logTopics(RSCP_MQTT::RscpMqttCache, cfg.logfile);

    // MQTT init
    mosquitto_lib_init();

#ifdef INFLUXDB
    // CURL init
    if (cfg.influxdb_on) curl = curl_easy_init();

    if (curl) {
        char buffer[512];

        headers = curl_slist_append(headers, "Content-Type: text/plain");
        headers = curl_slist_append(headers, "Accept: application/json");

        if (cfg.influxdb_version == 1) {
            if (cfg.influxdb_auth && strcmp(cfg.influxdb_user, "") && strcmp(cfg.influxdb_password, "")) {
                curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
                curl_easy_setopt(curl, CURLOPT_USERNAME, cfg.influxdb_user);
                curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg.influxdb_password);
            }
            sprintf(buffer, "http://%s:%d/write?db=%s", cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_db);
        } else {
            sprintf(buffer, "Authorization: Token %s", cfg.influxdb_token);
            headers = curl_slist_append(headers, buffer);
            sprintf(buffer, "http://%s:%d/api/v2/write?org=%s&bucket=%s", cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_orga, cfg.influxdb_bucket);
        }

        curl_easy_setopt(curl, CURLOPT_URL, buffer);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
#endif

    // endless application which re-connections to server on connection lost
    while (true) {
        // connect to server
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Connecting to server %s:%u\n", cfg.e3dc_ip, cfg.e3dc_port);
        iSocket = SocketConnect(cfg.e3dc_ip, cfg.e3dc_port);
        if (iSocket < 0) {
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: E3DC connection failed\n");
            sleep(1);
            continue;
        }
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Success: E3DC connected.\n");

        // reset authentication flag
        iAuthenticated = 0;

        // create AES key and set AES parameters
        {
            // initialize AES encryptor and decryptor IV
            memset(ucDecryptionIV, 0xff, AES_BLOCK_SIZE);
            memset(ucEncryptionIV, 0xff, AES_BLOCK_SIZE);

            // limit password length to AES_KEY_SIZE
            int iPasswordLength = strlen(cfg.aes_password);
            if (iPasswordLength > AES_KEY_SIZE)
                iPasswordLength = AES_KEY_SIZE;

            // copy up to 32 bytes of AES key password
            uint8_t ucAesKey[AES_KEY_SIZE];
            memset(ucAesKey, 0xff, AES_KEY_SIZE);
            memcpy(ucAesKey, cfg.aes_password, iPasswordLength);

            // set encryptor and decryptor parameters
            aesDecrypter.SetParameters(AES_KEY_SIZE * 8, AES_BLOCK_SIZE * 8);
            aesEncrypter.SetParameters(AES_KEY_SIZE * 8, AES_BLOCK_SIZE * 8);
            aesDecrypter.StartDecryption(ucAesKey);
            aesEncrypter.StartEncryption(ucAesKey);
        }

        // enter the main transmit / receive loop
        mainLoop();

        // close socket connection
        SocketClose(iSocket);
        iSocket = -1;
    }

    // MQTT cleanup
    mosquitto_lib_cleanup();

#ifdef INFLUXDB
    // CURL cleanup
    if (curl) curl_easy_cleanup(curl);
    if (headers) curl_slist_free_all(headers);
#endif

    if (cfg.logfile) free(cfg.logfile);
    if (cfg.historyfile) free(cfg.historyfile);

    exit(EXIT_SUCCESS);
}
