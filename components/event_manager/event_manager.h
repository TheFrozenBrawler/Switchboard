#ifndef COMPONENTS_EVENT_MANAGER_EVENT_MANAGER_H_
#define COMPONENTS_EVENT_MANAGER_EVENT_MANAGER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WAIT_UNTILL_BIT portMAX_DELAY

// __________ EVENT BITS __________
//  --- WIFI ISR / STATUS GROUP BITS ---
#define WIFI_CONNECTED    BIT0
#define WIFI_DISCONNECTED BIT1
#define WIFI_STARTED      BIT2
#define WIFI_IDLE         BIT3
#define WIFI_ALL          WIFI_CONNECTED | WIFI_DISCONNECTED | WIFI_STARTED | WIFI_IDLE

//  --- WIFI RUNNING GROUP BITS ---
#define WIFI_RUNNING BIT0
#define WIFI_STOPPED BIT1

// --- IP BITS ---
#define IP_GOT  BIT0
#define IP_LOST BIT1

// --- MQTT ISR / STATUS GROUP BITS ---
#define MQTT_CONNECTED    BIT0
#define MQTT_DISCONNECTED BIT1
#define MQTT_ALL          MQTT_CONNECTED | MQTT_DISCONNECTED

// --- MAIN BITS---
#define MAIN_WIFI_INITIALIZED    BIT0
#define MAIN_MQTT_INITIALIZED    BIT1
#define MAIN_BATTERY_INITIALIZED BIT2
#define MAIN_ALL_INITIALIZED     MAIN_WIFI_INITIALIZED | MAIN_MQTT_INITIALIZED | MAIN_BATTERY_INITIALIZED

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIN_EVENT_GROUP,
    WIFI_ISR_EVENT_GROUP,
    WIFI_STATUS_EVENT_GROUP,
    WIFI_RUNNING_EVENT_GROUP,
    IP_STATUS_EVENT_GROUP,
    MQTT_ISR_GROUP,
    MQTT_STATUS_GROUP,
    EVENT_GROUPS_SENTINEL,
} event_groups_enum;

EventBits_t wait_event_bit(event_groups_enum event_group,
                           EventBits_t bit,
                           BaseType_t clear_bit,
                           BaseType_t wait_for_all,
                           TickType_t ticks_to_wait);

EventBits_t set_event_bit(event_groups_enum event_group, EventBits_t bit);

EventBits_t set_event_bit_from_isr(event_groups_enum event_group, EventBits_t bit);

EventBits_t get_event_bits(event_groups_enum event_group);

EventBits_t clear_event_bits(event_groups_enum event_group, EventBits_t bits_to_clear);

void event_manager_init();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_EVENT_MANAGER_EVENT_MANAGER_H_
