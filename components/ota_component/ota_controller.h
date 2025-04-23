/**
 * @file ota_controller.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief OTA (Over-The-Air) updating controller
 * @version 0.1
 * @date 2025-04-14
 *
 */

#ifndef OTA_CONTROLLER_H
#define OTA_CONTROLLER_H

#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA (Over-The-Air) updating initialization
 * 
 */
esp_err_t ota_start(void);

#ifdef __cplusplus
}
#endif

#endif  // OTA_CONTROLLER_H