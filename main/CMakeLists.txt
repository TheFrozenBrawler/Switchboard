idf_component_register(
    SRCS
        "main.cpp"
    REQUIRES
        global
        mqtt_task
        wifi_task
        switch_task
        event_manager
        queue_types_manager
        nvs_flash
        driver
        battery
        ui_task
        message_task
    )
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
