#include "mqtt_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buzzer_handler.h"
#include "common.h"
#include "event_manager.h"
#include "mqtt_driver.h"

#if CONFIG_MQTT_LOGGER_EN
#include "logger.h"
DECLARE_LOG(MQTT TASK)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#if CONFIG_MQTT_TASK_STACK_SIZE % 4 != 0
#error "Task Stack size must be divisible by 4"
#endif

#define MQTT_INIT_MAX_RETRIES 20

static StackType_t mqttTaskStack[CONFIG_MQTT_TASK_STACK_SIZE];
static StaticTask_t mqttTaskBuffer;

static void mqtt_task(void *);

void mqtt_task_init() {
    xTaskCreateStaticPinnedToCore(mqtt_task,
                                  "MQTT TASK",
                                  CONFIG_MQTT_TASK_STACK_SIZE,
                                  NULL,
                                  PRIORITY_NORMAL,
                                  mqttTaskStack,
                                  &mqttTaskBuffer,
                                  CONFIG_MQTT_TASK_CORE);
}

static void mqtt_task(void *) {
    LOG_I(TAG, "Started task");

    wait_event_bit(IP_STATUS_EVENT_GROUP, IP_GOT, pdFALSE, pdTRUE, WAIT_UNTILL_BIT);

    uint8_t init_counter = 1;
    while (mqtt_init() != ESP_OK) {
        LOG_W(TAG, "Trying again to init mqtt for %d time", init_counter);
        vTaskDelay(pdMS_TO_TICKS(100));
        init_counter++;
        if (init_counter == MQTT_INIT_MAX_RETRIES) {
            buzzer_signal_failure();  // require manual reset
            LOG_E(TAG, "MQTT init error!!!");
        }
    }

    set_event_bit(MAIN_EVENT_GROUP, MAIN_MQTT_INITIALIZED);

    while (1) {
        mqtt_run_process();
    }
}
