idf_component_register(
    SRCS "gpio_controller.c" "led.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer custom_data_types
)