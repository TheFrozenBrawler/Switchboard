#ifndef COMPONENTS_SWITCH_TASK_SWITCH_MANAGER_H_
#define COMPONENTS_SWITCH_TASK_SWITCH_MANAGER_H_

#include "common.h"
#include "driver/gpio.h"
#include "switch.h"

#define NUMBER_OF_SWITCHES COMMON_NUMBER_OF_SWITCHES
#define EMPTY_TIMER_BUFFER NULL
#define EMPTY_TIMER        NULL

typedef enum {
    SWITCH_0,
    SWITCH_1,
    SWITCH_2,
    SWITCH_3,
    SWITCH_4,
    SWITCH_5,
    SWITCH_6,
    SWITCH_7,
    SWITCH_8,
    SWITCH_9,
    SWITCH_10,
    SWITCH_11
} switch_number;

enum class mqtt_bit_action {
    SET,
    CLEAR,
};

class SwitchManager {
 public:
    static switch_t switches[NUMBER_OF_SWITCHES];
    static bool isr_running;

    SwitchManager();
    ~SwitchManager();

    switch_number_t give_binary_msg();
    esp_err_t update_switch_binary_msg();
    esp_err_t check_switch_set_binMsg(switch_number_t sw_number);
    esp_err_t modify_binary_msg(switch_number_t binary_switches_new);
    bool switch_debouncer(switch_number_t sw_number);

 private:
    void switchSetup(switch_number_t sw);
    static void IRAM_ATTR switches_isr(void *arg);

    esp_err_t change_binary_msg_bit(mqtt_bit_action action, switch_number_t mqtt_bit);

    // debouncer
    void debouncer_init();
};

void debounce_timer_cb(TimerHandle_t cb_timer_handle);

#endif  // COMPONENTS_SWITCH_TASK_SWITCH_MANAGER_H_
