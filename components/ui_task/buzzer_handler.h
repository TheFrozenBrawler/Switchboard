#ifndef COMPONENTS_UI_TASK_BUZZER_HANDLER_H_
#define COMPONENTS_UI_TASK_BUZZER_HANDLER_H_

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

enum level {
    LOW,
    HIGH,
};

esp_err_t buzzer_handler_init();

esp_err_t buzzer_signal_init(uint16_t perdiod_ms, uint16_t duty_ms);
esp_err_t buzzer_stop();
esp_err_t buzzer_signal_failure();

#endif  // COMPONENTS_UI_TASK_BUZZER_HANDLER_H_
