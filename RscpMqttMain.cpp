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
#include "RscpTagsOverview.h"
#include "RscpMqttMapping.h"
#include "RscpMqttConfig.h"
#include "SocketConnection.h"
#include "AES.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <mosquitto.h>
#include <thread>
#include <regex>
#include <mutex>

#define RSCP2MQTT_VERSION       "3.38"

#define AES_KEY_SIZE            32
#define AES_BLOCK_SIZE          32

#define E3DC_FOUNDED            2010
#define CONFIG_FILE             ".config"
#define DEFAULT_INTERVAL_MIN    1
#define DEFAULT_INTERVAL_MAX    300
#define IDLE_PERIOD_CACHE_SIZE  14
#define DEFAULT_PREFIX          "e3dc"
#define SUBSCRIBE_TOPIC         "set/#"
#define RECURSION_MAX_LEVEL     7

#define MQTT_PORT_DEFAULT       1883

#ifdef INFLUXDB
    #define RSCP2MQTT               RSCP2MQTT_VERSION".influxdb"
    #define INFLUXDB_PORT_DEFAULT   8086
    #define CURL_BUFFER_SIZE        65536
#else
    #define RSCP2MQTT               RSCP2MQTT_VERSION
#endif

#define CHARGE_LOCK_TRUE        "today:charge:true:00:00-23:59"
#define CHARGE_LOCK_FALSE       "today:charge:false:00:00-23:59"
#define DISCHARGE_LOCK_TRUE     "today:discharge:true:00:00-23:59"
#define DISCHARGE_LOCK_FALSE    "today:discharge:false:00:00-23:59"
#define PV_SOLAR_MIN            200
#define MAX_DAYS_PER_ITERATION  12
#define DELAY_BEFORE_RECONNECT  10

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
static wb_t wb_stat;
static int day, leap_day, year, curr_day, curr_year, battery_nr, pm_nr, wb_nr;
static uint8_t period_change_nr = 0;
static bool period_trigger = false;
static bool day_end = false;
static bool new_day = false;

std::mutex mtx;

static bool mqttRcvd = false;
static bool go = true;
static bool tags_added = false;

#ifdef INFLUXDB
    CURL *curl = NULL;
    struct curl_slist *headers = NULL;
#endif

void logMessage(char *file, char *srcfile, int line, char *format, ...);
void logMessageCache(char *file, bool clear);

void signal_handler(int sig) {
    if (sig == SIGINT) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Program exits by signal SIGINT\n");
    else if (sig == SIGTERM) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Program exits by signal SIGTERM\n");
    else logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Program exits by signal >%d<\n", sig);
    go = false;
}

void wsleep(double seconds) {
    while (seconds > 0.0) {
        mtx.lock();
        if (mqttRcvd || !go) {
            mtx.unlock();
            return;
        }
        mtx.unlock();
        if (seconds >= 1.0) {
            sleep(1);
        } else {
            useconds_t usec = (useconds_t)(seconds * 1e6);
            if (usec > 0) usleep(usec);
        }
        seconds = seconds - 1.0;
    }
    return;
}

bool hasSpaces(char *s) {
    for (size_t i = 0; i < strlen(s); i++) {
        if (isspace(s[i])) return(true);
    }
    return(false);
}

char *tagName(std::vector<RSCP_TAGS::tag_overview_t> & v, uint32_t tag) {
    for (std::vector<RSCP_TAGS::tag_overview_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (it->tag == tag) {
            return(it->name);
        }
    }
    char name[128];
    sprintf(name, "0x%02X", tag);
    RSCP_TAGS::tag_overview_t tag_overview = { tag, "", 0 };
    strcpy(tag_overview.name, name);
    RSCP_TAGS::RscpTagsOverview.push_back(tag_overview);
    logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"tagName: new tag >%s<\n", name);
    return(tagName(v, tag));
}

uint32_t tagID(std::vector<RSCP_TAGS::tag_overview_t> & v, char *name) {
    for (std::vector<RSCP_TAGS::tag_overview_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (!strcmp(it->name, name)) {
            return(it->tag);
        }
    }
    return(0); 
}

bool isTag(std::vector<RSCP_TAGS::tag_overview_t> & v, char *name, bool must_be_request_tag) {
    for (std::vector<RSCP_TAGS::tag_overview_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (!strcmp(it->name, name) && (!must_be_request_tag || (must_be_request_tag && it->request))) {
            return(true);
        }
    }
    return(false); 
}

char *typeName(std::vector<RSCP_TAGS::rscp_types_t> & v, uint8_t code) {
    char *unknown = NULL;
    for (std::vector<RSCP_TAGS::rscp_types_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (!it->code) unknown = it->type;
        if (it->code == code) {
            return(it->type);
        }
    }
    return(unknown);
}

uint8_t typeID(std::vector<RSCP_TAGS::rscp_types_t> & v, char *type) {
    for (std::vector<RSCP_TAGS::rscp_types_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (!strcmp(it->type, type)) {
            return(it->code);
        }
    }
    return(0);
}

char *errName(std::vector<RSCP_TAGS::rscp_err_codes_t> & v, uint32_t code) {
    char *unknown = NULL;
    for (std::vector<RSCP_TAGS::rscp_err_codes_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (!it->code) unknown = it->error;
        if (it->code == code) {
            return(it->error);
        }
    }    
    return(unknown); 
}

int storeMQTTReceivedValue(std::vector<RSCP_MQTT::rec_cache_t> & c, char *topic_in, char *payload) {
    char topic[TOPIC_SIZE];
    mtx.lock();
    mqttRcvd = true;

    snprintf(topic, TOPIC_SIZE, "%s/set/up", cfg.prefix);
    if (cfg.store_setup && !strncmp(topic_in, topic, strlen(topic))) {
        char *t = strdup(topic_in);
        char *p = strdup(payload);
        RSCP_MQTT::mqttQ.push({t, p});
    } else {
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
    }
    mtx.unlock();
    return(0);
}

// topic: <prefix>/set/power_mode payload: auto / idle:60 (cycles) / discharge:2000:60 (Watt:cycles) / charge:2000:60 (Watt:cycles) / grid_charge:2000:60 (Watt:cycles)
int handleSetPower(std::vector<RSCP_MQTT::rec_cache_t> & c, uint32_t container, char *payload) {
    char cycles[12];
    char cmd[12];
    char power[12];
    char modus[2];
    time_t now;
    time(&now);

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
                    it->refresh_until = abs(atoi(cycles)) * cfg.interval + now;
                    break;
                }
                case TAG_EMS_REQ_SET_POWER_VALUE: {
                    strcpy(it->payload, power);
                    it->refresh_until = abs(atoi(cycles)) * cfg.interval + now;
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

void setTopicAttr() {
    for (std::vector<RSCP_MQTT::topic_store_t>::iterator i = RSCP_MQTT::TopicStore.begin(); i != RSCP_MQTT::TopicStore.end(); ++i) {
        switch (i->type) {
            case FORCED_TOPIC: {
                for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCache.begin(); it != RSCP_MQTT::RscpMqttCache.end(); ++it) {
                    if (std::regex_match(it->topic, std::regex(i->topic))) it->forced = true;
                }
                break;
            }
            case LOG_TOPIC: {
                for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCache.begin(); it != RSCP_MQTT::RscpMqttCache.end(); ++it) {
                    if (std::regex_match(it->topic, std::regex(i->topic))) it->history_log = true;
                }
                break;
            }
#ifdef INFLUXDB
            case INFLUXDB_TOPIC: {
                for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCache.begin(); it != RSCP_MQTT::RscpMqttCache.end(); ++it) {
                    if (std::regex_match(it->topic, std::regex(i->topic))) it->influx = true;
                }
                break;
            }
#endif
        }
    }
    return;
}

void addTemplTopicsIdx(int index, char *seg, int start, int n, int inc, bool finalize) {
    for (int c = start; c < n; c++) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCacheTempl.begin(); it != RSCP_MQTT::RscpMqttCacheTempl.end(); ++it) {
            if (it->index == index) {
                RSCP_MQTT::cache_t cache = { it->container, it->tag, index + c, "", "", it->format, "", it->divisor, it->bit_to_bool, it->history_log, it->handled, it->changed, it->influx, it->forced };
                if (n == 1) {
                    snprintf(cache.topic, TOPIC_SIZE, it->topic, seg);
                } else if (seg == NULL) {
                    snprintf(cache.topic, TOPIC_SIZE, it->topic, c + inc);
                } else {
                    char buffer[TOPIC_SIZE];
                    snprintf(buffer, TOPIC_SIZE, "%s/%d", seg, c + inc);
                    snprintf(cache.topic, TOPIC_SIZE, it->topic, buffer);
                }
                strcpy(cache.unit, it->unit);
                RSCP_MQTT::RscpMqttCache.push_back(cache);
            }
        }
    }
    if (finalize) {
        sort(RSCP_MQTT::RscpMqttCache.begin(), RSCP_MQTT::RscpMqttCache.end(), RSCP_MQTT::compareCache);
        setTopicAttr();
    }
    return;
}

void addTemplTopics(uint32_t container, int index, char *seg, int start, int n, int inc, int counter, bool finalize) {
    for (int c = start; c < n; c++) {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCacheTempl.begin(); it != RSCP_MQTT::RscpMqttCacheTempl.end(); ++it) {
            if (it->container == container) {
                RSCP_MQTT::cache_t cache = { it->container, it->tag, c, "", "", it->format, "", it->divisor, it->bit_to_bool, it->history_log, it->handled, it->changed, it->influx, it->forced };
                if (it->index > 1) cache.index = it->index + c * 10;
                if ((seg == NULL) && (index == 0)) { // no args
                    cache.index = it->index;
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
                if (!counter) {
                    RSCP_MQTT::RscpMqttCache.push_back(cache);
                } else {
                    char buffer[TOPIC_SIZE];
                    strcpy(buffer, cache.topic);
                    for (int i = 1; i <= counter; i++) {
                        if (counter >= 10) snprintf(cache.topic, TOPIC_SIZE, "%s/%.2d", buffer, i);
                        else snprintf(cache.topic, TOPIC_SIZE, "%s/%d", buffer, i);
                        RSCP_MQTT::RscpMqttCache.push_back(cache);
                    }
                }
            }
        }
    }
    if (finalize) {
        sort(RSCP_MQTT::RscpMqttCache.begin(), RSCP_MQTT::RscpMqttCache.end(), RSCP_MQTT::compareCache);
        setTopicAttr();
    }
    return;
}

void addTopic(uint32_t container, uint32_t tag, int index, char *topic, char *unit, int format, int divisor, int bit_to_bool, bool finalize) {
    RSCP_MQTT::cache_t cache = { container, tag, index, "", "", format, "", divisor, bit_to_bool, false, false, false, false, false };
    strcpy(cache.topic, topic);
    strcpy(cache.unit, unit);
    RSCP_MQTT::RscpMqttCache.push_back(cache);

    if (finalize) {
        sort(RSCP_MQTT::RscpMqttCache.begin(), RSCP_MQTT::RscpMqttCache.end(), RSCP_MQTT::compareCache);
        setTopicAttr();
    }
    return;
}

void addSetTopic(uint32_t container, uint32_t tag, int index, char *topic, char *regex_true, char *value_true, char *regex_false, char *value_false, int type, bool finalize) {
    RSCP_MQTT::rec_cache_t cache = { container, tag, index, "", "", "1", "", "0", "", UNIT_NONE, type, 0, false, true };
    strcpy(cache.topic, topic);
    strcpy(cache.regex_true, regex_true);
    strcpy(cache.value_true, value_true);
    strcpy(cache.regex_false, regex_false);
    strcpy(cache.value_false, value_false);
    RSCP_MQTT::RscpMqttReceiveCache.push_back(cache);

    if (finalize) sort(RSCP_MQTT::RscpMqttReceiveCache.begin(), RSCP_MQTT::RscpMqttReceiveCache.end(), RSCP_MQTT::compareRecCache);
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
    return;
}

int mqttCallbackTlsPassword(char *buf, int size, int rwflag, void *userdata) {
    int len = strlen(cfg.mqtt_tls_password);
    if (len >= size) return 0;
    strcpy(buf, cfg.mqtt_tls_password);
    return(len);
}

void mqttCallbackOnMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    if (mosq && msg && msg->topic && msg->payloadlen) {
        storeMQTTReceivedValue(RSCP_MQTT::RscpMqttReceiveCache, msg->topic, (char *)msg->payload);
    }
    return;
}

void mqttListener(struct mosquitto *m) {
    if (m) {
         if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"MQTT: starting listener loop\n");
         mosquitto_loop_forever(m, -1, 1);
    }
    return;
}

#ifdef INFLUXDB
void insertInfluxDb(char *buffer) {
    CURLcode res = CURLE_OK;

    if (strlen(buffer)) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"InfluxDB: %s\n", curl_easy_strerror(res));
    }
    return;
}

void handleInfluxDb(char *buffer, char *topic, char *payload, char *unit, char *timestamp) {
    char line[CURL_BUFFER_SIZE];

    if (topic == NULL) {
        insertInfluxDb(buffer);
        return;
    }

    if (!strcmp(payload, "true")) sprintf(line, "%s,topic=%s value=1", cfg.influxdb_measurement, topic);
    else if (!strcmp(payload, "false")) sprintf(line, "%s,topic=%s value=0", cfg.influxdb_measurement, topic);
    else if (std::regex_match(payload, std::regex("[+-]?([0-9]*[.]?[0-9]+)"))) {
        if (unit && strcmp(unit, "") && timestamp) sprintf(line, "%s,topic=%s,unit=%s value=%s %s", cfg.influxdb_measurement, topic, unit, payload, timestamp);
        else if (unit && !strcmp(unit, "") && timestamp) sprintf(line, "%s,topic=%s value=%s %s", cfg.influxdb_measurement, topic, payload, timestamp);
        else if (unit && strcmp(unit, "")) sprintf(line, "%s,topic=%s,unit=%s value=%s", cfg.influxdb_measurement, topic, unit, payload);
        else sprintf(line, "%s,topic=%s value=%s", cfg.influxdb_measurement, topic, payload);
    } else {
        sprintf(line, "%s,topic=%s value=\"%s\"", cfg.influxdb_measurement_meta, topic, payload);
    }

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

void publishImmediately(char *t, char *p, bool influx) {
    char topic[TOPIC_SIZE];

    snprintf(topic, TOPIC_SIZE, "%s/%s", cfg.prefix, t);
    if (mosq && (mosquitto_pub_topic_check(topic) == MOSQ_ERR_SUCCESS) && p && strlen(p)) mosquitto_publish(mosq, NULL, topic, strlen(p), p, cfg.mqtt_qos, cfg.mqtt_retain);
#ifdef INFLUXDB
    if (cfg.influxdb_on && curl && influx) {
        char buffer[CURL_BUFFER_SIZE];
        strcpy(buffer, "");
        handleInfluxDb(buffer, topic, p, NULL, NULL);
        handleInfluxDb(buffer, NULL, NULL, NULL, NULL);
    }
#endif
    return;
}

int handleMQTT(std::vector<RSCP_MQTT::cache_t> & v, int qos, bool retain) {
#ifdef INFLUXDB
    char buffer[CURL_BUFFER_SIZE];
    strcpy(buffer, "");
#endif
    int rc = 0;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if ((it->changed || it->forced) && strcmp(it->topic, "") && strcmp(it->payload, "")) {
            if (cfg.verbose) {
#ifdef INFLUXDB
               logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Publish topic >%s< payload >%s< unit >%s< mqtt:%s influx:%s\n", it->topic, it->payload, it->unit, cfg.mqtt_pub?"✓":"✗", (cfg.influxdb_on && it->influx)?"✓":"✗");
#else
               logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Publish topic >%s< payload >%s< unit >%s< mqtt:%s\n", it->topic, it->payload, it->unit, cfg.mqtt_pub?"✓":"✗");
#endif
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
        if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Publish topic >%s< payload >%s< mqtt:%s\n", topic, payload, cfg.mqtt_pub?"✓":"✗");
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
        if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Publish topic >%s< payload >%s< mqtt:%s\n", topic, payload, cfg.mqtt_pub?"✓":"✗");
        if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, qos, retain);
        snprintf(topic, TOPIC_SIZE, "%s/error_message/%d", cfg.prefix, i);
        if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Publish topic >%s< payload >%s< mqtt:%s\n", topic, e.message, cfg.mqtt_pub?"✓":"✗");
        if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(e.message), e.message, qos, retain);
        v.pop_back();
    }
    return(rc);
}

void storeSetupItem(uint32_t container, uint32_t tag, int index, int value) {
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);

    snprintf(topic, TOPIC_SIZE, "%s/set/up/0x%02X_0x%02X_0x%02X", cfg.prefix, container, tag, index);
    snprintf(payload, PAYLOAD_SIZE, "%.4d%.2d%.2d:%d", l->tm_year + 1900, l->tm_mon + 1, l->tm_mday, value);
    if (cfg.mqtt_pub && mosq) mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, cfg.mqtt_qos, true);
    return;
}

void setupItem(std::vector<RSCP_MQTT::cache_t> & v, char *topic_in, char* payload) {
    char topic[TOPIC_SIZE];
    char value[15];
    char date[10];
    char date_in[10];
    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);

    if (snprintf(date, sizeof(date), "%.4d%.2d%.2d", l->tm_year + 1900, l->tm_mon + 1, l->tm_mday) >= sizeof(date)) {
        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"setupItem: Buffer overflow\n");
        return;
    }

    if (sscanf(payload, "%9[^:]:%14[^:]", date_in, value) == 2) {
        if (strcmp(date, date_in)) return;
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            snprintf(topic, TOPIC_SIZE, "%s/set/up/0x%02X_0x%02X_0x%02X", cfg.prefix, it->container, it->tag, it->index);
            if (!strcmp(topic, topic_in) && std::regex_match(value, std::regex("[+-]?([0-9]*[.]?[0-9]+)"))) {
                strcpy(it->payload, value);
                if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Setup topic >%s< payload >%s< date >%s<\n", topic, value, date_in);
                break;
            }
        }
    }
    return;
}

void logCache(std::vector<RSCP_MQTT::cache_t> & v, char *file) {
    if (file) {
        FILE *fp;
        fp = fopen(file, "a");
        if (!fp) return;
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            fprintf(fp, "container >%s< tag >%s< index >%d< topic >%s< payload >%s< unit >%s<%s\n", tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag), it->index, it->topic, it->payload, it->unit, (it->forced?" forced":""));
        }
        fflush(fp);
        fclose(fp);
    } else {
        for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            printf("container >%s< tag >%s< index >%d< topic >%s< payload >%s< unit >%s<%s\n", tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag), it->index, it->topic, it->payload, it->unit, (it->forced?" forced":""));
        }
        fflush(NULL);
    }
    return;
}

void logRecCache(std::vector<RSCP_MQTT::rec_cache_t> & v, char *file) {
    if (file) {
        FILE *fp;
        fp = fopen(file, "a");
        if (!fp) return;        
        for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            fprintf(fp, "container >%s< tag >%s< index >%d< topic >%s< regex_true >%s< value_true >%s< regex_false >%s< value_false >%s< unit >%s< type >%s<\n", tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag), it->index, it->topic, it->regex_true, it->value_true, it->regex_false, it->value_false, it->unit, typeName(RSCP_TAGS::RscpTypeNames, it->type));
        }
        fflush(fp);
        fclose(fp);
    } else {
        for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
            printf("container >%s< tag >%s< index >%d< topic >%s< regex_true >%s< value_true >%s< regex_false >%s< value_false >%s< unit >%s< type >%s<\n", tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag), it->index, it->topic, it->regex_true, it->value_true, it->regex_false, it->value_false, it->unit, typeName(RSCP_TAGS::RscpTypeNames, it->type));
        }
        fflush(NULL);
    }
    return;
}

void logTopics(std::vector<RSCP_MQTT::cache_t> & v, char *file, bool check_payload) {
    int total = 0;
    int forced = 0;
    int influx = 0;
    int history_log = 0;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (it->forced) { logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< forced\n", it->topic); forced++; }
        if (it->influx) { logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< marked influx relevant\n", it->topic); influx++; }
        if (it->history_log) { logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< marked for logging\n", it->topic); history_log++; }
        if (check_payload && (!strcmp(it->payload, ""))) logMessage(file, (char *)__FILE__, __LINE__, (char *)"Topic >%s< unused? Payload is empty\n", it->topic);
        total++;
    }
    logMessage(file, (char *)__FILE__, __LINE__, (char *)"Number of topics >%d<\n", total);
    logMessage(file, (char *)__FILE__, __LINE__, (char *)"Number of topics with flag \"forced\" >%d<\n", forced);
    logMessage(file, (char *)__FILE__, __LINE__, (char *)"Number of topics with flag \"influx\" >%d<\n", influx);
    logMessage(file, (char *)__FILE__, __LINE__, (char *)"Number of topics with flag \"history_log\" >%d<\n", history_log);
    return;
}

void logHealth(char *file) {
    logMessage(file, (char *)__FILE__, __LINE__, (char *)"[%s] PVI %s | PM %s | DCB %s (%d) | Wallbox %s | Autorefresh %s | Raw data %s | Interval %d\n", RSCP2MQTT, cfg.pvi_requests?"✓":"✗", cfg.pm_requests?"✓":"✗", cfg.dcb_requests?"✓":"✗", cfg.battery_strings, cfg.wallbox?"✓":"✗", cfg.auto_refresh?"✓":"✗", cfg.raw_mode?"✓":"✗", cfg.interval);
    for (uint8_t i = 0; i < cfg.pm_number; i++) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"PM_INDEX >%d<\n", cfg.pm_indexes[i]);
    }
    for (uint8_t i = 0; i < cfg.wb_number; i++) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"WB_INDEX >%d<\n", cfg.wb_indexes[i]);
    }
    logTopics(RSCP_MQTT::RscpMqttCache, file, true);
    for (std::vector<RSCP_MQTT::additional_tags_t>::iterator it = RSCP_MQTT::AdditionalTags.begin(); it != RSCP_MQTT::AdditionalTags.end(); ++it) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"Container >%s< Tag >%s< added (one shot: %s).\n", tagName(RSCP_TAGS::RscpTagsOverview, it->req_container), tagName(RSCP_TAGS::RscpTagsOverview, it->req_tag), it->one_shot?"true":"false");
    }
    for (std::vector<RSCP_MQTT::not_supported_tags_t>::iterator it = RSCP_MQTT::NotSupportedTags.begin(); it != RSCP_MQTT::NotSupportedTags.end(); ++it) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"Container >%s< Tag >%s< not supported.\n", tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag));
    }
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = RSCP_MQTT::RscpMqttCacheTempl.begin(); it != RSCP_MQTT::RscpMqttCacheTempl.end(); ++it) {
        logMessage(file, (char *)__FILE__, __LINE__, (char *)"Container >%s< Tag >%s< in temporary cache.\n", tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag));
    }
    logMessageCache(file, false);
    return;
}

void logMessage(char *file, char *srcfile, int line, char *format, ...) {
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
            RSCP_MQTT::cache_t cache = { it->container, it->tag, it->index, "", "", it->format, "", it->divisor, it->bit_to_bool, it->history_log, it->handled, it->changed, it->influx, it->forced };
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

bool updMessageByTag(uint32_t container, uint32_t tag, int error, int type) {
    for (std::vector<RSCP_MQTT::message_cache_t>::iterator it = RSCP_MQTT::MessageCache.begin(); it != RSCP_MQTT::MessageCache.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->type == type) && (it->error == error)) {
            it->count += 1;
            return(true);
        }
    }
    return(false);
}

void logMessageByTag(uint32_t container, uint32_t tag, int error, int type, char *message) {
    if (!cfg.log_level) return;
    if (updMessageByTag(container, tag, error, type)) return;
    RSCP_MQTT::message_cache_t v;
    v.container = container;
    v.tag = tag;
    v.type = type;
    v.error = error;
    v.count = 1;
    strcpy(v.message, message);
    RSCP_MQTT::MessageCache.push_back(v);
    if (cfg.log_level == 1) logMessageCache(cfg.logfile, true);
    return;
}

void logMessageCache(char *file, bool clear) {
    for (std::vector<RSCP_MQTT::message_cache_t>::iterator it = RSCP_MQTT::MessageCache.begin(); it != RSCP_MQTT::MessageCache.end(); ++it) {
        if (it->count) {
            if (it->container && it->tag && it->error) logMessage(file, (char *)__FILE__, it->type, it->message, tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag), errName(RSCP_TAGS::RscpErrorOverview, it->error), it->count);
            else if (!it->container && it->tag && it->error) logMessage(file, (char *)__FILE__, it->type, it->message, tagName(RSCP_TAGS::RscpTagsOverview, it->tag), errName(RSCP_TAGS::RscpErrorOverview, it->error), it->count);
            else if (it->container && it->tag && !it->error) logMessage(file, (char *)__FILE__, it->type, it->message, tagName(RSCP_TAGS::RscpTagsOverview, it->container), tagName(RSCP_TAGS::RscpTagsOverview, it->tag), it->count);
            else logMessage(file, (char *)__FILE__, it->type, it->message, it->count);
            if (clear) it->count = 0;
        }
    }
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

bool existsAdditionalTag(uint32_t container, uint32_t tag, int index, int order) {
    for (std::vector<RSCP_MQTT::additional_tags_t>::iterator it = RSCP_MQTT::AdditionalTags.begin(); it != RSCP_MQTT::AdditionalTags.end(); ++it) {
        if ((it->req_container == container) && (it->req_tag == tag) && (it->req_index == index) && (it->order == order)) return(true);          
    }           
    return(false);
}

void pushAdditionalTag(uint32_t req_container, uint32_t req_tag, int req_index, int order, bool one_shot) {
    if (existsAdditionalTag(req_container, req_tag, req_index, order)) return;
    RSCP_MQTT::additional_tags_t v;
    v.req_container = req_container;
    v.req_tag = req_tag;
    v.req_index = req_index;
    v.order = order;
    v.one_shot = one_shot;
    RSCP_MQTT::AdditionalTags.push_back(v);
    tags_added = true;
    return;
}

void initRawData() {
    for (std::vector<RSCP_MQTT::raw_data_t>::iterator it = RSCP_MQTT::rawData.begin(); it != RSCP_MQTT::rawData.end(); ++it) {
        it->handled = false;
        it->changed = false;
    }
    return;
}

int mergeRawData(char *topic, char *payload, bool *changed) {
    int i = 0;
    if (topic && payload) {
        for (std::vector<RSCP_MQTT::raw_data_t>::iterator it = RSCP_MQTT::rawData.begin(); it != RSCP_MQTT::rawData.end(); ++it) {
            if (!strcmp(it->topic, topic) && !it->handled && (i == it->nr)) {
                if (strcmp(it->payload, payload)) {
                    if (strlen(it->payload) != strlen(payload)) it->payload = (char *)realloc(it->payload, strlen(payload) + 1);
                    if (it->payload) strcpy(it->payload, payload);
                    it->changed = true;
                    *changed = true;
                }
                it->handled = true;
                return(i);
            }
            if (!strcmp(it->topic, topic) && it->handled) i++;
        }
        RSCP_MQTT::raw_data_t v;
        v.topic = strdup(topic);
        v.payload = strdup(payload);
        v.handled = true;
        v.changed = true;
        v.nr = i;
        RSCP_MQTT::rawData.push_back(v);
        *changed = true;
    }
    return(i);
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

// Issue #9
void correctExternalPM(std::vector<RSCP_MQTT::cache_t> & v, int pm, char *unit, int divisor) {
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (((pm == 0) && (it->tag == TAG_DB_PM_0_POWER)) || ((pm == 1) && (it->tag == TAG_DB_PM_1_POWER))) {
            if (unit && (!strcmp(unit, UNIT_KWH) || !strcmp(unit, UNIT_WH))) strcpy(it->unit, unit);
            if ((divisor == 1) || (divisor == 1000)) it->divisor = divisor;
            if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"correctExternalPM pm %d unit >%s< divisor %d topic >%s<\n", pm, it->unit, it->divisor, it->topic);
        }
    }
    return;
}

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
            // only float is handled
            switch (it->format) {
                case F_FLOAT_0 : {
                    snprintf(payload, PAYLOAD_SIZE, "%.0f", protocol->getValueAsFloat32(response) / it->divisor);
                    break;
                }
                case F_FLOAT_1 : {
                    snprintf(payload, PAYLOAD_SIZE, "%0.1f", protocol->getValueAsFloat32(response) / it->divisor);
                    break;
                }
                case F_AUTO :
                case F_FLOAT_2 : {
                    snprintf(payload, PAYLOAD_SIZE, "%0.2f", protocol->getValueAsFloat32(response) / it->divisor);
                    break;
                }
            }
            if (cfg.mqtt_pub && mosq) rc = mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, cfg.mqtt_qos, cfg.mqtt_retain);
#ifdef INFLUXDB
            l->tm_mday = day;
            l->tm_mon = month - 1;
            l->tm_year = year - 1900;
            sprintf(ts, "%llu000000000", (long long unsigned)(timegm(l) - gmt_diff));
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

int storeIntegerValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, int value, int index, bool dobreak) {
    char buf[PAYLOAD_SIZE];
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
            if (it->bit_to_bool) {
                if (value & it->bit_to_bool) strcpy(buf, cfg.true_value);
                else strcpy(buf, cfg.false_value);
            } else {
                snprintf(buf, PAYLOAD_SIZE, "%d", value);
            }
            if (strcmp(it->payload, buf)) {
                strcpy(it->payload, buf);
                it->changed = true;
            }
            if (dobreak) break;
        }
    }
    return(value);
}

float storeFloatValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, float value, int index, bool dobreak) {
    char buf[PAYLOAD_SIZE];     
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
            switch (it->format) {
                case F_FLOAT_0 : {
                    snprintf(buf, PAYLOAD_SIZE, "%.0f", value / it->divisor);
                    break;
                }
                case F_FLOAT_1 : {
                    snprintf(buf, PAYLOAD_SIZE, "%0.1f", value / it->divisor);
                    break;
                }
                case F_AUTO :
                case F_FLOAT_2 : {
                    snprintf(buf, PAYLOAD_SIZE, "%0.2f", value / it->divisor);
                    break;
                } 
            }
            if (strcmp(it->payload, buf)) {
                strcpy(it->payload, buf);
                it->changed = true; 
            }
            if (dobreak) break;
        }
    }
    return(value);
}

void storeStringValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, char *value, int index) {
    char buf[PAYLOAD_SIZE];
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
                snprintf(buf, PAYLOAD_SIZE, "%s", value);
            if (strcmp(it->payload, buf)) {
                strcpy(it->payload, buf);
                it->changed = true;
            }
            break;
        }
    }
    return;
}

int getIntegerValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, int index) {
    int value = 0;
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index) && (it->bit_to_bool == 0)) {
            value = atoi(it->payload);
            break;
        }
    }
    return(value);
}

float getFloatValue(std::vector<RSCP_MQTT::cache_t> & c, uint32_t container, uint32_t tag, int index) {
    float value = 0.0;
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container == container) && (it->tag == tag) && (it->index == index)) {
            value = atof(it->payload);
            break;
        }
    }
    return(value);
}

void resetHandleFlag(std::vector<RSCP_MQTT::cache_t> & c) {
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        it->handled = false;
    }
    return;
}

void preparePayload(RscpProtocol *protocol, SRscpValue *response, char **buf) {
    switch (response->dataType) {
        case RSCP::eTypeBool: {
            if (protocol->getValueAsBool(response)) strcpy(*buf, cfg.true_value);
            else strcpy(*buf, cfg.false_value);
            break;
        }
        case RSCP::eTypeInt16: {
            snprintf(*buf, PAYLOAD_SIZE, "%i", protocol->getValueAsInt16(response));
            break;
        }
        case RSCP::eTypeTimestamp:
        case RSCP::eTypeInt32: {
            snprintf(*buf, PAYLOAD_SIZE, "%i", protocol->getValueAsInt32(response));
            break;
        }
        case RSCP::eTypeUInt16: {
            snprintf(*buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUInt16(response));
            break;
        }
        case RSCP::eTypeUInt32: {
            snprintf(*buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUInt32(response));
            break;
        }
        case RSCP::eTypeChar8: {
            snprintf(*buf, PAYLOAD_SIZE, "%i", protocol->getValueAsChar8(response));
            break;
        }
        case RSCP::eTypeUChar8: {
            snprintf(*buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUChar8(response));
            break;
        }
        case RSCP::eTypeFloat32: {
            snprintf(*buf, PAYLOAD_SIZE, "%0.2f", protocol->getValueAsFloat32(response));
            break;
        }
        case RSCP::eTypeDouble64: {
            snprintf(*buf, PAYLOAD_SIZE, "%0.2f", protocol->getValueAsDouble64(response));
            break;
        }
        case RSCP::eTypeString: {
            snprintf(*buf, PAYLOAD_SIZE, "%s", protocol->getValueAsString(response).c_str());
            break;
        }
        default: {
            strcpy(*buf, "");
            break;
        }
    }
    return;
}

int storeResponseValue(std::vector<RSCP_MQTT::cache_t> & c, RscpProtocol *protocol, SRscpValue *response, uint32_t container, int index) {
    char buf[PAYLOAD_SIZE];
    static int battery_soc = -1;
    int rc = -1;

    // Issue #102
    uint32_t old_container = 0;
    uint32_t old_tag = 0;
    int old_index = -1;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->container > container) || ((old_container == it->container) && (old_tag == it->tag) && (old_index == it->index) && (it->bit_to_bool == 0))) break;
        if ((!it->container || (it->container == container)) && (it->tag == response->tag) && (it->index == index) && !it->handled) {
            switch (response->dataType) {
                case RSCP::eTypeBool: {
                    if (protocol->getValueAsBool(response)) strcpy(buf, cfg.true_value);
                    else strcpy(buf, cfg.false_value);
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
                        if (protocol->getValueAsUInt16(response) & it->bit_to_bool) strcpy(buf, cfg.true_value);
                        else strcpy(buf, cfg.false_value);
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
                        if (protocol->getValueAsUInt32(response) & it->bit_to_bool) strcpy(buf, cfg.true_value);
                        else strcpy(buf, cfg.false_value);
                    } else {
                        snprintf(buf, PAYLOAD_SIZE, "%u", protocol->getValueAsUInt32(response));
                    }
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
                        if (protocol->getValueAsUChar8(response) & it->bit_to_bool) strcpy(buf, cfg.true_value);
                        else strcpy(buf, cfg.false_value);
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
                        case F_AUTO :
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
                    switch (it->format) {
                        case F_FLOAT_0 : {
                            snprintf(buf, PAYLOAD_SIZE, "%.0f", protocol->getValueAsDouble64(response) / it->divisor);
                            break;
                        }
                        case F_FLOAT_1 : {
                            snprintf(buf, PAYLOAD_SIZE, "%0.1f", protocol->getValueAsDouble64(response) / it->divisor);
                            break;
                        }
                        case F_AUTO :
                        case F_FLOAT_2 : {
                            snprintf(buf, PAYLOAD_SIZE, "%0.2f", protocol->getValueAsDouble64(response) / it->divisor);
                            break;
                        }
                    }
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
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            rc = response->dataType;
            if (cfg.daily_values && (container == TAG_DB_HISTORY_DATA_DAY) && (index == 0) && (it->changed || new_day)) {
                time_t rawtime; 
                time(&rawtime);
                struct tm *l = localtime(&rawtime);
                handleImmediately(protocol, response, TAG_DB_HISTORY_DATA_DAY, l->tm_year + 1900, l->tm_mon + 1, l->tm_mday);
            }
            // Issue #44
            if ((it->tag == TAG_EMS_BAT_SOC) && (it->changed)) {
                if ((atoi(it->payload) == 0) && (battery_soc > 1)) snprintf(it->payload, PAYLOAD_SIZE, "%d", battery_soc--);
                else battery_soc = atoi(it->payload);
            }
            it->handled = true;
            old_container = it->container;
            old_tag = it->tag;
            old_index = it->index;
        }
    }
    return(rc);
}

void socLimiter(std::vector<RSCP_MQTT::cache_t> & c, RscpProtocol *protocol, SRscpValue *rootContainer, bool day_switch) {
    static int charge_locked = 0;
    static int discharge_locked = 0;
    int solar_power = getIntegerValue(c, 0, TAG_EMS_POWER_PV, 0);
    int battery_soc = getIntegerValue(c, 0, TAG_EMS_BAT_SOC, 0);
    int home_power = getIntegerValue(c, 0, TAG_EMS_POWER_HOME, 0);
    int limit_charge_soc = getIntegerValue(c, 0, 0, IDX_LIMIT_CHARGE_SOC);
    int limit_discharge_soc = getIntegerValue(c, 0, 0, IDX_LIMIT_DISCHARGE_SOC);
    int limit_discharge_by_home_power = getIntegerValue(c, 0, 0, IDX_LIMIT_DISCHARGE_BY_HOME_POWER);

    // reset for the next day if durable is false
    if (day_switch) {
        if (!getIntegerValue(c, 0, 0, IDX_LIMIT_CHARGE_DURABLE)) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, IDX_LIMIT_CHARGE_SOC, true);
        if (!getIntegerValue(c, 0, 0, IDX_LIMIT_DISCHARGE_DURABLE)) {
            storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, IDX_LIMIT_DISCHARGE_SOC, true);
            storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, IDX_LIMIT_DISCHARGE_BY_HOME_POWER, true);
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

    if (battery_power < 0) storeStringValue(c, 0, 0, (char *)"DISCHARGING", IDX_BATTERY_STATE);
    else if ((battery_power == 0) && (!battery_soc)) storeStringValue(c, 0, 0, (char *)"EMPTY", IDX_BATTERY_STATE);
    else if ((battery_power == 0) && (battery_soc == 100)) storeStringValue(c, 0, 0, (char *)"FULL", IDX_BATTERY_STATE);
    else if (battery_power == 0) storeStringValue(c, 0, 0, (char *)"PENDING", IDX_BATTERY_STATE);
    else storeStringValue(c, 0, 0, (char *)"CHARGING", IDX_BATTERY_STATE);
    if (grid_power <= 0) storeStringValue(c, 0, 0, (char *)"IN", IDX_GRID_STATE);
    else storeStringValue(c, 0, 0, (char *)"OUT", IDX_GRID_STATE);
    return;
}

void pmSummation(std::vector<RSCP_MQTT::cache_t> & c, int pm_number) {
    float pm_power, pm_energy;

    for (uint8_t i = 0; i < pm_number; i++) {
        pm_power = getFloatValue(c, TAG_PM_DATA, TAG_PM_POWER_L1, i);
        pm_power += getFloatValue(c, TAG_PM_DATA, TAG_PM_POWER_L2, i);
        pm_power += getFloatValue(c, TAG_PM_DATA, TAG_PM_POWER_L3, i);
        pm_energy = getFloatValue(c, TAG_PM_DATA, TAG_PM_ENERGY_L1, i);
        pm_energy += getFloatValue(c, TAG_PM_DATA, TAG_PM_ENERGY_L2, i);
        pm_energy += getFloatValue(c, TAG_PM_DATA, TAG_PM_ENERGY_L3, i);
        storeFloatValue(c, 0, 0, pm_power, i + IDX_PM_POWER, true);
        storeFloatValue(c, 0, 0, pm_energy, i + IDX_PM_ENERGY, true);
    }
    return;
}

void pviString(std::vector<RSCP_MQTT::cache_t> & c, int pvi_number, bool reset) {
    int pvi_energy_start;
    int pvi_energy;

    for (uint8_t i = 0; i < pvi_number; i++) {
        pvi_energy = getIntegerValue(c, TAG_PVI_DC_STRING_ENERGY_ALL, TAG_PVI_VALUE, i);
        if (reset) {
            if (cfg.store_setup) storeSetupItem(0, 0, i + IDX_PVI_ENERGY_START, pvi_energy);
            storeIntegerValue(c, 0, 0, pvi_energy, i + IDX_PVI_ENERGY_START, true);
            storeIntegerValue(c, 0, 0, 0, i + IDX_PVI_ENERGY, true);
        } else {
            pvi_energy_start = getIntegerValue(c, 0, 0, i + IDX_PVI_ENERGY_START);
            storeIntegerValue(c, 0, 0, pvi_energy - pvi_energy_start, i + IDX_PVI_ENERGY, true);
        }
    }
    return;
}

void pviStringNull(std::vector<RSCP_MQTT::cache_t> & c, int pvi_number) {
    int pvi_energy;

    for (uint8_t i = 0; i < pvi_number; i++) {
        pvi_energy = getIntegerValue(c, TAG_PVI_DC_STRING_ENERGY_ALL, TAG_PVI_VALUE, i);
        storeIntegerValue(c, 0, 0, pvi_energy, i + IDX_PVI_ENERGY_START, true);
        storeIntegerValue(c, 0, 0, 0, i + IDX_PVI_ENERGY, true);
    }
    return; 
}

void statisticValues(std::vector<RSCP_MQTT::cache_t> & c, bool reset) {
    static time_t last = 0;
    static int solar_power_max = 0;
    static int home_power_min = getIntegerValue(c, 0, TAG_EMS_POWER_HOME, 0);
    static int home_power_max = 0;
    static int grid_power_min = 0;
    static int grid_power_max = 0;
    static int battery_soc_min = 101;
    static int battery_soc_max = 0;
    static int grid_in_limit_duration = 60 * getIntegerValue(c, 0, 0, IDX_GRID_IN_DURATION);
    static int sun_duration = 60 * getIntegerValue(c, 0, 0, IDX_SUN_DURATION);
    int battery_soc = getIntegerValue(c, 0, TAG_EMS_BAT_SOC, 0);
    int grid_power = getIntegerValue(c, 0, TAG_EMS_POWER_GRID, 0);
    int solar_power = getIntegerValue(c, 0, TAG_EMS_POWER_PV, 0);
    int home_power = getIntegerValue(c, 0, TAG_EMS_POWER_HOME, 0);
    int grid_in_limit = getIntegerValue(c, 0, TAG_EMS_STATUS, 0) & 16;
    time_t now;
    time(&now); 

    if (reset) {
        solar_power_max = storeIntegerValue(c, 0, 0, solar_power - 1, IDX_SOLAR_POWER_MAX, true);
        home_power_min = storeIntegerValue(c, 0, 0, home_power + 1, IDX_HOME_POWER_MIN, true);
        home_power_max = storeIntegerValue(c, 0, 0, home_power - 1, IDX_HOME_POWER_MAX, true);
        grid_power_min = storeIntegerValue(c, 0, 0, grid_power + 1, IDX_GRID_POWER_MIN, true);
        grid_power_max = storeIntegerValue(c, 0, 0, grid_power - 1, IDX_GRID_POWER_MAX, true);
        battery_soc_min = storeIntegerValue(c, 0, 0, battery_soc + 1, IDX_BATTERY_SOC_MIN, true);
        battery_soc_max = storeIntegerValue(c, 0, 0, battery_soc - 1, IDX_BATTERY_SOC_MAX, true);
        grid_in_limit_duration = storeIntegerValue(c, 0, 0, 0, IDX_GRID_IN_DURATION, true);
        sun_duration = storeIntegerValue(c, 0, 0, 0, IDX_SUN_DURATION, true);
    }
    if (solar_power > solar_power_max) {
        solar_power_max = storeIntegerValue(c, 0, 0, solar_power, IDX_SOLAR_POWER_MAX, true);
    }
    if (home_power && (home_power < home_power_min)) {
        home_power_min = storeIntegerValue(c, 0, 0, home_power, IDX_HOME_POWER_MIN, true);
    }
    if (home_power > home_power_max) {
        home_power_max = storeIntegerValue(c, 0, 0, home_power, IDX_HOME_POWER_MAX, true);
    }
    if (grid_power < grid_power_min) {
        grid_power_min = storeIntegerValue(c, 0, 0, grid_power, IDX_GRID_POWER_MIN, true);
    }
    if (grid_power > grid_power_max) {
        grid_power_max = storeIntegerValue(c, 0, 0, grid_power, IDX_GRID_POWER_MAX, true);
    }
    if (battery_soc < battery_soc_min) {
        battery_soc_min = storeIntegerValue(c, 0, 0, battery_soc, IDX_BATTERY_SOC_MIN, true);
    }
    if (battery_soc > battery_soc_max) {
        battery_soc_max = storeIntegerValue(c, 0, 0, battery_soc, IDX_BATTERY_SOC_MAX, true);
    }
    if (grid_in_limit) {
        grid_in_limit_duration += (int)(last?now - last:0);
        storeIntegerValue(c, 0, 0, grid_in_limit_duration / 60, IDX_GRID_IN_DURATION, true);
    }
    if (solar_power) {
        sun_duration += (int)(last?now - last:0);
        storeIntegerValue(c, 0, 0, sun_duration / 60, IDX_SUN_DURATION, true);
    }
    last = now;
    return;
}

void wallboxDailyNull(std::vector<RSCP_MQTT::cache_t> & c, int wb_number, bool reset) {
    for (uint8_t i = 0; i < wb_number; i++) {
        int wallbox_energy_total = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_ALL, i);
        int wallbox_energy_solar = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_SOLAR, i);
        storeIntegerValue(c, 0, 0, wallbox_energy_total, i + IDX_WALLBOX_ENERGY_ALL_START, true);
        if (reset) storeIntegerValue(c, 0, 0, 0, i + IDX_WALLBOX_DAY_ENERGY_ALL, true);
        storeIntegerValue(c, 0, 0, wallbox_energy_solar, i + IDX_WALLBOX_ENERGY_SOLAR_START, true);
        if (reset) storeIntegerValue(c, 0, 0, 0, i + IDX_WALLBOX_DAY_ENERGY_SOLAR, true);
        if (cfg.store_setup) {
            storeSetupItem(0, 0, i + IDX_WALLBOX_ENERGY_ALL_START, wallbox_energy_total);
            storeSetupItem(0, 0, i + IDX_WALLBOX_ENERGY_SOLAR_START, wallbox_energy_solar);
        }
    }
    return;
}

void wallboxDaily(std::vector<RSCP_MQTT::cache_t> & c, int wb_number, bool reset) {
    int wallbox_energy_total;
    int wallbox_energy_solar;
    int energy_start;
    bool correct = false;

    for (uint8_t i = 0; i < wb_number; i++) {
        wallbox_energy_total = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_ALL, i);
        wallbox_energy_solar = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_SOLAR, i);
        if (reset) {
            if (cfg.store_setup) {
                storeSetupItem(0, 0, i + IDX_WALLBOX_ENERGY_ALL_START, wallbox_energy_total);
                storeSetupItem(0, 0, i + IDX_WALLBOX_ENERGY_SOLAR_START, wallbox_energy_solar);
            }
            storeIntegerValue(c, 0, 0, wallbox_energy_total, i + IDX_WALLBOX_ENERGY_ALL_START, true);
            storeIntegerValue(c, 0, 0, 0, i + IDX_WALLBOX_DAY_ENERGY_ALL, true);
            storeIntegerValue(c, 0, 0, wallbox_energy_solar, i + IDX_WALLBOX_ENERGY_SOLAR_START, true);
            storeIntegerValue(c, 0, 0, 0, i + IDX_WALLBOX_DAY_ENERGY_SOLAR, true);
            wb_stat.day_add_total[i] = 0;
            wb_stat.day_total[i] = 0;
            wb_stat.day_add_solar[i] = 0;
            wb_stat.day_solar[i] = 0;
        } else {
            energy_start = getIntegerValue(c, 0, 0, i + IDX_WALLBOX_ENERGY_ALL_START);
            if (wallbox_energy_total - energy_start >= wb_stat.day_total[i]) {
                storeIntegerValue(c, 0, 0, wb_stat.day_add_total[i] + wallbox_energy_total - energy_start, i + IDX_WALLBOX_DAY_ENERGY_ALL, true);
                wb_stat.day_total[i] = wallbox_energy_total - energy_start; 
            } else {
                correct = true;
                wb_stat.day_add_total[i] += wb_stat.day_total[i];
                wb_stat.day_total[i] = 0;
            }
            energy_start = getIntegerValue(c, 0, 0, i + IDX_WALLBOX_ENERGY_SOLAR_START);
            if (wallbox_energy_solar - energy_start >= wb_stat.day_solar[i]) {
                storeIntegerValue(c, 0, 0, wb_stat.day_add_solar[i] + wallbox_energy_solar - energy_start, i + IDX_WALLBOX_DAY_ENERGY_SOLAR, true);
                wb_stat.day_solar[i] = wallbox_energy_solar - energy_start;
            } else {
                correct = true;
                wb_stat.day_add_solar[i] += wb_stat.day_solar[i];
                wb_stat.day_solar[i] = 0;
            }
        }
        if (correct) wallboxDailyNull(c, wb_number, false);
    }
    return;
}

void wallboxLastCharging(std::vector<RSCP_MQTT::cache_t> & c, int wb_number) {
    static bool start = true;
    int wallbox_energy_total;
    int wallbox_energy_solar;
    int wallbox_plugged;

    for (uint8_t i = 0; i < wb_number; i++) {
        if (start) {
            wb_stat.last_wallbox_energy_total_start[i] = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_ALL, i);
            wb_stat.last_wallbox_energy_solar_start[i] = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_SOLAR, i);
        }
        wallbox_energy_total = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_ALL, i);
        wallbox_energy_solar = getIntegerValue(c, TAG_WB_DATA, TAG_WB_ENERGY_SOLAR, i);
        wallbox_plugged = getIntegerValue(c, TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2 + i * 10) & 8;
        if (!wb_stat.last_wallbox_plugged_last[i] && wallbox_plugged) {
            storeIntegerValue(c, 0, 0, 0, i + IDX_WALLBOX_LAST_ENERGY_ALL, true);
            storeIntegerValue(c, 0, 0, 0, i + IDX_WALLBOX_LAST_ENERGY_SOLAR, true);
            wb_stat.last_wallbox_energy_total_start[i] = wallbox_energy_total;
            wb_stat.last_wallbox_energy_solar_start[i] = wallbox_energy_solar;
            wb_stat.last_add_total[i] = 0;
            wb_stat.last_add_solar[i] = 0;
            wb_stat.last_diff_total[i] = 0;
            wb_stat.last_diff_solar[i] = 0;
        }
        if ((wallbox_energy_total - wb_stat.last_wallbox_energy_total_start[i] < 0) || !wallbox_energy_total) {
            wb_stat.last_wallbox_energy_total_start[i] = wallbox_energy_total;
            wb_stat.last_add_total[i] = wb_stat.last_diff_total[i];
            wb_stat.last_diff_total[i] = 0;
        }
        if ((wallbox_energy_solar - wb_stat.last_wallbox_energy_solar_start[i] < 0) || !wallbox_energy_solar) {
            wb_stat.last_wallbox_energy_solar_start[i] = wallbox_energy_solar;
            wb_stat.last_add_solar[i] = wb_stat.last_diff_solar[i];
            wb_stat.last_diff_solar[i] = 0;
        }
        if (wb_stat.last_wallbox_plugged_last[i] != wallbox_plugged) {
            wb_stat.last_wallbox_plugged_last[i] = wallbox_plugged;
        }
        if (wallbox_energy_total > (wb_stat.last_wallbox_energy_total_start[i] + wb_stat.last_diff_total[i] - wb_stat.last_add_total[i])) {
            wb_stat.last_diff_total[i] = storeIntegerValue(c, 0, 0, wb_stat.last_add_total[i] + wallbox_energy_total - wb_stat.last_wallbox_energy_total_start[i], i + IDX_WALLBOX_LAST_ENERGY_ALL, true);
        }
        if (wallbox_energy_solar > (wb_stat.last_wallbox_energy_solar_start[i] + wb_stat.last_diff_solar[i] - wb_stat.last_add_solar[i])) {
            wb_stat.last_diff_solar[i] = storeIntegerValue(c, 0, 0, wb_stat.last_add_solar[i] + wallbox_energy_solar - wb_stat.last_wallbox_energy_solar_start[i], i + IDX_WALLBOX_LAST_ENERGY_SOLAR, true);
        }
    }
    if (start) start = false;
    return;
}

void createRequest(SRscpFrameBuffer * frameBuffer) {
    RscpProtocol protocol;
    SRscpValue rootValue;

    // The root container is create with the TAG ID 0 which is not used by any device.
    protocol.createContainerValue(&rootValue, 0);

    SRscpTimestamp start, interval, span;

    char buffer[64];
    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);
    int day_iteration;

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", l);

    day_end = false;
    if ((l->tm_hour == 23) && (l->tm_min >= (59 - cfg.interval / 60))) day_end = true;

    new_day = false;
    if (curr_day != l->tm_mday) {
        curr_day = l->tm_mday;
        new_day = true;
    }

    if (curr_year < l->tm_year + 1900) {
        addTemplTopics(TAG_DB_HISTORY_DATA_YEAR, 1, NULL, curr_year, l->tm_year + 1900, 0, 0, true);
        curr_year = l->tm_year + 1900;
    } 

    l->tm_sec = 0;
    l->tm_min = 0;
    l->tm_hour = 0;
    l->tm_isdst = -1;

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
        if (cfg.ems_requests) {
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
        }

        if (tags_added) {
            for (std::vector<RSCP_MQTT::additional_tags_t>::iterator it = RSCP_MQTT::AdditionalTags.begin(); it != RSCP_MQTT::AdditionalTags.end(); ++it) {
                if ((it->req_container == 0) && ((it->one_shot == false) || (it->one_shot && !e3dc_ts))) {
                    if (it->req_index >= 0) protocol.appendValue(&rootValue, it->req_tag, it->req_index);
                    else protocol.appendValue(&rootValue, it->req_tag);
                }
            }
        }

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
            protocol.appendValue(&batteryContainer, TAG_BAT_REQ_RSOC_REAL);
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
                    protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DCB_ALL_CELL_TEMPERATURES, j); // Issue #103
                    protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DCB_ALL_CELL_VOLTAGES, j); // Issue #103
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
            for (uint8_t i = 0; i < cfg.pm_number; i++) {
                protocol.createContainerValue(&PMContainer, TAG_PM_REQ_DATA);
                protocol.appendValue(&PMContainer, TAG_PM_INDEX, (uint8_t)cfg.pm_indexes[i]);
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
        }

        // request PVI information
        if (cfg.pvi_requests) {
            SRscpValue PVIContainer;
            protocol.createContainerValue(&PVIContainer, TAG_PVI_REQ_DATA);
            protocol.appendValue(&PVIContainer, TAG_PVI_INDEX, (uint8_t)0);
            if (cfg.pvi_tracker == 0) {
                protocol.appendValue(&PVIContainer, TAG_PVI_REQ_USED_STRING_COUNT); // auto detection
            }
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
            for (uint8_t i = 0; i < cfg.wb_number; i++) {
                protocol.createContainerValue(&WBContainer, TAG_WB_REQ_DATA) ;
                protocol.appendValue(&WBContainer, TAG_WB_INDEX, (uint8_t)cfg.wb_indexes[i]);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_SOC);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_ACTIVE_PHASES);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_EXTERN_DATA_ALG);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_NUMBER_PHASES);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_KEY_STATE);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_POWER_L1);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_POWER_L2);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_POWER_L3);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_ENERGY_ALL);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_ENERGY_SOLAR);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_ENERGY_L1);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_ENERGY_L2);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_PM_ENERGY_L3);
                protocol.appendValue(&WBContainer, TAG_WB_REQ_MIN_CHARGE_CURRENT); // Issue #84
                protocol.appendValue(&rootValue, WBContainer);
                protocol.destroyValueData(WBContainer);
            }
        }

        // request db history information
        if (cfg.hst_requests && e3dc_ts) {
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
                if ((x.year >= cfg.history_start_year) && (x.year <= curr_year) && (x.month > 0) && (x.month <= 12) && (x.day > 0) && (x.day <= (RSCP_MQTT::nr_of_days[x.month - 1] + leap_day))) {
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

        if (tags_added) {
            uint32_t container = 0;
            SRscpValue ReqContainer;
            for (std::vector<RSCP_MQTT::additional_tags_t>::iterator it = RSCP_MQTT::AdditionalTags.begin(); it != RSCP_MQTT::AdditionalTags.end(); ++it) {
                if ((it->req_container != 0) && ((it->one_shot == false) || (it->one_shot && !e3dc_ts))) {
                    if (container && (it->req_container != container)) {
                        protocol.appendValue(&rootValue, ReqContainer);
                        protocol.destroyValueData(ReqContainer);
                    }
                    if (it->req_container && (it->req_container != container)) {
                        protocol.createContainerValue(&ReqContainer, it->req_container);
                        container = it->req_container;
                    }
                    if (it->req_index >= 0) protocol.appendValue(&ReqContainer, it->req_tag, it->req_index);
                    else protocol.appendValue(&ReqContainer, it->req_tag);
                }
            }
            if (container) {
                protocol.appendValue(&rootValue, ReqContainer);
                protocol.destroyValueData(ReqContainer);
            }
        }

        // handle incoming MQTT requests
        if (mqttRcvd || cfg.auto_refresh) {
            mqttRcvd = false;
            uint32_t container = 0;
            SRscpValue ReqContainer;
            int sun_mode = 0;
            int max_current = -1;
            time_t now;
            time(&now);

            for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = RSCP_MQTT::RscpMqttReceiveCache.begin(); it != RSCP_MQTT::RscpMqttReceiveCache.end(); ++it) {
                if ((it->done == false) || (it->refresh_until > now)) {
                    if (!it->container && !it->tag) { //system call
                        if (!strcmp(it->topic, "set/log") || !strcmp(it->topic, "set/log/cache")) logCache(RSCP_MQTT::RscpMqttCache, cfg.logfile);
                        if (!strcmp(it->topic, "set/log/rcache")) logRecCache(RSCP_MQTT::RscpMqttReceiveCache, cfg.logfile);
                        if (!strcmp(it->topic, "set/log/errors")) logMessageCache(cfg.logfile, false);
                        if (!strcmp(it->topic, "set/health")) logHealth(cfg.logfile);
                        if (!strcmp(it->topic, "set/force")) refreshCache(RSCP_MQTT::RscpMqttCache, it->payload);
                        if ((!strcmp(it->topic, "set/interval")) && (atoi(it->payload) >= DEFAULT_INTERVAL_MIN) && (atoi(it->payload) <= DEFAULT_INTERVAL_MAX)) cfg.interval = atoi(it->payload);
                        if ((!strcmp(it->topic, "set/power_mode")) && cfg.auto_refresh) handleSetPower(RSCP_MQTT::RscpMqttReceiveCache, TAG_EMS_REQ_SET_POWER, it->payload);
                        if (!strcmp(it->topic, "set/limit/charge/soc")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), IDX_LIMIT_CHARGE_SOC, true);
                        if (!strcmp(it->topic, "set/limit/discharge/soc")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), IDX_LIMIT_DISCHARGE_SOC, true);
                        if (!strcmp(it->topic, "set/limit/charge/durable")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), IDX_LIMIT_CHARGE_DURABLE, true);
                        if (!strcmp(it->topic, "set/limit/discharge/durable")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), IDX_LIMIT_DISCHARGE_DURABLE, true);
                        if (!strcmp(it->topic, "set/limit/discharge/by_home_power")) storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(it->payload), IDX_LIMIT_DISCHARGE_BY_HOME_POWER, true);
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
                        if (!strcmp(it->topic, "set/requests/ems")) {
                            if (!strcmp(it->payload, "true")) cfg.ems_requests = true; else cfg.ems_requests = false;
                        }
                        if (!strcmp(it->topic, "set/requests/history")) {
                            if (!strcmp(it->payload, "true")) cfg.hst_requests = true; else cfg.hst_requests = false;
                        }
                        if (!strcmp(it->topic, "set/soc_limiter")) {
                            if (!strcmp(it->payload, "true")) cfg.soc_limiter = true; else cfg.soc_limiter = false;
                        }
                        if (!strcmp(it->topic, "set/raw_mode")) {
                            if (!strcmp(it->payload, "true")) cfg.raw_mode = true; else cfg.raw_mode = false;
                        }
                        if (!strcmp(it->topic, "set/daily_values")) {
                            if (!strcmp(it->payload, "true")) cfg.daily_values = true; else cfg.daily_values = false;
                        }
                        if (!strcmp(it->topic, "set/statistic_values")) {
                            if (!strcmp(it->payload, "true")) cfg.statistic_values = true; else cfg.statistic_values = false;
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
                        if (it->container == TAG_WB_REQ_DATA) protocol.appendValue(&ReqContainer, TAG_WB_INDEX, (uint8_t)cfg.wb_indexes[it->index]);
                        container = it->container;
                    }
                    // Wallbox
                    if (cfg.wallbox && (container == TAG_WB_REQ_DATA) && (it->tag == TAG_WB_EXTERN_DATA)) {
                        uint8_t wallboxExt[6];

                        if (max_current < 0) {
                            sun_mode = getIntegerValue(RSCP_MQTT::RscpMqttCache, TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2 + it->index * 10) & 128;
                            max_current = getIntegerValue(RSCP_MQTT::RscpMqttCache, TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 3 + it->index * 10);
                        }
                        for (size_t i = 0; i < sizeof(wallboxExt); ++i) wallboxExt[i] = 0;
                        if (strstr(it->topic, "toggle")) {
                            wallboxExt[4] = 1;
                        } else if (strstr(it->topic, "suspended")) {
                            int suspended = getIntegerValue(RSCP_MQTT::RscpMqttCache, TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2 + it->index * 10) & 64;
                            if ((!suspended && !strcmp(it->payload, "1")) || (suspended && !strcmp(it->payload, "0"))) {
                                wallboxExt[4] = 1;
                            }
                        } else if (strstr(it->topic, "sun_mode")) {
                            sun_mode = (!strcmp(it->payload, "1"))?1:0;
                        } else if (strstr(it->topic, "max_current")) {
                            max_current = atoi(it->payload);
                        }
                        wallboxExt[0] = sun_mode?1:2;
                        wallboxExt[1] = max_current;

                        if (wallboxExt[1] < 1) wallboxExt[1] = 1; else if (wallboxExt[1] > 32) wallboxExt[1] = 32; // max 32A
                        SRscpValue WBExtContainer;
                        protocol.createContainerValue(&WBExtContainer, TAG_WB_REQ_SET_EXTERN);
                        protocol.appendValue(&WBExtContainer, TAG_WB_EXTERN_DATA_LEN, (int)sizeof(wallboxExt));
                        protocol.appendValue(&WBExtContainer, TAG_WB_EXTERN_DATA, wallboxExt, sizeof(wallboxExt));
                        protocol.appendValue(&ReqContainer, WBExtContainer);
                        protocol.destroyValueData(WBExtContainer);
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

        if (cfg.soc_limiter) socLimiter(RSCP_MQTT::RscpMqttCache, &protocol, &rootValue, day_end);
    }

    // create buffer frame to send data to the S10
    protocol.createFrameAsBuffer(frameBuffer, rootValue.data, rootValue.length, true); // true to calculate CRC on for transfer
    // the root value object should be destroyed after the data is copied into the frameBuffer and is not needed anymore
    protocol.destroyValueData(rootValue);

    return;
}

void publishRaw(RscpProtocol *protocol, SRscpValue *response, char *topic_in) {
    char topic[TOPIC_SIZE];
    char *payload = (char *)malloc(PAYLOAD_SIZE * sizeof(char) + 1);
    bool changed = false;
    if (payload) {
        memset(payload, 0, PAYLOAD_SIZE);
        preparePayload(protocol, response, &payload);
    
        int nr = mergeRawData(topic_in, payload, &changed);
        if (nr > 0) {
            if (snprintf(topic, TOPIC_SIZE, "%s/%d", topic_in, nr) >= TOPIC_SIZE) {
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"publishRaw: Buffer overflow\n");
                return;
            }
            if (changed) publishImmediately(topic, payload, false);
        } else if (changed) publishImmediately(topic_in, payload, false);
        free(payload);
    }
    return;
}

void handleRaw(RscpProtocol *protocol, SRscpValue *response, uint32_t *cache, int level) {
    int l = level + 1;
    char topic[TOPIC_SIZE];
    memset(topic, 0, sizeof(topic));

    if (response->dataType == RSCP::eTypeError) return;

    if (!l && (response->dataType != RSCP::eTypeContainer)) {
        if (snprintf(topic, TOPIC_SIZE, "raw/%s", tagName(RSCP_TAGS::RscpTagsOverview, response->tag)) >= TOPIC_SIZE) {
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"handleRaw: Buffer overflow\n");
            return;
        }
        if (!cfg.raw_topic_regex || std::regex_match(topic, std::regex(cfg.raw_topic_regex))) publishRaw(protocol, response, topic);
        return;
    }
    std::vector<SRscpValue> data = protocol->getValueAsContainer(response);
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].dataType != RSCP::eTypeError) {
            cache[l] = response->tag;
            if (data[i].dataType == RSCP::eTypeContainer) {
                if (l < RECURSION_MAX_LEVEL) handleRaw(protocol, &data[i], cache, l);
                else logMessageByTag(response->tag, data[i].tag, 0, __LINE__, (char *)"Error: Next recursion level not possible. Container >%s< Tag >%s< [%d]\n");
            } else {
                cache[l + 1] = data[i].tag;
                strcpy(topic, "raw");
                for (int i = 0; i <= l + 1; i++) {
                    if (cache[i]) {
                        if (strlen(topic) + strlen(tagName(RSCP_TAGS::RscpTagsOverview, cache[i])) + 2 < TOPIC_SIZE) {
                            strcat(topic, "/");
                            strcat(topic, tagName(RSCP_TAGS::RscpTagsOverview, cache[i]));
                        } else logMessageByTag(response->tag, data[i].tag, 0, __LINE__, (char *)"Error: Topic name too long. Container >%s< Tag >%s< [%d]\n");
                    }
                }
                if (!cfg.raw_topic_regex || std::regex_match(topic, std::regex(cfg.raw_topic_regex))) publishRaw(protocol, &data[i], topic);
            }
        }
    }
    protocol->destroyValueData(data);
    return;
}

void handleContainer(RscpProtocol *protocol, SRscpValue *response, uint32_t *cache, int level) {
    int l = level + 1;

    if ((response->dataType == RSCP::eTypeError) && (response->dataType != RSCP::eTypeContainer)) return;
    std::vector<SRscpValue> data = protocol->getValueAsContainer(response);
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].dataType != RSCP::eTypeError) {
            cache[l] = response->tag;
            if (data[i].dataType == RSCP::eTypeContainer) {
                if (l < RECURSION_MAX_LEVEL) handleContainer(protocol, &data[i], cache, l);
                else logMessageByTag(response->tag, data[i].tag, 0, __LINE__, (char *)"Error: Next recursion level not possible. Container >%s< Tag >%s< [%d]\n");
            } else {
                cache[l + 1] = data[i].tag;
                storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &data[i], cache[l], 0);
            }
        }
    }
    protocol->destroyValueData(data);
    return;
}

int handleResponseValue(RscpProtocol *protocol, SRscpValue *response) {
    RSCP_MQTT::date_t x;
    uint32_t cache[RECURSION_MAX_LEVEL];

    x.year = 0;
    x.month = 0;
    x.day = 0;

    // check if any of the response has the error flag set and react accordingly
    if (response->dataType == RSCP::eTypeError) {
        // handle error for example access denied errors
        uint32_t uiErrorCode = protocol->getValueAsUInt32(response);
        logMessageByTag(0, response->tag, uiErrorCode, __LINE__, (char *)"Error: Container/Tag >%s< received >%s< [%d]\n");
        if ((response->tag == TAG_DB_HISTORY_DATA_YEAR) && (cfg.history_start_year < curr_year) && (year < curr_year)) {
            cfg.history_start_year++;
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"History start year has been corrected >%d<\n", cfg.history_start_year);
        } else if ((response->tag == TAG_BAT_DATA) && (uiErrorCode == 6) && (cfg.battery_strings > 1)) { // Battery string not available
            cfg.battery_strings = cfg.battery_strings - 1;
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Number of battery strings has been corrected >%d<\n", cfg.battery_strings);
        } else if (uiErrorCode == 6) pushNotSupportedTag(response->tag, 0);
        return(-1);
    }

    if (cfg.raw_mode) {
        if (mosq) handleRaw(protocol, response, cache, -1);
    }

    // check the SRscpValue TAG to detect which response it is
    switch (response->tag) {
    case TAG_RSCP_AUTHENTICATION: {
        // It is possible to check the response->dataType value to detect correct data type
        // and call the correct function. If data type is known,
        // the correct function can be called directly like in this case.
        uint8_t ucAccessLevel = protocol->getValueAsUChar8(response);
        if (ucAccessLevel > 0) {
            iAuthenticated = 1;
            if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"RSCP authentication level %i\n", ucAccessLevel);
        } else logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"RSCP authentication failed. Please check your configuration (E3DC_USER, E3DC_PASSWORD).\n");
        break;
    }
    case TAG_INFO_TIME: {
        time_t now;
        time(&now);
        e3dc_ts = protocol->getValueAsInt32(response);
        e3dc_diff = (e3dc_ts - now + 60) / 3600;
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
                logMessageByTag(response->tag, containerData[i].tag, uiErrorCode, __LINE__, (char *)"Error: Container >%s< Tag >%s< received >%s< [%d]\n");
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, containerData[i].tag);
            } else {
                switch (containerData[i].tag) {
                    case TAG_BAT_DCB_INFO: {
                        int dcb_nr = 0;
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
                        cfg.bat_dcb_count[battery_nr] = protocol->getValueAsUChar8(&containerData[i]);
                        cfg.bat_dcb_start[battery_nr] = battery_nr?cfg.bat_dcb_count[battery_nr - 1]:0;
                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                        addTemplTopics(TAG_BAT_DCB_INFO, 1, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1, 0, true);
                        if (cfg.bat_dcb_cell_voltages) addTemplTopics(TAG_BAT_DCB_ALL_CELL_VOLTAGES, 1, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1, cfg.bat_dcb_cell_voltages, true);
                        if (cfg.bat_dcb_cell_temperatures) addTemplTopics(TAG_BAT_DCB_ALL_CELL_TEMPERATURES, 1, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1, cfg.bat_dcb_cell_temperatures, true);
                        addTemplTopicsIdx(IDX_BATTERY_DCB_TEMPERATURE_MIN, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1, false);
                        addTemplTopicsIdx(IDX_BATTERY_DCB_TEMPERATURE_MAX, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1, false);
                        addTemplTopicsIdx(IDX_BATTERY_DCB_CELL_VOLTAGE, NULL, cfg.bat_dcb_start[battery_nr], cfg.bat_dcb_start[battery_nr] + cfg.bat_dcb_count[battery_nr], 1, true);
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
                    case TAG_BAT_DCB_ALL_CELL_VOLTAGES:
                    case TAG_BAT_DCB_ALL_CELL_TEMPERATURES: {
                        int dcb_nr = 0;
                        std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                        for (size_t j = 0; j < container.size(); j++) {
                            if (container[j].tag == TAG_BAT_DCB_INDEX) {
                                dcb_nr = protocol->getValueAsUInt16(&container[j]) + cfg.bat_dcb_start[battery_nr];
                            } else {
                                if (container[j].dataType == RSCP::eTypeContainer) {
                                    std::vector<SRscpValue> subcontainer = protocol->getValueAsContainer(&container[j]);
                                    if (containerData[i].tag == TAG_BAT_DCB_ALL_CELL_TEMPERATURES) {
                                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(subcontainer[0]), 0, IDX_BATTERY_DCB_TEMPERATURE_MIN + dcb_nr);
                                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(subcontainer[1]), 0, IDX_BATTERY_DCB_TEMPERATURE_MAX + dcb_nr);
                                    } else if (containerData[i].tag == TAG_BAT_DCB_ALL_CELL_VOLTAGES) {
                                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(subcontainer[0]), 0, IDX_BATTERY_DCB_CELL_VOLTAGE + dcb_nr);
                                    }
                                    if (cfg.bat_dcb_cell_voltages || cfg.bat_dcb_cell_temperatures) {
                                        for (size_t k = 0; k < subcontainer.size(); k++) {
                                            if (storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(subcontainer[k]), containerData[i].tag, dcb_nr) < 0) break;
                                        }
                                    }
                                    protocol->destroyValueData(subcontainer); // Issue #107
                                }
                            }
                        }
                        protocol->destroyValueData(container);
                        break;
                    }
                    default: {
                        if (response->tag == TAG_BAT_DATA) storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, battery_nr);
                        else storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, (response->tag == TAG_PM_DATA)?pm_nr:0);
                    }
                }
            }
        }
        if (response->tag == TAG_BAT_DATA) battery_nr++;
        if (response->tag == TAG_PM_DATA) pm_nr++;
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_WB_DATA: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                logMessageByTag(response->tag, containerData[i].tag, uiErrorCode, __LINE__, (char *)"Error: Container >%s< Tag >%s< received >%s< [%d]\n");
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, containerData[i].tag);
            } else if (containerData[i].tag == TAG_WB_EXTERN_DATA_ALG) {
                std::vector<SRscpValue> wallboxContent = protocol->getValueAsContainer(&containerData[i]);
                for (size_t j = 0; j < wallboxContent.size(); ++j) {
                    if (wallboxContent[j].dataType == RSCP::eTypeError) {
                         uint32_t uiErrorCode = protocol->getValueAsUInt32(&wallboxContent[j]);
                         logMessageByTag(0, wallboxContent[j].tag, uiErrorCode, __LINE__, (char *)"Error: Tag >%s< received >%s< [%d]\n");
                         if (uiErrorCode == 6) pushNotSupportedTag(response->tag, wallboxContent[i].tag);
                    } else {
                        switch (wallboxContent[j].tag) {
                            case TAG_WB_EXTERN_DATA: {
                                uint8_t wallboxExt[8];
                                memcpy(&wallboxExt, &wallboxContent[j].data[0], sizeof(wallboxExt));
                                for (size_t b = 0; b < sizeof(wallboxExt); ++b) {
                                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, wallboxExt[b], b + wb_nr * 10, false);
                                }
                            }
                         }
                     }
                 }
            } else {
                storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, wb_nr);
            }
        }
        wb_nr++;
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_PVI_DATA: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                if (uiErrorCode == 6) {
                    pushNotSupportedTag(response->tag, containerData[i].tag);
                } else logMessageByTag(response->tag, containerData[i].tag, uiErrorCode, __LINE__, (char *)"Error: Container >%s< Tag >%s< received >%s< [%d]\n");
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
                    int tracker = 0;
                    std::vector<SRscpValue> container = protocol->getValueAsContainer(&containerData[i]);
                    for (size_t j = 0; j < container.size(); j++) {
                        if (container[j].tag == TAG_PVI_INDEX) {
                            tracker = protocol->getValueAsUInt16(&container[j]);
                        }
                        else if (container[j].tag == TAG_PVI_VALUE) {
                            storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(container[j]), containerData[i].tag, tracker);
                        }
                    }
                    protocol->destroyValueData(container);
                    break;
                }
                case TAG_PVI_TEMPERATURE_COUNT: {
                    cfg.pvi_temp_count = protocol->getValueAsUChar8(&containerData[i]);
                    storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                    addTemplTopics(TAG_PVI_TEMPERATURE, 1, NULL, 0, cfg.pvi_temp_count, 1, 0, true);
                    break;
                }
                case TAG_PVI_USED_STRING_COUNT: {
                    cfg.pvi_tracker = protocol->getValueAsUChar8(&containerData[i]);
                    addTemplTopics(TAG_PVI_DC_POWER, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
                    addTemplTopics(TAG_PVI_DC_VOLTAGE, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
                    addTemplTopics(TAG_PVI_DC_CURRENT, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
                    addTemplTopics(TAG_PVI_DC_STRING_ENERGY_ALL, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
                    addTemplTopicsIdx(IDX_PVI_ENERGY, NULL, 0, cfg.pvi_tracker, 1, false);
                    addTemplTopicsIdx(IDX_PVI_ENERGY_START, NULL, 0, cfg.pvi_tracker, 1, true);
                    storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), response->tag, 0);
                    logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Number of PVI strings: %d (auto detection)\n", cfg.pvi_tracker);
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
                logMessageByTag(response->tag, containerData[i].tag, uiErrorCode, __LINE__, (char *)"Error: Container >%s< Tag >%s< received >%s< [%d]\n");
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, containerData[i].tag);
            } else {
                switch (containerData[i].tag) {
                    case TAG_EMS_IDLE_PERIOD: {
                        uint8_t period = 0;
                        uint8_t type = 0;
                        uint8_t active = 0;
                        uint8_t starthour = 0;
                        uint8_t startminute = 0;
                        uint8_t endhour = 0;
                        uint8_t endminute = 0;
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
                logMessageByTag(response->tag, historyData[i].tag, uiErrorCode, __LINE__, (char *)"Error: Container >%s< Tag >%s< received >%s< [%d]\n");
                if (uiErrorCode == 6) pushNotSupportedTag(response->tag, 0);
            } else {
                switch (historyData[i].tag) {
                case TAG_DB_SUM_CONTAINER: {
                    std::vector<SRscpValue> dbData = protocol->getValueAsContainer(&(historyData[i]));
                    for (size_t j = 0; j < dbData.size(); ++j) {
                        if (response->tag == TAG_DB_HISTORY_DATA_DAY) {
                            if ((day >= 2) && x.year) {
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
        if (response->dataType == RSCP::eTypeContainer) {
            handleContainer(protocol, response, cache, -1);
        } else storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, response, 0, 0);
        break;
    }
    return(0);
}

static int processReceiveBuffer(const unsigned char * ucBuffer, int iLength) {
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
    pm_nr = 0;
    wb_nr = 0;
    if (cfg.raw_mode) initRawData();
    for (size_t i = 0; i < frame.data.size(); i++)
        handleResponseValue(&protocol, &frame.data[i]);

    // destroy frame data and free memory
    protocol.destroyFrameData(frame);

    // returned processed amount of bytes
    return(iResult);
}

static void receiveLoop(bool & bStopExecution) {
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
    int iResult;

    while (!bStopExecution && ((iReceivedBytes > 0) || iReceivedRscpFrames == 0)) {
        // check and expand buffer
        if ((vecDynamicBuffer.size() - iReceivedBytes) < 4096) {
            // check maximum size
            if (vecDynamicBuffer.size() > RSCP_MAX_FRAME_LENGTH) {
                // something went wrong and the size is more than possible by the RSCP protocol
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Maximum buffer size exceeded %zu\n", vecDynamicBuffer.size());
                vecDynamicBuffer.resize(0); // Issue #55
                iReceivedBytes = 0; // Issue #55
                bStopExecution = true;
                break;
            }
            // increase buffer size by 4096 bytes each time the remaining size is smaller than 4096
            vecDynamicBuffer.resize(vecDynamicBuffer.size() + 4096);
        }
        // receive data
        // Issues #55, #61 and #67
        while (1) {
            errno = 0; // Issue #87
            iResult = SocketRecvData(iSocket, &vecDynamicBuffer[0] + iReceivedBytes, vecDynamicBuffer.size() - iReceivedBytes);
            if (iResult < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) continue;
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Socket receive error. errno %i\n", errno);
                bStopExecution = true;
                vecDynamicBuffer.resize(0); // Issue #87
                iReceivedBytes = 0; // Issue #87
                break;
            } else if (iResult == 0) {
                // connection was closed regularly by peer
                // if this happens on startup each time the possible reason is
                // wrong AES password or wrong network subnet (adapt hosts.allow file required)
                logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Connection closed by peer. Perhaps E3DC_AES_PASSWORD is not configured correctly.\n");
                bStopExecution = true;
                vecDynamicBuffer.resize(0); // Issue #87
                iReceivedBytes = 0; // Issue #87
                break;
            }
            break;
        }

        // increment amount of received bytes
        if (!bStopExecution) iReceivedBytes += iResult; // Issue #87

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
                vecDynamicBuffer.resize(0); // Issue #87
                iReceivedBytes = 0; // Issue #87
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
    return;
}

static void mainLoop(void) {
    RscpProtocol protocol;
    bool bStopExecution = false;
    struct timeval start, end;
    double elapsed;
    int countdown = 3;

    while (go && !bStopExecution) {
        //--------------------------------------------------------------------------------------------------------------
        // RSCP Transmit Frame Block Data
        //--------------------------------------------------------------------------------------------------------------
        SRscpFrameBuffer frameBuffer;
        memset(&frameBuffer, 0, sizeof(frameBuffer));

        gettimeofday(&start, NULL);

        resetHandleFlag(RSCP_MQTT::RscpMqttCache);

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

        if (bStopExecution) return; // Issue #87

        if (countdown <= 0) {
            classifyValues(RSCP_MQTT::RscpMqttCache);
            if (cfg.pm_requests) pmSummation(RSCP_MQTT::RscpMqttCache, cfg.pm_number);
            if (cfg.pvi_requests) pviString(RSCP_MQTT::RscpMqttCache, cfg.pvi_tracker, new_day);
            if (cfg.wallbox) wallboxDaily(RSCP_MQTT::RscpMqttCache, cfg.wb_number, new_day);
            if (cfg.statistic_values) statisticValues(RSCP_MQTT::RscpMqttCache, new_day);
            if (cfg.wallbox) wallboxLastCharging(RSCP_MQTT::RscpMqttCache, cfg.wb_number);
            if (new_day && cfg.log_level) logMessageCache(cfg.logfile, true);
        }

        // MQTT connection
        if (!mosq) {
            if (strcmp(cfg.mqtt_client_id, ""))
                mosq = mosquitto_new(cfg.mqtt_client_id, true, NULL);
            else
                mosq = mosquitto_new(NULL, true, NULL);
            if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Connecting to broker %s:%u\n", cfg.mqtt_host, cfg.mqtt_port);
            if (mosq) {
                char topic[TOPIC_SIZE];
                snprintf(topic, TOPIC_SIZE, "%s/rscp2mqtt/status", cfg.prefix);
                // mosquitto_threaded_set(mosq, true); // necessary?
                if (cfg.mqtt_tls && cfg.mqtt_tls_password) {
                    if (mosquitto_tls_set(mosq, cfg.mqtt_tls_cafile, cfg.mqtt_tls_capath, cfg.mqtt_tls_certfile, cfg.mqtt_tls_keyfile, mqttCallbackTlsPassword) != MOSQ_ERR_SUCCESS) {
                        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Unable to set TLS options.\n");
                    }
                } else if (cfg.mqtt_tls) {
                    if (mosquitto_tls_set(mosq, cfg.mqtt_tls_cafile, cfg.mqtt_tls_capath, cfg.mqtt_tls_certfile, cfg.mqtt_tls_keyfile, NULL) != MOSQ_ERR_SUCCESS) {
                        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: Unable to set TLS options.\n");
                    }
                }
                mosquitto_connect_callback_set(mosq, (void (*)(mosquitto*, void*, int))mqttCallbackOnConnect);
                mosquitto_message_callback_set(mosq, (void (*)(mosquitto*, void*, const mosquitto_message*))mqttCallbackOnMessage);
                if (cfg.mqtt_auth && strcmp(cfg.mqtt_user, "") && strcmp(cfg.mqtt_password, "")) mosquitto_username_pw_set(mosq, cfg.mqtt_user, cfg.mqtt_password);
                mosquitto_will_set(mosq, topic, strlen("disconnected"), "disconnected", cfg.mqtt_qos, cfg.mqtt_retain);
                if (!mosquitto_connect(mosq, cfg.mqtt_host, cfg.mqtt_port, 10)) {
                    if (!cfg.once) {
                        std::thread th(mqttListener, mosq);
                        th.detach();
                    }
                    if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Success: MQTT broker connected.\n");
                } else {
                    logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: MQTT broker connection failed.\n");
                    mosquitto_destroy(mosq);
                    mosq = NULL;
                }
                if (mosq) {
                    mosquitto_publish(mosq, NULL, topic, strlen("connected"), "connected", cfg.mqtt_qos, cfg.mqtt_retain);
                }
            }
        }

        // MQTT publish
        if (mosq) {
            if (handleMQTT(RSCP_MQTT::RscpMqttCache, cfg.mqtt_qos, cfg.mqtt_retain)) {
                if (mosquitto_reconnect(mosq) == MOSQ_ERR_SUCCESS) {
                    logMessageByTag(0, 0, 0, __LINE__, (char *)"Message: MQTT broker successfully re-connected. [%d]\n");
                } else {
                    logMessageByTag(0, 0, 0, __LINE__, (char *)"Error: MQTT broker re-connection failed. [%d]\n");
                }
            }
            if (handleMQTTIdlePeriods(RSCP_MQTT::IdlePeriodCache, cfg.mqtt_qos, false)) {
                if (mosquitto_reconnect(mosq) == MOSQ_ERR_SUCCESS) {
                    logMessageByTag(0, 0, 0, __LINE__, (char *)"Message: MQTT broker successfully re-connected. [%d]\n");
                } else {
                    logMessageByTag(0, 0, 0, __LINE__, (char *)"Error: MQTT broker re-connection failed. [%d]\n");
                }
            }
            if (handleMQTTErrorMessages(RSCP_MQTT::ErrorCache, cfg.mqtt_qos, false)) {
                if (mosquitto_reconnect(mosq) == MOSQ_ERR_SUCCESS) {
                    logMessageByTag(0, 0, 0, __LINE__, (char *)"Message: MQTT broker successfully re-connected. [%d]\n");
                } else {
                    logMessageByTag(0, 0, 0, __LINE__, (char *)"Error: MQTT broker re-connection failed. [%d]\n");
                }
            }
        }

        if (countdown >= 0) {
            countdown--;
            if (countdown == 0) {
                cleanupCache(RSCP_MQTT::RscpMqttCacheTempl);
                if (cfg.pvi_requests) pviStringNull(RSCP_MQTT::RscpMqttCache, cfg.pvi_tracker);
                if (cfg.wallbox) wallboxDailyNull(RSCP_MQTT::RscpMqttCache, cfg.wb_number, true);
                while (!RSCP_MQTT::mqttQ.empty()) {
                    RSCP_MQTT::mqtt_data_t x;
                    x = RSCP_MQTT::mqttQ.front();
                    setupItem(RSCP_MQTT::RscpMqttCache, x.topic, x.payload);
                    if (x.topic) {free(x.topic); x.topic = NULL;}
                    if (x.payload) {free(x.payload); x.payload = NULL;}
                    RSCP_MQTT::mqttQ.pop();
                }
                publishImmediately((char *)"rscp2mqtt/version", (char *)RSCP2MQTT_VERSION, true);
                publishImmediately((char *)"rscp2mqtt/long_version", (char *)RSCP2MQTT, true);
                if (cfg.mqtt_tls) publishImmediately((char *)"rscp2mqtt/mqtt/encryption", (char *)"tls", true);
                else publishImmediately((char *)"rscp2mqtt/mqtt/encryption", (char *)"none", true);
#ifdef INFLUXDB
                if (cfg.curl_https) publishImmediately((char *)"rscp2mqtt/curl/protocol", (char *)"https", true);
                else publishImmediately((char *)"rscp2mqtt/curl/protocol", (char *)"http", true);
#endif
                if (cfg.once) go = false;
            }
            sleep(1);
        } else {
            gettimeofday(&end, NULL);
            elapsed = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) * 1e-6;
            elapsed = (double)cfg.interval - elapsed;
            if (elapsed > 0.0) wsleep(elapsed);
        }
    }
    return;
}

int main(int argc, char *argv[], char *envp[]) {
    char key[128], value[128], line[256];
    char *cfile = NULL;
    int i = 0;

    cfg.daemon = false;
    cfg.verbose = false;
    cfg.once = false;

    while (i < argc) {
        if (!strcmp(argv[i], "--daemon") || !strcmp(argv[i], "-d")) cfg.daemon = true;
        if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) cfg.verbose = true;
        if ((!strcmp(argv[i], "--config") || !strcmp(argv[i], "-c")) && (i+1 < argc)) cfile = argv[++i];
        if (!strcmp(argv[i], "--once") || !strcmp(argv[i], "-1")) cfg.once = true;
        if (!strcmp(argv[i], "--help")) {
            printf("rscp2mqtt [%s] - Bridge between an E3/DC home power station and an MQTT broker\n\n", RSCP2MQTT);
            printf("Usage: rscp2mqtt [options]\n\n");
            printf("Options:\n  -c, --config <file>\n  -d, --daemon\t\tdaemon mode\n  -v, --verbose\t\tverbose mode\n  -1, --once\t\tone interval runtime\n\n");
            printf("For more details please visit https://github.com/pvtom/rscp2mqtt\n");
            exit(EXIT_SUCCESS);
        }
        i++;
    }

    FILE *fp;
    if (cfile) fp = fopen(cfile, "r");
    else fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        printf("Config file %s not found.\n", cfile?cfile:CONFIG_FILE);
    }

    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);
    curr_year = l->tm_year + 1900;
    curr_day = l->tm_mday;

    strcpy(cfg.mqtt_host, "");
    cfg.mqtt_port = MQTT_PORT_DEFAULT;
    strcpy(cfg.mqtt_client_id, "");
    cfg.mqtt_auth = false;
    cfg.mqtt_tls = false;
    cfg.mqtt_tls_cafile = NULL;
    cfg.mqtt_tls_capath = NULL;
    cfg.mqtt_tls_certfile = NULL;
    cfg.mqtt_tls_keyfile = NULL;
    cfg.mqtt_tls_password = NULL;
    cfg.mqtt_qos = 0;
    cfg.mqtt_retain = false;
    cfg.interval = 4;
    cfg.battery_strings = 1;
    cfg.pvi_requests = true;
    cfg.pvi_tracker = 0;
    cfg.pvi_temp_count = 0;
    memset((void *)&wb_stat, 0, sizeof(wb_t));
    for (uint8_t i = 0; i < MAX_DCB_COUNT; i++) {
        cfg.bat_dcb_count[i] = 0;
        cfg.bat_dcb_start[i] = 0;
    }
    for (uint8_t i = 0; i < MAX_PM_COUNT; i++) {
        cfg.pm_indexes[i] = 0;
    }
    for (uint8_t i = 0; i < MAX_WB_COUNT; i++) {
        cfg.wb_indexes[i] = 0;
    }
    cfg.bat_dcb_cell_voltages = 0;
    cfg.bat_dcb_cell_temperatures = 0;
    cfg.pm_number = 0;
    cfg.wb_number = 0;
    cfg.pm_extern = false;
    cfg.pm_requests = true;
    cfg.dcb_requests = true;
    cfg.soc_limiter = false;
    cfg.ems_requests = true;
    cfg.hst_requests = true;
    cfg.daily_values = true;
    cfg.statistic_values = true;
    cfg.wallbox = false;
    cfg.auto_refresh = false;
    cfg.store_setup = false;
    strcpy(cfg.prefix, DEFAULT_PREFIX);
    cfg.history_start_year = curr_year - 1;
#ifdef INFLUXDB
    bool influx_reduced = false;
    strcpy(cfg.influxdb_host, "");
    cfg.influxdb_port = INFLUXDB_PORT_DEFAULT;
    cfg.curl_https = false;
    cfg.curl_protocol = NULL;
    cfg.curl_opt_ssl_verifypeer = true;
    cfg.curl_opt_ssl_verifyhost = true;
    cfg.curl_opt_cainfo = NULL;
    cfg.curl_opt_sslcert = NULL;
    cfg.curl_opt_sslkey = NULL;
    cfg.influxdb_on = false;
    cfg.influxdb_auth = false;
    strcpy(cfg.influxdb_user, "");
    strcpy(cfg.influxdb_password, "");
    strcpy(cfg.influxdb_measurement, "e3dc");
    strcpy(cfg.influxdb_measurement_meta, "e3dc_meta");
#endif
    cfg.mqtt_pub = true;
    cfg.logfile = NULL;
    cfg.historyfile = NULL;
    cfg.log_level = 0;
    strcpy(cfg.true_value, "true");
    strcpy(cfg.false_value, "false");
    cfg.raw_mode = false;
    cfg.raw_topic_regex = NULL;

    // signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    RSCP_MQTT::topic_store_t store = { 0, "" };

    if (fp) {
        bool skip = false;
        while (fgets(line, sizeof(line), fp)) {
            if (!strncmp(line, "EXIT", 4)) break;
            if (!strncmp(line, "/*", 2)) skip = true;
            if (!strncmp(line, "*/", 2)) skip = false;
            if (skip) continue;
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
                else if (strcasecmp(key, "MQTT_CLIENT_ID") == 0)
                    strcpy(cfg.mqtt_client_id, value);
                else if ((strcasecmp(key, "MQTT_AUTH") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.mqtt_auth = true;
                else if (strcasecmp(key, "MQTT_TLS_CAFILE") == 0)
                    cfg.mqtt_tls_cafile = strdup(value);
                else if (strcasecmp(key, "MQTT_TLS_CAPATH") == 0)
                    cfg.mqtt_tls_capath = strdup(value);
                else if (strcasecmp(key, "MQTT_TLS_CERTFILE") == 0)
                    cfg.mqtt_tls_certfile = strdup(value);
                else if (strcasecmp(key, "MQTT_TLS_KEYFILE") == 0)
                    cfg.mqtt_tls_keyfile = strdup(value);
                else if (strcasecmp(key, "MQTT_TLS_PASSWORD") == 0)
                    cfg.mqtt_tls_password = strdup(value);
                else if ((strcasecmp(key, "MQTT_TLS") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.mqtt_tls = true;
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
                else if ((strcasecmp(key, "CURL_HTTPS") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.curl_https = true;
                else if ((strcasecmp(key, "CURL_OPT_SSL_VERIFYPEER") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.curl_opt_ssl_verifypeer = false;
                else if ((strcasecmp(key, "CURL_OPT_SSL_VERIFYHOST") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.curl_opt_ssl_verifyhost = false;
                else if (strcasecmp(key, "CURL_OPT_CAINFO") == 0)
                    cfg.curl_opt_cainfo = strdup(value);
                else if (strcasecmp(key, "CURL_OPT_SSLCERT") == 0)
                    cfg.curl_opt_sslcert = strdup(value);
                else if (strcasecmp(key, "CURL_OPT_SSLKEY") == 0)
                    cfg.curl_opt_sslkey = strdup(value);
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
                else if (strcasecmp(key, "INFLUXDB_MEASUREMENT_META") == 0)
                    strcpy(cfg.influxdb_measurement_meta, value);
                else if ((strcasecmp(key, "ENABLE_INFLUXDB") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.influxdb_on = true;
#endif
                else if (strcasecmp(key, "LOGFILE") == 0) {
                    cfg.logfile = strdup(value);
                } else if ((strcasecmp(key, "LOG_MODE") == 0) && (strcasecmp(value, "ON") == 0)) {
                    cfg.log_level = 1;
                } else if ((strcasecmp(key, "LOG_MODE") == 0) && (strcasecmp(value, "BUFFERED") == 0)) {
                    cfg.log_level = 2;
                } else if (strcasecmp(key, "LOG_LEVEL") == 0)
                    cfg.log_level = atoi(value);
                else if (strcasecmp(key, "TOPIC_LOG_FILE") == 0) {
                    cfg.historyfile = strdup(value);
                } else if (strcasecmp(key, "INTERVAL") == 0)
                    cfg.interval = atoi(value);
                else if ((strcasecmp(key, "VERBOSE") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.verbose = true;
                else if ((strcasecmp(key, "PVI_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.pvi_requests = false;
                else if (strcasecmp(key, "PVI_TRACKER") == 0)
                    cfg.pvi_tracker = atoi(value);
                else if (strcasecmp(key, "BATTERY_STRINGS") == 0)
                    cfg.battery_strings = atoi(value);
                else if ((strcasecmp(key, "PM_EXTERN") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.pm_extern = true;
                else if (strcasecmp(key, "PM_INDEX") == 0) {
                    if ((cfg.pm_number < MAX_PM_COUNT) && (atoi(value) >= 0) && (atoi(value) <= 127)) cfg.pm_indexes[cfg.pm_number++] = atoi(value);
                } else if (strcasecmp(key, "WB_INDEX") == 0) {
                    if ((cfg.wb_number < MAX_WB_COUNT) && (atoi(value) >= 0) && (atoi(value) < MAX_WB_COUNT)) cfg.wb_indexes[cfg.wb_number++] = atoi(value);
                } else if (strcasecmp(key, "DCB_CELL_VOLTAGES") == 0) {
                    if ((atoi(value) >= 0) && (atoi(value) <= 99)) cfg.bat_dcb_cell_voltages = atoi(value);
                } else if (strcasecmp(key, "DCB_CELL_TEMPERATURES") == 0) {
                    if ((atoi(value) >= 0) && (atoi(value) <= 99)) cfg.bat_dcb_cell_temperatures = atoi(value);
                } else if ((strcasecmp(key, "PM_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.pm_requests = false;
                else if ((strcasecmp(key, "EMS_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.ems_requests = false;
                else if ((strcasecmp(key, "HST_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.hst_requests = false;
                else if ((strcasecmp(key, "DCB_REQUESTS") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.dcb_requests = false;
                else if ((strcasecmp(key, "SOC_LIMITER") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.soc_limiter = true;
                else if ((strcasecmp(key, "DAILY_VALUES") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.daily_values = false;
                else if ((strcasecmp(key, "STATISTIC_VALUES") == 0) && (strcasecmp(value, "false") == 0))
                    cfg.statistic_values = false;
                else if ((strcasecmp(key, "WALLBOX") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.wallbox = true;
                else if ((strcasecmp(key, "LIMIT_CHARGE_SOC") == 0) && (atoi(value) >= 0) && (atoi(value) <= 100))
                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(value), IDX_LIMIT_CHARGE_SOC, true);
                else if ((strcasecmp(key, "LIMIT_DISCHARGE_SOC") == 0) && (atoi(value) >= 0) && (atoi(value) <= 100))
                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(value), IDX_LIMIT_DISCHARGE_SOC, true);
                else if ((strcasecmp(key, "LIMIT_CHARGE_DURABLE") == 0) && (strcasecmp(value, "true") == 0))
                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 1, IDX_LIMIT_CHARGE_DURABLE, true);
                else if ((strcasecmp(key, "LIMIT_DISCHARGE_DURABLE") == 0) && (strcasecmp(value, "true") == 0))
                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 1, IDX_LIMIT_DISCHARGE_DURABLE, true);
                else if ((strcasecmp(key, "LIMIT_DISCHARGE_BY_HOME_POWER") == 0) && (atoi(value) >= 0) && (atoi(value) <= 99999))
                    storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(value), IDX_LIMIT_DISCHARGE_BY_HOME_POWER, true);
                else if (((strcasecmp(key, "DISABLE_MQTT_PUBLISH") == 0) || (strcasecmp(key, "DRYRUN") == 0)) && (strcasecmp(value, "true") == 0))
                    cfg.mqtt_pub = false;
                else if ((strcasecmp(key, "AUTO_REFRESH") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.auto_refresh = true;
                else if ((strcasecmp(key, "RETAIN_FOR_SETUP") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.store_setup = true;
                else if ((strcasecmp(key, "USE_TRUE_FALSE") == 0) && (strcasecmp(value, "false") == 0)) {
                    strcpy(cfg.true_value, "1");
                    strcpy(cfg.false_value, "0");
                } else if (strcasecmp(key, "FORCE_PUB") == 0) {
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
                else if ((strcasecmp(key, "RAW_MODE") == 0) && (strcasecmp(value, "true") == 0))
                    cfg.raw_mode = true;
                else if (strcasecmp(key, "RAW_TOPIC_REGEX") == 0)
                    cfg.raw_topic_regex = strdup(value);
// Issue #9 
                else if (strcasecmp(key, "CORRECT_PM_0_UNIT") == 0) {
                    correctExternalPM(RSCP_MQTT::RscpMqttCache, 0, value, 0);
                }
                else if (strcasecmp(key, "CORRECT_PM_0_DIVISOR") == 0) {
                    correctExternalPM(RSCP_MQTT::RscpMqttCache, 0, NULL, atoi(value));
                }
                else if (strcasecmp(key, "CORRECT_PM_1_UNIT") == 0) {
                    correctExternalPM(RSCP_MQTT::RscpMqttCache, 1, value, 0);
                }
                else if (strcasecmp(key, "CORRECT_PM_1_DIVISOR") == 0) {
                    correctExternalPM(RSCP_MQTT::RscpMqttCache, 1, NULL, atoi(value));
                }
                else if (strncasecmp(key, "ADD_NEW_REQUEST", strlen("ADD_NEW_REQUEST")) == 0) {
                    int order = 0;
                    int index = -1;
                    char container[128];
                    char tag[128];
                    bool one_shot = false;
                    memset(container, 0, sizeof(container));
                    memset(tag, 0, sizeof(tag));
                    if (!strcasecmp(key, "ADD_NEW_REQUEST_AT_START")) one_shot = true;
                    if ((sscanf(value, "%127[^:-]:%127[^:-]:%d-%d", container, tag, &index, &order) == 4) || (sscanf(value, "%127[^:-]:%127[^:-]-%d", container, tag, &order) == 3)) {
                        if (isTag(RSCP_TAGS::RscpTagsOverview, container, true) && isTag(RSCP_TAGS::RscpTagsOverview, tag, false)) pushAdditionalTag(tagID(RSCP_TAGS::RscpTagsOverview, container), tagID(RSCP_TAGS::RscpTagsOverview, tag), index, order, one_shot);
                    } else logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"key >%s< value >%s< not enough attributes.\n", key, value);
                }
                else if (strcasecmp(key, "ADD_NEW_TOPIC") == 0) {
                    int divisor = 1;
                    int bit = 1;
                    int index = 0;
                    char container[128];
                    char tag[128];
                    char topic[TOPIC_SIZE];
                    char unit[128];
                    memset(container, 0, sizeof(container));
                    memset(tag, 0, sizeof(tag));
                    memset(topic, 0, sizeof(topic));
                    memset(unit, 0, sizeof(unit));
                    if ((sscanf(value, "%127[^:]:%127[^:]:%d:%127[^:]:%d:%d:%127[^:]", container, tag, &index, unit, &divisor, &bit, topic) == 7) ||
                        (sscanf(value, "%127[^:]:%127[^:]:%127[^:]:%d:%d:%127[^:]", container, tag, unit, &divisor, &bit, topic) == 6)) {
                        if (isTag(RSCP_TAGS::RscpTagsOverview, container, false) && isTag(RSCP_TAGS::RscpTagsOverview, tag, false)) addTopic(tagID(RSCP_TAGS::RscpTagsOverview, container), tagID(RSCP_TAGS::RscpTagsOverview, tag), index, topic, unit, F_AUTO, divisor, bit, true);
                    } else logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"key >%s< value >%s< not enough attributes.\n", key, value);
                }
                else if (strcasecmp(key, "ADD_NEW_SET_TOPIC") == 0) {
                    char container[128];
                    char tag[128];
                    int index = 0;
                    char topic[TOPIC_SIZE];
                    char regex_true[128];
                    char value_true[128];
                    char regex_false[128];
                    char value_false[128];
                    char type[128];
                    memset(container, 0, sizeof(container));
                    memset(tag, 0, sizeof(tag));
                    memset(topic, 0, sizeof(topic));
                    memset(regex_true, 0, sizeof(regex_true));
                    memset(value_true, 0, sizeof(value_true));
                    memset(regex_false, 0, sizeof(regex_false));
                    memset(value_false, 0, sizeof(value_false));
                    memset(type, 0, sizeof(type));
                    if ((sscanf(value, "%127[^:#]:%127[^:#]:%d:%127[^:#]:%127[^:#]:%127[^:#]:%127[^:#]:%127[^:#]#%127[^:#]", container, tag, &index, topic, regex_true, value_true, regex_false, value_false, type) == 9) || (sscanf(value, "%127[^:#]:%127[^:#]:%d:%127[^:#]:%127[^:#]#%127[^:#]", container, tag, &index, topic, regex_true, type) == 6)) {
                        if (isTag(RSCP_TAGS::RscpTagsOverview, container, false) && isTag(RSCP_TAGS::RscpTagsOverview, tag, false) && (index >= 0)) addSetTopic(tagID(RSCP_TAGS::RscpTagsOverview, container), tagID(RSCP_TAGS::RscpTagsOverview, tag), index, topic, regex_true, value_true, regex_false, value_false, typeID(RSCP_TAGS::RscpTypeNames, type), true);
                    } else {
                        logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"key >%s< value >%s< not enough attributes.\n", key, value);
                    }
                }
            }
        }
        fclose(fp);
    }

    // Overwrite with environment parameters
    ENV_STRING("E3DC_IP", cfg.e3dc_ip);
    ENV_INT_RANGE("E3DC_PORT", cfg.e3dc_port, 0, 65535);
    ENV_STRING("E3DC_USER", cfg.e3dc_user);
    ENV_STRING("E3DC_PASSWORD", cfg.e3dc_password);
    ENV_STRING("E3DC_AES_PASSWORD", cfg.aes_password);
    ENV_STRING("MQTT_HOST", cfg.mqtt_host);
    ENV_INT_RANGE("MQTT_PORT", cfg.mqtt_port, 0, 65535);
    ENV_STRING("MQTT_USER", cfg.mqtt_user);
    ENV_STRING("MQTT_PASSWORD", cfg.mqtt_password);
    ENV_BOOL("MQTT_AUTH", cfg.mqtt_auth);
    ENV_STRING("MQTT_TLS_CAFILE", cfg.mqtt_tls_cafile);
    ENV_STRING("MQTT_TLS_CAPATH", cfg.mqtt_tls_capath);
    ENV_STRING("MQTT_TLS_CERTFILE", cfg.mqtt_tls_certfile);
    ENV_STRING("MQTT_TLS_KEYFILE", cfg.mqtt_tls_keyfile);
    ENV_STRING("MQTT_TLS_PASSWORD", cfg.mqtt_tls_password);
    ENV_BOOL("MQTT_TLS", cfg.mqtt_tls);
    ENV_BOOL("MQTT_RETAIN", cfg.mqtt_retain);
    ENV_INT_RANGE("MQTT_QOS", cfg.mqtt_qos, 0, 2);
    ENV_STRING_N("PREFIX", cfg.prefix, 24);
    ENV_INT("HISTORY_START_YEAR", cfg.history_start_year);
    ENV_INT("INTERVAL", cfg.interval);
    ENV_BOOL("RAW_MODE", cfg.raw_mode);
    ENV_STRING("RAW_TOPIC_REGEX", cfg.raw_topic_regex);
    ENV_BOOL("WALLBOX", cfg.wallbox);
    ENV_BOOL("VERBOSE", cfg.verbose);
    ENV_INT("PVI_TRACKER", cfg.pvi_tracker);
    ENV_INT("BATTERY_STRINGS", cfg.battery_strings);
    ENV_BOOL("PM_REQUESTS", cfg.pm_requests);
    ENV_BOOL("EMS_REQUESTS", cfg.ems_requests);
    ENV_BOOL("HST_REQUESTS", cfg.hst_requests);
    ENV_BOOL("DCB_REQUESTS", cfg.dcb_requests);
    ENV_BOOL("SOC_LIMITER", cfg.soc_limiter);
    ENV_BOOL("DAILY_VALUES", cfg.daily_values);
    ENV_BOOL("STATISTIC_VALUES", cfg.statistic_values);
    ENV_BOOL("AUTO_REFRESH", cfg.auto_refresh);
    ENV_BOOL("RETAIN_FOR_SETUP", cfg.store_setup);
    ENV_BOOL("PM_EXTERN", cfg.pm_extern);

    for (i = 0; envp[i] != NULL; i++) {
        if (sscanf(envp[i], "%127[^ \t=]=%127[^\n]", key, value) == 2) {
            if (strcasestr(key, "FORCE_PUB")) {
                store.type = FORCED_TOPIC;
                strcpy(store.topic, value);
                RSCP_MQTT::TopicStore.push_back(store);
#ifdef INFLUXDB
            } else if (strcasestr(key, "INFLUXDB_TOPIC")) {
                store.type = INFLUXDB_TOPIC;
                strcpy(store.topic, value);
                RSCP_MQTT::TopicStore.push_back(store);
                influx_reduced = true;
#endif
            } else if (strcasestr(key, "PM_INDEX")) {
                if ((cfg.pm_number < MAX_PM_COUNT) && (atoi(value) >= 0) && (atoi(value) <= 127)) cfg.pm_indexes[cfg.pm_number++] = atoi(value);
            } else if (strcasestr(key, "WB_INDEX")) {
                if ((cfg.wb_number < MAX_WB_COUNT) && (atoi(value) >= 0) && (atoi(value) < MAX_WB_COUNT)) cfg.wb_indexes[cfg.wb_number++] = atoi(value);
            } else if ((strcasecmp(key, "USE_TRUE_FALSE") == 0) && (strcasecmp(value, "false") == 0)) {
                strcpy(cfg.true_value, "1");
                strcpy(cfg.false_value, "0");
            } else if ((strcasecmp(key, "LIMIT_CHARGE_SOC") == 0) && (atoi(value) >= 0) && (atoi(value) <= 100))
                storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(value), IDX_LIMIT_CHARGE_SOC, true);
            else if ((strcasecmp(key, "LIMIT_DISCHARGE_SOC") == 0) && (atoi(value) >= 0) && (atoi(value) <= 100))
                storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(value), IDX_LIMIT_DISCHARGE_SOC, true);
            else if ((strcasecmp(key, "LIMIT_CHARGE_DURABLE") == 0) && (strcasecmp(value, "true") == 0))
                storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 1, IDX_LIMIT_CHARGE_DURABLE, true);
            else if ((strcasecmp(key, "LIMIT_DISCHARGE_DURABLE") == 0) && (strcasecmp(value, "true") == 0))
                storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 1, IDX_LIMIT_DISCHARGE_DURABLE, true);
            else if ((strcasecmp(key, "LIMIT_DISCHARGE_BY_HOME_POWER") == 0) && (atoi(value) >= 0) && (atoi(value) <= 99999))
                storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, atoi(value), IDX_LIMIT_DISCHARGE_BY_HOME_POWER, true);
        }
    }

#ifdef INFLUXDB
    ENV_STRING("INFLUXDB_HOST", cfg.influxdb_host);
    ENV_INT_RANGE("INFLUXDB_PORT", cfg.influxdb_port, 0, 65535);
    ENV_INT("INFLUXDB_VERSION", cfg.influxdb_version);
    ENV_STRING("INFLUXDB_1_DB", cfg.influxdb_db);
    ENV_STRING("INFLUXDB_1_USER", cfg.influxdb_user);
    ENV_STRING("INFLUXDB_1_PASSWORD", cfg.influxdb_password);
    ENV_BOOL("INFLUXDB_1_AUTH", cfg.influxdb_auth);
    ENV_STRING("INFLUXDB_2_ORGA", cfg.influxdb_orga);
    ENV_STRING("INFLUXDB_2_BUCKET", cfg.influxdb_bucket);
    ENV_STRING("INFLUXDB_2_TOKEN", cfg.influxdb_token);
    ENV_STRING("INFLUXDB_MEASUREMENT", cfg.influxdb_measurement); 
    ENV_STRING("INFLUXDB_MEASUREMENT_META", cfg.influxdb_measurement_meta);
    ENV_BOOL("ENABLE_INFLUXDB", cfg.influxdb_on);
    ENV_BOOL("CURL_HTTPS", cfg.curl_https);
    ENV_BOOL("CURL_OPT_SSL_VERIFYPEER", cfg.curl_opt_ssl_verifypeer);
    ENV_BOOL("CURL_OPT_SSL_VERIFYHOST", cfg.curl_opt_ssl_verifyhost);
    ENV_STRING("CURL_OPT_CAINFO", cfg.curl_opt_cainfo);
    ENV_STRING("CURL_OPT_SSLCERT", cfg.curl_opt_sslcert);
    ENV_STRING("CURL_OPT_SSLKEY", cfg.curl_opt_sslkey);

    if (!influx_reduced) {
        store.type = INFLUXDB_TOPIC;
        strcpy(store.topic, ".*");
        RSCP_MQTT::TopicStore.push_back(store);
    }
#endif

    if (cfg.interval < DEFAULT_INTERVAL_MIN) cfg.interval = DEFAULT_INTERVAL_MIN;
    if (cfg.interval > DEFAULT_INTERVAL_MAX) cfg.interval = DEFAULT_INTERVAL_MAX;
    if ((cfg.mqtt_qos < 0) || (cfg.mqtt_qos > 2)) cfg.mqtt_qos = 0;
    if ((cfg.pvi_tracker < 0) || (cfg.pvi_tracker > 127)) cfg.pvi_tracker = 0;
    if ((cfg.battery_strings < 1) || (cfg.battery_strings > 127)) cfg.battery_strings = 1;
    if (cfg.pm_number == 0) cfg.pm_number = 1;
    if (cfg.pm_extern) cfg.pm_indexes[0] = 6;
#ifdef INFLUXDB
    if ((cfg.influxdb_version < 1) || (cfg.influxdb_version > 2)) cfg.influxdb_version = 1;
#endif
    if ((cfg.history_start_year < E3DC_FOUNDED) || (cfg.history_start_year > (curr_year - 1))) cfg.history_start_year = curr_year - 1;

    // prepare RscpMqttReceiveCache
    for (uint8_t c = 0; c < IDLE_PERIOD_CACHE_SIZE; c++) {
        addSetTopic(0, 0, 0, (char *)"set/idle_period", (char *)SET_IDLE_PERIOD_REGEX, (char *)"", (char *)"", (char *)"", RSCP::eTypeBool, true);
    }
    if (cfg.wallbox) {
        for (uint8_t c = 0; c < cfg.wb_number; c++) {
            char topic[TOPIC_SIZE];
            sprintf(topic, "set/wallbox/%d/sun_mode", c + 1);
            addSetTopic(TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, c, topic, (char *)"^true|on|1$", (char *)"1", (char *)"^false|off|0$", (char *)"0", RSCP::eTypeBool, false);
            sprintf(topic, "set/wallbox/%d/toggle", c + 1);
            addSetTopic(TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, c, topic, (char *)"^true|on|1$", (char *)"1", (char *)"", (char *)"", RSCP::eTypeBool, false);
            sprintf(topic, "set/wallbox/%d/suspended", c + 1);
            addSetTopic(TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, c, topic, (char *)"^true|on|1$", (char *)"1", (char *)"^false|off|0$", (char *)"0", RSCP::eTypeBool, false);
            sprintf(topic, "set/wallbox/%d/max_current", c + 1);
            addSetTopic(TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, c, topic, (char *)"^[0-9]{1,2}$", (char *)"", (char *)"", (char *)"", RSCP::eTypeUChar8, false);

// Issue #84
            sprintf(topic, "set/wallbox/%d/min_current", c + 1);
            addSetTopic(TAG_WB_REQ_DATA, TAG_WB_REQ_SET_MIN_CHARGE_CURRENT, c, topic, (char *)"^[0-9]{1,2}$", (char *)"", (char *)"", (char *)"", RSCP::eTypeUChar8, false);

            sprintf(topic, "set/wallbox/%d/number_phases", c + 1);
            addSetTopic(TAG_WB_REQ_DATA, TAG_WB_REQ_SET_NUMBER_PHASES, c, topic, (char *)"^1|3$", (char *)"", (char *)"", (char *)"", RSCP::eTypeUChar8, true);
        }
    }

    printf("rscp2mqtt [%s]\n", RSCP2MQTT);
    printf("E3DC system >%s:%u< user: >%s<\n", cfg.e3dc_ip, cfg.e3dc_port, cfg.e3dc_user);
    if (!std::regex_match(cfg.e3dc_ip, std::regex("^(?:\\b\\.?(?:25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){4}$"))) {
        printf("Error: >%s< is not a valid IP address. Please correct the config!\n", cfg.e3dc_ip);
        exit(EXIT_FAILURE);
    }
    if (hasSpaces(cfg.e3dc_user)) {
        printf("Error: E3DC_USER >%s< contains spaces. Please correct the config!\n", cfg.e3dc_user);
        exit(EXIT_FAILURE);
    }
    if (hasSpaces(cfg.e3dc_password)) {
        printf("Error: E3DC_PASSWORD contains spaces. Please correct the config!\n");
        exit(EXIT_FAILURE);
    }
    if (hasSpaces(cfg.aes_password)) {
        printf("Error: E3DC_AES_PASSWORD contains spaces. Please correct the config!\n");
        exit(EXIT_FAILURE);
    }
    if (hasSpaces(cfg.mqtt_host)) {
        printf("Error: MQTT_HOST >%s< contains spaces. Please correct the config!\n", cfg.mqtt_host);
        exit(EXIT_FAILURE);
    }
    if (mosquitto_sub_topic_check(cfg.prefix) != MOSQ_ERR_SUCCESS) {
        printf("Error: MQTT prefix >%s< is not accepted. Please correct the config!\n", cfg.prefix);
        exit(EXIT_FAILURE);
    }

    addPrefix(RSCP_MQTT::RscpMqttCache, cfg.prefix);
    addPrefix(RSCP_MQTT::RscpMqttCacheTempl, cfg.prefix);

    // Additional Tags
    sort(RSCP_MQTT::AdditionalTags.begin(), RSCP_MQTT::AdditionalTags.end(), RSCP_MQTT::compareAdditionalTags);

    // History year
    if (cfg.verbose) printf("History year range from %d to %d\n", cfg.history_start_year, curr_year);
    if (cfg.history_start_year < curr_year) addTemplTopics(TAG_DB_HISTORY_DATA_YEAR, 1, NULL, cfg.history_start_year, curr_year, 0, 0, false);

    // Battery Strings
    addTemplTopics(TAG_BAT_DATA, (cfg.battery_strings == 1)?0:1, (char *)"battery", 0, cfg.battery_strings, 1, 0, false);
    addTemplTopics(TAG_BAT_SPECIFICATION, (cfg.battery_strings == 1)?0:1, (char *)"battery", 0, cfg.battery_strings, 1, 0, false);

    // PVI strings (no auto detection)
    if (cfg.pvi_tracker) {
        addTemplTopics(TAG_PVI_DC_POWER, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
        addTemplTopics(TAG_PVI_DC_VOLTAGE, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
        addTemplTopics(TAG_PVI_DC_CURRENT, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
        addTemplTopics(TAG_PVI_DC_STRING_ENERGY_ALL, 1, NULL, 0, cfg.pvi_tracker, 1, 0, false);
        addTemplTopicsIdx(IDX_PVI_ENERGY, NULL, 0, cfg.pvi_tracker, 1, false);
        addTemplTopicsIdx(IDX_PVI_ENERGY_START, NULL, 0, cfg.pvi_tracker, 1, false);
    }

    // Wallbox
    if (cfg.wallbox) {
        if (cfg.wb_number == 0) cfg.wb_number = 1;
        addTemplTopics(TAG_WB_DATA, (cfg.wb_number == 1)?0:1, (char *)"wallbox", 0, cfg.wb_number, 1, 0, false);
        addTemplTopics(TAG_WB_EXTERN_DATA_ALG, (cfg.wb_number == 1)?0:1, (char *)"wallbox", 0, cfg.wb_number, 1, 0, false);
        addTemplTopicsIdx(IDX_WALLBOX_DAY_ENERGY_ALL, (char *)"wallbox", 0, cfg.wb_number, 1, false);
        addTemplTopicsIdx(IDX_WALLBOX_DAY_ENERGY_SOLAR, (char *)"wallbox", 0, cfg.wb_number, 1, false);
        addTemplTopicsIdx(IDX_WALLBOX_ENERGY_ALL_START, (char *)"wallbox", 0, cfg.wb_number, 1, false);
        addTemplTopicsIdx(IDX_WALLBOX_ENERGY_SOLAR_START, (char *)"wallbox", 0, cfg.wb_number, 1, false);
        addTemplTopicsIdx(IDX_WALLBOX_LAST_ENERGY_ALL, (char *)"wallbox", 0, cfg.wb_number, 1, false);
        addTemplTopicsIdx(IDX_WALLBOX_LAST_ENERGY_SOLAR, (char *)"wallbox", 0, cfg.wb_number, 1, false);
        for (uint8_t i = 0; i < cfg.wb_number; i++) {
            storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, i + IDX_WALLBOX_LAST_ENERGY_ALL, true);
            storeIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, 0, i + IDX_WALLBOX_LAST_ENERGY_SOLAR, true);
        }
    }

    // Power Meters 
    addTemplTopics(TAG_PM_DATA, (cfg.pm_number == 1)?0:1, (char *)"pm", 0, cfg.pm_number, 1, 0, false);
    addTemplTopicsIdx(IDX_PM_POWER, (char *)"pm", 0, cfg.pm_number, 1, false);
    addTemplTopicsIdx(IDX_PM_ENERGY, (char *)"pm", 0, cfg.pm_number, 1, true);

    printf("MQTT broker >%s:%u< qos = >%i< retain = >%s< tls >%s< client id >%s< prefix >%s<\n", cfg.mqtt_host, cfg.mqtt_port, cfg.mqtt_qos, cfg.mqtt_retain?"✓":"✗", cfg.mqtt_tls?"✓":"✗", strcmp(cfg.mqtt_client_id, "")?cfg.mqtt_client_id:"✗", cfg.prefix);

#ifdef INFLUXDB
    if (cfg.curl_https) cfg.curl_protocol = strdup("https");
    else cfg.curl_protocol = strdup("http");

    if (cfg.influxdb_on && (cfg.influxdb_version == 1)) {
        printf("INFLUXDB v1 >%s://%s:%u< db = >%s< measurements = >%s< and >%s<\n", cfg.curl_protocol, cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_db, cfg.influxdb_measurement, cfg.influxdb_measurement_meta);
        if (!strcmp(cfg.influxdb_host, "") || !strcmp(cfg.influxdb_db, "") || !strcmp(cfg.influxdb_measurement, "") || !strcmp(cfg.influxdb_measurement_meta, "") || !strcmp(cfg.influxdb_measurement, cfg.influxdb_measurement_meta)) {
            printf("Error: INFLUXDB not configured correctly. Program starts without INFLUXDB support!\n");
            cfg.influxdb_on = false;
        }
    } else if (cfg.influxdb_on && (cfg.influxdb_version == 2)) {
        printf("INFLUXDB v2 >%s://%s:%u< orga = >%s< bucket = >%s< measurements = >%s< and >%s<\n", cfg.curl_protocol, cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_orga, cfg.influxdb_bucket, cfg.influxdb_measurement, cfg.influxdb_measurement_meta);
        if (!strcmp(cfg.influxdb_host, "") || !strcmp(cfg.influxdb_orga, "") || !strcmp(cfg.influxdb_bucket, "") || !strcmp(cfg.influxdb_measurement, "") || !strcmp(cfg.influxdb_measurement_meta, "") || !strcmp(cfg.influxdb_measurement, cfg.influxdb_measurement_meta)) {
            printf("Error: INFLUXDB not configured correctly. Program starts without INFLUXDB support!\n");
            cfg.influxdb_on = false;
        }
    }
#endif
    printf("Requesting PVI %s | PM (", cfg.pvi_requests?"✓":"✗");
    for (uint8_t i = 0; i < cfg.pm_number; i++) {
        if (i) printf(",");
        printf("%d", cfg.pm_indexes[i]);
    }
    printf(") | DCB %s (%d battery string%s) | Wallbox%s %s", cfg.dcb_requests?"✓":"✗", cfg.battery_strings, (cfg.battery_strings > 1)?"s":"", (cfg.wallbox && (cfg.wb_number > 1))?"es":"", cfg.wallbox?"✓":"✗"); 
    if (cfg.wallbox) {
        printf(" (");
        for (uint8_t i = 0; i < cfg.wb_number; i++) {
            if (i) printf(","); 
            printf("%d", cfg.wb_indexes[i]); 
        }
        printf(")");
    }
    printf(" | Interval %d | Autorefresh %s | Raw data %s | ", cfg.interval, cfg.auto_refresh?"✓":"✗", cfg.raw_mode?"✓":"✗");

    switch (cfg.log_level) {
        case 1: {
                    printf("Logging ON\n");
                    break;
                }
        case 2: {
                    printf("Logging BUFFERED\n");
                    break;
                }
        default: {
                    printf("Logging OFF\n");
                    break;
                }
    }

    if (!cfg.mqtt_pub) printf("DISABLE_MQTT_PUBLISH / DRYRUN mode\n");

    if (!isatty(STDOUT_FILENO)) {
      printf("Stdout to pipe/file\n");
      cfg.daemon = false;
      cfg.verbose = false;
    }

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
            sprintf(buffer, "%s://%s:%d/write?db=%s", cfg.curl_protocol, cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_db);
        } else {
            sprintf(buffer, "Authorization: Token %s", cfg.influxdb_token);
            headers = curl_slist_append(headers, buffer);
            sprintf(buffer, "%s://%s:%d/api/v2/write?org=%s&bucket=%s", cfg.curl_protocol, cfg.influxdb_host, cfg.influxdb_port, cfg.influxdb_orga, cfg.influxdb_bucket);
        }

        if (cfg.curl_https) {
            if (!cfg.curl_opt_ssl_verifypeer) curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            if (!cfg.curl_opt_ssl_verifyhost) curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            if (cfg.curl_opt_cainfo) curl_easy_setopt(curl, CURLOPT_CAINFO, cfg.curl_opt_cainfo);
            if (cfg.curl_opt_sslcert) curl_easy_setopt(curl, CURLOPT_SSLCERT, cfg.curl_opt_sslcert);
            if (cfg.curl_opt_sslkey) curl_easy_setopt(curl, CURLOPT_SSLKEY, cfg.curl_opt_sslkey);
        }
        curl_easy_setopt(curl, CURLOPT_URL, buffer);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
#endif

    // application which re-connections to server on connection lost
    while (go) {
        // connect to server
        if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Connecting to server %s:%u\n", cfg.e3dc_ip, cfg.e3dc_port);
        iSocket = SocketConnect(cfg.e3dc_ip, cfg.e3dc_port);
        if (iSocket < 0) {
            logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Error: E3DC connection failed\n");
            sleep(1);
            continue;
        }
        if (cfg.verbose) logMessage(cfg.logfile, (char *)__FILE__, __LINE__, (char *)"Success: E3DC connected.\n");

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

        wsleep((double)DELAY_BEFORE_RECONNECT);
    }

    // retain some values for a restart of the program
    if (cfg.store_setup) {
        storeSetupItem(0, 0, IDX_GRID_IN_DURATION, getIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, IDX_GRID_IN_DURATION));
        storeSetupItem(0, 0, IDX_SUN_DURATION, getIntegerValue(RSCP_MQTT::RscpMqttCache, 0, 0, IDX_SUN_DURATION));
    }

    if (cfg.log_level) {
        logMessageCache(cfg.logfile, false);
    }

    for (std::vector<RSCP_MQTT::raw_data_t>::iterator it = RSCP_MQTT::rawData.begin(); it != RSCP_MQTT::rawData.end(); ++it) {
        if (it->topic) free(it->topic);
        if (it->payload) free(it->payload);
    }

    RSCP_MQTT::RscpMqttCache.clear();
    RSCP_MQTT::RscpMqttCacheTempl.clear();
    RSCP_MQTT::RscpMqttReceiveCache.clear();
    RSCP_MQTT::MessageCache.clear();
    RSCP_MQTT::NotSupportedTags.clear();
    RSCP_MQTT::AdditionalTags.clear();
    RSCP_MQTT::TopicStore.clear();
    RSCP_MQTT::IdlePeriodCache.clear();
    RSCP_MQTT::ErrorCache.clear();
    RSCP_MQTT::rawData.clear();

    // MQTT disconnect
    mosquitto_disconnect(mosq);
    mosq = NULL;

    // MQTT cleanup
    mosquitto_lib_cleanup();

#ifdef INFLUXDB
    // CURL cleanup
    if (curl) curl_easy_cleanup(curl);
    if (headers) curl_slist_free_all(headers);
    if (cfg.curl_opt_cainfo) free(cfg.curl_opt_cainfo);
    if (cfg.curl_opt_sslcert) free (cfg.curl_opt_sslcert);
    if (cfg.curl_opt_sslkey) free(cfg.curl_opt_sslkey);
    if (cfg.curl_protocol) free(cfg.curl_protocol);
#endif

    if (cfg.logfile) free(cfg.logfile);
    if (cfg.historyfile) free(cfg.historyfile);
    if (cfg.mqtt_tls_cafile) free(cfg.mqtt_tls_cafile);
    if (cfg.mqtt_tls_capath) free(cfg.mqtt_tls_capath);
    if (cfg.mqtt_tls_certfile) free(cfg.mqtt_tls_certfile);
    if (cfg.mqtt_tls_keyfile) free(cfg.mqtt_tls_keyfile);
    if (cfg.mqtt_tls_password) free(cfg.mqtt_tls_password);
    if (cfg.raw_topic_regex) free(cfg.raw_topic_regex);

    exit(EXIT_SUCCESS);
}
