#ifndef COMPONENTS_UI_TASK_LED_HANDLER_H_
#define COMPONENTS_UI_TASK_LED_HANDLER_H_

#include "common.h"
#include "esp_err.h"
#include "queue_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LED_COLOR_ON  CONFIG_ON_COLOR_RED, CONFIG_ON_COLOR_GREEN, CONFIG_ON_COLOR_BLUE
#define LED_COLOR_OFF CONFIG_OFF_COLOR_RED, CONFIG_OFF_COLOR_GREEN, CONFIG_OFF_COLOR_BLUE

esp_err_t led_rmt_driver_init();
esp_err_t led_handler_set_color(uint8_t red, uint8_t green, uint8_t blue, led_bit_t led_bit);
esp_err_t led_handler_clear_all();
esp_err_t led_rmt_driver_send();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_UI_TASK_LED_HANDLER_H_
