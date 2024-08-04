## Topics

All topics are listed with the default prefix "e3dc".

### Readable Topics

| Device / Tag | MQTT Topic | Values / [Unit] |
| --- | --- | --- |
| Battery Charge Cycles * | e3dc/battery/cycles | |
| Battery Current * | e3dc/battery/current | [A] |
| Battery Design Capacity * | e3dc/battery/design_capacity | [Ah] |
| Battery Device Name * | e3dc/battery/name | "BAT_XXX" |
| Battery Energy Charge (today) | e3dc/battery/energy/charge | [kWh] |
| Battery Energy Discharge (today) | e3dc/battery/energy/discharge | [kWh] |
| Battery Error Code * | e3dc/battery/error | |
| Battery Module Charge Cycles | e3dc/battery/dcb/<#>/cycles | |
| Battery Module Count | e3dc/battery/dcb/count | <count#> |
| Battery Module Current 30s Average | e3dc/battery/dcb/<#>/current_avg_30s | [A] |
| Battery Module Current | e3dc/battery/dcb/<#>/current | [A] |
| Battery Module Design Capacity | e3dc/battery/dcb/<#>/design_capacity | [Ah] |
| Battery Module Design Voltage | e3dc/battery/dcb/<#>/design_voltage | [V] |
| Battery Module Device Name | e3dc/battery/dcb/<#>/device_name | |
| Battery Module Firmware | e3dc/battery/dcb/<#>/firmware_version | |
| Battery Module Full Charge Capacity | e3dc/battery/dcb/<#>/full_charge_capacity | [Ah] |
| Battery Module High Temperature | e3dc/battery/dcb/<#>/charge_high_temperature | [°C] |
| Battery Module Low Temperature | e3dc/battery/dcb/<#>/charge_low_temperature | [°C] |
| Battery Module Manufacture Date | e3dc/battery/dcb/<#>/manufacture_date | |
| Battery Module Manufacture Name | e3dc/battery/dcb/<#>/manufacture_name | |
| Battery Module Max Charge Current | e3dc/battery/dcb/<#>/max_charge_current | [A] |
| Battery Module Max Charge Voltage | e3dc/battery/dcb/<#>/max_charge_voltage | [V] |
| Battery Module Max Discharge Current | e3dc/battery/dcb/<#>/max_discharge_current | [A] |
| Battery Module Number of Parallel Cells | e3dc/battery/dcb/<#>/nr_parallel_cell | |
| Battery Module Number of Series Cells | e3dc/battery/dcb/<#>/nr_series_cell | |
| Battery Module Number Sensor | e3dc/battery/dcb/<#>/nr_sensor | |
| Battery Module PCB Version | e3dc/battery/dcb/<#>/pcb_version | |
| Battery Module Protocol Version | e3dc/battery/dcb/<#>/protocol_version | |
| Battery Module Remaining Capacity | e3dc/battery/dcb/<#>/remaining_capacity | [Ah] |
| Battery Module Serial Code | e3dc/battery/dcb/<#>/serial_code | |
| Battery Module Serial Number | e3dc/battery/dcb/<#>/serial_number | |
| Battery Module State of Charge (SOC) | e3dc/battery/dcb/<#>/soc | [%] |
| Battery Module State of Health (SOH) | e3dc/battery/dcb/<#>/soh | [%] | 
| Battery Module Table Version | e3dc/battery/dcb/<#>/table_version | |
| Battery Module Voltage 30s Average | e3dc/battery/dcb/<#>/voltage_avg_30s | [V] |
| Battery Module Voltage | e3dc/battery/dcb/<#>/voltage | [V] |
| Battery Remaining Capacity * | e3dc/battery/usable_remaining_capacity | [Ah] |
| Battery Role (?) * | e3dc/battery/role | |
| Battery RSOC * | e3dc/battery/rsoc_real | [%] |
| Battery SOC | e3dc/battery/soc | [%] |
| Battery SOC Min (today) | e3dc/battery/soc_min | [%] |
| Battery SOC Max (today) | e3dc/battery/soc_max | [%] |
| Battery SOC * | e3dc/battery/soc_float | [%] |
| Battery SOH * | e3dc/battery/soh | [%] |
| Battery Specified Capacity * | e3dc/battery/specified_capacity | [Wh] |
| Battery Specified Charge Power * | e3dc/battery/specified_charge_power | [W] |
| Battery Specified Discharge Power * | e3dc/battery/specified_discharge_power | [W] |
| Battery Specified Max DCB Count * | e3dc/battery/specified_max_dcb_count | |
| Battery Status Code * | e3dc/battery/status | |
| Battery Status | e3dc/battery/state | "EMPTY", "CHARGING", "DISCHARGING", "FULL", "PENDING" |
| Battery Temperature Max * | e3dc/battery/temperature/max | [°C] |
| Battery Temperature Min * | e3dc/battery/temperature/min | [°C] |
| Battery Training Mode * | e3dc/battery/training | (0-2) |
| Battery Usable Capacity * | e3dc/battery/usable_capacity | [Ah] |
| Battery Voltage * | e3dc/battery/voltage | [V] |
| Current Autarky | e3dc/autarky | [%] |
| Current Consumed Production | e3dc/consumed | [%] |
| EMS Addon Power | e3dc/addon/power | [W] |
| EMS Balanced Phase | e3dc/ems/balanced_phases/L1 | (true/false) |
| EMS Balanced Phase | e3dc/ems/balanced_phases/L2 | (true/false) |
| EMS Balanced Phase | e3dc/ems/balanced_phases/L3 | (true/false) |
| EMS Battery Charge Limit | e3dc/ems/battery_charge_limit | [W] |
| EMS Battery Count | e3dc/system/battery_count | |
| EMS Battery Discharge Limit | e3dc/ems/battery_discharge_limit | [W] |
| EMS Battery Power | e3dc/battery/power | [W] |
| EMS Charging Lock | e3dc/ems/charging_lock | (true/false) |
| EMS Charging Throttled | e3dc/ems/charging_throttled | (true/false) |
| EMS Charging Time Lock | e3dc/ems/charging_time_lock | (true/false) |
| EMS Coupling Mode | e3dc/coupling/mode | (0-4) |
| EMS DCDC Charge Limit | e3dc/ems/dcdc_charge_limit | [W] |
| EMS DCDC Discharge Limit | e3dc/ems/dcdc_discharge_limit | [W] |
| EMS Derate at percent value | e3dc/system/derate_at_percent_value | 0.7 |
| EMS Derate at power value | e3dc/system/derate_at_power_value | [W] |
| EMS Discharge Start Power | e3dc/ems/discharge_start/power | [W] |
| EMS Discharging Lock | e3dc/ems/discharging_lock | (true/false) |
| EMS Discharging Time Lock | e3dc/ems/discharging_time_lock | (true/false) |
| EMS Emergency Power Available | e3dc/ems/emergency_power_available | (true/false) |
| EMS Emergency Power Status | e3dc/ems/emergency_power_status | |
| EMS Error Messages | e3dc/error_message/<#> | "Error Message" |
| EMS Error Messages | e3dc/error_message/<#>/meta< | "type=<nr#> code=<nr#> source=<device#>" |
| EMS Grid In Limit (Einspeisebegrenzung aktiv) | e3dc/grid_in_limit | (true/false) |
| EMS Grid In Limit Duration (today) | e3dc/grid_in_limit_duration | [min] |
| EMS Grid Power | e3dc/grid/power | [W] |
| EMS Grid Power Min (today) | e3dc/grid/power_min | [W] |
| EMS Grid Power Max (today) | e3dc/grid/power_max | [W] |
| EMS Home Power | e3dc/home/power | [W] |
| EMS Home Power Min (today) | e3dc/home/power_min | [W] |
| EMS Home Power Max (today) | e3dc/home/power_max | [W] |
| EMS Idle Periods | e3dc/idle_period/change | <change#> |
| EMS Idle Periods | e3dc/idle_period/<change#>/<#> | "day:mode:active:hh:mi-hh:mi" |
| EMS Installed Peak Power | e3dc/system/installed_peak_power | [W] |
| EMS Inverter Count | e3dc/system/inverter_count | |
| EMS Max Charge Power | e3dc/ems/max_charge/power | [W] |
| EMS Max Discharge Power | e3dc/ems/max_discharge/power | [W] |
| EMS Mode | e3dc/mode | |
| EMS Power Limits Used | e3dc/ems/power_limits | (true/false) |
| EMS Powersave Enabled | e3dc/ems/power_save | (true/false) |
| EMS Remaining Battery Charge Power | e3dc/ems/remaining_battery_charge_power | [W] |
| EMS Remaining Battery Discharge Power | e3dc/ems/remaining_battery_discharge_power | [W] |
| EMS Set Power | e3dc/ems/set_power/power | [W] |
| EMS Solar Power | e3dc/solar/power | [W] |
| EMS Solar Power Max (today) | e3dc/solar/power_max | [W] |
| EMS Used Charge Limit | e3dc/ems/used_charge_limit | [W] |
| EMS Used Charge Limit | e3dc/ems/used_discharge_limit | [W] |
| EMS User Charge Limit | e3dc/ems/user_charge_limit | [W] |
| EMS User Discharge Limit | e3dc/ems/user_discharge_limit | [W] |
| EMS Weather Regulation Enable | e3dc/ems/weather_regulation | (true/false) |
| EP Reserve | e3dc/reserve/procent | [%] |
| EP Reserve Energy | e3dc/reserve/energy | [Wh] |
| EP Reserve Last SOC | e3dc/reserve/last_soc | [%] |
| EP Reserve Max Energy | e3dc/reserve/max | [Wh] |
| Grid In Energy "Einspeisung" (today) | e3dc/grid/energy/in | [kWh] |
| Grid Out Energy "Netzbezug" (today) | e3dc/grid/energy/out | [kWh] |
| Grid Status | e3dc/grid/state | "IN", "OUT" |
| Historical Data Autarky ** | e3dc/day/<year#>/<month#>/<day#>/autarky | [%] |
| Historical Data Battery Energy Charge ** | e3dc/day/<year#>/<month#>/<day#>/battery/energy/charge | [kWh] |
| Historical Data Consumed Production ** | e3dc/day/<year#>/<month#>/<day#>/consumed | [%] |
| Historical Data Energy Discharge ** | e3dc/day/<year#>/<month#>/<day#>/battery/energy/discharge | [kWh] |
| Historical Data Grid In Energy "Einspeisung" ** | e3dc/day/<year#>/<month#>/<day#>/grid/energy/in | [kWh] |
| Historical Data Grid Out Energy "Netzbezug" ** | e3dc/day/<year#>/<month#>/<day#>/grid/energy/out | [kWh] |
| Historical Data Home Energy ** | e3dc/day/<year#>/<month#>/<day#>/home/energy | [kWh] | 
| Historical Data SOC ** | e3dc/day/<year#>/<month#>/<day#>/battery/rsoc | [%] |
| Historical Data Solar Energy ** | e3dc/day/<year#>/<month#>/<day#>/solar/energy | [kWh] |
| Home Energy | e3dc/home/energy (today) | [kWh] |
| Month Autarky | e3dc/month/autarky | [%] |
| Month Battery Energy Charge | e3dc/month/battery/energy/charge | [kWh] |
| Month Consumed Production | e3dc/month/consumed | [%] |
| Month Energy Discharge | e3dc/month/battery/energy/discharge | [kWh] |
| Month Grid In Energy "Einspeisung" | e3dc/month/grid/energy/in | [kWh] |
| Month Grid Out Energy "Netzbezug" | e3dc/month/grid/energy/out | [kWh] |
| Month Home Energy | e3dc/month/home/energy | [kWh] |
| Month Solar Energy | e3dc/month/solar/energy | [kWh] |
| PM Active Phase L1 *** | e3dc/pm/active_phases/L1 | (true/false) |
| PM Active Phase L2 *** | e3dc/pm/active_phases/L2 | (true/false) |
| PM Active Phase L3 *** | e3dc/pm/active_phases/L3 | (true/false) |
| PM Energy *** | e3dc/pm/energy | [kWh] |
| PM Energy L1 *** | e3dc/pm/energy/L1 | [kWh] |
| PM Energy L2 *** | e3dc/pm/energy/L2 | [kWh] |
| PM Energy L3 *** | e3dc/pm/energy/L3 | [kWh] |
| PM Power *** | e3dc/pm/power | [W] |
| PM Power L1 *** | e3dc/pm/power/L1 | [W] |
| PM Power L2 *** | e3dc/pm/power/L2 | [W] |
| PM Power L2 *** | e3dc/pm/power/L2 | [W] |
| PM Voltage L1 *** | e3dc/pm/voltage/L1 | [V] |
| PM Voltage L2 *** | e3dc/pm/voltage/L2 | [V] |
| PM Voltage L3 *** | e3dc/pm/voltage/L3 | [V] |
| Production Date | e3dc/system/production_date | "KWXX_XXXX" |
| PVI Apparent Power L1 | e3dc/pvi/apparent_power/L1 | [VA] |
| PVI Apparent Power L2 | e3dc/pvi/apparent_power/L2 | [VA] |
| PVI Apparent Power L3 | e3dc/pvi/apparent_power/L3 | [VA] |
| PVI COS Phi | e3dc/pvi/cos_phi_excited | |
| PVI COS Phi | e3dc/pvi/cos_phi_is_active | |
| PVI COS Phi | e3dc/pvi/cos_phi_value | |
| PVI Current L1 | e3dc/pvi/current/L1 | [A] |
| PVI Current L2 | e3dc/pvi/current/L2 | [A] |
| PVI Current L3 | e3dc/pvi/current/L3 | [A] |
| PVI Energy Day L1 | e3dc/pvi/energy_day/L1 | [Wh] |
| PVI Energy Day L2 | e3dc/pvi/energy_day/L2 | [Wh] |
| PVI Energy Day L3 | e3dc/pvi/energy_day/L3 | [Wh] |
| PVI Energy Grid L1 | e3dc/pvi/energy_grid_consumption/L1 | [Wh] |
| PVI Energy Grid L2 | e3dc/pvi/energy_grid_consumption/L2 | [Wh] |
| PVI Energy Grid L3 | e3dc/pvi/energy_grid_consumption/L3 | [Wh] |
| PVI Energy L1 | e3dc/pvi/energy_all/L1 | [Wh] |
| PVI Energy L2 | e3dc/pvi/energy_all/L2 | [Wh] |
| PVI Energy L3 | e3dc/pvi/energy_all/L3 | [Wh] |
| PVI Energy String1 (today) | e3dc/pvi/energy/string_1 | Wh |
| PVI Energy String2 (today) | e3dc/pvi/energy/string_2 | Wh |
| PVI Energy String1 (all-time) | e3dc/pvi/energy_all/string_1 | Wh |
| PVI Energy String2 (all-time) | e3dc/pvi/energy_all/string_2 | Wh |
| PVI Energy String1 (midnight) ***** | e3dc/pvi/energy_start/string_1 | Wh |
| PVI Energy String2 (midnight) ***** | e3dc/pvi/energy_start/string_2 | Wh |
| PVI Frequency | e3dc/pvi/frequency_over | [Hz] |
| PVI Frequency | e3dc/pvi/frequency_under | [Hz] |
| PVI Max Apparent Power L1 | e3dc/pvi/max_apparent_power/L1 | [VA] |
| PVI Max Apparent Power L2 | e3dc/pvi/max_apparent_power/L2 | [VA] |
| PVI Max Apparent Power L3 | e3dc/pvi/max_apparent_power/L3 | [VA] |
| PVI On Grid | e3dc/pvi/on_grid | (true/false) |
| PVI Power L1 | e3dc/pvi/power/L1 | [W] |
| PVI Power L2 | e3dc/pvi/power/L2 | [W] |
| PVI Power L3 | e3dc/pvi/power/L3 | [W] |
| PVI Reactive Power L1 | e3dc/pvi/reactive_power/L1 | [var] |
| PVI Reactive Power L2 | e3dc/pvi/reactive_power/L2 | [var] |
| PVI Reactive Power L3 | e3dc/pvi/reactive_power/L3 | [var] |
| PVI Slope | e3dc/pvi/voltage_monitoring_slope_down | [V] |
| PVI Slope | e3dc/pvi/voltage_monitoring_slope_up | [V] |
| PVI String1 Current | e3dc/pvi/current/string_1 | [A] |
| PVI String1 Power | e3dc/pvi/power/string_1 | [W] |
| PVI String1 Voltage | e3dc/pvi/voltage/string_1 | [V] |
| PVI String2 Current | e3dc/pvi/current/string_2 | [A] |
| PVI String2 Power | e3dc/pvi/power/string_2 | [W] |
| PVI String2 Voltage | e3dc/pvi/voltage/string_2 | [V] |
| PVI String Count | e3dc/pvi/used_string_count | |
| PVI Temperature 1 | e3dc/pvi/temperature/1  | [°C] |
| PVI Temperature 2 | e3dc/pvi/temperature/2  | [°C] |
| PVI Temperature 3 | e3dc/pvi/temperature/3  | [°C] |
| PVI Temperature 4 | e3dc/pvi/temperature/4  | [°C] |
| PVI Threshold | e3dc/pvi/voltage_monitoring_threshold_bottom | [V] |
| PVI Threshold | e3dc/pvi/voltage_monitoring_threshold_top | [V] |
| PVI Voltage L1 | e3dc/pvi/voltage/L1 | [V] |
| PVI Voltage L2 | e3dc/pvi/voltage/L2 | [V] |
| PVI Voltage L3 | e3dc/pvi/voltage/L3 | [V] |
| Sunshine Duration (today) | e3dc/sunshine_duration | [min] |
| Serial Number | e3dc/system/serial_number | "S10-XXXXXXXXXXXX" |
| SOC limiter | e3dc/limit/charge/durable | (0/1) |
| SOC limiter | e3dc/limit/charge/soc | [%] |
| SOC limiter | e3dc/limit/discharge/by_home_power | [W] |
| SOC limiter | e3dc/limit/discharge/durable | (0/1) |
| SOC limiter | e3dc/limit/discharge/soc | [%] |
| Software Release | e3dc/system/software | "S10_XXXX_XXX" |
| Solar Energy | e3dc/solar/energy | [kWh] |
| Time Zone | e3dc/time/zone | "Europe/City" |
| Program Status | e3dc/rscp2mqtt/status | "connected" |
| Program Version | e3dc/rscp2mqtt/long_version | "3.23.influxdb" |
| Program Version | e3dc/rscp2mqtt/version | "3.23" |
| Wallbox Battery | e3dc/wallbox/charge_battery_before_car | (true/false) |
| Wallbox Battery | e3dc/wallbox/discharge_battery_to_car | (true/false) |
| Wallbox Battery | e3dc/wallbox/discharge_battery_until | [%] |
| Wallbox Battery | e3dc/wallbox/disable_battery_at_mix_mode | (true/false) |
| Wallbox Suspended **** | e3dc/wallbox/suspended | (true/false) |
| Wallbox Charging ****| e3dc/wallbox/charging | (true/false) |
| Wallbox Min Current **** | e3dc/wallbox/min_current | [A] |
| Wallbox Max Current **** | e3dc/wallbox/max_current | [A] |
| Wallbox Energy L1 **** | e3dc/wallbox/energy/L1 | [Wh] |
| Wallbox Energy L2 **** | e3dc/wallbox/energy/L2 | [Wh] |
| Wallbox Energy L3 **** | e3dc/wallbox/energy/L3 | [Wh] |
| Wallbox Energy Solar **** | e3dc/wallbox/energy/solar | [Wh] |
| Wallbox Energy Solar (midnight) ***** | e3dc/wallbox/energy_start/solar | [Wh] |
| Wallbox Energy Solar (today) **** | e3dc/wallbox/day/solar | [Wh] |
| Wallbox Energy Solar Last Charging **** | e3dc/wallbox/energy/last_charging/solar | [Wh] |
| Wallbox Energy Total **** | e3dc/wallbox/energy/total | [Wh] |
| Wallbox Energy Total (midnight) ***** | e3dc/wallbox/energy_start/total | [Wh] |
| Wallbox Energy Total (today) **** | e3dc/wallbox/energy/day/total | [Wh] |
| Wallbox Energy Total Last Charging **** | e3dc/wallbox/energy/last_charging/total | [Wh] |
| Wallbox Index **** | e3dc/wallbox/index | (0..7) |
| Wallbox Key State **** | e3dc/wallbox/key_state | (true/false) |
| Wallbox Locked **** | e3dc/wallbox/locked | (true/false) |
| Wallbox Mode **** | e3dc/wallbox/sun_mode | (true/false) |
| Wallbox Phases **** | e3dc/wallbox/phases/L1 | (true/false) |
| Wallbox Phases **** | e3dc/wallbox/phases/L2 | (true/false) |
| Wallbox Phases **** | e3dc/wallbox/phases/L3 | (true/false) |
| Wallbox Phases **** | e3dc/wallbox/number_phases | (1/3) |
| Wallbox Plugged **** | e3dc/wallbox/plugged | (true/false) |
| Wallbox Power All | e3dc/wallbox/solar/power | [W] |
| Wallbox Power Solar | e3dc/wallbox/total/power | [W] |
| Wallbox Power L1 **** | e3dc/wallbox/power/L1 | [W] |
| Wallbox Power L2 **** | e3dc/wallbox/power/L2 | [W] |
| Wallbox Power L3 **** | e3dc/wallbox/power/L3 | [W] |
| Wallbox SOC **** | e3dc/wallbox/soc | [%] |
| Week Autarky | e3dc/week/autarky | [%] |
| Week Battery Energy Charge | e3dc/week/battery/energy/charge | [kWh] |
| Week Consumed Production | e3dc/week/consumed | [%] |
| Week Energy Discharge | e3dc/week/battery/energy/discharge | [kWh] |
| Week Grid In Energy "Einspeisung" | e3dc/week/grid/energy/in | [kWh] |
| Week Grid Out Energy "Netzbezug" | e3dc/week/grid/energy/out | [kWh] |
| Week Home Energy | e3dc/week/home/energy | [kWh] |
| Week Solar Energy | e3dc/week/solar/energy | [kWh] |
| Year Autarky | e3dc/year/autarky | [%] |
| Year Battery Energy Charge | e3dc/year/battery/energy/charge | [kWh] |
| Year Consumed Production | e3dc/year/consumed | [%] |
| Year Energy Discharge | e3dc/year/battery/energy/discharge | [kWh] |
| Year Grid In Energy "Einspeisung" | e3dc/year/grid/energy/in | [kWh] |
| Year Grid Out Energy "Netzbezug" | e3dc/year/grid/energy/out | [kWh] |
| Year History Autarky | e3dc/history/<year#>/autarky | [%] |
| Year History Battery Energy Charge | e3dc/history/<year#>/battery/energy/charge | [kWh] |
| Year History Consumed Production | e3dc/history/<year#>/consumed | [%] |
| Year History Energy Discharge | e3dc/history/<year#>/battery/energy/discharge | [kWh] |
| Year History Grid In Energy "Einspeisung" | e3dc/history/<year#>/grid/energy/in | [kWh] |
| Year History Grid Out Energy "Netzbezug" | e3dc/history/<year#>/grid/energy/out | [kWh] |
| Year History Home Energy | e3dc/history/<year#>/home/energy | [kWh] |
| Year History Solar Energy | e3dc/history/<year#>/solar/energy | [kWh] |
| Year Home Energy | e3dc/year/home/energy | [kWh] |
| Year Solar Energy | e3dc/year/solar/energy | [kWh] |
| Yesterday Autarky | e3dc/yesterday/autarky | [%] |
| Yesterday Battery Energy Charge | e3dc/yesterday/battery/energy/charge | [kWh] |
| Yesterday Battery SOC | e3dc/yesterday/battery/rsoc | [%] |
| Yesterday Consumed Production | e3dc/yesterday/consumed | [%] |
| Yesterday Energy Discharge | e3dc/yesterday/battery/energy/discharge | [kWh] |
| Yesterday Grid In Energy "Einspeisung" | e3dc/yesterday/grid/energy/in | [kWh] |
| Yesterday Grid Out Energy "Netzbezug" | e3dc/yesterday/grid/energy/out | [kWh] |
| Yesterday Home Energy | e3dc/yesterday/home/energy | [kWh] |
| Yesterday Solar Energy | e3dc/yesterday/solar/energy | [kWh] |

*) If your system has more than one battery string (e.g. S10 Pro), you have to configure the parameter BATTERY_STRINGS accordingly. Battery topics that belong to a battery string are extended by the number of the battery string. Battery modules (DCB topics) are numbered consecutively.

Energy topics are collected for today, yesterday and the current week, month, year and historical years if configured.
**) Historical data for a specific day can be queried by publishing "e3dc/set/request/day".

***) If more than one power meter exists (PM_INDEX configured multiple times), topics are extended by the number of the power meter

****) If more than one wallbox exists (WB_INDEX configured multiple times), topics are extended by the number of the wallbox

*****) The value is required to be able to calculate the daily value. To ensure that the value survives a restart, set RETAIN_FOR_SETUP=true in .config.

The boolean values "true" and "false" are saved in the InfluxDB as "1" and "0". By setting the configuration parameter `USE_TRUE_FALSE=false`, this behavior can also be set for the MQTT payload.

### Writeable Topics

Please find detailled information and examples in the [README](README.md).

| Device / Tag | MQTT Topic | Values / [Unit] |
| --- | --- | --- |
| Start battery charging manually | e3dc/set/manual_charge | [Wh] |
| Set weather regulation | e3dc/set/weather_regulation | (true/false) |
| Set limits for battery charging and discharging | e3dc/set/power_limits | (true/false) |
|  - set the charging and discharging power limits | e3dc/set/max_charge_power | [W] |
|  - set the charging and discharging power limits | e3dc/set/max_discharge_power | [W] |
|  - set the charging and discharging power limits | e3dc/set/discharge_start_power | [W] |
| Set idle periods to lock battery charging or discharging | e3dc/set/idle_period | "day:mode:active:hh:mi-hh:mi", e.g. "sunday:charge:true:00:00-23:59" |
| SOC Limiter | | |
| Limit discharging of the house battery to SOC | e3dc/set/limit/discharge/soc | (0..100) |
| Set the home power value which stops discharging the battery | e3dc/set/limit/discharge/by_home_power | [W] |
| Keep the limiter setting even after the day change | e3dc/set/limit/discharge/durable | (true/false) |
| Limit charging of the house battery to SOC | e3dc/set/limit/charge/soc | (0-100) |
| Keep the limiter setting even after the day change | e3dc/set/limit/charge/durable | (true/false) |
| Emergency Power | | |
| Set battery reserve for emergency power in [Wh] | e3dc/set/reserve/energy | [Wh] |
| or set battery reserve for emergency power in [%] | e3dc/set/reserve/percent | [%] |
| Power Management | | |
| Control the power management (normal mode) | e3dc/set/power_mode | "auto" |
| Control the power management (idle mode) | e3dc/set/power_mode | "idle:60" |
| Control the power management (discharge in [W], number of cycles) | e3dc/set/power_mode | "discharge:2000:60" |
| Control the power management (charge in [W], number of cycles) | e3dc/set/power_mode | "charge:2000:60" |
| Control the power management (charge from grid in [W], number of cycles)  | e3dc/set/power_mode | "grid_charge:2000:60" |
| Wallbox | | |
| Set sun mode | e3dc/set/wallbox/sun_mode | (true/false) |
| Set max current | e3dc/set/wallbox/max_current in [A] | (1..32) |
| Suspend charging | e3dc/set/wallbox/suspended | (true/false) |
| Toggle suspend charging | e3dc/set/wallbox/toggle | true |
| Set battery to car mode | e3dc/set/wallbox/discharge_battery_to_car | (true/false) |
| Set battery before car mode | e3dc/set/wallbox/charge_battery_before_car | (true/false) |
| Set battery discharge until [%] | e3dc/set/wallbox/discharge_battery_until | [%] |
| Set disable charging battery at mix mode | e3dc/set/wallbox/disable_battery_at_mix_mode | (true/false) |
| Set number of phases | e3dc/set/wallbox/number_phases | (1/3) |
| Historical daily data | | |
| Query data for a specific day | e3dc/set/request/day | "2023-12-31" |
| Settings and others | | |
| Refresh all topics | e3dc/set/force | 1 |
| Refresh specific topics | e3dc/set/force | "e3dc/history/2021.*" |
| Log all topics and payloads to the log file | e3dc/set/log/cache | 1 |
| Log collected error messages to the log file | e3dc/set/log/errors | 1 |
| Log internal stuff to the log file | e3dc/set/health | 1 |
| Set refresh interval [sec] | e3dc/set/interval | (1-300) |
| Enable PM requests | e3dc/set/requests/pm | (true/false) |
| Enable PVI requests | e3dc/set/requests/pvi | (true/false) |
| Enable DCB requests | e3dc/set/requests/dcb | (true/false) |
| Enable History requests | e3dc/set/requests/history | (true/false) |
| Enable EMS requests | e3dc/set/requests/ems | (true/false) |
| Enable SOC limiter | e3dc/set/soc_limiter | (true/false) |
| Enable daily historical values | e3dc/set/daily_values | (true/false) |
| Enable statistic values | e3dc/set/statistic_values | (true/false) |
| Enable raw data mode | e3dc/set/raw_mode | (true/false) |

Instead of the values "true" or "false", you can also use "1" or "0".
