#ifndef COMPONENTS_QUEUE_TYPES_MANAGER_QUEUE_TYPES_MANAGER_H_
#define COMPONENTS_QUEUE_TYPES_MANAGER_QUEUE_TYPES_MANAGER_H_

#include "freertos/FreeRTOS.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SWITCHES_ISR_QUEUE,
    BATTERY_MSG_QUEUE,
    SWITCHES_MSG_QUEUE,
    QUEUES_SENTINEL,
} queues_enum;

typedef enum {
    SEMPHR_MQTT_CON,
    SEMPHR_MQTT_DISCON,
    SEMPHR_SENTINEL,
} semphr_enum;

typedef enum {
    // switch queue set
    QUEUE_TYPE_SWITCH_ISR_Q,
    QUEUE_TYPE_MQTT_CON,
    QUEUE_TYPE_MQTT_DISCON,

    // message queue set
    QUEUE_TYPE_BATTERY_MSG_Q,
    QUEUE_TYPE_SWITCHES_MSG_Q
} queue_type_enum;

void queue_types_manager_init();

// QUEUES
BaseType_t queue_switch_sendfrom_isr(queues_enum QUEUE, void *data);
BaseType_t queue_send(queues_enum QUEUE, void *data);
BaseType_t queue_receive_item(queues_enum queue, void *data_buffer, TickType_t ticks_to_wait);
BaseType_t queue_set_empty(queues_enum queue);

// SEMAPHORES
esp_err_t semphr_give(semphr_enum semphr);
esp_err_t semphr_take(semphr_enum semphr, TickType_t ticks_to_wait);

// QUEUE SETS
queue_type_enum queue_set_switches_type_select(TickType_t ticks_to_wait);
queue_type_enum queue_set_message_type_select(TickType_t ticks_to_wait);

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_QUEUE_TYPES_MANAGER_QUEUE_TYPES_MANAGER_H_
