#ifndef RSCP_MQTT_MAPPING_H_
#define RSCP_MQTT_MAPPING_H_

#include "RscpTags.h"
#include "RscpTypes.h"

#define TOPIC_SIZE        128
#define PAYLOAD_SIZE      80
#define REGEX_SIZE        160

#define PAYLOAD_REGEX_2_DIGIT      "(^[0-9]{1,2}$)"
#define PAYLOAD_REGEX_5_DIGIT      "(^[0-9]{1,5}$)"
#define SET_IDLE_PERIOD_REGEX      "^(mon|tues|wednes|thurs|fri|satur|sun|to)day:(charge|discharge):(true|false):([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]-([0-9]|0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]$"

namespace RSCP_MQTT {

std::string days[8] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", "today"};

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
    char source[PAYLOAD_SIZE];
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
    int type;
    int divisor;
    int bit_to_bool;
    int publish;
} cache_t;

cache_t cache[] = {
    { 0, TAG_INFO_SW_RELEASE, 0, "e3dc/system/software", "%s", "", RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_INFO_PRODUCTION_DATE, 0, "e3dc/system/production_date", "%s", "", RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_INFO_SERIAL_NUMBER, 0, "e3dc/system/serial_number", "%s", "", RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_INFO_TIME_ZONE, 0, "e3dc/time/zone", "%s", "", RSCP::eTypeString, 1, 0, 0 },
    { 0, TAG_EMS_POWER_PV, 0, "e3dc/solar/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_BAT, 0, "e3dc/battery/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_HOME, 0, "e3dc/home/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_GRID, 0, "e3dc/grid/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_ADD, 0, "e3dc/addon/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_BAT_SOC, 0, "e3dc/battery/soc", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/ems/charging_lock", "%s", "", RSCP::eTypeUInt32, 1, 1, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/ems/discharging_lock", "%s", "", RSCP::eTypeUInt32, 1, 2, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/ems/emergency_power_available", "%s", "", RSCP::eTypeUInt32, 1, 4, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/ems/charging_throttled", "%s", "", RSCP::eTypeUInt32, 1, 8, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/grid_in_limit", "%s", "", RSCP::eTypeUInt32, 1, 16, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/ems/charging_time_lock", "%s", "", RSCP::eTypeUInt32, 1, 32, 0 },
    { 0, TAG_EMS_STATUS, 0, "e3dc/ems/discharging_time_lock", "%s", "", RSCP::eTypeUInt32, 1, 64, 0 },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "e3dc/ems/balanced_phases/L1", "%i", "", RSCP::eTypeUChar8, 1, 1, 0 },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "e3dc/ems/balanced_phases/L2", "%i", "", RSCP::eTypeUChar8, 2, 1, 0 },
    { 0, TAG_EMS_BALANCED_PHASES, 0, "e3dc/ems/balanced_phases/L3", "%i", "", RSCP::eTypeUChar8, 4, 1, 0 },
    { 0, TAG_EMS_INSTALLED_PEAK_POWER, 0, "e3dc/system/installed_peak_power", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { 0, TAG_EMS_DERATE_AT_PERCENT_VALUE, 0, "e3dc/system/derate_at_percent_value", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { 0, TAG_EMS_DERATE_AT_POWER_VALUE, 0, "e3dc/system/derate_at_power_value", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { 0, TAG_EMS_SET_POWER, 0, "e3dc/ems/set_power/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_MODE, 0, "e3dc/mode", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_COUPLING_MODE, 0, "e3dc/coupling/mode", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_IDLE_PERIOD_CHANGE_MARKER, 0, "e3dc/idle_period/change", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    // CONTAINER TAG_BAT_DATA --------------------------------------------------------------------
    { TAG_BAT_DATA, TAG_BAT_RSOC, 0, "e3dc/battery/rsoc", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_MODULE_VOLTAGE, 0, "e3dc/battery/voltage", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_CURRENT, 0, "e3dc/battery/current", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_CHARGE_CYCLES, 0, "e3dc/battery/cycles", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_STATUS_CODE, 0, "e3dc/battery/status", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_ERROR_CODE, 0, "e3dc/battery/error", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_DEVICE_NAME, 0, "e3dc/battery/name", "%s", "", RSCP::eTypeString, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_DCB_COUNT, 0, "e3dc/battery/dcb_count", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_TRAINING_MODE, 0, "e3dc/battery/training", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_MAX_DCB_CELL_TEMPERATURE, 0, "e3dc/battery/temperature/max", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_BAT_DATA, TAG_BAT_MIN_DCB_CELL_TEMPERATURE, 0, "e3dc/battery/temperature/min", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // CONTAINER TAG_EMS_GET_POWER_SETTINGS ------------------------------------------------------
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_MAX_CHARGE_POWER, 0, "e3dc/ems/max_charge/power", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_MAX_DISCHARGE_POWER, 0, "e3dc/ems/max_discharge/power", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_POWER_LIMITS_USED, 0, "e3dc/ems/power_limits", "%s", "", RSCP::eTypeBool, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_DISCHARGE_START_POWER, 0, "e3dc/ems/discharge_start/power", "%u", "", RSCP::eTypeUInt32, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_POWERSAVE_ENABLED, 0, "e3dc/ems/power_save", "%s", "", RSCP::eTypeBool, 1, 0, 0 },
    { TAG_EMS_GET_POWER_SETTINGS, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, 0, "e3dc/ems/weather_regulation", "%s", "", RSCP::eTypeBool, 1, 0, 0 },
    // CONTAINER TAG_EMS_SET_POWER_SETTINGS ------------------------------------------------------
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_DISCHARGE_START_POWER, 0, "e3dc/ems/discharge_start/status", "%i", "", RSCP::eTypeChar8, 1, 0, 0 },
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_MAX_CHARGE_POWER, 0, "e3dc/ems/max_charge/status", "%i", "", RSCP::eTypeChar8, 1, 0, 0 },
    { TAG_EMS_SET_POWER_SETTINGS, TAG_EMS_RES_MAX_DISCHARGE_POWER, 0, "e3dc/ems/max_discharge/status", "%i", "", RSCP::eTypeChar8, 1, 0, 0 },
    // CONTAINER TAG_PM_DATA ---------------------------------------------------------------------
    { TAG_PM_DATA, TAG_PM_POWER_L1, 0, "e3dc/pm/power/L1", "%.0lf", "", RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_POWER_L2, 0, "e3dc/pm/power/L2", "%.0lf", "", RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_POWER_L3, 0, "e3dc/pm/power/L3", "%.0lf", "", RSCP::eTypeDouble64, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ENERGY_L1, 0, "e3dc/pm/energy/L1", "%.0lf", "", RSCP::eTypeDouble64, 1000, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ENERGY_L2, 0, "e3dc/pm/energy/L2", "%.0lf", "", RSCP::eTypeDouble64, 1000, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ENERGY_L3, 0, "e3dc/pm/energy/L3", "%.0lf", "", RSCP::eTypeDouble64, 1000, 0, 0 },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L1, 0, "e3dc/pm/voltage/L1", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L2, 0, "e3dc/pm/voltage/L2", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_VOLTAGE_L3, 0, "e3dc/pm/voltage/L3", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "e3dc/pm/active_phases/L1", "%i", "", RSCP::eTypeUChar8, 1, 1, 0 },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "e3dc/pm/active_phases/L2", "%i", "", RSCP::eTypeUChar8, 2, 1, 0 },
    { TAG_PM_DATA, TAG_PM_ACTIVE_PHASES, 0, "e3dc/pm/active_phases/L3", "%i", "", RSCP::eTypeUChar8, 4, 1, 0 },
    // CONTAINER TAG_PVI_DATA --------------------------------------------------------------------
    { TAG_PVI_DC_POWER, TAG_PVI_VALUE, 0, "e3dc/pvi/power/string_1", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_POWER, TAG_PVI_VALUE, 1, "e3dc/pvi/power/string_2", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_VOLTAGE, TAG_PVI_VALUE, 0, "e3dc/pvi/voltage/string_1", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_VOLTAGE, TAG_PVI_VALUE, 1, "e3dc/pvi/voltage/string_2", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_CURRENT, TAG_PVI_VALUE, 0, "e3dc/pvi/current/string_1", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DC_CURRENT, TAG_PVI_VALUE, 1, "e3dc/pvi/current/string_2", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 0, "e3dc/pvi/power/L1", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 1, "e3dc/pvi/power/L2", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_POWER, TAG_PVI_VALUE, 2, "e3dc/pvi/power/L3", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 0, "e3dc/pvi/voltage/L1", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 1, "e3dc/pvi/voltage/L2", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_VOLTAGE, TAG_PVI_VALUE, 2, "e3dc/pvi/voltage/L3", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 0, "e3dc/pvi/current/L1", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 1, "e3dc/pvi/current/L2", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_AC_CURRENT, TAG_PVI_VALUE, 2, "e3dc/pvi/current/L3", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_PVI_DATA, TAG_PVI_TEMPERATURE_COUNT, 0, "e3dc/pvi/temperature/count", "%i", "", RSCP::eTypeUChar8, 1, 0, false },
    { TAG_PVI_DATA, TAG_PVI_ON_GRID, 0, "e3dc/pvi/on_grid", "%s", "", RSCP::eTypeBool, 1, 0, 0 },
    // CONTAINER TAG_SE_EP_RESERVE ---------------------------------------------------------------
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE, 0, "e3dc/reserve/percent", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_W, 0, "e3dc/reserve/energy", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_MAX_W, 0, "e3dc/reserve/max", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_SE_EP_RESERVE, TAG_SE_PARAM_LAST_SOC, 0, "e3dc/reserve/last_soc", "%0.2f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // CONTAINER TAG_DB_HISTORY_DATA_... ---------------------------------------------------------
    // TODAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 0, "e3dc/battery/energy/charge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 0, "e3dc/battery/energy/discharge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 0, "e3dc/solar/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 0, "e3dc/grid/energy/in", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 0, "e3dc/grid/energy/out", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 0, "e3dc/home/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 0, "e3dc/pm_0/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 0, "e3dc/pm_1/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 0, "e3dc/consumed", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 0, "e3dc/autarky", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // YESTERDAY
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_IN, 1, "e3dc/yesterday/battery/energy/charge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_POWER_OUT, 1, "e3dc/yesterday/battery/energy/discharge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_DC_POWER, 1, "e3dc/yesterday/solar/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_IN, 1, "e3dc/yesterday/grid/energy/in", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_GRID_POWER_OUT, 1, "e3dc/yesterday/grid/energy/out", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMPTION, 1, "e3dc/yesterday/home/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_0_POWER, 1, "e3dc/yesterday/pm_0/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_PM_1_POWER, 1, "e3dc/yesterday/pm_1/energy", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_BAT_CHARGE_LEVEL, 1, "e3dc/yesterday/battery/rsoc", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_CONSUMED_PRODUCTION, 1, "e3dc/yesterday/consumed", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_DAY, TAG_DB_AUTARKY, 1, "e3dc/yesterday/autarky", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // WEEK
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_POWER_IN, 0, "e3dc/week/battery/energy/charge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_BAT_POWER_OUT, 0, "e3dc/week/battery/energy/discharge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_DC_POWER, 0, "e3dc/week/solar/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_GRID_POWER_IN, 0, "e3dc/week/grid/energy/in", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_GRID_POWER_OUT, 0, "e3dc/week/grid/energy/out", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_CONSUMPTION, 0, "e3dc/week/home/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_CONSUMED_PRODUCTION, 0, "e3dc/week/consumed", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_WEEK, TAG_DB_AUTARKY, 0, "e3dc/week/autarky", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // MONTH
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_POWER_IN, 0, "e3dc/month/battery/energy/charge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_BAT_POWER_OUT, 0, "e3dc/month/battery/energy/discharge", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_DC_POWER, 0, "e3dc/month/solar/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_GRID_POWER_IN, 0, "e3dc/month/grid/energy/in", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_GRID_POWER_OUT, 0, "e3dc/month/grid/energy/out", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_CONSUMPTION, 0, "e3dc/month/home/energy", "%0.2f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_CONSUMED_PRODUCTION, 0, "e3dc/month/consumed", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_MONTH, TAG_DB_AUTARKY, 0, "e3dc/month/autarky", "%0.1f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // YEAR
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_IN, 0, "e3dc/year/battery/energy/charge", "%.0f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_BAT_POWER_OUT, 0, "e3dc/year/battery/energy/discharge", "%.0f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_DC_POWER, 0, "e3dc/year/solar/energy", "%.0f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_IN, 0, "e3dc/year/grid/energy/in", "%.0f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_GRID_POWER_OUT, 0, "e3dc/year/grid/energy/out", "%.0f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMPTION, 0, "e3dc/year/home/energy", "%.0f", "", RSCP::eTypeFloat32, 1000, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_CONSUMED_PRODUCTION, 0, "e3dc/year/consumed", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    { TAG_DB_HISTORY_DATA_YEAR, TAG_DB_AUTARKY, 0, "e3dc/year/autarky", "%.0f", "", RSCP::eTypeFloat32, 1, 0, 0 },
    // WALLBOX
    { 0, TAG_EMS_POWER_WB_ALL, 0, "e3dc/wallbox/total/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_POWER_WB_SOLAR, 0, "e3dc/wallbox/solar/power", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { 0, TAG_EMS_BATTERY_TO_CAR_MODE, 0, "e3dc/wallbox/battery_to_car", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_BATTERY_BEFORE_CAR_MODE, 0, "e3dc/wallbox/battery_before_car", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_GET_WB_DISCHARGE_BAT_UNTIL, 0, "e3dc/wallbox/battery_discharge_until", "%i", "", RSCP::eTypeUChar8, 1, 0, 0 },
    { 0, TAG_EMS_GET_WALLBOX_ENFORCE_POWER_ASSIGNMENT, 0, "e3dc/wallbox/disable_battery_at_mix_mode", "%s", "", RSCP::eTypeBool, 1, 0, 0 },
    { TAG_WB_DATA, TAG_WB_DEVICE_STATE, 0, "e3dc/wallbox/status", "%s", "", RSCP::eTypeBool, 1, 0, 0 },
    { TAG_WB_DATA, TAG_WB_PM_ACTIVE_PHASES, 0, "e3dc/wallbox/active_phases", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 1, "e3dc/wallbox/number_used_phases", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 3, "e3dc/wallbox/max_current", "%i", "", RSCP::eTypeInt32, 1, 0, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "e3dc/wallbox/plugged", "%s", "", RSCP::eTypeBool, 1, 8, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "e3dc/wallbox/locked", "%s", "", RSCP::eTypeBool, 1, 16, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "e3dc/wallbox/charging", "%s", "", RSCP::eTypeBool, 1, 32, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "e3dc/wallbox/canceled", "%s", "", RSCP::eTypeBool, 1, 64, 0 },
    { TAG_WB_EXTERN_DATA_ALG, TAG_WB_EXTERN_DATA, 2, "e3dc/wallbox/sun_mode", "%s", "", RSCP::eTypeBool, 1, 128, 0 }
};

std::vector<cache_t> RscpMqttCache(cache, cache + sizeof(cache) / sizeof(cache_t));

typedef struct _rec_cache_t {
    uint32_t container;
    uint32_t tag;
    char topic[TOPIC_SIZE];
    char regex_true[REGEX_SIZE];
    char value_true[PAYLOAD_SIZE];
    char regex_false[REGEX_SIZE];
    char value_false[PAYLOAD_SIZE];
    char payload[PAYLOAD_SIZE];
    int type;
    int refresh_count;
    bool done;
} rec_cache_t;

rec_cache_t rec_cache[] = {
    { 0, TAG_EMS_REQ_START_MANUAL_CHARGE, "e3dc/set/manual_charge", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", RSCP::eTypeUInt32, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED, "e3dc/set/weather_regulation", "^true|on|1$", "1", "^false|off|0$", "0", "", RSCP::eTypeUChar8, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_POWER_LIMITS_USED, "e3dc/set/power_limits", "^true|on|1$", "true", "^false|off|0$", "false", "", RSCP::eTypeBool, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_MAX_CHARGE_POWER, "e3dc/set/max_charge_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", RSCP::eTypeUInt32, -1, true },
    { TAG_EMS_REQ_SET_POWER_SETTINGS, TAG_EMS_MAX_DISCHARGE_POWER, "e3dc/set/max_discharge_power", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", RSCP::eTypeUInt32, -1, true },
    { TAG_EMS_REQ_SET_POWER, TAG_EMS_REQ_SET_POWER_MODE, "power_mode: mode", "", "", "", "", "", RSCP::eTypeUChar8, 0, true },
    { TAG_EMS_REQ_SET_POWER, TAG_EMS_REQ_SET_POWER_VALUE, "power_mode: value", "", "", "", "", "", RSCP::eTypeInt32, 0, true },
    { TAG_SE_REQ_SET_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE, "e3dc/set/reserve/percent", PAYLOAD_REGEX_2_DIGIT, "", "", "", "", RSCP::eTypeFloat32, -1, true },
    { TAG_SE_REQ_SET_EP_RESERVE, TAG_SE_PARAM_EP_RESERVE_W, "e3dc/set/reserve/energy", PAYLOAD_REGEX_5_DIGIT, "", "", "", "", RSCP::eTypeFloat32, -1, true },
    { 0, 0, "e3dc/set/power_mode", "^auto$|^idle:[0-9]{1,4}$|^charge:[0-9]{1,5}:[0-9]{1,4}$|^discharge:[0-9]{1,5}:[0-9]{1,4}$|^grid_charge:[0-9]{1,5}:[0-9]{1,4}$", "", "", "", "", RSCP::eTypeBool, -1, true },
    { 0, 0, "e3dc/set/idle_period", SET_IDLE_PERIOD_REGEX, "", "", "", "", RSCP::eTypeBool, -1, true },
    { 0, TAG_EMS_REQ_SET_BATTERY_TO_CAR_MODE, "e3dc/set/wallbox/battery_to_car", "^true|on|1$", "1", "^false|off|0$", "0", "", RSCP::eTypeUChar8, -1, true },
    { 0, TAG_EMS_REQ_SET_BATTERY_BEFORE_CAR_MODE, "e3dc/set/wallbox/battery_before_car", "^true|on|1$", "1", "^false|off|0$", "0", "", RSCP::eTypeUChar8, -1, true },
    { 0, TAG_EMS_REQ_SET_WB_DISCHARGE_BAT_UNTIL, "e3dc/set/wallbox/battery_discharge_until", PAYLOAD_REGEX_2_DIGIT, "", "", "", "", RSCP::eTypeUChar8, -1, true },
    { 0, TAG_EMS_REQ_SET_WALLBOX_ENFORCE_POWER_ASSIGNMENT, "e3dc/set/wallbox/disable_battery_at_mix_mode", "^true|on|1$", "true", "^false|off|0$", "false", "", RSCP::eTypeBool, -1, true },
    { TAG_WB_REQ_DATA, TAG_WB_EXTERN_DATA, "e3dc/set/wallbox/control", "^solar:[0-9]{1,2}$|^mix:[0-9]{1,2}$|^stop$", "", "", "", "", RSCP::eTypeBool, -1, true },
    { 0, 0, "e3dc/set/requests/pm", "^true|on|1$", "true", "^false|off|0$", "false", "", RSCP::eTypeBool, -1, true },
    { 0, 0, "e3dc/set/requests/pvi", "^true|on|1$", "true", "^false|off|0$", "false", "", RSCP::eTypeBool, -1, true },
    { 0, 0, "e3dc/set/log", "^true|on|1$", "true", "", "", "", RSCP::eTypeBool, -1, true },
    { 0, 0, "e3dc/set/force", "^true|on|1$", "true", "", "", "", RSCP::eTypeBool, -1, true },
    { 0, 0, "e3dc/set/interval", "(^[1-9]|10$)", "", "", "", "", RSCP::eTypeUChar8, -1, true }
};

std::vector<rec_cache_t> RscpMqttReceiveCache(rec_cache, rec_cache + sizeof(rec_cache) / sizeof(rec_cache_t));

bool compareRecCache(RSCP_MQTT::rec_cache_t c1, RSCP_MQTT::rec_cache_t c2) {
    return (c1.container < c2.container);
}

}

#endif
