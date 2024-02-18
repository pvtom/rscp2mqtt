#ifndef RSCP_MQTT_MAPPING_H_
#define RSCP_MQTT_MAPPING_H_

#include "RscpTags.h"
#include "RscpTypes.h"
#include <queue>

#define TOPIC_SIZE        128
#define PAYLOAD_SIZE      128
#define UNIT_SIZE         8
#define SOURCE_SIZE       15
#define REGEX_SIZE        160

#define PAYLOAD_REGEX_0_100        "(^[0-9]{1,2}|100$)"
#define PAYLOAD_REGEX_2_DIGIT      "(^[0-9]{1,2}$)"
#define PAYLOAD_REGEX_5_DIGIT      "(^[0-9]{1,5}$)"
#define SET_IDLE_PERIOD_REGEX      "^(mon|tues|wednes|thurs|fri|satur|sun|to)day:(charge|discharge):(true|false):([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]-([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]$"

#define UNIT_NONE         ""
#define UNIT_PERCENT      "%"
#define UNIT_GRAD_C       "°C"
#define UNIT_VA           "VA"
#define UNIT_VAR          "var"
#define UNIT_W            "W"
#define UNIT_WH           "Wh"
#define UNIT_KWH          "kWh"
#define UNIT_V            "V"
#define UNIT_A            "A"
#define UNIT_AH           "Ah"
#define UNIT_SEC          "s"
#define UNIT_MIN          "min"
#define UNIT_HZ           "Hz"

#define F_AUTO            0
#define F_FLOAT_0         1 // "%.0f"
#define F_FLOAT_1         2 // "%0.1f"
#define F_FLOAT_2         3 // "%0.2f"

#define FORCED_TOPIC      0
#define LOG_TOPIC         1
#define INFLUXDB_TOPIC    2

#define IDX_LIMIT_CHARGE_SOC               0
#define IDX_LIMIT_DISCHARGE_SOC            1
#define IDX_LIMIT_CHARGE_DURABLE           2
#define IDX_LIMIT_DISCHARGE_DURABLE        3
#define IDX_LIMIT_DISCHARGE_BY_HOME_POWER  4
#define IDX_BATTERY_STATE                  5
#define IDX_GRID_STATE                     6
#define IDX_SOLAR_POWER_MAX                7
#define IDX_HOME_POWER_MIN                 8
#define IDX_HOME_POWER_MAX                 9
#define IDX_GRID_POWER_MIN                 10
#define IDX_GRID_POWER_MAX                 11
#define IDX_BATTERY_SOC_MIN                12
#define IDX_BATTERY_SOC_MAX                13
#define IDX_GRID_IN_DURATION               14
#define IDX_GRID_SUN_DURATION              15
#define IDX_WALLBOX_INDEX                  16

namespace RSCP_MQTT {

std::string days[8] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", "today"};
int nr_of_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

typedef struct _topic_store_t {
    uint8_t type;
    char topic[TOPIC_SIZE];
} topic_store_t;

std::vector<topic_store_t> TopicStore;

typedef struct _idle_period_t {
    uint8_t marker;
    uint8_t type;
    uint8_t day;
    uint8_t starthour;
    uint8_t startminute;
    uint8_t endhour;
    uint8_t endminute;
    bool active;
} idle_period_t;

std::vector<idle_period_t> IdlePeriodCache;

typedef struct _error_t {
    uint8_t type;
    int32_t code;
    char source[SOURCE_SIZE];
    char message[PAYLOAD_SIZE];
} error_t;

std::vector<error_t> ErrorCache;

typedef struct _not_supported_tags_t {
    uint32_t container;
    uint32_t tag;
} not_supported_tags_t;

std::vector<not_supported_tags_t> NotSupportedTags;

typedef struct _date_t {
    int day;
    int month;
    int year;
} date_t;

std::queue<date_t> requestQ;
std::queue<date_t> paramQ;

typedef struct _cache_t {
    uint32_t container;
    uint32_t tag;
    int index;
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    int format;
    char unit[UNIT_SIZE];
    int divisor;
    int bit_to_bool;
    bool history_log;
    bool changed;
    bool influx;
    bool forced;
} cache_t;

cache_t cache[] = {
    { 0, 0, IDX_LIMIT_CHARGE_SOC, "limit/charge/soc", "0", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, 0, IDX_LIMIT_DISCHARGE_SOC, "limit/discharge/soc", "0", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, 0, IDX_LIMIT_CHARGE_DURABLE, "limit/charge/durable", "0", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, 0, IDX_LIMIT_DISCHARGE_DURABLE, "limit/discharge/durable", "0", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, 0, IDX_LIMIT_DISCHARGE_BY_HOME_POWER, "limit/discharge/by_home_power", "0", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, 0, IDX_BATTERY_STATE, "battery/state", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, 0, IDX_GRID_STATE, "grid/state", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, 0, IDX_SOLAR_POWER_MAX, "solar/power_max", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, 0, IDX_HOME_POWER_MIN, "home/power_min", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, 0, IDX_HOME_POWER_MAX, "home/power_max", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, 0, IDX_GRID_POWER_MIN, "grid/power_min", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, 0, IDX_GRID_POWER_MAX, "grid/power_max", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, 0, IDX_BATTERY_SOC_MIN, "battery/soc_min", "", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, 0, IDX_BATTERY_SOC_MAX, "battery/soc_max", "", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, 0, IDX_GRID_IN_DURATION, "grid_in_limit_duration", "", F_AUTO, UNIT_MIN, 1, 0, false, false, false },
    { 0, 0, IDX_GRID_SUN_DURATION, "sunshine_duration", "", F_AUTO, UNIT_MIN, 1, 0, false, false, false },
    { 0, 0, IDX_WALLBOX_INDEX, "wallbox/index", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_INFO_SW_RELEASE, 0, "system/software", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_INFO_PRODUCTION_DATE, 0, "system/production_date", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_INFO_SERIAL_NUMBER, 0, "system/serial_number", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_INFO_TIME, 0, "time/local", "", F_AUTO, UNIT_SEC, 1, 0, false, false, false },
    { 0, TAG_INFO_UTC_TIME, 0, "time/utc", "", F_AUTO, UNIT_SEC, 1, 0, false, false, false },
    { 0, TAG_INFO_TIME_ZONE, 0, "time/zone", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_POWER_PV, 0, "solar/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_POWER_BAT, 0, "battery/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_POWER_HOME, 0, "home/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_POWER_GRID, 0, "grid/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_POWER_ADD, 0, "addon/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_BAT_SOC, 0, "battery/soc", "", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/charging_lock", "", F_AUTO, UNIT_NONE, 1, 1, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/discharging_lock", "", F_AUTO, UNIT_NONE, 1, 2, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/emergency_power_available", "", F_AUTO, UNIT_NONE, 1, 4, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/charging_throttled", "", F_AUTO, UNIT_NONE, 1, 8, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "grid_in_limit", "", F_AUTO, UNIT_NONE, 1, 16, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/charging_time_lock", "", F_AUTO, UNIT_NONE, 1, 32, false, false, false },
    { 0, TAG_EMS_STATUS, 0, "ems/discharging_time_lock", "", F_AUTO, UNIT_NONE, 1, 64, false, false, false },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "ems/balanced_phases/L1", "", F_AUTO, UNIT_NONE, 1, 1, false, false, false },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "ems/balanced_phases/L2", "", F_AUTO, UNIT_NONE, 2, 1, false, false, false },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "ems/balanced_phases/L3", "", F_AUTO, UNIT_NONE, 4, 1, false, false, false },
    { 0, TAG_EMS_INSTALLED_PEAK_POWER, 0, "system/installed_peak_power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_DERATE_AT_PERCENT_VALUE, 0, "system/derate_at_percent_value", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, TAG_EMS_DERATE_AT_POWER_VALUE, 0, "system/derate_at_power_value", "", F_FLOAT_1, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_SET_POWER, 0, "ems/set_power/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_MODE, 0, "mode", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_COUPLING_MODE, 0, "coupling/mode", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_IDLE_PERIOD_CHANGE_MARKER, 0, "idle_period/change", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_PVI_INVERTER_COUNT, 0, "system/inverter_count", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_BAT_AVAILABLE_BATTERIES, 0, "system/battery_count", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_USED_CHARGE_LIMIT, 0, "ems/used_charge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_BAT_CHARGE_LIMIT, 0, "ems/battery_charge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_DCDC_CHARGE_LIMIT, 0, "ems/dcdc_charge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_USER_CHARGE_LIMIT, 0, "ems/user_charge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_USED_DISCHARGE_LIMIT, 0, "ems/used_discharge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_BAT_DISCHARGE_LIMIT, 0, "ems/battery_discharge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_DCDC_DISCHARGE_LIMIT, 0, "ems/dcdc_discharge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_USER_DISCHARGE_LIMIT, 0, "ems/user_discharge_limit", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_REMAINING_BAT_CHARGE_POWER, 0, "ems/remaining_battery_charge_power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_REMAINING_BAT_DISCHARGE_POWER, 0, "ems/remaining_battery_discharge_power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_EMERGENCY_POWER_STATUS, 0, "ems/emergency_power_status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    // CONTAINER TAG_EMS_GET_POWER_SETTINGS
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_MAX_CHARGE_POWER, 0, "ems/max_charge/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_MAX_DISCHARGE_POWER, 0, "ems/max_discharge/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_POWER_LIMITS_USED, 0, "ems/power_limits", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_DISCHARGE_START_POWER, 0, "ems/discharge_start/power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_POWERSAVE_ENABLED, 0, "ems/power_save", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, 0, "ems/weather_regulation", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    // CONTAINER TAG_EMS_SET_POWER_SETTINGS
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_DISCHARGE_START_POWER, 0, "ems/discharge_start/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_MAX_CHARGE_POWER, 0, "ems/max_charge/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_MAX_DISCHARGE_POWER, 0, "ems/max_discharge/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    // CONTAINER TAG_PVI_DATA
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 0, "pvi/power/L1", "", F_FLOAT_0, UNIT_W, 1, 0, false, false, false },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 1, "pvi/power/L2", "", F_FLOAT_0, UNIT_W, 1, 0, false, false, false },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 2, "pvi/power/L3", "", F_FLOAT_0, UNIT_W, 1, 0, false, false, false },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 0, "pvi/voltage/L1", "", F_FLOAT_0, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 1, "pvi/voltage/L2", "", F_FLOAT_0, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 2, "pvi/voltage/L3", "", F_FLOAT_0, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 0, "pvi/current/L1", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 1, "pvi/current/L2", "", F_FLOAT_2, UNIT_A, 1, 0,false, false, false },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 2, "pvi/current/L3", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_PVI_AC_APPARENTPOWER, TAG_PVI_VALUE, 0, "pvi/apparent_power/L1", "", F_FLOAT_0, UNIT_VA, 1, 0, false, false, false },
    { TAG_PVI_AC_APPARENTPOWER, TAG_PVI_VALUE, 1, "pvi/apparent_power/L2", "", F_FLOAT_0, UNIT_VA, 1, 0, false, false, false },
    { TAG_PVI_AC_APPARENTPOWER, TAG_PVI_VALUE, 2, "pvi/apparent_power/L3", "", F_FLOAT_0, UNIT_VA, 1, 0, false, false, false },
    { TAG_PVI_AC_REACTIVEPOWER, TAG_PVI_VALUE, 0, "pvi/reactive_power/L1", "", F_FLOAT_0, UNIT_VAR, 1, 0, false, false, false },
    { TAG_PVI_AC_REACTIVEPOWER, TAG_PVI_VALUE, 1, "pvi/reactive_power/L2", "", F_FLOAT_0, UNIT_VAR, 1, 0, false, false, false },
    { TAG_PVI_AC_REACTIVEPOWER, TAG_PVI_VALUE, 2, "pvi/reactive_power/L3", "", F_FLOAT_0, UNIT_VAR, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_ALL, TAG_PVI_VALUE, 0, "pvi/energy_all/L1", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_ALL, TAG_PVI_VALUE, 1, "pvi/energy_all/L2", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_ALL, TAG_PVI_VALUE, 2, "pvi/energy_all/L3", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_MAX_APPARENTPOWER, TAG_PVI_VALUE, 0, "pvi/max_apparent_power/L1", "", F_FLOAT_0, UNIT_VA, 1, 0, false, false, false },
    { TAG_PVI_AC_MAX_APPARENTPOWER, TAG_PVI_VALUE, 1, "pvi/max_apparent_power/L2", "", F_FLOAT_0, UNIT_VA, 1, 0, false, false, false },
    { TAG_PVI_AC_MAX_APPARENTPOWER, TAG_PVI_VALUE, 2, "pvi/max_apparent_power/L3", "", F_FLOAT_0, UNIT_VA, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_DAY, TAG_PVI_VALUE, 0, "pvi/energy_day/L1", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_DAY, TAG_PVI_VALUE, 1, "pvi/energy_day/L2", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_DAY, TAG_PVI_VALUE, 2, "pvi/energy_day/L3", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_GRID_CONSUMPTION, TAG_PVI_VALUE, 0, "pvi/energy_grid_consumption/L1", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_GRID_CONSUMPTION, TAG_PVI_VALUE, 1, "pvi/energy_grid_consumption/L2", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_ENERGY_GRID_CONSUMPTION, TAG_PVI_VALUE, 2, "pvi/energy_grid_consumption/L3", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    { TAG_PVI_AC_FREQUENCY, TAG_PVI_VALUE, 0, "pvi/frequency", "", F_FLOAT_2, UNIT_HZ, 1, 0, false, false, false },
    { TAG_PVI_DATA, TAG_PVI_TEMPERATURE_COUNT, 0, "pvi/temperature/count", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_PVI_DATA, TAG_PVI_ON_GRID, 0, "pvi/on_grid", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_PVI_DATA, TAG_PVI_USED_STRING_COUNT, 0, "pvi/used_string_count", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_PVI_COS_PHI, TAG_PVI_COS_PHI_VALUE, 0, "pvi/cos_phi_value", "n/a", F_FLOAT_2, UNIT_NONE, 1000, 0, false, false, false },
    { TAG_PVI_COS_PHI, TAG_PVI_COS_PHI_IS_AKTIV, 0, "pvi/cos_phi_is_active", "n/a", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_PVI_COS_PHI, TAG_PVI_COS_PHI_EXCITED, 0, "pvi/cos_phi_excited", "n/a", F_FLOAT_2, UNIT_NONE, 1000, 0, false, false, false },
    { TAG_PVI_VOLTAGE_MONITORING, TAG_PVI_VOLTAGE_MONITORING_THRESHOLD_TOP, 0, "pvi/voltage_monitoring_threshold_top", "n/a", F_AUTO, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_VOLTAGE_MONITORING, TAG_PVI_VOLTAGE_MONITORING_THRESHOLD_BOTTOM, 0, "pvi/voltage_monitoring_threshold_bottom", "n/a", F_AUTO, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_VOLTAGE_MONITORING, TAG_PVI_VOLTAGE_MONITORING_SLOPE_UP, 0, "pvi/voltage_monitoring_slope_up", "n/a", F_AUTO, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_VOLTAGE_MONITORING, TAG_PVI_VOLTAGE_MONITORING_SLOPE_DOWN, 0, "pvi/voltage_monitoring_slope_down", "n/a", F_AUTO, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_FREQUENCY_UNDER_OVER, TAG_PVI_FREQUENCY_UNDER, 0, "pvi/frequency_under", "n/a", F_FLOAT_2, UNIT_HZ, 1, 0, false, false, false },
    { TAG_PVI_FREQUENCY_UNDER_OVER, TAG_PVI_FREQUENCY_OVER, 0, "pvi/frequency_over", "n/a", F_FLOAT_2, UNIT_HZ, 1, 0, false, false, false },
    // CONTAINER TAG_SE_EP_RESERVE
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE, 0, "reserve/percent", "", F_FLOAT_2, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_W, 0, "reserve/energy", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_MAX_W, 0, "reserve/max", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_LAST_SOC, 0, "reserve/last_soc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // CONTAINER TAG_DB_HISTORY_DATA_...
    // TODAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 0, "battery/energy/charge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 0, "battery/energy/discharge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 0, "solar/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 0, "grid/energy/in", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 0, "grid/energy/out", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 0, "home/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 0, "pm_0/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 0, "pm_1/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_CHARGE_LEVEL, 0, "battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 0, "consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 0, "autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // YESTERDAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 1, "yesterday/battery/energy/charge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 1, "yesterday/battery/energy/discharge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 1, "yesterday/solar/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 1, "yesterday/grid/energy/in", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 1, "yesterday/grid/energy/out", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 1, "yesterday/home/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 1, "yesterday/pm_0/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 1, "yesterday/pm_1/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_CHARGE_LEVEL, 1, "yesterday/battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 1, "yesterday/consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 1, "yesterday/autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // WEEK
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_POWER_IN, 0, "week/battery/energy/charge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_POWER_OUT, 0, "week/battery/energy/discharge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_DC_POWER, 0, "week/solar/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_GRID_POWER_IN, 0, "week/grid/energy/in", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_GRID_POWER_OUT, 0, "week/grid/energy/out", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_CONSUMPTION, 0, "week/home/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_PM_0_POWER, 0, "week/pm_0/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_PM_1_POWER, 0, "week/pm_1/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_CHARGE_LEVEL, 0, "week/battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_CONSUMED_PRODUCTION, 0, "week/consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_AUTARKY, 0, "week/autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // MONTH
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_POWER_IN, 0, "month/battery/energy/charge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_POWER_OUT, 0, "month/battery/energy/discharge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_DC_POWER, 0, "month/solar/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_GRID_POWER_IN, 0, "month/grid/energy/in", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_GRID_POWER_OUT, 0, "month/grid/energy/out", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_CONSUMPTION, 0, "month/home/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_PM_0_POWER, 0, "month/pm_0/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_PM_1_POWER, 0, "month/pm_1/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_CHARGE_LEVEL, 0, "month/battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_CONSUMED_PRODUCTION, 0, "month/consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_AUTARKY, 0, "month/autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // YEAR
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_IN, 0, "year/battery/energy/charge", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_OUT, 0, "year/battery/energy/discharge", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_DC_POWER, 0, "year/solar/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_IN, 0, "year/grid/energy/in", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_OUT, 0, "year/grid/energy/out", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMPTION, 0, "year/home/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_PM_0_POWER, 0, "year/pm_0/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_PM_1_POWER, 0, "year/pm_1/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_CHARGE_LEVEL, 0, "year/battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMED_PRODUCTION, 0, "year/consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_AUTARKY, 0, "year/autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // WALLBOX
    { 0, TAG_EMS_POWER_WB_ALL, 0, "wallbox/power/total", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_POWER_WB_SOLAR, 0, "wallbox/power/solar", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { 0, TAG_EMS_BATTERY_TO_CAR_MODE, 0, "wallbox/battery_to_car", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_BATTERY_BEFORE_CAR_MODE, 0, "wallbox/battery_before_car", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { 0, TAG_EMS_GET_WB_DISCHARGE_BAT_UNTIL, 0, "wallbox/battery_discharge_until", "", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { 0, TAG_EMS_GET_WALLBOX_ENFORCE_POWER_ASSIGNMENT, 0, "wallbox/disable_battery_at_mix_mode", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_DEVICE_STATE, 0, "wallbox/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_ACTIVE_PHASES, 0, "wallbox/active_phases", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_NUMBER_PHASES, 0, "wallbox/number_phases", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_SOC, 0, "wallbox/soc", "", F_AUTO, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_KEY_STATE, 0, "wallbox/key_state", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_ENERGY_ALL, 0, "wallbox/energy/total", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_ENERGY_SOLAR, 0, "wallbox/energy/solar", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_ENERGY_L1, 0, "wallbox/energy/L1", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_ENERGY_L2, 0, "wallbox/energy/L2", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_ENERGY_L3, 0, "wallbox/energy/L3", "", F_FLOAT_2, UNIT_WH, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_POWER_L1, 0, "wallbox/power/L1", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_POWER_L2, 0, "wallbox/power/L2", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_WB_DATA, TAG_WB_PM_POWER_L3, 0, "wallbox/power/L3", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 1, "wallbox/number_used_phases", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 3, "wallbox/max_current", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/plugged", "", F_AUTO, UNIT_NONE, 1, 8, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/locked", "", F_AUTO, UNIT_NONE, 1, 16, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/charging", "", F_AUTO, UNIT_NONE, 1, 32, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/canceled", "", F_AUTO, UNIT_NONE, 1, 64, false, false, false },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/sun_mode", "", F_AUTO, UNIT_NONE, 1, 128, false, false, false }
};

std::vector<cache_t> RscpMqttCache(cache, cache + sizeof(cache) / sizeof(cache_t));

cache_t templates[] = {
    // DAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 1, "day/%d/%d/%d/battery/energy/charge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 1, "day/%d/%d/%d/battery/energy/discharge", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 1, "day/%d/%d/%d/solar/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 1, "day/%d/%d/%d/grid/energy/in", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 1, "day/%d/%d/%d/grid/energy/out", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 1, "day/%d/%d/%d/home/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 1, "day/%d/%d/%d/pm_0/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 1, "day/%d/%d/%d/pm_1/energy", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_CHARGE_LEVEL, 1, "day/%d/%d/%d/battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 1, "day/%d/%d/%d/consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 1, "day/%d/%d/%d/autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // YEAR
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_IN, 1, "history/%d/battery/energy/charge", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_OUT, 1, "history/%d/battery/energy/discharge", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_DC_POWER, 1, "history/%d/solar/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_IN, 1, "history/%d/grid/energy/in", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_OUT, 1, "history/%d/grid/energy/out", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMPTION, 1, "history/%d/home/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_PM_0_POWER, 1, "history/%d/pm_0/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_PM_1_POWER, 1, "history/%d/pm_1/energy", "", F_FLOAT_0, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_CHARGE_LEVEL, 1, "history/%d/battery/rsoc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMED_PRODUCTION, 1, "history/%d/consumed", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_AUTARKY, 1, "history/%d/autarky", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    // TAG_BAT_DATA
    { TAG_BAT_DATA, TAG_BAT_RSOC, 1, "%s/soc_float", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_RSOC_REAL, 1, "%s/rsoc_real", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_MODULE_VOLTAGE, 1, "%s/voltage", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_CURRENT, 1, "%s/current", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_CHARGE_CYCLES, 1, "%s/cycles", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_STATUS_CODE, 1, "%s/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_ERROR_CODE, 1, "%s/error", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_DEVICE_NAME, 1, "%s/name", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_DCB_COUNT, 1, "%s/dcb/count", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_TRAINING_MODE, 1, "%s/training", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_MAX_DCB_CELL_TEMPERATURE, 1, "%s/temperature/max", "", F_FLOAT_1, UNIT_GRAD_C, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_MIN_DCB_CELL_TEMPERATURE, 1, "%s/temperature/min", "", F_FLOAT_1, UNIT_GRAD_C, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_ASOC, 1, "%s/soh", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_BAT_SPECIFICATION, TAG_BAT_ROLE, 1, "%s/role", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_SPECIFICATION, TAG_BAT_SPECIFIED_CAPACITY, 1, "%s/specified_capacity", "", F_AUTO, UNIT_WH, 1, 0, false, false, false },
    { TAG_BAT_SPECIFICATION, TAG_BAT_SPECIFIED_DSCHARGE_POWER, 1, "%s/specified_discharge_power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_BAT_SPECIFICATION, TAG_BAT_SPECIFIED_CHARGE_POWER, 1, "%s/specified_charge_power", "", F_AUTO, UNIT_W, 1, 0, false, false, false },
    { TAG_BAT_SPECIFICATION, TAG_BAT_SPECIFIED_MAX_DCB_COUNT, 1, "%s/specified_max_dcb_count", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_USABLE_CAPACITY, 1, "%s/usable_capacity", "", F_AUTO, UNIT_AH, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_USABLE_REMAINING_CAPACITY, 1, "%s/usable_remaining_capacity", "", F_AUTO, UNIT_AH, 1, 0, false, false, false },
    { TAG_BAT_DATA, TAG_BAT_DESIGN_CAPACITY, 1, "%s/design_capacity", "", F_AUTO, UNIT_AH, 1, 0, false, false, false },
    // DCB
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MAX_CHARGE_VOLTAGE, 1, "battery/dcb/%d/max_charge_voltage", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MAX_CHARGE_CURRENT, 1, "battery/dcb/%d/max_charge_current", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MAX_DISCHARGE_CURRENT, 1, "battery/dcb/%d/max_discharge_current", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_FULL_CHARGE_CAPACITY, 1, "battery/dcb/%d/full_charge_capacity", "", F_FLOAT_2, UNIT_AH, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_REMAINING_CAPACITY, 1, "battery/dcb/%d/remaining_capacity", "", F_FLOAT_2, UNIT_AH, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SOC, 1, "battery/dcb/%d/soc", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SOH, 1, "battery/dcb/%d/soh", "", F_FLOAT_1, UNIT_PERCENT, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CYCLE_COUNT, 1, "battery/dcb/%d/cycles", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CURRENT, 1, "battery/dcb/%d/current", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_VOLTAGE, 1, "battery/dcb/%d/voltage", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CURRENT_AVG_30S, 1, "battery/dcb/%d/current_avg_30s", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_VOLTAGE_AVG_30S, 1, "battery/dcb/%d/voltage_avg_30s", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_NR_SENSOR, 1, "battery/dcb/%d/nr_sensor", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DESIGN_CAPACITY, 1, "battery/dcb/%d/design_capacity", "", F_FLOAT_2, UNIT_AH, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DESIGN_VOLTAGE, 1, "battery/dcb/%d/design_voltage", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CHARGE_LOW_TEMPERATURE, 1, "battery/dcb/%d/charge_low_temperature", "", F_FLOAT_1, UNIT_GRAD_C, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CHARGE_HIGH_TEMPERATURE, 1, "battery/dcb/%d/charge_high_temperature", "", F_FLOAT_1, UNIT_GRAD_C, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MANUFACTURE_DATE, 1, "battery/dcb/%d/manufacture_date", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SERIALNO, 1, "battery/dcb/%d/serial_number", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_PROTOCOL_VERSION, 1, "battery/dcb/%d/protocol_version", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_FW_VERSION, 1, "battery/dcb/%d/firmware_version", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DATA_TABLE_VERSION, 1, "battery/dcb/%d/table_version", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_PCB_VERSION, 1, "battery/dcb/%d/pcb_version", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_NR_SERIES_CELL, 1, "battery/dcb/%d/nr_series_cell", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_NR_PARALLEL_CELL, 1, "battery/dcb/%d/nr_parallel_cell", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MANUFACTURE_NAME, 1, "battery/dcb/%d/manufacture_name", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DEVICE_NAME, 1, "battery/dcb/%d/device_name", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SERIALCODE, 1, "battery/dcb/%d/serial_code", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_STATUS, 1, "battery/dcb/%d/status", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_WARNING, 1, "battery/dcb/%d/warning", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_ALARM, 1, "battery/dcb/%d/alarm", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_ERROR, 1, "battery/dcb/%d/error", "", F_AUTO, UNIT_NONE, 1, 0, false, false, false },
    // PVI
    { TAG_PVI_TEMPERATURE, TAG_PVI_VALUE, 1, "pvi/temperature/%d", "", F_FLOAT_1, UNIT_GRAD_C, 1, 0, false, false, false },
    { TAG_PVI_DC_POWER, TAG_PVI_VALUE, 1, "pvi/power/string_%d", "", F_FLOAT_0, UNIT_W, 1, 0, false, false, false },
    { TAG_PVI_DC_VOLTAGE, TAG_PVI_VALUE, 1, "pvi/voltage/string_%d", "", F_FLOAT_0, UNIT_V, 1, 0, false, false, false },
    { TAG_PVI_DC_CURRENT, TAG_PVI_VALUE, 1, "pvi/current/string_%d", "", F_FLOAT_2, UNIT_A, 1, 0, false, false, false },
    { TAG_PVI_DC_STRING_ENERGY_ALL, TAG_PVI_VALUE, 1, "pvi/energy_all/string_%d", "", F_FLOAT_0, UNIT_WH, 1, 0, false, false, false },
    // PM
    { TAG_PM_DATA, TAG_PM_POWER_L1, 0, "%s/power/L1", "", F_FLOAT_2, UNIT_W, 1, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_POWER_L2, 0, "%s/power/L2", "", F_FLOAT_2, UNIT_W, 1, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_POWER_L3, 0, "%s/power/L3", "", F_FLOAT_2, UNIT_W, 1, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_ENERGY_L1, 0, "%s/energy/L1", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_ENERGY_L2, 0, "%s/energy/L2", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_ENERGY_L3, 0, "%s/energy/L3", "", F_FLOAT_2, UNIT_KWH, 1000, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L1, 0, "%s/voltage/L1", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L2, 0, "%s/voltage/L2", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L3, 0, "%s/voltage/L3", "", F_FLOAT_2, UNIT_V, 1, 0, false, false, false },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "%s/active_phases/L1", "", F_AUTO, UNIT_NONE, 1, 1, false, false, false },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "%s/active_phases/L2", "", F_AUTO, UNIT_NONE, 1, 2, false, false, false },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "%s/active_phases/L3", "", F_AUTO, UNIT_NONE, 1, 4, false, false, false }
};

std::vector<cache_t> RscpMqttCacheTempl(templates, templates + sizeof(templates) / sizeof(cache_t));

typedef struct _rec_cache_t {
    uint32_t container;
    uint32_t tag;
    char topic[TOPIC_SIZE];
    char regex_true[REGEX_SIZE];
    char value_true[PAYLOAD_SIZE];
    char regex_false[REGEX_SIZE];
    char value_false[PAYLOAD_SIZE];
    char payload[PAYLOAD_SIZE];
    char unit[UNIT_SIZE];
    int type;
    int refresh_count;
    bool queue;
    bool done;
} rec_cache_t;

rec_cache_t rec_cache[] = {
    { 0, TAG_EMS_REQ_START_MANUAL_CHARGE, "set/manual_charge", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, false, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, "set/weather_regulation", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_POWER_LIMITS_USED, "set/power_limits", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_MAX_CHARGE_POWER, "set/max_charge_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, false, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_MAX_DISCHARGE_POWER, "set/max_discharge_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, false, true },
    { TAG_EMS_REQ_SET_POWER, TAG_EMS_REQ_SET_POWER_MODE, "power_mode: mode", "", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, 0, false, true },
    { TAG_EMS_REQ_SET_POWER, TAG_EMS_REQ_SET_POWER_VALUE, "power_mode: value", "", "", "", "", "", UNIT_NONE, RSCP::eTypeInt32, 0, false, true },
    { TAG_SE_REQ_SET_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE, "set/reserve/percent", PAYLOAD_REGEX_0_100, "", "", "", "", UNIT_PERCENT, RSCP::eTypeFloat32, -1, false, true },
    { TAG_SE_REQ_SET_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_W, "set/reserve/energy", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_WH, RSCP::eTypeFloat32, -1, false, true },
    { 0, 0, "set/power_mode", "^auto$|^idle:[0-9]{1,4}$|^charge:[0-9]{1,5}:[0-9]{1,4}$|^discharge:[0-9]{1,5}:[0-9]{1,4}$|^grid_charge:[0-9]{1,5}:[0-9]{1,4}$", "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/idle_period", SET_IDLE_PERIOD_REGEX, "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, TAG_EMS_REQ_SET_BATTERY_TO_CAR_MODE, "set/wallbox/battery_to_car", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, TAG_EMS_REQ_SET_BATTERY_BEFORE_CAR_MODE, "set/wallbox/battery_before_car", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, TAG_EMS_REQ_SET_WB_DISCHARGE_BAT_UNTIL, "set/wallbox/battery_discharge_until", PAYLOAD_REGEX_2_DIGIT, "", "", "", "", UNIT_PERCENT, RSCP::eTypeUChar8, -1, false, true },
    { 0, TAG_EMS_REQ_SET_WALLBOX_ENFORCE_POWER_ASSIGNMENT, "set/wallbox/disable_battery_at_mix_mode", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, "set/wallbox/control", "^solar:[0-9]{1,2}$|^mix:[0-9]{1,2}$|^stop$", "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { TAG_WB_REQ_DATA, TAG_WB_REQ_SET_NUMBER_PHASES, "set/wallbox/number_phases", "^1|3$", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/requests/pm", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/requests/pvi", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/requests/dcb", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/soc_limiter", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/statistic_values", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/daily_values", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/limit/charge/soc", PAYLOAD_REGEX_0_100, "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/limit/charge/durable", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/limit/discharge/soc", PAYLOAD_REGEX_0_100, "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/limit/discharge/durable", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/limit/discharge/by_home_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, false, true },
    { 0, 0, "set/log", "^true|on|1$", "true", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/health", "^true|on|1$", "true", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/force", "[a-zA-z0-9/_.*]*", "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, false, true },
    { 0, 0, "set/interval", "^[1-9]|[1-9][0-9]|[1-2][0-9][0-9]|300$", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/wallbox/index", "^[0-7]$", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, false, true },
    { 0, 0, "set/request/day", "^[0-9]{4}-[0-1]?[0-9]-[0-3]?[0-9]$", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, true, true }
};

std::vector<rec_cache_t> RscpMqttReceiveCache(rec_cache, rec_cache + sizeof(rec_cache) / sizeof(rec_cache_t));

bool compareRecCache(RSCP_MQTT::rec_cache_t c1, RSCP_MQTT::rec_cache_t c2) {
    return (c1.container < c2.container);
}

}

#endif
