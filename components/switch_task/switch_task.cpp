#include "switch_task.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "common.h"
#include "event_manager.h"
#include "led_handler.h"
#include "message_manager.h"
#include "queue_types_manager.h"
#include "switch_manager.h"

#if CONFIG_SWITCH_LOGGER_EN
#include "logger.h"
DECLARE_LOG(SWITCH TASK)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

bool SwitchManager::isr_running = false;

#if CONFIG_SWITCH_TASK_STACK_SIZE % 4 != 0
#error "Task Stack size must be divisible by 4"
#endif

static StackType_t switchTaskStack[CONFIG_SWITCH_TASK_STACK_SIZE];
static StaticTask_t switchTaskBuffer;

static void switch_task(void *);

void switch_task_init() {
    xTaskCreateStaticPinnedToCore(switch_task,
                                  "SWITCH TASK",
                                  CONFIG_SWITCH_TASK_STACK_SIZE,
                                  NULL,
                                  PRIORITY_HIGH,
                                  switchTaskStack,
                                  &switchTaskBuffer,
                                  CONFIG_SWITCH_TASK_CORE);
}

static void switch_task(void *) {
    LOG_I(TAG, "Initialisation of switch task...");

    SwitchManager SwitchManager;
    queue_packet_switches_isr_t sw_number = 0;

    LOG_I(TAG, "Switch task initiated");

    wait_event_bit(MQTT_STATUS_GROUP, MQTT_CONNECTED, pdFALSE, pdTRUE, WAIT_UNTILL_BIT);

    LOG_I(TAG, "Ready to send message");

    while (1) {
        queue_type_enum queue_type_handler = queue_set_switches_type_select(WAIT_FOREVER);
        if (queue_type_handler == QUEUE_TYPE_SWITCH_ISR_Q) {
            if (queue_receive_item(SWITCHES_ISR_QUEUE, &sw_number, WAIT_UNTILL_BIT) == pdPASS
                && get_event_bits(MQTT_STATUS_GROUP) == MQTT_CONNECTED) {
                if (SwitchManager.switch_debouncer(sw_number)) {
                    SwitchManager.check_switch_set_binMsg(sw_number);
                    queue_packet_switches_t binary_msg = SwitchManager.give_binary_msg();
                    LOG_I(TAG, "added to queue - bin msg: %d", binary_msg);
                    queue_send(SWITCHES_MSG_QUEUE, &binary_msg);
                }
            }
        } else if (queue_type_handler == QUEUE_TYPE_MQTT_CON) {
            semphr_take(SEMPHR_MQTT_CON, WAIT_FOREVER);
            queue_set_empty(SWITCHES_MSG_QUEUE);
            SwitchManager.update_switch_binary_msg();
            queue_packet_switches_t binary_msg = SwitchManager.give_binary_msg();
            queue_send(SWITCHES_MSG_QUEUE, &binary_msg);

        } else if (queue_type_handler == QUEUE_TYPE_MQTT_DISCON) {
            semphr_take(SEMPHR_MQTT_DISCON, WAIT_FOREVER);

        } else {
            LOG_E("Queue set: wrong queue type received");
        }
    }
}
