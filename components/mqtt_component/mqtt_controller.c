/**
 * @file mqtt_controller.c
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief MQTT global controller
 * @version 0.1
 * @date 2025-04-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "mqtt_controller.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "custom_data_types.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

// General
static const char* TAG = "mqtt5";
static esp_mqtt_client_handle_t mqtt_client;
// Certificate file
extern const uint8_t _binary_cacert_pem_start[];
extern const uint8_t _binary_cacert_pem_end[];

esp_mqtt5_user_property_item_t user_property_arr[] = {
    {"board", "esp32"}, {"u", "user"}, {"p", "password"}};

const size_t user_property_arr_size =
    sizeof(user_property_arr) / sizeof(esp_mqtt5_user_property_item_t);

esp_mqtt5_disconnect_property_config_t disconnect_property = {
    .session_expiry_interval = 60,
    .disconnect_reason = 0,
};

static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .response_topic = RESPONSE_TOPIC,
    .correlation_data = "123456",
    .correlation_data_len = 6,
};

void log_error_if_nonzero(const char* message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void print_user_property(mqtt5_user_property_handle_t user_property) {
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            esp_mqtt5_user_property_item_t* item =
                malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item,
                                                   &count) == ESP_OK) {
                for (int i = 0; i < count; i++) {
                    esp_mqtt5_user_property_item_t* t = &item[i];
                    ESP_LOGI(TAG, "key is %s, value is %s", t->key, t->value);
                    free((char*)t->key);
                    free((char*)t->value);
                }
            }
            free(item);
        }
    }
}

static void mqtt5_app_start(mqtt5_event_handler_t mqtt5_event_handler) {
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = RESPONSE_TOPIC,
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .broker.address.port = 8883,
        .broker.verification.certificate =
            (const char*)_binary_cacert_pem_start,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = true,
        .credentials.username = "testuser",
        .credentials.authentication.password = "testpassword",
        .session.last_will.topic = DEFAULT_TOPIC,
        .session.last_will.msg = "Hello from ESP32",
        .session.last_will.msg_len = 16,
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt5_cfg);

    /* Set connection properties and user properties */
    esp_mqtt5_client_set_user_property(&connect_property.user_property,
                                       user_property_arr,
                                       user_property_arr_size);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property,
                                       user_property_arr,
                                       user_property_arr_size);
    esp_mqtt5_client_set_connect_property(mqtt_client, &connect_property);

    /* If you call esp_mqtt5_client_set_user_property to set user properties, DO
     * NOT forget to delete them. esp_mqtt5_client_set_connect_property will
     * malloc buffer to store the user_property and you can delete it after
     */
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    /* The last argument may be used to pass data to the event handler, in this
     * example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt5_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

esp_err_t mqtt_controller_init(mqtt5_event_handler_t mqtt5_event_handler) {
    esp_err_t result = ESP_OK;

    ESP_LOGI(TAG, "[mqtt5] Startup..");
    ESP_LOGI(TAG, "[mqtt5] Free memory: %" PRIu32 " bytes",
             esp_get_free_heap_size());
    ESP_LOGI(TAG, "[mqtt5] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    mqtt5_app_start(mqtt5_event_handler);

    return result;
}

void mqtt_controller_publish(char* data) {
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "cannot publish, client not initialized!");
        return;
    }
    esp_mqtt5_client_set_user_property(&publish_property.user_property,
                                       user_property_arr,
                                       user_property_arr_size);
    esp_mqtt5_client_set_publish_property(mqtt_client, &publish_property);
    int msg_id =
        esp_mqtt_client_publish(mqtt_client, DEFAULT_TOPIC, data, 0, 1, 1);
    esp_mqtt5_client_delete_user_property(publish_property.user_property);
    publish_property.user_property = NULL;
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}
