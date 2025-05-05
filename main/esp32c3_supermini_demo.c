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

#define DEBOUNCE_TIME_US 300000  // 300ms

// General
static const char* TAG = "matic's supermini demo";
static char message_buffer[200] = {0};
static volatile int64_t last_isr_time = 0;
// Queues
static QueueHandle_t general_event_queue = NULL;
static QueueHandle_t gpio_event_queue = NULL;
// Timers
TimerHandle_t read_publish_timer;
TimerHandle_t button_hold_timer;
bool button_hold_flag = false;
// ChipCap2 sensor
static i2c_chipcap2_data_t chipcap2_out_data = {0};

int64_t start_time;
int64_t end_time;

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

                // Reset the button hold flag, this cancels the hold, as it
                // detected a change in the button state during the check period
                button_hold_flag = false;

                // Add a button event to the queue
                event_t new_event = EVENT_BUTTON;
                xQueueSend(general_event_queue, &new_event, portMAX_DELAY);
            }
        }
    }
}

/**
 * @brief Callback that fires when the timer elapses
 *
 * @param xTimer Timer handle to the timer that spawned the event
 */
static void timer_callback(TimerHandle_t xTimer) {
    event_t new_event = EVENT_TIMER;
    xQueueSend(general_event_queue, &new_event, portMAX_DELAY);
}

/**
 * @brief Callback that fires when the timer elapses
 *
 * @param xTimer Timer handle to the timer that spawned the event
 */
static void button_check_callback(TimerHandle_t xTimer) {
    if ((gpio_controller_get_button_state() == GPIO_BUTTON_STATE_PRESSED) &&
        (button_hold_flag == true)) {
        event_t new_event = EVENT_BUTTON_HOLD;
        xQueueSend(general_event_queue, &new_event, portMAX_DELAY);
    }
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

    // Create a software timer for checking a button hold (one_shot)
    button_hold_timer =
        xTimerCreate("ButtonHoldTimer",     // Timer name (for debugging)
                     pdMS_TO_TICKS(2000),   // Timer period in ticks (2000 ms)
                     pdTRUE,                // Auto-reload (true = periodic)
                     NULL,                  // Optional timer ID
                     button_check_callback  // Callback function
        );
    if (read_publish_timer == NULL) {
        uart_comm_vsend("Failed to create button-hold timer!\r\n");
    }
}

// Start the timer (anywhere you want)
void timer_button_hold_start() {
    if (xTimerStart(button_hold_timer, 0) != pdPASS) {
        uart_comm_vsend("Failed to start button-hold timer!\n");
    }
    button_hold_flag = true;
}

// Stop/cancel the timer
void timer_button_hold_stop() {
    if (xTimerStop(button_hold_timer, 0) != pdPASS) {
        uart_comm_vsend("Failed to stop button-hold timer!\r\n");
    }
}

/**
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt5_event_handler(void* handler_args, esp_event_base_t base,
                                int32_t event_id, void* event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32,
             base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    event_t new_event;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32,
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            // Send a connection message to the general queue
            new_event = EVENT_MQTT_CONNECTED;
            xQueueSend(general_event_queue, &new_event, portMAX_DELAY);

            // Subscribe to the default topic to receive data
            esp_mqtt_client_subscribe(client, DEFAULT_TOPIC, 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

            // Send a disconnection message to the general queue
            new_event = EVENT_MQTT_DISCONNECTED;
            xQueueSend(general_event_queue, &new_event, portMAX_DELAY);

            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            esp_mqtt5_client_set_user_property(
                &disconnect_property.user_property, user_property_arr,
                user_property_arr_size);
            esp_mqtt5_client_set_disconnect_property(client,
                                                     &disconnect_property);
            esp_mqtt5_client_delete_user_property(
                disconnect_property.user_property);
            disconnect_property.user_property = NULL;
            esp_mqtt_client_disconnect(client);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            print_user_property(event->property->user_property);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            print_user_property(event->property->user_property);
            ESP_LOGI(TAG, "payload_format_indicator is %d",
                     event->property->payload_format_indicator);
            ESP_LOGI(TAG, "response_topic is %.*s",
                     event->property->response_topic_len,
                     event->property->response_topic);
            ESP_LOGI(TAG, "correlation_data is %.*s",
                     event->property->correlation_data_len,
                     event->property->correlation_data);
            ESP_LOGI(TAG, "content_type is %.*s",
                     event->property->content_type_len,
                     event->property->content_type);
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

            if (event->data_len == 16 &&
                strncmp(event->data, "read-and-publish", event->data_len) ==
                    0) {
                start_time = esp_timer_get_time();
                event_t new_event = EVENT_MESSAGE_READ_AND_PUBLISH;
                xQueueSend(general_event_queue, &new_event, portMAX_DELAY);
            } else if (event->data_len == 17 &&
                       strncmp(event->data, "update-firmware\r\n",
                               event->data_len) == 0) {
                event_t new_event = EVENT_MESSAGE_UPDATE_FIRMWARE;
                xQueueSend(general_event_queue, &new_event, portMAX_DELAY);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            print_user_property(event->property->user_property);
            ESP_LOGI(TAG, "MQTT5 return code is %d",
                     event->error_handle->connect_return_code);
            if (event->error_handle->error_type ==
                MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls",
                                     event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack",
                                     event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero(
                    "captured as transport's socket errno",
                    event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(
                    TAG, "Last errno string (%s)",
                    strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
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
    led_toggle();
    esp_err_t result = ESP_OK;

    // Two read's are needed, the first one doesn't retrieve the humidity data
    // correctly!
    for (int i = 0; i < 2; i++) {
        result = i2c_chipcap2_read(&chipcap2_out_data);
    }
    
    if (result == ESP_OK) {
        snprintf(message_buffer, sizeof(message_buffer),
                 "{'sensor-data': {'humidity': {'value': "
                 "%.2f}}, {'temperature': {'value': %.2f}}}",
                 chipcap2_out_data.humidity.value,
                 chipcap2_out_data.temperature.value);

        mqtt_controller_publish(message_buffer);
    } else {
        uart_comm_vsend(
            "[CHIPCAP2-ERROR] Something went wrong with the "
            "measurement!\r\n");
        return;
    }

    uart_comm_vsend(message);
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
    gpio_controller_init(gpio_isr_handler);
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

    // Create a queue to handle gpio event from isr
    gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO queue.\n");
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
    uart_comm_vsend("Initialising MQTT ...\r\n");
    ESP_ERROR_CHECK(mqtt_controller_init(mqtt5_event_handler));
    uart_comm_vsend("MQTT initialised.\r\n");

    timers_init();

    // Start gpio task
    xTaskCreate(gpio_task_callback, "gpio_task_callback", 2048, NULL, 10, NULL);

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
                case EVENT_BUTTON:
                    // Start the timer to detect a button press-and-hold
                    timer_button_hold_start();

                    read_and_publish_sensor_data("[EVENT] BUTTON-CLICKED\r\n");
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
                    uart_comm_vsend("[TIMING] %lld ms", delta_us);
                    event = EVENT_NONE;
                    break;

                case EVENT_MESSAGE_UPDATE_FIRMWARE:
                    uart_comm_vsend(
                        "[EVENT] MQTT-UPDATE-FIRMWARE-RECEIVED\r\n");
                    ota_start();
                    event = EVENT_NONE;
                    break;

                case EVENT_TIMER:
                    read_and_publish_sensor_data("[EVENT] TIMER-ELAPSED\r\n");
                    event = EVENT_NONE;
                    break;

                case EVENT_MQTT_CONNECTED:
                    uart_comm_vsend("[EVENT] MQTT-CONNECTED\r\n");
                    event = EVENT_NONE;
                    break;

                case EVENT_MQTT_DISCONNECTED:
                    uart_comm_vsend("[EVENT] MQTT-DISCONNECTED\r\n");
                    uart_comm_vsend("Re-nitialising MQTT ...\r\n");
                    ESP_ERROR_CHECK(mqtt_controller_init(mqtt5_event_handler));
                    uart_comm_vsend("MQTT re-initialised.\r\n");
                    blink(5);
                    event = EVENT_NONE;
                    break;

                default:
                    break;
            }
        }
    }
}
