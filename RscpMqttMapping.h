#ifndef RSCP_MQTT_MAPPING_H_
#define RSCP_MQTT_MAPPING_H_

#include "RscpTags.h"
#include "RscpTypes.h"

#define TOPIC_SIZE        128
#define PAYLOAD_SIZE      128
#define UNIT_SIZE         8
#define SOURCE_SIZE       15
#define REGEX_SIZE        160

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

#define FORCED_TOPIC      0

namespace RSCP_MQTT {

std::string days[8] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", "today"};

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

typedef struct _cache_t {
    uint32_t container;
    uint32_t tag;
    int index;
    char topic[TOPIC_SIZE];
    char fstring[PAYLOAD_SIZE];
    char payload[PAYLOAD_SIZE];
    char unit[UNIT_SIZE];
    int type;
    int divisor;
    int bit_to_bool;
    int publish;
} cache_t;

cache_t cache[] = {
    { 0, TAG_INFO_SW_RELEASE, 0, "system/software", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_INFO_PRODUCTION_DATE, 0, "system/production_date", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_INFO_SERIAL_NUMBER, 0, "system/serial_number", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_INFO_TIME_ZONE, 0, "time/zone", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_EMS_POWER_PV, 0, "solar/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_BAT, 0, "battery/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_HOME, 0, "home/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_GRID, 0, "grid/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_ADD, 0, "addon/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_BAT_SOC, 0, "battery/soc", "%u", "", UNIT_PERCENT, RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_STATUS, 0, "ems/charging_lock", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 1, 0 },
    { 0, TAG_EMS_STATUS, 0, "ems/discharging_lock", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 2, 0 },
    { 0, TAG_EMS_STATUS, 0, "ems/emergency_power_available", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 4, 0 },
    { 0, TAG_EMS_STATUS, 0, "ems/charging_throttled", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 8, 0 },
    { 0, TAG_EMS_STATUS, 0, "grid_in_limit", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 16, 0 },
    { 0, TAG_EMS_STATUS, 0, "ems/charging_time_lock", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 32, 0 },
    { 0, TAG_EMS_STATUS, 0, "ems/discharging_time_lock", "%s", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 64, 0 },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "ems/balanced_phases/L1", "%s", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 1, 0 },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "ems/balanced_phases/L2", "%s", "", UNIT_NONE, RSCP::eTypeUChar8, 2, 1, 0 },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "ems/balanced_phases/L3", "%s", "", UNIT_NONE, RSCP::eTypeUChar8, 4, 1, 0 },
    { 0, TAG_EMS_INSTALLED_PEAK_POWER, 0, "system/installed_peak_power", "%u", "", UNIT_W, RSCP::eTypeUInt32, 1, 0, 0 },
    { 0, TAG_EMS_DERATE_AT_PERCENT_VALUE, 0, "system/derate_at_percent_value", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { 0, TAG_EMS_DERATE_AT_POWER_VALUE, 0, "system/derate_at_power_value", "%0.1f", "", UNIT_W, RSCP::eTypeFloat32, 1, 0, 0 },
    { 0, TAG_EMS_SET_POWER, 0, "ems/set_power/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_MODE, 0, "mode", "%i", "", UNIT_NONE, RSCP::eTypeChar8, 1, 0, 0 },
    { 0, TAG_EMS_COUPLING_MODE, 0, "coupling/mode", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_IDLE_PERIOD_CHANGE_MARKER, 0, "idle_period/change", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    // CONTAINER TAG_BAT_DATA --------------------------------------------------------------------
    { TAG_BAT_DATA, TAG_BAT_RSOC, 0, "battery/rsoc", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_MODULE_VOLTAGE, 0, "battery/voltage", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_CURRENT, 0, "battery/current", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_CHARGE_CYCLES, 0, "battery/cycles", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_STATUS_CODE, 0, "battery/status", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_ERROR_CODE, 0, "battery/error", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_DEVICE_NAME, 0, "battery/name", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_DCB_COUNT, 0, "battery/dcb/count", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_TRAINING_MODE, 0, "battery/training", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_MAX_DCB_CELL_TEMPERATURE, 0, "battery/temperature/max", "%0.1f", "", UNIT_GRAD_C, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_MIN_DCB_CELL_TEMPERATURE, 0, "battery/temperature/min", "%0.1f", "", UNIT_GRAD_C, RSCP::eTypeFloat32, 1, 0, 0 },
    // CONTAINER TAG_EMS_GET_POWER_SETTINGS ------------------------------------------------------
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_MAX_CHARGE_POWER, 0, "ems/max_charge/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_MAX_DISCHARGE_POWER, 0, "ems/max_discharge/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_POWER_LIMITS_USED, 0, "ems/power_limits", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_DISCHARGE_START_POWER, 0, "ems/discharge_start/power", "%u", "", UNIT_W, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_POWERSAVE_ENABLED, 0, "ems/power_save", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, 0, "ems/weather_regulation", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 0, 0 },
    // CONTAINER TAG_EMS_SET_POWER_SETTINGS ------------------------------------------------------
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_DISCHARGE_START_POWER, 0, "ems/discharge_start/status", "%i", "", UNIT_NONE, RSCP::eTypeChar8, 1, 0, 0 },
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_MAX_CHARGE_POWER, 0, "ems/max_charge/status", "%i", "", UNIT_NONE, RSCP::eTypeChar8, 1, 0, 0 },
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_MAX_DISCHARGE_POWER, 0, "ems/max_discharge/status", "%i", "", UNIT_NONE, RSCP::eTypeChar8, 1, 0, 0 },
    // CONTAINER TAG_PM_DATA ---------------------------------------------------------------------
    { TAG_PM_DATA, TAG_PM_POWER_L1, 0, "pm/power/L1", "%.0lf", "", UNIT_W, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_POWER_L2, 0, "pm/power/L2", "%.0lf", "", UNIT_W, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_POWER_L3, 0, "pm/power/L3", "%.0lf", "", UNIT_W, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ENERGY_L1, 0, "pm/energy/L1", "%.0lf", "", UNIT_WH, RSCP::eTypeDouble64, 1000, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ENERGY_L2, 0, "pm/energy/L2", "%.0lf", "", UNIT_WH, RSCP::eTypeDouble64, 1000, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ENERGY_L3, 0, "pm/energy/L3", "%.0lf", "", UNIT_WH, RSCP::eTypeDouble64, 1000, 0, 0 },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L1, 0, "pm/voltage/L1", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L2, 0, "pm/voltage/L2", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L3, 0, "pm/voltage/L3", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "pm/active_phases/L1", "%s", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 1, 0 },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "pm/active_phases/L2", "%s", "", UNIT_NONE, RSCP::eTypeUChar8, 2, 1, 0 },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "pm/active_phases/L3", "%s", "", UNIT_NONE, RSCP::eTypeUChar8, 4, 1, 0 },
    // CONTAINER TAG_PVI_DATA --------------------------------------------------------------------
    { TAG_PVI_DC_POWER, TAG_PVI_VALUE, 0, "pvi/power/string_1", "%.0f", "", UNIT_W, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_POWER, TAG_PVI_VALUE, 1, "pvi/power/string_2", "%.0f", "", UNIT_W, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_VOLTAGE, TAG_PVI_VALUE, 0, "pvi/voltage/string_1", "%.0f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_VOLTAGE, TAG_PVI_VALUE, 1, "pvi/voltage/string_2", "%.0f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_CURRENT, TAG_PVI_VALUE, 0, "pvi/current/string_1", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_CURRENT, TAG_PVI_VALUE, 1, "pvi/current/string_2", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_STRING_ENERGY_ALL, TAG_PVI_VALUE, 0, "pvi/energy_all/string_1", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_DC_STRING_ENERGY_ALL, TAG_PVI_VALUE, 1, "pvi/energy_all/string_2", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 0, "pvi/power/L1", "%.0f", "", UNIT_W, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 1, "pvi/power/L2", "%.0f", "", UNIT_W, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 2, "pvi/power/L3", "%.0f", "", UNIT_W, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 0, "pvi/voltage/L1", "%.0f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 1, "pvi/voltage/L2", "%.0f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 2, "pvi/voltage/L3", "%.0f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 0, "pvi/current/L1", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 1, "pvi/current/L2", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 2, "pvi/current/L3", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_APPARENTPOWER, TAG_PVI_VALUE, 0, "pvi/apparent_power/L1", "%.0f", "", UNIT_VA, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_APPARENTPOWER, TAG_PVI_VALUE, 1, "pvi/apparent_power/L2", "%.0f", "", UNIT_VA, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_APPARENTPOWER, TAG_PVI_VALUE, 2, "pvi/apparent_power/L3", "%.0f", "", UNIT_VA, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_REACTIVEPOWER, TAG_PVI_VALUE, 0, "pvi/reactive_power/L1", "%.0f", "", UNIT_VAR, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_REACTIVEPOWER, TAG_PVI_VALUE, 1, "pvi/reactive_power/L2", "%.0f", "", UNIT_VAR, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_REACTIVEPOWER, TAG_PVI_VALUE, 2, "pvi/reactive_power/L3", "%.0f", "", UNIT_VAR, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_ALL, TAG_PVI_VALUE, 0, "pvi/energy_all/L1", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_ALL, TAG_PVI_VALUE, 1, "pvi/energy_all/L2", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_ALL, TAG_PVI_VALUE, 2, "pvi/energy_all/L3", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_AC_MAX_APPARENTPOWER, TAG_PVI_VALUE, 0, "pvi/max_apparent_power/L1", "%.0f", "", UNIT_VA, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_MAX_APPARENTPOWER, TAG_PVI_VALUE, 1, "pvi/max_apparent_power/L2", "%.0f", "", UNIT_VA, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_MAX_APPARENTPOWER, TAG_PVI_VALUE, 2, "pvi/max_apparent_power/L3", "%.0f", "", UNIT_VA, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_DAY, TAG_PVI_VALUE, 0, "pvi/energy_day/L1", "%.0f", "", UNIT_WH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_DAY, TAG_PVI_VALUE, 1, "pvi/energy_day/L2", "%.0f", "", UNIT_WH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_DAY, TAG_PVI_VALUE, 2, "pvi/energy_day/L3", "%.0f", "", UNIT_WH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_GRID_CONSUMPTION, TAG_PVI_VALUE, 0, "pvi/energy_grid_consumption/L1", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_GRID_CONSUMPTION, TAG_PVI_VALUE, 1, "pvi/energy_grid_consumption/L2", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_AC_ENERGY_GRID_CONSUMPTION, TAG_PVI_VALUE, 2, "pvi/energy_grid_consumption/L3", "%.0f", "", UNIT_WH, RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PVI_DATA, TAG_PVI_TEMPERATURE_COUNT, 0, "pvi/temperature/count", "%i", "", UNIT_NONE, RSCP::eTypeInt32, 1, 0, false },
    { TAG_PVI_DATA, TAG_PVI_ON_GRID, 0, "pvi/on_grid", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 0, 0 },
    // CONTAINER TAG_SE_EP_RESERVE ---------------------------------------------------------------
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE, 0, "reserve/percent", "%0.2f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_W, 0, "reserve/energy", "%0.2f", "", UNIT_WH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_MAX_W, 0, "reserve/max", "%0.2f", "", UNIT_WH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_LAST_SOC, 0, "reserve/last_soc", "%0.2f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    // CONTAINER TAG_DB_HISTORY_DATA_... ---------------------------------------------------------
    // TODAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 0, "battery/energy/charge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 0, "battery/energy/discharge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 0, "solar/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 0, "grid/energy/in", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 0, "grid/energy/out", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 0, "home/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 0, "pm_0/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 0, "pm_1/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 0, "consumed", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 0, "autarky", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    // YESTERDAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 1, "yesterday/battery/energy/charge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 1, "yesterday/battery/energy/discharge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 1, "yesterday/solar/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 1, "yesterday/grid/energy/in", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 1, "yesterday/grid/energy/out", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 1, "yesterday/home/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 1, "yesterday/pm_0/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 1, "yesterday/pm_1/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_CHARGE_LEVEL, 1, "yesterday/battery/rsoc", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 1, "yesterday/consumed", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 1, "yesterday/autarky", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    // WEEK
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_POWER_IN, 0, "week/battery/energy/charge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_POWER_OUT, 0, "week/battery/energy/discharge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_DC_POWER, 0, "week/solar/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_GRID_POWER_IN, 0, "week/grid/energy/in", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_GRID_POWER_OUT, 0, "week/grid/energy/out", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_CONSUMPTION, 0, "week/home/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_CONSUMED_PRODUCTION, 0, "week/consumed", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_AUTARKY, 0, "week/autarky", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    // MONTH
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_POWER_IN, 0, "month/battery/energy/charge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_POWER_OUT, 0, "month/battery/energy/discharge", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_DC_POWER, 0, "month/solar/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_GRID_POWER_IN, 0, "month/grid/energy/in", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_GRID_POWER_OUT, 0, "month/grid/energy/out", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_CONSUMPTION, 0, "month/home/energy", "%0.2f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_CONSUMED_PRODUCTION, 0, "month/consumed", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_AUTARKY, 0, "month/autarky", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    // YEAR
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_IN, 0, "year/battery/energy/charge", "%.0f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_OUT, 0, "year/battery/energy/discharge", "%.0f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_DC_POWER, 0, "year/solar/energy", "%.0f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_IN, 0, "year/grid/energy/in", "%.0f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_OUT, 0, "year/grid/energy/out", "%.0f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMPTION, 0, "year/home/energy", "%.0f", "", UNIT_KWH, RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMED_PRODUCTION, 0, "year/consumed", "%.0f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_AUTARKY, 0, "year/autarky", "%.0f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    // WALLBOX
    { 0, TAG_EMS_POWER_WB_ALL, 0, "wallbox/total/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_WB_SOLAR, 0, "wallbox/solar/power", "%i", "", UNIT_W, RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_BATTERY_TO_CAR_MODE, 0, "wallbox/battery_to_car", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_BATTERY_BEFORE_CAR_MODE, 0, "wallbox/battery_before_car", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_GET_WB_DISCHARGE_BAT_UNTIL, 0, "wallbox/battery_discharge_until", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_GET_WALLBOX_ENFORCE_POWER_ASSIGNMENT, 0, "wallbox/disable_battery_at_mix_mode", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 0, 0 },
    { TAG_WB_DATA, TAG_WB_DEVICE_STATE, 0, "wallbox/status", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 0, 0 },
    { TAG_WB_DATA, TAG_WB_PM_ACTIVE_PHASES, 0, "wallbox/active_phases", "%i", "", UNIT_NONE, RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 1, "wallbox/number_used_phases", "%i", "", UNIT_NONE, RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 3, "wallbox/max_current", "%i", "", UNIT_NONE, RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/plugged", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 8, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/locked", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 16, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/charging", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 32, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/canceled", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 64, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "wallbox/sun_mode", "%s", "", UNIT_NONE, RSCP::eTypeBool, 1, 128, 0 }
};

std::vector<cache_t> RscpMqttCache(cache, cache + sizeof(cache) / sizeof(cache_t));

cache_t templates[] = {
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MAX_CHARGE_VOLTAGE, -1, "battery/dcb/%d/max_charge_voltage", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MAX_CHARGE_CURRENT, -1, "battery/dcb/%d/max_charge_current", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MAX_DISCHARGE_CURRENT, -1, "battery/dcb/%d/max_discharge_current", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_FULL_CHARGE_CAPACITY, -1, "battery/dcb/%d/full_charge_capacity", "%0.2f", "", UNIT_AH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_REMAINING_CAPACITY, -1, "battery/dcb/%d/remaining_capacity", "%0.2f", "", UNIT_AH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SOC, -1, "battery/dcb/%d/soc", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SOH, -1, "battery/dcb/%d/soh", "%0.1f", "", UNIT_PERCENT, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CYCLE_COUNT, -1, "battery/dcb/%d/cycles", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CURRENT, -1, "battery/dcb/%d/current", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_VOLTAGE, -1, "battery/dcb/%d/voltage", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CURRENT_AVG_30S, -1, "battery/dcb/%d/current_avg_30s", "%0.2f", "", UNIT_A, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_VOLTAGE_AVG_30S, -1, "battery/dcb/%d/voltage_avg_30s", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_NR_SENSOR, -1, "battery/dcb/%d/nr_sensor", "%u", "", UNIT_NONE, RSCP::eTypeUChar8, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DESIGN_CAPACITY, -1, "battery/dcb/%d/design_capacity", "%0.2f", "", UNIT_AH, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DESIGN_VOLTAGE, -1, "battery/dcb/%d/design_voltage", "%0.2f", "", UNIT_V, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CHARGE_LOW_TEMPERATURE, -1, "battery/dcb/%d/charge_low_temperature", "%0.1f", "", UNIT_GRAD_C, RSCP::eTypeFloat32, 1, 0, 0 }, 
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_CHARGE_HIGH_TEMPERATURE, -1, "battery/dcb/%d/charge_high_temperature", "%0.1f", "", UNIT_GRAD_C, RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MANUFACTURE_DATE, -1, "battery/dcb/%d/manufacture_date", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SERIALNO, -1, "battery/dcb/%d/serial_number", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_PROTOCOL_VERSION, -1, "battery/dcb/%d/protocol_version", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_FW_VERSION, -1, "battery/dcb/%d/firmware_version", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DATA_TABLE_VERSION, -1, "battery/dcb/%d/table_version", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_PCB_VERSION, -1, "battery/dcb/%d/pcb_version", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_NR_SERIES_CELL, -1, "battery/dcb/%d/nr_series_cell", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_NR_PARALLEL_CELL, -1, "battery/dcb/%d/nr_parallel_cell", "%u", "", UNIT_NONE, RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_MANUFACTURE_NAME, -1, "battery/dcb/%d/manufacture_name", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_DEVICE_NAME, -1, "battery/dcb/%d/device_name", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_SERIALCODE, -1, "battery/dcb/%d/serial_code", "%s", "", UNIT_NONE, RSCP::eTypeString, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_STATUS, -1, "battery/dcb/%d/status", "%u", "", UNIT_NONE, RSCP::eTypeUInt16, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_WARNING, -1, "battery/dcb/%d/warning", "%u", "", UNIT_NONE, RSCP::eTypeUInt16, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_ALARM, -1, "battery/dcb/%d/alarm", "%u", "", UNIT_NONE, RSCP::eTypeUInt16, 1, 0, 0 },
    { TAG_BAT_DCB_INFO, TAG_BAT_DCB_ERROR, -1, "battery/dcb/%d/error", "%u", "", UNIT_NONE, RSCP::eTypeUInt16, 1, 0, 0 },
    { TAG_PVI_TEMPERATURE, TAG_PVI_VALUE, -1, "pvi/temperature/%d", "%0.1f", "", UNIT_GRAD_C, RSCP::eTypeFloat32, 1, 0, 0 }
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
    bool done;
} rec_cache_t;

rec_cache_t rec_cache[] = {
    { 0, TAG_EMS_REQ_START_MANUAL_CHARGE, "set/manual_charge", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, "set/weather_regulation", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_POWER_LIMITS_USED, "set/power_limits", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_MAX_CHARGE_POWER, "set/max_charge_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_MAX_DISCHARGE_POWER, "set/max_discharge_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_W, RSCP::eTypeUInt32, -1, true },
    { TAG_EMS_REQ_SET_POWER, TAG_EMS_REQ_SET_POWER_MODE, "power_mode: mode", "", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, 0, true },
    { TAG_EMS_REQ_SET_POWER, TAG_EMS_REQ_SET_POWER_VALUE, "power_mode: value", "", "", "", "", "", UNIT_NONE, RSCP::eTypeInt32, 0, true },
    { TAG_SE_REQ_SET_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE, "set/reserve/percent", PAYLOAD_REGEX_2_DIGIT, "", "", "", "", UNIT_PERCENT, RSCP::eTypeFloat32, -1, true },
    { TAG_SE_REQ_SET_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_W, "set/reserve/energy", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", UNIT_WH, RSCP::eTypeFloat32, -1, true },
    { 0, 0, "set/power_mode", "^auto$|^idle:[0-9]{1,4}$|^charge:[0-9]{1,5}:[0-9]{1,4}$|^discharge:[0-9]{1,5}:[0-9]{1,4}$|^grid_charge:[0-9]{1,5}:[0-9]{1,4}$", "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/idle_period", SET_IDLE_PERIOD_REGEX, "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, TAG_EMS_REQ_SET_BATTERY_TO_CAR_MODE, "set/wallbox/battery_to_car", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, true },
    { 0, TAG_EMS_REQ_SET_BATTERY_BEFORE_CAR_MODE, "set/wallbox/battery_before_car", "^true|on|1$", "1", "^false|off|0$", "0", "", UNIT_NONE, RSCP::eTypeUChar8, -1, true },
    { 0, TAG_EMS_REQ_SET_WB_DISCHARGE_BAT_UNTIL, "set/wallbox/battery_discharge_until", PAYLOAD_REGEX_2_DIGIT, "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, true },
    { 0, TAG_EMS_REQ_SET_WALLBOX_ENFORCE_POWER_ASSIGNMENT, "set/wallbox/disable_battery_at_mix_mode", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, "set/wallbox/control", "^solar:[0-9]{1,2}$|^mix:[0-9]{1,2}$|^stop$", "", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/requests/pm", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/requests/pvi", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/requests/dcb", "^true|on|1$", "true", "^false|off|0$", "false", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/log", "^true|on|1$", "true", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/force", "^true|on|1$", "true", "", "", "", UNIT_NONE, RSCP::eTypeBool, -1, true },
    { 0, 0, "set/interval", "(^[1-9]|10$)", "", "", "", "", UNIT_NONE, RSCP::eTypeUChar8, -1, true }
};

std::vector<rec_cache_t> RscpMqttReceiveCache(rec_cache, rec_cache + sizeof(rec_cache) / sizeof(rec_cache_t));

bool compareRecCache(RSCP_MQTT::rec_cache_t c1, RSCP_MQTT::rec_cache_t c2) {
    return (c1.container < c2.container);
}

}

#endif
