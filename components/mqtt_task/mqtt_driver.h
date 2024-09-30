#ifndef COMPONENTS_MQTT_TASK_MQTT_DRIVER_H_
#define COMPONENTS_MQTT_TASK_MQTT_DRIVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t mqtt_init();

esp_err_t mqtt_publish(const char *topic, const char *msg, uint8_t size, uint8_t qos);

void mqtt_run_process();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_MQTT_TASK_MQTT_DRIVER_H_
