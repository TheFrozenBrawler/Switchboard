#include "switch_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "inttypes.h"

#include "common.h"
#include "event_manager.h"
#include "led_handler.h"
#include "message_manager.h"
#include "queue_types_manager.h"
#include "switch.h"

#if CONFIG_SWITCH_LOGGER_EN
#include "logger.h"
DECLARE_LOG(SWITCH MANAGER)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

// elements corresponde to numbers of switches.
// If element is true - it was already used and
// debouncer should prevent it from beeing used
static bool debouncing_switch_arr[NUMBER_OF_SWITCHES];

StaticTimer_t debouncing_timer_buffer[NUMBER_OF_SWITCHES];
TimerHandle_t debouncing_timers_table[NUMBER_OF_SWITCHES];

static StaticSemaphore_t mutex_bin_msg_buffer;

struct message_manager_context {
    switch_bits_t binary_switches_message;
    SemaphoreHandle_t mutex_bin_msg;
};

static struct message_manager_context ctx = {};

switch_t SwitchManager::switches[NUMBER_OF_SWITCHES] = {
        [SWITCH_0] = {
            SWITCH_0,
            GPIO_NUM_16,
            CONFIG_SWITCH_0_MQTT_BIT,
            CONFIG_SWITCH_0_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_1] ={   // unused switch
            SWITCH_1,
            GPIO_NUM_15,
            CONFIG_SWITCH_1_MQTT_BIT,
            CONFIG_SWITCH_1_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_2] ={
            SWITCH_2,
            GPIO_NUM_18,
            CONFIG_SWITCH_2_MQTT_BIT,
            CONFIG_SWITCH_2_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_3] ={
            SWITCH_3,
            GPIO_NUM_17,
            CONFIG_SWITCH_3_MQTT_BIT,
            CONFIG_SWITCH_3_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_4] ={
            SWITCH_4,
            GPIO_NUM_9,
            CONFIG_SWITCH_4_MQTT_BIT,
            CONFIG_SWITCH_4_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_5] ={
            SWITCH_5,
            GPIO_NUM_8,
            CONFIG_SWITCH_5_MQTT_BIT,
            CONFIG_SWITCH_5_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_6] ={
            SWITCH_6,
            GPIO_NUM_40,
            CONFIG_SWITCH_6_MQTT_BIT,
            CONFIG_SWITCH_6_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_7] ={
            SWITCH_7,
            GPIO_NUM_41,
            CONFIG_SWITCH_7_MQTT_BIT,
            CONFIG_SWITCH_7_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_8] ={
            SWITCH_8,
            GPIO_NUM_2,
            CONFIG_SWITCH_8_MQTT_BIT,
            CONFIG_SWITCH_8_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_9] ={
            SWITCH_9,
            GPIO_NUM_42,
            CONFIG_SWITCH_9_MQTT_BIT,
            CONFIG_SWITCH_9_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_10] ={
            SWITCH_10,
            GPIO_NUM_39,
            CONFIG_SWITCH_10_MQTT_BIT,
            CONFIG_SWITCH_10_LED_BIT,
            switch_state::UNKNOWN,
        },
        [SWITCH_11] ={
            SWITCH_11,
            GPIO_NUM_38,
            CONFIG_SWITCH_11_MQTT_BIT,
            CONFIG_SWITCH_11_LED_BIT,
            switch_state::UNKNOWN,
        },
    };

SwitchManager::SwitchManager() {
    LOG_I(TAG, "Initialisation of switch manager...");

    ctx.mutex_bin_msg           = xSemaphoreCreateMutexStatic(&mutex_bin_msg_buffer);
    ctx.binary_switches_message = 0;

    debouncer_init();

    for (uint8_t sw = 0; sw < NUMBER_OF_SWITCHES; sw++) {
        switchSetup(sw);
    }
}

SwitchManager::~SwitchManager() {
}

void SwitchManager::switchSetup(switch_number_t sw) {
    switch_number_t switch_number = switches[sw].switch_number;
    gpio_num_t gpio_num           = switches[sw].pin;
    LOG_I(TAG, "Init of switch no.: %d", switch_number);

    // debouncing timer
    char timer_name[16];
    snprintf(timer_name, sizeof(timer_name), "Timer nb %d", switch_number);
    debouncing_timers_table[sw] = xTimerCreateStatic(timer_name,
                                                     pdMS_TO_TICKS(CONFIG_DEBOUNCING_TIME),
                                                     pdFALSE,
                                                     reinterpret_cast<void *>(sw),
                                                     debounce_timer_cb,
                                                     &debouncing_timer_buffer[sw]);

    // ISR
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_intr_type(gpio_num, GPIO_INTR_ANYEDGE);

    gpio_isr_handler_add(gpio_num, switches_isr, reinterpret_cast<void *>(switch_number));
}

void IRAM_ATTR SwitchManager::switches_isr(void *arg) {
    queue_packet_switches_isr_t sw_numb = (int)arg;
    queue_switch_sendfrom_isr(SWITCHES_ISR_QUEUE, &sw_numb);
}

// binnary message handle functions
esp_err_t SwitchManager::modify_binary_msg(switch_number_t binary_switches_new) {
    xSemaphoreTake(ctx.mutex_bin_msg, portMAX_DELAY);

    ctx.binary_switches_message = binary_switches_new;

    xSemaphoreGive(ctx.mutex_bin_msg);

    return ESP_OK;
}

switch_number_t SwitchManager::give_binary_msg() {
    return ctx.binary_switches_message;
}

esp_err_t SwitchManager::change_binary_msg_bit(mqtt_bit_action action, switch_number_t mqtt_bit) {
    switch_number_t binary_switches_message;
    switch (action) {
    case mqtt_bit_action::SET:
        binary_switches_message = give_binary_msg();
        binary_switches_message &= ~((switch_number_t)1 << mqtt_bit);
        modify_binary_msg(binary_switches_message);
        break;

    case mqtt_bit_action::CLEAR:
        binary_switches_message = give_binary_msg();
        binary_switches_message |= ((switch_number_t)1 << mqtt_bit);
        modify_binary_msg(binary_switches_message);
        break;

    default:
        LOG_W(TAG, "incorrect mqtt bit action");
        break;
    }

    return ESP_OK;
}

esp_err_t SwitchManager::check_switch_set_binMsg(switch_number_t sw_number) {
    gpio_num_t sw_pin           = switches[sw_number].pin;
    switch_number_t sw_mqtt_bit = switches[sw_number].mqtt_bit;

    // check gpio level and set switch state, binary_switches_message and led
    if (gpio_get_level(sw_pin)) {
        switches[sw_number].last_state = switch_state::ON;
        RET_AND_LOG_ON_ERR(change_binary_msg_bit(mqtt_bit_action::SET, sw_mqtt_bit), "Set mqtt bit");
        led_handler_set_color(LED_COLOR_ON, switches[sw_number].led_bit);

    } else {
        switches[sw_number].last_state = switch_state::OFF;
        RET_AND_LOG_ON_ERR(change_binary_msg_bit(mqtt_bit_action::CLEAR, sw_mqtt_bit), "set mqtt bit");
        led_handler_set_color(LED_COLOR_OFF, switches[sw_number].led_bit);
    }
    LOG_I(TAG, "switch number: %d", sw_number);

    return ESP_OK;
}

esp_err_t SwitchManager::update_switch_binary_msg() {
    for (switch_number_t sw = 0; sw < NUMBER_OF_SWITCHES; sw++) {
        check_switch_set_binMsg(sw);
    }

    return ESP_OK;
}

// debouner functions
void SwitchManager::debouncer_init() {
    for (uint8_t sw = 0; sw < NUMBER_OF_SWITCHES; sw++) {
        debouncing_switch_arr[sw] = false;
    }
}

bool SwitchManager::switch_debouncer(switch_number_t sw_number) {
    if (debouncing_switch_arr[sw_number] == true) {
        return false;
    }

    debouncing_switch_arr[sw_number] = true;
    xTimerStart(debouncing_timers_table[sw_number], 0);
    return true;
}

void debounce_timer_cb(TimerHandle_t debouncing_timer_handle) {
    uint32_t sw_number               = (uint32_t)pvTimerGetTimerID(debouncing_timer_handle);
    debouncing_switch_arr[sw_number] = false;
}
