#ifndef RSCP_MQTT_MAPPING_H_
#define RSCP_MQTT_MAPPING_H_

#include "RscpTags.h"
#include "RscpTypes.h"

#define TOPIC_SIZE        128
#define PAYLOAD_SIZE      80

#define PAYLOAD_REGEX     "(^[1-9]00$|^[1-9][0-9]00$|^[1-2][0-9][0-9]00$|^30000$)"

namespace RSCP_MQTT {

typedef struct _cache_t {
    int day;
    uint32_t tag;
    char topic[TOPIC_SIZE];
    char fstring[PAYLOAD_SIZE];
    char payload[PAYLOAD_SIZE];
    int type;
    int divisor;
    bool publish;
} cache_t;

cache_t cache[] = {
    { 0, TAG_EMS_POWER_PV, "e3dc/solar/power", "%i", "", RSCP::eTypeInt32, 1, false },
    { 0, TAG_EMS_POWER_BAT, "e3dc/battery/power", "%i", "", RSCP::eTypeInt32, 1, false },
    { 0, TAG_EMS_POWER_HOME, "e3dc/home/power", "%i", "", RSCP::eTypeInt32, 1, false },
    { 0, TAG_EMS_POWER_GRID, "e3dc/grid/power", "%i", "", RSCP::eTypeInt32, 1, false },
    { 0, TAG_EMS_POWER_ADD, "e3dc/addon/power", "%i", "", RSCP::eTypeInt32, 1, false },
    { 0, TAG_EMS_COUPLING_MODE, "e3dc/coupling/mode", "%i", "", RSCP::eTypeUChar8, 1, false },
    { 0, TAG_INFO_TIME_ZONE, "e3dc/time/zone", "%s", "", RSCP::eTypeString, 1, false },
    // CONTAINER TAG_BAT_DATA --------------------------------------------------------------------
    { 0, TAG_BAT_RSOC, "e3dc/battery/rsoc", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_BAT_MODULE_VOLTAGE, "e3dc/battery/voltage", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_BAT_CURRENT, "e3dc/battery/current", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_BAT_CHARGE_CYCLES, "e3dc/battery/cycles", "%u", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_BAT_STATUS_CODE, "e3dc/battery/status", "%u", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_BAT_ERROR_CODE, "e3dc/battery/error", "%u", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_BAT_DEVICE_NAME, "e3dc/battery/name", "%s", "", RSCP::eTypeString, 1, false },
    // CONTAINER TAG_EMS_GET_POWER_SETTINGS ------------------------------------------------------
    { 0, TAG_EMS_MAX_CHARGE_POWER, "e3dc/ems/max_charge/power", "%u", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_MAX_DISCHARGE_POWER, "e3dc/ems/max_discharge/power", "%u", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_POWER_LIMITS_USED, "e3dc/ems/power_limits", "%s", "", RSCP::eTypeBool, 1, false },
    { 0, TAG_EMS_DISCHARGE_START_POWER, "e3dc/ems/discharge_start/power", "%u", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_POWERSAVE_ENABLED, "e3dc/ems/power_save", "%s", "", RSCP::eTypeBool, 1, false },
    { 0, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, "e3dc/ems/weather_regulation", "%s", "", RSCP::eTypeBool, 1, false },
    // CONTAINER TAG_EMS_SET_POWER_SETTINGS ------------------------------------------------------
    { 0, TAG_EMS_RES_DISCHARGE_START_POWER, "e3dc/ems/discharge_start/status", "%i", "", RSCP::eTypeChar8, 1, false },
    { 0, TAG_EMS_RES_MAX_CHARGE_POWER, "e3dc/ems/max_charge/status", "%i", "", RSCP::eTypeChar8, 1, false },
    { 0, TAG_EMS_RES_MAX_DISCHARGE_POWER, "e3dc/ems/max_discharge/status", "%i", "", RSCP::eTypeChar8, 1, false },
    // CONTAINER TAG_DB_HISTORY_DATA_DAY ---------------------------------------------------------
    // CONTAINER TAG_DB_SUM_CONTAINER ------------------------------------------------------------
    // today
    { 0, TAG_DB_BAT_POWER_IN, "e3dc/battery/energy/charge", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 0, TAG_DB_BAT_POWER_OUT, "e3dc/battery/energy/discharge", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 0, TAG_DB_DC_POWER, "e3dc/solar/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 0, TAG_DB_GRID_POWER_IN, "e3dc/grid/energy/in", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 0, TAG_DB_GRID_POWER_OUT, "e3dc/grid/energy/out", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 0, TAG_DB_CONSUMPTION, "e3dc/home/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 0, TAG_DB_PM_0_POWER, "e3dc/pm_0/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_DB_PM_1_POWER, "e3dc/pm_1/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_DB_BAT_CHARGE_LEVEL, "e3dc/battery/soc", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_DB_CONSUMED_PRODUCTION, "e3dc/consumed", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_DB_AUTARKY, "e3dc/autarky", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    // yesterday
    { 1, TAG_DB_BAT_POWER_IN, "e3dc/yesterday/battery/energy/charge", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 1, TAG_DB_BAT_POWER_OUT, "e3dc/yesterday/battery/energy/discharge", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 1, TAG_DB_DC_POWER, "e3dc/yesterday/solar/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 1, TAG_DB_GRID_POWER_IN, "e3dc/yesterday/grid/energy/in", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 1, TAG_DB_GRID_POWER_OUT, "e3dc/yesterday/grid/energy/out", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 1, TAG_DB_CONSUMPTION, "e3dc/yesterday/home/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, false },
    { 1, TAG_DB_PM_0_POWER, "e3dc/yesterday/pm_0/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 1, TAG_DB_PM_1_POWER, "e3dc/yesterday/pm_1/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 1, TAG_DB_BAT_CHARGE_LEVEL, "e3dc/yesterday/battery/soc", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 1, TAG_DB_CONSUMED_PRODUCTION, "e3dc/yesterday/consumed", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 1, TAG_DB_AUTARKY, "e3dc/yesterday/autarky", "%0.1f", "", RSCP::eTypeFloat32, 1, false }
};

std::vector<cache_t> RscpMqttCache(cache, cache + sizeof(cache) / sizeof(cache_t));

typedef struct _rec_cache_t {
    uint32_t tag;
    uint32_t container;
    char topic[TOPIC_SIZE];
    char regex_true[PAYLOAD_SIZE];
    char value_true[PAYLOAD_SIZE];
    char regex_false[PAYLOAD_SIZE];
    char value_false[PAYLOAD_SIZE];
    char payload[PAYLOAD_SIZE];
    int type;
    bool done;
} rec_cache_t;

rec_cache_t rec_cache[] = {
    { TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, TAG_EMS_REQ_SET_POWER_SETTINGS, "e3dc/set/weather_regulation", "^true|on|1$", "1", "^false|off|0$", "0", "", RSCP::eTypeUChar8, true },
    { TAG_EMS_REQ_START_MANUAL_CHARGE, 0, "e3dc/set/manual_charge", PAYLOAD_REGEX, "", "0", "", "", RSCP::eTypeUInt32, true },
    { TAG_EMS_POWER_LIMITS_USED, TAG_EMS_REQ_SET_POWER_SETTINGS, "e3dc/set/power_limits", "^true|on|1$", "true", "^false|off|0$", "false", "", RSCP::eTypeBool, true },
    { TAG_EMS_MAX_CHARGE_POWER, TAG_EMS_REQ_SET_POWER_SETTINGS, "e3dc/set/max_charge_power", PAYLOAD_REGEX, "", "", "", "", RSCP::eTypeUInt32, true },
    { TAG_EMS_MAX_DISCHARGE_POWER, TAG_EMS_REQ_SET_POWER_SETTINGS, "e3dc/set/max_discharge_power", PAYLOAD_REGEX, "", "", "", "", RSCP::eTypeUInt32, true },
    { 0, 0, "e3dc/set/log", "^true|1$", "true", "", "", "", RSCP::eTypeBool, true },
    { 0, 0, "e3dc/set/force", "^true|1$", "true", "", "", "", RSCP::eTypeBool, true }
};

std::vector<rec_cache_t> RscpMqttReceiveCache(rec_cache, rec_cache + sizeof(rec_cache) / sizeof(rec_cache_t));

bool compareRecCache(RSCP_MQTT::rec_cache_t c1, RSCP_MQTT::rec_cache_t c2) {
    return (c1.container < c2.container);
}

}

#endif
