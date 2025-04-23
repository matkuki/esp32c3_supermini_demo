/**
 * @file gpio_controller.c
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief GPIO functionality
 * @version 0.1
 * @date 2025-04-12
 *
 */

#include "gpio_controller.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "led.h"
#include "sdkconfig.h"

#define BUTTON_INPUT_GPIO CONFIG_BUTTON_INPUT
#define ESP_INTR_FLAG_DEFAULT 0

void gpio_controller_init(void (*gpio_isr_handler)(void*)) {
    // Configure the peripheral according to the LED type
    led_initialize();

    // Configure the button GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_INPUT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Use internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_conf);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(BUTTON_INPUT_GPIO, gpio_isr_handler,
                         (void*)BUTTON_INPUT_GPIO);
}

int gpio_controller_get_button_state(void) {
    return gpio_get_level(BUTTON_INPUT_GPIO);
}