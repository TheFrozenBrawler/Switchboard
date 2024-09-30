#ifndef COMPONENTS_WIFI_TASK_NTP_DRIVER_H_
#define COMPONENTS_WIFI_TASK_NTP_DRIVER_H_

#include "esp_err.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ntp_driver_init();
esp_err_t ntp_driver_start();
void ntp_driver_stop();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_WIFI_TASK_NTP_DRIVER_H_
