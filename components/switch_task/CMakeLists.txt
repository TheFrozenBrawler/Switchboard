idf_component_register(
    SRCS
        "switch_task.cpp"
        "switch_manager.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        global
        queue_types_manager
        event_manager
        message_task
        ui_task
    PRIV_REQUIRES
        driver
        logger
)
