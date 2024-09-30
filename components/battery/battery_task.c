#include "battery_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "battery_manager.h"
#include "common.h"
#include "event_manager.h"
#include "queue_types_manager.h"

#if CONFIG_BATTERY_LOGGER_EN
#include "logger.h"
DECLARE_LOG(BATTERY_TASK)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#if CONFIG_BATTERY_TASK_STACK_SIZE % 4 != 0
#error "Task Stack size must be divisible by 4"
#endif

static StackType_t batteryTaskStack[CONFIG_BATTERY_TASK_STACK_SIZE];
static StaticTask_t batteryTaskBuffer;

static void battery_task(void *);

void battery_task_init() {
    xTaskCreateStaticPinnedToCore(battery_task,
                                  "BATTERY TASK",
                                  CONFIG_BATTERY_TASK_STACK_SIZE,
                                  NULL,
                                  PRIORITY_NORMAL,
                                  batteryTaskStack,
                                  &batteryTaskBuffer,
                                  CONFIG_BATTERY_TASK_CORE);
}

static void battery_task(void *) {
    LOG_I(TAG, "Battery task init");

    if (battery_manager_init() != ESP_OK) {
        LOG_E(TAG, "battery manager init fail, exiting");
        vTaskDelete(NULL);
    }
    LOG_I(TAG, "Battery task initiated");

    wait_event_bit(MAIN_EVENT_GROUP, MAIN_ALL_INITIALIZED, pdFALSE, pdTRUE, WAIT_UNTILL_BIT);

    while (1) {
        wait_for_batt_timer_smph();
        if (get_event_bits(MQTT_STATUS_GROUP) == MQTT_CONNECTED) {
            queue_packet_battery_t battery_data;
            battery_manager_take_measurements(&battery_data);
            queue_send(BATTERY_MSG_QUEUE, &battery_data);
        } else {
            battery_timer_stop();
            wait_event_bit(MQTT_STATUS_GROUP, MQTT_CONNECTED, pdFALSE, pdFALSE, WAIT_UNTILL_BIT);
            battery_timer_start();
        }
    }
}
