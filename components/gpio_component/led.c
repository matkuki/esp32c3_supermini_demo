/**
 * @file led.c
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief LED control for the ESP32-C3 SuperMini board
 * @version 0.1
 * @date 2025-04-11
 *
 */

#include "led.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BLINK_PERIOD CONFIG_BLINK_PERIOD

static const char *TAG = "LED";
static uint8_t cache_led_state = 0;

void led_initialize(void) {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    // Set the GPIO as a push/pull output
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void led_on(void) { gpio_set_level(BLINK_GPIO, 0); }

void led_off(void) { gpio_set_level(BLINK_GPIO, 1); }

void led_toggle(void) {
    // Set the GPIO level according to the state (LOW or HIGH)
    gpio_set_level(BLINK_GPIO, cache_led_state);
    // Toggle the LED state
    cache_led_state = !cache_led_state;
}

void led_delay(void) { vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS); }