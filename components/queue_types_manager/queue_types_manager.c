#include "queue_types_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "battery_manager.h"
#include "common.h"
#include "queue_packet.h"

#define SWITCHES_ISR_QUEUE_MAX_ITEMS 20
#define SWITCHES_ISR_QUEUE_ITEM_SIZE sizeof(queue_packet_switches_isr_t)

#define BATTERY_MSG_QUEUE_MAX_ITEMS 20
#define BATTERY_MSG_QUEUE_ITEM_SIZE sizeof(queue_packet_battery_t)

#define SWITCHES_MSG_QUEUE_MAX_ITEMS 20
#define SWITCHES_MSG_QUEUE_ITEM_SIZE sizeof(queue_packet_switches_t)

#define QUEUE_SET_SMPHR_SIZE 1

// queue sets
// switches queue
#define SWITCHES_QUEUE_SET_SIZE SWITCHES_ISR_QUEUE_MAX_ITEMS + 2 * QUEUE_SET_SMPHR_SIZE
#define MESSAGE_QUEUE_SET_SIZE  BATTERY_MSG_QUEUE_MAX_ITEMS + SWITCHES_MSG_QUEUE_MAX_ITEMS

// item queue
static QueueHandle_t queues_arr[QUEUES_SENTINEL];

// switches isr queue
static uint8_t switches_isr_queue_storage_buffer[SWITCHES_ISR_QUEUE_MAX_ITEMS * SWITCHES_ISR_QUEUE_ITEM_SIZE];
static StaticQueue_t switches_isr_queue_buffer;

// battery msg queue
static uint8_t battery_queue_storage_buffer[BATTERY_MSG_QUEUE_MAX_ITEMS * BATTERY_MSG_QUEUE_ITEM_SIZE];
static StaticQueue_t battery_queue_buffer;

// switches msg queue
static uint8_t switches_queue_storage_buffer[SWITCHES_MSG_QUEUE_MAX_ITEMS * SWITCHES_MSG_QUEUE_ITEM_SIZE];
static StaticQueue_t switches_msg_queue_buffer;

// queue sets
static QueueSetHandle_t switch_queue_set;
static QueueSetHandle_t message_queue_set;

// semaphores
static SemaphoreHandle_t semphrs_arr[SEMPHR_SENTINEL];

void queue_types_manager_init() {
    // create queues
    queues_arr[SWITCHES_ISR_QUEUE] = xQueueCreateStatic(SWITCHES_ISR_QUEUE_MAX_ITEMS,
                                                        SWITCHES_ISR_QUEUE_ITEM_SIZE,
                                                        switches_isr_queue_storage_buffer,
                                                        &switches_isr_queue_buffer);

    queues_arr[BATTERY_MSG_QUEUE] = xQueueCreateStatic(
      BATTERY_MSG_QUEUE_MAX_ITEMS, BATTERY_MSG_QUEUE_ITEM_SIZE, battery_queue_storage_buffer, &battery_queue_buffer);

    queues_arr[SWITCHES_MSG_QUEUE] = xQueueCreateStatic(SWITCHES_MSG_QUEUE_MAX_ITEMS,
                                                        SWITCHES_MSG_QUEUE_ITEM_SIZE,
                                                        switches_queue_storage_buffer,
                                                        &switches_msg_queue_buffer);

    for (uint8_t i = 0; i < QUEUES_SENTINEL; i++) {
        if (queues_arr[i] == NULL) {
            ESP_LOGE("QUEUE MANAGER", "Queue nb %d wasn't created!", i);
        }
    }

    // create semaphors
    semphrs_arr[SEMPHR_MQTT_CON]    = xSemaphoreCreateBinary();
    semphrs_arr[SEMPHR_MQTT_DISCON] = xSemaphoreCreateBinary();

    // create switch queue set
    switch_queue_set = xQueueCreateSet(SWITCHES_QUEUE_SET_SIZE);
    xQueueAddToSet(queues_arr[SWITCHES_ISR_QUEUE], switch_queue_set);
    xQueueAddToSet(semphrs_arr[SEMPHR_MQTT_CON], switch_queue_set);
    xQueueAddToSet(semphrs_arr[SEMPHR_MQTT_DISCON], switch_queue_set);

    // create message queue set
    message_queue_set = xQueueCreateSet(MESSAGE_QUEUE_SET_SIZE);
    xQueueAddToSet(queues_arr[BATTERY_MSG_QUEUE], message_queue_set);
    xQueueAddToSet(queues_arr[SWITCHES_MSG_QUEUE], message_queue_set);
}

// QUEUES
BaseType_t queue_switch_sendfrom_isr(queues_enum QUEUE, void *data) {
    return xQueueSendFromISR(queues_arr[QUEUE], data, NULL);
}

BaseType_t queue_send(queues_enum QUEUE, void *data) {
    return xQueueSend(queues_arr[QUEUE], data, WAIT_FOREVER);
}

BaseType_t queue_receive_item(queues_enum QUEUE, void *data_buffer, TickType_t ticks_to_wait) {
    return xQueueReceive(queues_arr[QUEUE], data_buffer, ticks_to_wait);
}

BaseType_t queue_set_empty(queues_enum QUEUE) {
    return xQueueReset(queues_arr[QUEUE]);
}

// SEMAPHORES
esp_err_t semphr_give(semphr_enum semphr) {
    return xSemaphoreGive(semphrs_arr[semphr]);
}

esp_err_t semphr_take(semphr_enum semphr, TickType_t ticks_to_wait) {
    return xSemaphoreTake(semphrs_arr[semphr], ticks_to_wait);
}

// SWITCHES QUEUE SET
queue_type_enum queue_set_switches_type_select(TickType_t ticks_to_wait) {
    QueueSetMemberHandle_t queue_type_handler = xQueueSelectFromSet(switch_queue_set, ticks_to_wait);

    if (queue_type_handler == queues_arr[SWITCHES_ISR_QUEUE]) {
        return QUEUE_TYPE_SWITCH_ISR_Q;

    } else if (queue_type_handler == semphrs_arr[SEMPHR_MQTT_CON]) {
        return QUEUE_TYPE_MQTT_CON;

    } else if (queue_type_handler == semphrs_arr[SEMPHR_MQTT_DISCON]) {
        return QUEUE_TYPE_MQTT_DISCON;

    } else {
        ESP_LOGE("QUEUE MANAGER", "Switches queue set- wrong handler returned");
    }

    return ESP_FAIL;
}

// MESSAGE MANAGER QUEUE SET
queue_type_enum queue_set_message_type_select(TickType_t ticks_to_wait) {
    QueueSetMemberHandle_t queue_type_handler = xQueueSelectFromSet(message_queue_set, ticks_to_wait);

    if (queue_type_handler == queues_arr[BATTERY_MSG_QUEUE]) {
        return QUEUE_TYPE_BATTERY_MSG_Q;

    } else if (queue_type_handler == queues_arr[SWITCHES_MSG_QUEUE]) {
        return QUEUE_TYPE_SWITCHES_MSG_Q;

    } else {
        ESP_LOGE("QUEUE MANAGER", "Battery queue set- wrong handler returned");
    }

    return ESP_FAIL;
}
