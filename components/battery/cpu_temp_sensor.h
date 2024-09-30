#ifndef COMPONENTS_BATTERY_CPU_TEMP_SENSOR_H_
#define COMPONENTS_BATTERY_CPU_TEMP_SENSOR_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_CPU_T CONFIG_CPU_T_MIN
#define MAX_CPU_T CONFIG_CPU_T_MAX

esp_err_t cpu_temp_init();
float get_cpu_temp();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_BATTERY_CPU_TEMP_SENSOR_H_
