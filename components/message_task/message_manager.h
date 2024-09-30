#ifndef COMPONENTS_MESSAGE_TASK_MESSAGE_MANAGER_H_
#define COMPONENTS_MESSAGE_TASK_MESSAGE_MANAGER_H_

#define MQTT_QOS 2

#ifdef __cplusplus
extern "C" {
#endif

void message_manager_init();

void message_task_run_process();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_MESSAGE_TASK_MESSAGE_MANAGER_H_
