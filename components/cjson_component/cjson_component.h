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

#ifdef __cplusplus
}
#endif

#endif  // CJSON_COMPONENT_H