idf_component_register(
    SRCS "mqtt_controller.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event mqtt custom_data_types
    EMBED_TXTFILES cacert.pem
)