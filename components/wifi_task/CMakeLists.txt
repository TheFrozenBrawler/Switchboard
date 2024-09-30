idf_component_register(
    SRCS
        "wifi_driver.c"
        "wifi_task.cpp"
        "ntp_driver.c"
    INCLUDE_DIRS "."
    REQUIRES
        global
        event_manager
        ui_task
    PRIV_REQUIRES
        driver
        logger
        esp_wifi
)
