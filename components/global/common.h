/* SwitchBoard config */
#ifndef COMPONENTS_GLOBAL_COMMON_H_
#define COMPONENTS_GLOBAL_COMMON_H_

#include <esp_log.h>

#include "logger.h"

// __________GLOBAL MACROS __________
#define WAIT_FOREVER    portMAX_DELAY
#define PRIORITY_LOW    CONFIG_PRIORITY_LOW
#define PRIORITY_NORMAL CONFIG_PRIORITY_NORMAL
#define PRIORITY_HIGH   CONFIG_PRIORITY_HIGH

// __________ MISC __________
#define DELETE_TASK()  \
    vTaskDelete(NULL); \
    __builtin_unreachable()

#define RET_AND_LOG_ON_ERR(X, MSG) \
    do {                           \
        esp_err_t err = (X);       \
        if (err != ESP_OK) {       \
            LOG_I("ERROR", #MSG);  \
            return err;            \
        }                          \
    } while (0)

// --- SWITCH ---
#define COMMON_NUMBER_OF_SWITCHES 12

#endif  // COMPONENTS_GLOBAL_COMMON_H_
