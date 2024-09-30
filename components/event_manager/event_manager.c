#include "event_manager.h"

#include "inttypes.h"

#if CONFIG_EVENT_LOGGER_EN
#include "logger.h"
DECLARE_LOG(EVENT MANAGER)
#else
#undef LOG_I
#define LOG_I(...)
#endif

EventGroupHandle_t event_groups[EVENT_GROUPS_SENTINEL];
StaticEventGroup_t event_groups_buffers[EVENT_GROUPS_SENTINEL];

void event_manager_init() {
    for (uint8_t i = 0; i < EVENT_GROUPS_SENTINEL; i++) {
        event_groups[i] = xEventGroupCreateStatic(&event_groups_buffers[i]);
    }
}

EventBits_t wait_event_bit(event_groups_enum event_group,
                           EventBits_t bit,
                           BaseType_t clear_bit,
                           BaseType_t wait_for_all,
                           TickType_t ticks_to_wait) {
    return xEventGroupWaitBits(event_groups[event_group], bit, clear_bit, wait_for_all, ticks_to_wait);
}

EventBits_t set_event_bit(event_groups_enum event_group, EventBits_t bit) {
    EventBits_t bits_returned = xEventGroupSetBits(event_groups[event_group], bit);
    LOG_I(TAG, "Event group: %d, bits: %ld", event_group, bits_returned);

    return bits_returned;
}

EventBits_t set_event_bit_from_isr(event_groups_enum event_group, EventBits_t bit) {
    EventBits_t bits_returned = xEventGroupSetBitsFromISR(event_groups[event_group], bit, NULL);
    LOG_I(TAG, "Event group: %d, bits: %ld", event_group, bits_returned);

    return bits_returned;
}

EventBits_t get_event_bits(event_groups_enum event_group) {
    return xEventGroupGetBits(event_groups[event_group]);
}

EventBits_t clear_event_bits(event_groups_enum event_group, EventBits_t bits_to_clear) {
    return xEventGroupClearBits(event_groups[event_group], bits_to_clear);
}
