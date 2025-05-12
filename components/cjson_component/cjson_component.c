/**
 * @file cjson_component.c
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief Wrapper functionality around the cJSON library for easier use
 * @version 0.1
 * @date 2025-05-12
 *
 */

#include "cjson_component.h"

#include <stddef.h>

#include "cjson.h"
#include "uart_comm.h"

/**
 * @brief Helper function to create a JSON object and add a number
 *
 */
static cJSON *cJSON_CreateObject_AddNumber(const char *name, double number) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    if (cJSON_AddNumberToObject(obj, name, number) == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }

    return obj;
}

/**
 * @brief Helper function to create a JSON object and add a string
 *
 */
static cJSON *cJSON_CreateObject_AddString(const char *name,
                                           const char *string) {
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }

    if (cJSON_AddStringToObject(obj, name, string) == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }

    return obj;
}

char *cjson_format_chipcap2_data(i2c_chipcap2_data_t *chipcap2_data) {
    cJSON *data = cJSON_CreateObject();
    cJSON *sensor_data = cJSON_AddArrayToObject(data, "sensor-data");

    // Humidity
    cJSON_AddItemToArray(sensor_data,
                         cJSON_CreateObject_AddNumber(
                             "humidity", chipcap2_data->humidity.value));
    cJSON_AddItemToArray(sensor_data,
                         cJSON_CreateObject_AddString("unit", "%% (RH)"));

    // Temperature
    cJSON_AddItemToArray(sensor_data,
                         cJSON_CreateObject_AddNumber(
                             "temperature", chipcap2_data->temperature.value));
    cJSON_AddItemToArray(sensor_data,
                         cJSON_CreateObject_AddString("unit", "°C"));

    char *string = cJSON_PrintUnformatted(data);
    if (string == NULL) {
        uart_comm_vsend("[CJSON-ERROR] Failed to create JSON string!\r\n");
    }

    cJSON_Delete(data);
    return string;
}