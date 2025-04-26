/**
 * @file wifi_controller.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief WiFi functionality
 * @version 0.1
 * @date 2025-04-17
 *
 */

#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi connection function with ESP provisioning support
 *
 */
esp_err_t wifi_controller_connect(bool reprovision_override);

/**
 * @brief Reprovision the Wifi connection, so that it can be provisioned over
 * BLE
 *
 */
void wifi_controller_reprovision(void);

#ifdef __cplusplus
}
#endif

#endif  // WIFI_CONTROLLER_H