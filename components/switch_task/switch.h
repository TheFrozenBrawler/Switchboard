#ifndef COMPONENTS_SWITCH_TASK_SWITCH_H_
#define COMPONENTS_SWITCH_TASK_SWITCH_H_

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "queue_packet.h"

enum class switch_state {
    OFF,
    ON,
    UNKNOWN,
};

typedef struct {
    switch_number_t switch_number;
    gpio_num_t pin;
    mqtt_bit_t mqtt_bit;
    led_bit_t led_bit;
    switch_state last_state;
} switch_t;

#endif  // COMPONENTS_SWITCH_TASK_SWITCH_H_
