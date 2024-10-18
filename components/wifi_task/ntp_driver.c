#include "ntp_driver.h"

#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "common.h"

#if CONFIG_WIFI_LOGGER_EN
#include "logger.h"
DECLARE_LOG(NTP DRIVER)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

static SemaphoreHandle_t ntp_semaphore;
static StaticSemaphore_t ntp_semaphore_buffer;

static TimerHandle_t ntp_timer_handler;
static StaticTimer_t ntp_timer_buffer;

static bool ntp_running = false;

static void time_sync_cb(struct timeval *tv) {
    LOG_I(TAG, "TIME SYNC");
    xSemaphoreGive(ntp_semaphore);
}

void ntp_setup() {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, CONFIG_NTP_SERVER_ADDR);
    sntp_set_time_sync_notification_cb(time_sync_cb);
    esp_sntp_init();
}

void ntp_connect_timer_cb(TimerHandle_t timer) {
    if (xSemaphoreTake(ntp_semaphore, pdMS_TO_TICKS(CONFIG_NTP_CONNECT_TIMEOUT_MS)) == pdTRUE) {
        setenv("TZ", CONFIG_NTP_TIME_ZONE, 1);
        tzset();
        return;
    } else {
        LOG_W(TAG, "could not get time, stopping NTP");
        ntp_driver_stop();
        ntp_driver_start();
    }
}

esp_err_t ntp_driver_init() {
    ntp_semaphore     = xSemaphoreCreateBinaryStatic(&ntp_semaphore_buffer);
    ntp_timer_handler = xTimerCreateStatic("NTP Timer",
                                           pdMS_TO_TICKS(CONFIG_NTP_CONNECT_TIMEOUT_MS),
                                           pdFALSE,
                                           (void *)0,
                                           ntp_connect_timer_cb,
                                           &ntp_timer_buffer);

    if ((ntp_semaphore == NULL) || (ntp_timer_handler == NULL)) {
        LOG_E(TAG, "NTP semaphore or timer wasn't initialized properly");
        return ESP_FAIL;
    }

    LOG_I(TAG, "initialized");
    return ESP_OK;
}

esp_err_t ntp_driver_start() {
    if (ntp_running == true) {
        LOG_I(TAG, "already running");
        return ESP_ERR_INVALID_STATE;
    }
    LOG_I(TAG, "driver start");
    ntp_setup();
    ntp_running = true;
    xTimerStart(ntp_timer_handler, pdMS_TO_TICKS(CONFIG_NTP_CONNECT_TIMEOUT_MS));

    return ESP_OK;
}

void ntp_driver_stop() {
    if (!ntp_running) {
        return;
    }
    LOG_I(TAG, "stop");
    xTimerStop(ntp_timer_handler, pdMS_TO_TICKS(100));
    esp_sntp_stop();
    ntp_running = false;
}
