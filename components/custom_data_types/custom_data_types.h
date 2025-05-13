/**
 * @file custom_data_types.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief A module to hold custom data types
 * @version 0.1
 * @date 2025-04-14
 *
 */

#ifndef CUSTOM_DATA_TYPES_H
#define CUSTOM_DATA_TYPES_H

#define DEFAULT_TOPIC "/matic_esp32c3/testing"
#define RESPONSE_TOPIC "/matic_esp32c3/testing/response"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Event enumeration for use with the general queue
 *
 */
typedef enum {
    EVENT_NONE,
    EVENT_BUTTON_PRESS,
    EVENT_TIMER_ELAPSED,
    EVENT_MESSAGE_READ_AND_PUBLISH,
    EVENT_MESSAGE_UPDATE_FIRMWARE,
    EVENT_MQTT_CONNECTED,
    EVENT_MQTT_DISCONNECTED,
    EVENT_BUTTON_HOLD
} event_t;

extern int64_t start_time;

#ifdef __cplusplus
}
#endif

#endif  // CUSTOM_DATA_TYPES_H