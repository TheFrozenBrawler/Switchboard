#include "message_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "sys/time.h"

#include "nanopb/broker_msg_out.pb.h"
#include "pb.h"
#include "pb_common.h"
#include "pb_encode.h"

#include "battery_manager.h"
#include "common.h"
#include "cpu_temp_sensor.h"
#include "event_manager.h"
#include "mqtt_driver.h"
#include "queue_packet.h"
#include "queue_types_manager.h"

#if CONFIG_MESSAGE_LOGGER_EN
#include "logger.h"
DECLARE_LOG(MESSAGE MANAGER)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#if defined(CONFIG_MQTT_RELEASE_TOPIC) && !defined(CONFIG_MQTT_DEBUG_TOPIC)
#define SWITCHBOARD_TOPIC CONFIG_MQTT_RELEASE_TOPIC
#elif defined(CONFIG_MQTT_DEBUG_TOPIC) && !defined(CONFIG_MQTT_RELEASE_TOPIC)
#define SWITCHBOARD_TOPIC CONFIG_MQTT_DEBUG_TOPIC
#else
#error "Invalid Topic mode selected"
#endif

#if defined(CONFIG_SWITCHBOARD_NB_1) && !defined(CONFIG_SWITCHBOARD_NB_2)
#define HEADER_ORIGIN Origin_ORIGIN_SWITCHBOARD_1;
#elif defined(CONFIG_SWITCHBOARD_NB_2) && !defined(CONFIG_SWITCHBOARD_NB_1)
#define HEADER_ORIGIN Origin_ORIGIN_SWITCHBOARD_2;
#else
#error "Invalid switchboard type selected"
#endif

#define SWITCH_MSG_PROTO_BUFFER_SIZE  200
#define BATTERY_MSG_PROTO_BUFFER_SIZE 200

static uint32_t msg_counter;

void message_manager_init() {
    msg_counter = 0;
}

int64_t message_manager_get_timestamp() {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t time_us = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;

    return time_us;
}

static esp_err_t message_manager_create_header(broker_message_out_SwitchBoardMsgOut *switchboard_message) {
    switchboard_message->has_header       = true;
    switchboard_message->header.origin    = HEADER_ORIGIN;
    switchboard_message->header.timestamp = message_manager_get_timestamp();
    switchboard_message->header.counter   = msg_counter;

    return ESP_OK;
}

static esp_err_t increment_msg_counter() {
    msg_counter++;
    return ESP_OK;
}

esp_err_t message_manager_send_switch(switch_bits_t binary_switches_message) {
    uint8_t switchboardProtoBuffer[SWITCH_MSG_PROTO_BUFFER_SIZE];
    broker_message_out_SwitchBoardMsgOut switchboard_message = broker_message_out_SwitchBoardMsgOut_init_zero;
    pb_ostream_t proto_stream = pb_ostream_from_buffer(switchboardProtoBuffer, sizeof(switchboardProtoBuffer));

    increment_msg_counter();

    // header
    RET_AND_LOG_ON_ERR(message_manager_create_header(&switchboard_message), "message manager create header");

    // data
    switchboard_message.has_data            = true;
    switchboard_message.data.has_switches   = true;
    switchboard_message.data.switches.value = binary_switches_message;
    LOG_I(TAG, "binary message : %ld", switchboard_message.data.switches.value);

    if (!pb_encode(&proto_stream, &broker_message_out_SwitchBoardMsgOut_msg, &switchboard_message)) {
        LOG_E(TAG, "Failed to encode protobuffer. Error call: %s", PB_GET_ERROR(&proto_stream));
    }

    mqtt_publish(SWITCHBOARD_TOPIC, (const char *)switchboardProtoBuffer, proto_stream.bytes_written, MQTT_QOS);

    return ESP_OK;
}

esp_err_t message_manager_send_battery(queue_packet_battery_t battery_data) {
    uint8_t switchboardProtoBuffer[BATTERY_MSG_PROTO_BUFFER_SIZE];
    broker_message_out_SwitchBoardMsgOut switchboard_message = broker_message_out_SwitchBoardMsgOut_init_zero;
    pb_ostream_t proto_stream = pb_ostream_from_buffer(switchboardProtoBuffer, sizeof(switchboardProtoBuffer));

    increment_msg_counter();

    // header
    RET_AND_LOG_ON_ERR(message_manager_create_header(&switchboard_message), "message manager create header");

    // data
    switchboard_message.has_data = true;

    // CPU temperature
    switchboard_message.data.has_cpu_temperature   = true;
    switchboard_message.data.cpu_temperature.value = get_cpu_temp();
    LOG_I(TAG, "CPU temp: %f", switchboard_message.data.cpu_temperature.value);

    // battery data
    LOG_I(TAG, "battery message raw: %ld", battery_data.raw_voltage);
    LOG_I(TAG, "battery message voltage: %f", battery_data.voltage);

#if defined(CONFIG_BATTERY_3V3) && !defined(CONFIG_BATTERY_12V_1) && !defined(CONFIG_BATTERY_12V_2)
    switchboard_message.data.has_battery_3v3    = true;
    switchboard_message.data.battery_3v3.raw    = battery_data.raw_voltage;
    switchboard_message.data.battery_3v3.scaled = battery_data.voltage;

#elif defined(CONFIG_BATTERY_12V_1) && !defined(CONFIG_BATTERY_3V3) && !defined(CONFIG_BATTERY_12V_2)
    switchboard_message.data.battery_12v_1        = true;
    switchboard_message.data.battery_12v_1.raw    = battery_data.raw_voltage;
    switchboard_message.data.battery_21v_1.scaled = battery_data.voltage;

#elif defined(CONFIG_BATTERY_12V_2) && !defined(CONFIG_BATTERY_3V3) && !defined(CONFIG_BATTERY_12V_1)
    switchboard_message.data.battery_12v_2        = true;
    switchboard_message.data.battery_12v_2.raw    = battery_data.raw_voltage;
    switchboard_message.data.battery_12v_2.scaled = battery_data.voltage;

#else
#error "Ivalid battery type selected"
#endif

    if (!pb_encode(&proto_stream, &broker_message_out_SwitchBoardMsgOut_msg, &switchboard_message)) {
        LOG_E(TAG, "Failed to encode protobuffer. Error call: %s", PB_GET_ERROR(&proto_stream));
    }

    mqtt_publish(SWITCHBOARD_TOPIC, (const char *)switchboardProtoBuffer, proto_stream.bytes_written, MQTT_QOS);

    return ESP_OK;
}

void message_task_run_process() {
    queue_type_enum queue_type_handler = queue_set_message_type_select(WAIT_FOREVER);
    if (queue_type_handler == QUEUE_TYPE_SWITCHES_MSG_Q) {
        LOG_I(TAG, "aquired queue type: switches");
        queue_packet_switches_t switches_bin_msg;
        queue_receive_item(SWITCHES_MSG_QUEUE, &switches_bin_msg, WAIT_UNTILL_BIT);
        message_manager_send_switch(switches_bin_msg);

    } else if (queue_type_handler == QUEUE_TYPE_BATTERY_MSG_Q) {
        LOG_I(TAG, "aquired queue type: battery");
        queue_packet_battery_t battery_msg;
        queue_receive_item(BATTERY_MSG_QUEUE, &battery_msg, WAIT_UNTILL_BIT);
        message_manager_send_battery(battery_msg);
    } else {
        ESP_LOGE("MESSAGE MANAGER", "Message queue set- wrong handler returned");
    }
}
