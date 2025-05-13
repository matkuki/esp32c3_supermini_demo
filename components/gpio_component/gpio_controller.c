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

#include "custom_data_types.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "led.h"
#include "sdkconfig.h"

// Constants
#define BUTTON_INPUT_GPIO CONFIG_BUTTON_INPUT
#define ESP_INTR_FLAG_DEFAULT 0
#define DEBOUNCE_TIME_US 300000  // 300ms

static const char* TAG = "MQTT-Controller";
static volatile int64_t last_isr_time = 0;
static QueueHandle_t gpio_event_queue = NULL;
static QueueHandle_t* general_event_queue_reference;

/**
 * @brief GPIO interrupt service handler
 *
 * @param arg GPIO pin number
 */
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

/**
 * @brief GPIO callback that fires when a GPIO change is detected
 *
 * @param arg
 */
static void gpio_task_callback(void* arg) {
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_event_queue, &io_num, portMAX_DELAY)) {
            // Current time in microseconds
            int64_t now = esp_timer_get_time();

            if (now - last_isr_time > DEBOUNCE_TIME_US) {
                last_isr_time = now;

                // Add a button event to the queue
                event_t new_event = EVENT_BUTTON_PRESS;
                xQueueSend(*general_event_queue_reference, &new_event,
                           portMAX_DELAY);
            }
        }
    }
}

void gpio_controller_init(QueueHandle_t* general_event_queue) {
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

    general_event_queue_reference = general_event_queue;

    // Create a queue to handle gpio event from isr
    gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO queue.\n");
        return;
    }

    // Start gpio task
    xTaskCreate(gpio_task_callback, "gpio_task_callback", 2048, NULL, 10, NULL);
}

int gpio_controller_get_button_state(void) {
    return gpio_get_level(BUTTON_INPUT_GPIO);
}