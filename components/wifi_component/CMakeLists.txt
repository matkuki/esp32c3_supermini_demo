idf_component_register(
    SRCS "wifi_controller.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_netif esp_wifi nvs_flash bt wifi_provisioning qrcode
)