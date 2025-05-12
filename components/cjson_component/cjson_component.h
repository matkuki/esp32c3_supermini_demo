/**
 * @file cjson_component.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief Wrapper functionality around the cJSON library for easier use
 * @version 0.1
 * @date 2025-05-12
 *
 */

#ifndef CJSON_COMPONENT_H
#define CJSON_COMPONENT_H

#include "i2c_chipcap2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Format a JSON string with ChipCap2 sensor data
 *
 * @param chipcap2_data ChipCap2 sensor data refeence
 *
 */
char *cjson_format_chipcap2_data(i2c_chipcap2_data_t *chipcap2_data);

/**
 * @brief Format a JSON string with ChipCap2 sensor data using a pre-buffer
 *
 * @param chipcap2_data ChipCap2 sensor data refeence
 * @param buffer Buffer for holding the generated JSON string
 * @param buffer_lenght Size of the pre-buffer that will hold the generated JSON
 * string
 * @return esp_err_t
 */
esp_err_t cjson_format_chipcap2_data_prebuffered(
    i2c_chipcap2_data_t *chipcap2_data, char *buffer, uint16_t buffer_lenght);

#ifdef __cplusplus
}
#endif

#endif  // CJSON_COMPONENT_H