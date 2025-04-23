/**
 * @file i2c_controller.h
 * @author Matic Kukovec (https://github.com/matkuki/ExCo/)
 * @brief Global I2C controller
 * @version 0.1
 * @date 2025-04-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef I2C_CONTROLLER_H
#define I2C_CONTROLLER_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C initialization routine
 *
 */
void i2c_controller_init(void);

#ifdef __cplusplus
}
#endif

#endif  // I2C_CONTROLLER_H