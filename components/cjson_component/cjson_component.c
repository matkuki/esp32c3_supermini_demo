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

char *cjson_format_chipcap2_data_unfomatted(
    i2c_chipcap2_data_t *chipcap2_data) {
    cJSON *data = cJSON_CreateObject();
    cJSON *sensor_data = cJSON_AddArrayToObject(data, "sensor-data");
    cJSON *obj = NULL;

    // Humidity
    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    if (cJSON_AddNumberToObject(obj, "humidity",
                                chipcap2_data->humidity.value) == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }
    if (cJSON_AddStringToObject(obj, "unit", "%% (RH)") == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }
    cJSON_AddItemToArray(sensor_data, obj);

    // Temperature
    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    if (cJSON_AddNumberToObject(obj, "temperature",
                                chipcap2_data->temperature.value) == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }
    if (cJSON_AddStringToObject(obj, "unit", "°C") == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }
    cJSON_AddItemToArray(sensor_data, obj);

    char *string = cJSON_PrintUnformatted(data);
    if (string == NULL) {
        uart_comm_vsend("[CJSON-ERROR] Failed to create JSON string!\r\n");
    }

    cJSON_Delete(data);
    return string;
}

esp_err_t cjson_format_chipcap2_data_prebuffered(
    i2c_chipcap2_data_t *chipcap2_data, char *buffer, uint16_t buffer_lenght) {
    cJSON *data = cJSON_CreateObject();
    cJSON *sensor_data = cJSON_AddArrayToObject(data, "sensor-data");
    cJSON *obj = NULL;

    // Humidity
    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return ESP_FAIL;
    }
    if (cJSON_AddNumberToObject(obj, "humidity",
                                chipcap2_data->humidity.value) == NULL) {
        cJSON_Delete(obj);
        return ESP_FAIL;
    }
    if (cJSON_AddStringToObject(obj, "unit", "%% (RH)") == NULL) {
        cJSON_Delete(obj);
        return ESP_FAIL;
    }
    cJSON_AddItemToArray(sensor_data, obj);

    // Temperature
    obj = cJSON_CreateObject();
    if (obj == NULL) {
        return ESP_FAIL;
    }
    if (cJSON_AddNumberToObject(obj, "temperature",
                                chipcap2_data->temperature.value) == NULL) {
        cJSON_Delete(obj);
        return ESP_FAIL;
    }
    if (cJSON_AddStringToObject(obj, "unit", "°C") == NULL) {
        cJSON_Delete(obj);
        return ESP_FAIL;
    }
    cJSON_AddItemToArray(sensor_data, obj);

    cJSON_bool result =
        cJSON_PrintPreallocated(data, buffer, buffer_lenght, false);
    cJSON_Delete(data);

    if (result == false) {
        uart_comm_vsend("[CJSON-ERROR] Failed to create JSON string!\r\n");
        return ESP_FAIL;
    }

    return ESP_OK;
}