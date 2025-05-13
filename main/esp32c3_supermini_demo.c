/**
 * @file i2c_humidity_main.c
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief WIFI, MQTT, I2C, UART and GPIO demo for the ESP32-C3-SUPERMINI
 * development board
 * @version 0.1
 * @date 2025-04-14
 *
 */
#include <stdio.h>
#include <string.h>

#include "cjson_component.h"
#include "custom_data_types.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_controller.h"
#include "i2c_chipcap2.h"
#include "i2c_controller.h"
#include "led.h"
#include "mqtt_controller.h"
#include "ota_controller.h"
#include "sdkconfig.h"
#include "uart_comm.h"
#include "wifi_controller.h"

// Functionality enabling/disabling macros
#define MQTT_ENABLED 1

// General
static const char* TAG = "matic's supermini demo";
static char message_buffer[200] = {0};
// Queues
static QueueHandle_t general_event_queue = NULL;
// Mutexes
SemaphoreHandle_t read_and_publish_mutex;
// Timers
TimerHandle_t read_publish_timer;
bool button_hold_flag = false;
// ChipCap2 sensor
static i2c_chipcap2_data_t chipcap2_out_data = {0};

int64_t start_time = 0;
int64_t end_time = 0;

/**
 * @brief Callback that fires when the timer elapses
 *
 * @param xTimer Timer handle to the timer that spawned the event
 */
static void timer_callback(TimerHandle_t xTimer) {
    event_t new_event = EVENT_TIMER_ELAPSED;
    xQueueSend(general_event_queue, &new_event, portMAX_DELAY);
}

/**
 * @brief Initialize all global timers
 *
 */
void timers_init(void) {
    // Create a software timer for read&publish ChipCap2 data every 5 seconds
    // (periodic, 5 seconds)
    read_publish_timer =
        xTimerCreate("ReadPublishTimer",   // Timer name (for debugging)
                     pdMS_TO_TICKS(5000),  // Timer period in ticks (5000 ms)
                     pdTRUE,               // Auto-reload (true = periodic)
                     NULL,                 // Optional timer ID
                     timer_callback        // Callback function
        );
    if (read_publish_timer != NULL) {
        // Start the timer (no delay before starting)
        if (xTimerStart(read_publish_timer, 0) != pdPASS) {
            uart_comm_vsend("Failed to start read&publish timer!\r\n");
        }
    } else {
        uart_comm_vsend("Failed to create read&publish timer!\r\n");
    }
}

/**
 * @brief Reads ChipCap2 sensor data through I2C and publishes it to the MQTT
 * broker as a JSON string
 *
 * @param message A message that is sent on the UART when the data is read and
 * published
 */
static void read_and_publish_sensor_data(const char* message) {
    if (xSemaphoreTake(read_and_publish_mutex, portMAX_DELAY) == pdTRUE) {
        led_toggle();
        esp_err_t result = ESP_OK;

        // Two read's are needed, the first one doesn't retrieve the humidity
        // data correctly!
        for (int i = 0; i < 2; i++) {
            result = i2c_chipcap2_read(&chipcap2_out_data);
        }

        if (result == ESP_OK) {
            result = cjson_format_chipcap2_data_prebuffered(
                &chipcap2_out_data, message_buffer, 200);
            if (result == ESP_OK) {
#if MQTT_ENABLED == 1
                mqtt_controller_publish(message_buffer);
#else
                uart_comm_vsend("MQTT not enabled, skipping publishing.\r\n");
#endif
                uart_comm_vsend("ChipCap2 JSON data:\r\n");
                uart_comm_vsend(message_buffer);
                uart_comm_vsend("\r\n");
            }
        } else {
            uart_comm_vsend(
                "[CHIPCAP2-ERROR] Something went wrong with the "
                "measurement!\r\n");
            return;
        }

        uart_comm_vsend(message);

        xSemaphoreGive(read_and_publish_mutex);
    }
}

/**
 * @brief Toggle LED 'blink_count' number of times
 *
 * @param blink_count The number of times the LED is toggled
 */
void blink(int blink_count) {
    if (blink_count < 0) {
        blink_count = 1;
    }

    led_off();
    for (int i = 0; i < blink_count; i++) {
        led_toggle();
        led_delay();
    }
    led_off();
}

/**
 * @brief Main program entry point function
 *
 */
void app_main(void) {
    // Initialize the UART communication
    uart_comm_init();
    uart_comm_vsend("UART COMM initialised.\r\n");

    // GPIO initialization
    uart_comm_vsend("Initialising GPIO ...\r\n");
    gpio_controller_init(&general_event_queue);
    uart_comm_vsend("GPIO initialised.\r\n");

    // Re-provision check
    bool reprovision_flag = false;
    if (gpio_controller_get_button_state() == GPIO_BUTTON_STATE_PRESSED) {
        // Wait for two se
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (gpio_controller_get_button_state() == GPIO_BUTTON_STATE_PRESSED) {
            reprovision_flag = true;
            for (int i = 0; i < 20; i++) {
                led_toggle();
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            uart_comm_vsend("Reprovisioning activated!\r\n");
        }
    }

    led_on();

    // Create general event queue
    uart_comm_vsend("Initialising queues ...\r\n");
    general_event_queue = xQueueCreate(10, sizeof(event_t));
    if (general_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create general queue.\n");
        return;
    }
    uart_comm_vsend("Queues initialised.\r\n");

    // I2C initialization
    uart_comm_vsend("Initialising I2C ...\r\n");
    i2c_controller_init();
    uart_comm_vsend("I2C initialised.\r\n");

    uart_comm_vsend("Initialising Wifi connection ...\r\n");
    ESP_ERROR_CHECK(wifi_controller_connect(reprovision_flag));
    uart_comm_vsend("Wifi connection initialised.\r\n");

// MQTT inizialization
#if MQTT_ENABLED == 1
    uart_comm_vsend("Initialising MQTT ...\r\n");
    ESP_ERROR_CHECK(mqtt_controller_init(&general_event_queue));
    uart_comm_vsend("MQTT initialised.\r\n");
#else
    uart_comm_vsend("MQTT not enabled, skipping initialization.\r\n");
#endif

    timers_init();

    // Initialize mutexes
    read_and_publish_mutex = xSemaphoreCreateMutex();
    if (read_and_publish_mutex == NULL) {
        uart_comm_vsend("Error initializing mutexes!\r\n");
        return;
    }

    led_off();

    // Visual signal for initialization completion
    blink(10);

    led_off();

    // Main event handling loop
    event_t event = EVENT_NONE;
    while (1) {
        // Get an item from the general queue and handle it accordingly
        if (xQueueReceive(general_event_queue, &event, portMAX_DELAY) ==
            pdPASS) {
            switch (event) {
                case EVENT_BUTTON_PRESS:
                    read_and_publish_sensor_data("[EVENT] BUTTON-PRESSED\r\n");
                    event = EVENT_NONE;
                    break;

                case EVENT_BUTTON_HOLD:
                    uart_comm_vsend("[EVENT] BUTTON-HOLD\r\n");
                    uart_comm_vsend("Reprovisioning the Wifi ...\r\n");
                    wifi_controller_reprovision();
                    uart_comm_vsend("Wifi provisioned again.\r\n");
                    event = EVENT_NONE;
                    break;

                case EVENT_MESSAGE_READ_AND_PUBLISH:
                    read_and_publish_sensor_data(
                        "[EVENT] MQTT-READ-AND-PUBLISH-RECEIVED\r\n");
                    end_time = esp_timer_get_time();
                    int64_t delta_us = (end_time - start_time) / 1000;
                    uart_comm_vsend("[TIMING] %lld ms\r\n", delta_us);
                    event = EVENT_NONE;
                    break;

                case EVENT_MESSAGE_UPDATE_FIRMWARE:
                    uart_comm_vsend(
                        "[EVENT] MQTT-UPDATE-FIRMWARE-RECEIVED\r\n");
                    ota_start();
                    event = EVENT_NONE;
                    break;

                case EVENT_TIMER_ELAPSED:
                    read_and_publish_sensor_data("[EVENT] TIMER-ELAPSED\r\n");
                    event = EVENT_NONE;
                    break;

                case EVENT_MQTT_CONNECTED:
                    uart_comm_vsend("[EVENT] MQTT-CONNECTED\r\n");
                    event = EVENT_NONE;
                    break;

                case EVENT_MQTT_DISCONNECTED:
#if MQTT_ENABLED == 1
                    uart_comm_vsend("[EVENT] MQTT-DISCONNECTED\r\n");
                    uart_comm_vsend("Re-initialising MQTT ...\r\n");
                    ESP_ERROR_CHECK(mqtt_controller_init(&general_event_queue));
                    uart_comm_vsend("MQTT re-initialised.\r\n");
#else
                    uart_comm_vsend(
                        "[EVENT] MQTT not enabled, skipping "
                        "re-initialization.\r\n");
#endif

                    blink(5);
                    event = EVENT_NONE;
                    break;

                default:
                    break;
            }
        }
    }
}
