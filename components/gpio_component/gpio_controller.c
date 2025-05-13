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
#define BUTTON_DEBOUNCE_TIME_MS 50  // 300ms
#define BUTTON_HOLD_TIME_MS 2000    // 2 seconds hold time

static const char* TAG = "MQTT-Controller";
static volatile int64_t last_isr_time = 0;
static QueueHandle_t gpio_event_queue = NULL;
static QueueHandle_t* general_event_queue_reference;

// Button states
typedef enum { BUTTON_IDLE, BUTTON_PRESSED, BUTTON_HELD } button_state_t;

static void gpio_controller_button_task(void* pvParameter) {
    button_state_t button_state = BUTTON_IDLE;
    bool press_flag = false;
    int64_t press_start_time = 0;

    while (1) {
        // Read button state (assuming active low)
        bool button_pressed = (gpio_controller_get_button_state() == 0);

        switch (button_state) {
            case BUTTON_IDLE:
                if (button_pressed) {
                    press_start_time = esp_timer_get_time();
                    button_state = BUTTON_PRESSED;
                }

                press_flag = false;
                break;

            case BUTTON_PRESSED:
                if (!button_pressed) {
                    // Button released before hold time
                    button_state = BUTTON_IDLE;

                    if (press_flag) {
                        // Add a button press event to the queue
                        event_t new_event = EVENT_BUTTON_PRESS;
                        xQueueSend(*general_event_queue_reference, &new_event,
                                   portMAX_DELAY);
                    }
                    press_flag = false;
                } else {
                    // Check if hold time has elapsed
                    int64_t elapsed_time =
                        (esp_timer_get_time() - press_start_time) /
                        1000;  // Convert to ms
                    if (elapsed_time >= BUTTON_DEBOUNCE_TIME_MS) {
                        press_flag = true;

                        if (elapsed_time >= BUTTON_HOLD_TIME_MS) {
                            press_flag = false;
                            button_state = BUTTON_HELD;

                            // Add a button hold event to the queue
                            event_t new_event = EVENT_BUTTON_HOLD;
                            xQueueSend(*general_event_queue_reference,
                                       &new_event, portMAX_DELAY);
                        }
                    }
                }
                break;

            case BUTTON_HELD:
                if (!button_pressed) {
                    // Button released after being held
                    button_state = BUTTON_IDLE;
                }
                break;
        }

        // Small delay to prevent CPU hogging
        vTaskDelay(pdMS_TO_TICKS(10));
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

    // Initialize reference to the main module's general queue
    general_event_queue_reference = general_event_queue;

    // Create a queue to handle gpio event from isr
    gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO queue.\n");
        return;
    }

    // Start gpio task
    xTaskCreate(gpio_controller_button_task, "gpio_controller_button_task",
                2048, NULL, 10, NULL);
}

int gpio_controller_get_button_state(void) {
    return gpio_get_level(BUTTON_INPUT_GPIO);
}