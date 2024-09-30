#include "message_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "common.h"
#include "event_manager.h"
#include "message_manager.h"

#if CONFIG_MESSAGE_TASK_STACK_SIZE % 4 != 0
#error "Task Stack size must be divisible by 4"
#endif

static StackType_t messageTaskStack[CONFIG_MESSAGE_TASK_STACK_SIZE];
static StaticTask_t messageTaskBuffer;

static void message_task(void *);

void message_task_init() {
    xTaskCreateStaticPinnedToCore(message_task,
                                  "MESSAGE TASK",
                                  CONFIG_MESSAGE_TASK_STACK_SIZE,
                                  NULL,
                                  PRIORITY_HIGH,
                                  messageTaskStack,
                                  &messageTaskBuffer,
                                  CONFIG_MESSAGE_TASK_CORE);
}

static void message_task(void *) {
    while (1) {
        message_task_run_process();
    }
}
