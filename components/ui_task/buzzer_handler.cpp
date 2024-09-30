#include "buzzer_handler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "common.h"

#if CONFIG_UI_LOGGER_EN
#include "logger.h"
DECLARE_LOG(UI TASK)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#define BUZZER_PERIOD CONFIG_BUZZER_PERIOD
#define BUZZER_DUTY   CONFIG_BUZZER_DUTY
#define BUZZER_GPIO   GPIO_NUM_1

static TimerHandle_t timer_period;
static StaticTimer_t timer_period_buffer;
static TimerHandle_t timer_duty;
static StaticTimer_t timer_duty_buffer;

struct buzzer_handler_context {
    gpio_num_t pin;
};

static struct buzzer_handler_context ctx = {};

void start_buzzing() {
    gpio_set_level(ctx.pin, HIGH);
}

void stop_buzzing() {
    gpio_set_level(ctx.pin, LOW);
}

void buzzer_device_init(uint16_t period_ms, uint16_t duty_ms) {
    xTimerChangePeriod(timer_period, pdMS_TO_TICKS(period_ms), portMAX_DELAY);
    xTimerChangePeriod(timer_duty, pdMS_TO_TICKS(duty_ms), portMAX_DELAY);
    start_buzzing();
}

static void timer_duty_cb(TimerHandle_t timer) {
    stop_buzzing();
}

static void timer_period_cb(TimerHandle_t timer) {
    start_buzzing();
    xTimerStart(timer_duty, 0);
}

esp_err_t buzzer_handler_init() {
    LOG_I(TAG, "Buzzer init");
    timer_period = xTimerCreateStatic("Buzzer timer period",
                                      pdMS_TO_TICKS(BUZZER_PERIOD),
                                      pdTRUE,
                                      reinterpret_cast<void *>(0),
                                      timer_period_cb,
                                      &timer_period_buffer);

    timer_duty = xTimerCreateStatic("Buzzer timer duty",
                                    pdMS_TO_TICKS(BUZZER_DUTY),
                                    pdFALSE,
                                    reinterpret_cast<void *>(0),
                                    timer_duty_cb,
                                    &timer_duty_buffer);
    ctx.pin    = BUZZER_GPIO;

    gpio_set_direction(ctx.pin, GPIO_MODE_OUTPUT);

    return ESP_OK;
}

esp_err_t buzzer_signal_init(uint16_t period_ms, uint16_t duty_ms) {
    LOG_I(TAG, "Buzzer signal init");
    buzzer_device_init(period_ms, duty_ms);

    return ESP_OK;
}

esp_err_t buzzer_stop() {
    LOG_I(TAG, "Buzzer signal stop");
    xTimerStop(timer_period, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t buzzer_signal_failure() {
    LOG_I(TAG, "Buzzer signal failure");
    buzzer_stop();
    buzzer_device_init(CONFIG_BUZZER_SIGNAL_FAILURE_PERIOD, CONFIG_BUZZER_SIGNAL_FAILURE_DUTY);
    return ESP_OK;
}
