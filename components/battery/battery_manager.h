#ifndef COMPONENTS_BATTERY_BATTERY_MANAGER_H_
#define COMPONENTS_BATTERY_BATTERY_MANAGER_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "inttypes.h"

#include "queue_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t battery_manager_init();

esp_err_t battery_manager_measure_timer_init();

esp_err_t battery_manager_take_measurements(queue_packet_battery_t *battery_data);

void wait_for_batt_timer_smph();

void battery_timer_start();

void battery_timer_stop();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_BATTERY_BATTERY_MANAGER_H_
