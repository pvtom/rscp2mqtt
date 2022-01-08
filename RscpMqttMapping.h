#ifndef RSCP_MQTT_MAPPING_H_
#define RSCP_MQTT_MAPPING_H_

#include "RscpTags.h"
#include "RscpTypes.h"

#define TOPIC_SIZE        128
#define PAYLOAD_SIZE      64

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
    { 0, TAG_EMS_POWER_PV, "e3dc/solar/power", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_POWER_BAT, "e3dc/battery/power", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_POWER_HOME, "e3dc/home/power", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_POWER_GRID, "e3dc/grid/power", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_POWER_ADD, "e3dc/addon/power", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_EMS_COUPLING_MODE, "e3dc/coupling/mode", "%i", "", RSCP::eTypeUChar8, 1, false },
    { 0, TAG_INFO_TIME_ZONE, "e3dc/time/zone", "%s", "", RSCP::eTypeString, 1, false },
    // CONTAINER TAG_BAT_DATA --------------------------------------------------------------------
    { 0, TAG_BAT_RSOC, "e3dc/battery/rsoc", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_BAT_MODULE_VOLTAGE, "e3dc/battery/voltage", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_BAT_CURRENT, "e3dc/battery/current", "%0.1f", "", RSCP::eTypeFloat32, 1, false },
    { 0, TAG_BAT_CHARGE_CYCLES, "e3dc/battery/cycles", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_BAT_STATUS_CODE, "e3dc/battery/status", "%i", "", RSCP::eTypeUInt32, 1, false },
    { 0, TAG_BAT_ERROR_CODE, "e3dc/battery/error", "%i", "", RSCP::eTypeUInt32, 1, false },
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
}

#endif
