idf_component_register(
    SRCS
        "mqtt_driver.c"
        "mqtt_task.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        global
        event_manager
        switch_task
        queue_types_manager
        putrl_proto
        event_manager
    PRIV_REQUIRES
        driver
        logger
        mqtt
)
