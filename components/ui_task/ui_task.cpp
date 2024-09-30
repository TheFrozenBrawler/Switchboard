#include "ui_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buzzer_handler.h"
#include "common.h"
#include "event_manager.h"
#include "led_handler.h"

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

#if CONFIG_UI_TASK_STACK_SIZE % 4 != 0
#error "Task Stack size must be divisible by 4"
#endif

static StackType_t uiTaskStack[CONFIG_UI_TASK_STACK_SIZE];
static StaticTask_t uiTaskBuffer;

static void ui_task(void *);

void ui_task_init() {
    xTaskCreateStaticPinnedToCore(ui_task,
                                  "UI TASK",
                                  CONFIG_UI_TASK_STACK_SIZE,
                                  NULL,
                                  PRIORITY_LOW,
                                  uiTaskStack,
                                  &uiTaskBuffer,
                                  CONFIG_UI_TASK_CORE);
}

static void ui_task(void *) {
    LOG_I(TAG, "UI task init");
    buzzer_signal_init(BUZZER_PERIOD, BUZZER_DUTY);

    while (1) {
        wait_event_bit(MQTT_STATUS_GROUP, MQTT_CONNECTED, pdFALSE, pdTRUE, WAIT_UNTILL_BIT);
        buzzer_stop();
        wait_event_bit(MQTT_STATUS_GROUP, MQTT_DISCONNECTED, pdFALSE, pdTRUE, WAIT_UNTILL_BIT);
        led_handler_clear_all();
        buzzer_signal_init(BUZZER_PERIOD, BUZZER_DUTY);
    }
}
