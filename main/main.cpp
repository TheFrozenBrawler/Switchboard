#include "common.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "battery_task.h"
#include "buzzer_handler.h"
#include "event_manager.h"
#include "led_handler.h"
#include "message_manager.h"
#include "message_task.h"
#include "mqtt_task.h"
#include "queue_types_manager.h"
#include "switch_task.h"
#include "ui_task.h"
#include "wifi_task.h"

#define ESP_INTR_FLAG_DEFAULT 0

#ifdef __cplusplus
extern "C" {
#endif

void app_main(void) {
    nvs_flash_init();
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // additional components inits
    message_manager_init();
    event_manager_init();
    queue_types_manager_init();
    buzzer_handler_init();
    led_rmt_driver_init();

    // tasks inits
    ui_task_init();
    wifi_task_init();
    mqtt_task_init();
    switch_task_init();
    battery_task_init();
    message_task_init();
}

#ifdef __cplusplus
}
#endif
