idf_component_register(
    SRCS
        "event_manager.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        global
        logger
)
