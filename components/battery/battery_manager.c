#include "battery_manager.h"

#include "string.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"

#include "common.h"
#include "cpu_temp_sensor.h"
#include "event_manager.h"

#if CONFIG_BATTERY_LOGGER_EN
#include "logger.h"
DECLARE_LOG(BATTERY MANAGER)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#define BATTERY_GPIO       CONFIG_BATTERY_MEASUREMENT_PIN
#define ADC_UNIT           ADC_UNIT_1
#define BATTERY_TIMER_TIME CONFIG_BATTERY_TIMER_TIME

#define VOLTAGE_DIVIDER_COEFF (CONFIG_VOLTAGE_DIVIDER_R2 + CONFIG_VOLTAGE_DIVIDER_R1) / CONFIG_VOLTAGE_DIVIDER_R2

static TimerHandle_t battery_timer_handler;
static StaticTimer_t battery_timer_buffer;

static SemaphoreHandle_t battery_timer_semaphore;
static StaticSemaphore_t battery_timer_semaphore_buffer;

struct battery_manager_context {
    queue_packet_battery_t battery_data;
    adc_unit_t adc_unit;
    adc_channel_t adc_channel;
    adc_oneshot_unit_handle_t adc_unit_handle;
    adc_cali_handle_t adc_cali_handle;
};

static struct battery_manager_context ctx = {};

static esp_err_t battery_manager_get_channel() {
    return adc_oneshot_io_to_channel(BATTERY_GPIO, &ctx.adc_unit, &ctx.adc_channel);
}

static esp_err_t battery_manager_init_adc() {
    adc_oneshot_unit_init_cfg_t adc_init_cfg = {
      .unit_id  = ctx.adc_unit,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    return adc_oneshot_new_unit(&adc_init_cfg, &ctx.adc_unit_handle);
}

static esp_err_t battery_manager_init_channel() {
    static const adc_oneshot_chan_cfg_t chan_cfg = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten    = ADC_ATTEN_DB_12,
    };

    return adc_oneshot_config_channel(ctx.adc_unit_handle, ctx.adc_channel, &chan_cfg);
}

static esp_err_t battery_manager_init_adc_calibration() {
    adc_cali_curve_fitting_config_t config_cali = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten    = ADC_ATTEN_DB_12,
      .unit_id  = ctx.adc_unit,
    };

    return adc_cali_create_scheme_curve_fitting(&config_cali, &ctx.adc_cali_handle);
}

static esp_err_t battery_manager_get_raw_voltage() {
    return adc_oneshot_read(ctx.adc_unit_handle, ctx.adc_channel, (int *)&ctx.battery_data.raw_voltage);
}

static esp_err_t battery_manager_calibrate_voltage() {
    int voltage = 0.0f;

    esp_err_t err = adc_cali_raw_to_voltage(ctx.adc_cali_handle, (int)ctx.battery_data.raw_voltage, &voltage);
    if (err != ESP_OK) {
        return err;
    }

    ctx.battery_data.voltage = ((float)voltage / 1000.0f) * VOLTAGE_DIVIDER_COEFF;
    return err;
}

esp_err_t battery_manager_init() {
    LOG_I(TAG, "Battery manager init");
    memset(&ctx.battery_data, 0, sizeof(queue_packet_battery_t));

    RET_AND_LOG_ON_ERR(battery_manager_get_channel(), "wrong GPIO");
    RET_AND_LOG_ON_ERR(battery_manager_init_adc(), "init adc");
    RET_AND_LOG_ON_ERR(battery_manager_init_channel(), "init channel");
    RET_AND_LOG_ON_ERR(battery_manager_init_adc_calibration(), "init adc calibration");

    RET_AND_LOG_ON_ERR(cpu_temp_init(), "init cpu temperature sensor");

    battery_timer_semaphore = xSemaphoreCreateBinaryStatic(&battery_timer_semaphore_buffer);
    RET_AND_LOG_ON_ERR(battery_manager_measure_timer_init(), "init battery measure timer");

    set_event_bit(MAIN_EVENT_GROUP, MAIN_BATTERY_INITIALIZED);
    LOG_I(TAG, "battery manager initiated");
    return ESP_OK;
}

esp_err_t battery_manager_take_measurements(queue_packet_battery_t *battery_data) {
    assert(battery_data != NULL);

    RET_AND_LOG_ON_ERR(battery_manager_get_raw_voltage(), "read battery voltage");

    RET_AND_LOG_ON_ERR(battery_manager_calibrate_voltage(), "calibrate battery voltage");

    memcpy(battery_data, &ctx.battery_data, sizeof(queue_packet_battery_t));
    return ESP_OK;
}

void battery_manager_timer_callback() {
    xSemaphoreGiveFromISR(battery_timer_semaphore, NULL);
}

esp_err_t battery_manager_measure_timer_init() {
    battery_timer_handler = xTimerCreateStatic("Battery timer",
                                               pdMS_TO_TICKS((BATTERY_TIMER_TIME * 1000)),
                                               pdTRUE,
                                               (void *)0,
                                               battery_manager_timer_callback,
                                               &battery_timer_buffer);

    if (battery_timer_handler == NULL) {
        LOG_E(TAG, "Battery Timer wasn't initialized properly.");
        return ESP_FAIL;
    } else {
        battery_timer_start();
    }

    return ESP_OK;
}

void wait_for_batt_timer_smph() {
    xSemaphoreTake(battery_timer_semaphore, WAIT_UNTILL_BIT);
}

void battery_timer_start() {
    xTimerStart(battery_timer_handler, portMAX_DELAY);
    LOG_I(TAG, "Battery timer start");
}

void battery_timer_stop() {
    xTimerStop(battery_timer_handler, portMAX_DELAY);
    LOG_I(TAG, "Battery timer stop");
}
