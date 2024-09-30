#include "wifi_task.h"

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buzzer_handler.h"
#include "common.h"
#include "event_manager.h"
#include "wifi_driver.h"

#if CONFIG_SWITCH_LOGGER_EN
#include "logger.h"
DECLARE_LOG(WIFI TASK)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#if CONFIG_WIFI_TASK_STACK_SIZE % 4 != 0
#error "Task Stack size must be divisible by 4"
#endif

#define WIFI_INIT_MAX_RETRIES 20

static StackType_t wifiTaskStack[CONFIG_WIFI_TASK_STACK_SIZE];
static StaticTask_t wifiTaskBuffer;

static void wifi_task(void *);

void wifi_task_init() {
    xTaskCreateStaticPinnedToCore(wifi_task,
                                  "WIFI TASK",
                                  CONFIG_WIFI_TASK_STACK_SIZE,
                                  NULL,
                                  PRIORITY_NORMAL,
                                  wifiTaskStack,
                                  &wifiTaskBuffer,
                                  CONFIG_WIFI_TASK_CORE);
}

static void wifi_task(void *) {
    uint8_t init_counter = 1;
    while (wifi_init() != ESP_OK) {
        LOG_I(TAG, "Trying again to init wifi for %d time", init_counter);
        init_counter++;
        if (init_counter == WIFI_INIT_MAX_RETRIES) {
            buzzer_signal_failure();  // require manual reset
            LOG_E(TAG, "WIFI init error!!!");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    set_event_bit(MAIN_EVENT_GROUP, MAIN_WIFI_INITIALIZED);

    while (1) {
        wifi_run_process();
    }
}
