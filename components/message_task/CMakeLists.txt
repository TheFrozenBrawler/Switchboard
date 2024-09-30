idf_component_register(
    SRCS
        "message_manager.c"
        "message_task.c"
    INCLUDE_DIRS "."
    REQUIRES
        global
        battery
        event_manager
        queue_types_manager
        mqtt_task
        putrl_proto
    PRIV_REQUIRES
        logger
)
