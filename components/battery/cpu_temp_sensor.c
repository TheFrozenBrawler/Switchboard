#include "cpu_temp_sensor.h"
#include "common.h"
#include "driver/gpio.h"
#include "driver/temperature_sensor.h"
#include "esp_log.h"

#if CONFIG_CPU_TEMP_LOGGER_EN
#include "logger.h"
DECLARE_LOG(CPU_TEMP_SENS)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

temperature_sensor_handle_t temp_handle = NULL;

esp_err_t cpu_temp_init() {
    LOG_I(TAG, "Initializing CPU teperature sensor");
    static const temperature_sensor_config_t temp_sensor = {
      .range_min = MIN_CPU_T,
      .range_max = MAX_CPU_T,
    };

    RET_AND_LOG_ON_ERR(temperature_sensor_install(&temp_sensor, &temp_handle), "Temperature sensor install");

    return ESP_OK;
}

// --- CPU TEMP ---
float get_cpu_temp() {
    float CPU_temp;
    RET_AND_LOG_ON_ERR(temperature_sensor_enable(temp_handle), "temperature sensor enable");
    RET_AND_LOG_ON_ERR(temperature_sensor_get_celsius(temp_handle, &CPU_temp), "temperature sensor get C");
    RET_AND_LOG_ON_ERR(temperature_sensor_disable(temp_handle), "temperature sensor disable");

    return CPU_temp;
}
