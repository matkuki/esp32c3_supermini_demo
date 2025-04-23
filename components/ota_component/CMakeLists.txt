idf_component_register(
    SRCS "ota_controller.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_http_client esp_https_ota esp_partition esp_wifi nvs_flash app_update
    EMBED_TXTFILES ca_cert.pem
)