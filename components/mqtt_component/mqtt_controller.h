/**
 * @file mqtt_controller.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief MQTT global controller
 * @version 0.1
 * @date 2025-04-14
 *
 */

#ifndef MQTT_CONTROLLER_H
#define MQTT_CONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "mqtt_client.h"

typedef void (*mqtt5_event_handler_t)(void *handler_args, esp_event_base_t base,
                                      int32_t event_id, void *event_data);

extern esp_mqtt5_user_property_item_t user_property_arr[];
extern const size_t user_property_arr_size;
extern esp_mqtt5_disconnect_property_config_t disconnect_property;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initiazize MQTT client (and also the WiFi connection)
 *
 */
esp_err_t mqtt_controller_init(mqtt5_event_handler_t mqtt5_event_handler);
void log_error_if_nonzero(const char *message, int error_code);
void print_user_property(mqtt5_user_property_handle_t user_property);

/**
 * @brief Publish a message to the MQTT broker
 *
 */
void mqtt_controller_publish(char *data);

#ifdef __cplusplus
}
#endif

#endif  // MQTT_CONTROLLER_H