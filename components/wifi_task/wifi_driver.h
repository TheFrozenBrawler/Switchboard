#ifndef COMPONENTS_WIFI_TASK_WIFI_DRIVER_H_
#define COMPONENTS_WIFI_TASK_WIFI_DRIVER_H_

#include "esp_check.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_init(void);
void wifi_run_process();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_WIFI_TASK_WIFI_DRIVER_H_
