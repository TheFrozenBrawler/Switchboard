#ifndef COMPONENTS_QUEUE_TYPES_MANAGER_QUEUE_PACKET_H_
#define COMPONENTS_QUEUE_TYPES_MANAGER_QUEUE_PACKET_H_

/** just to indicate what is what in
 * some functions/queues */
typedef uint16_t switch_number_t;
typedef switch_number_t switch_bits_t;
typedef switch_number_t mqtt_bit_t;
typedef switch_number_t led_bit_t;

typedef switch_number_t queue_packet_switches_isr_t;
typedef switch_number_t queue_packet_switches_t;

typedef struct {
    uint32_t raw_voltage;
    float voltage;
    float cpu_temperature;
} queue_packet_battery_t;

#endif  // COMPONENTS_QUEUE_TYPES_MANAGER_QUEUE_PACKET_H_
