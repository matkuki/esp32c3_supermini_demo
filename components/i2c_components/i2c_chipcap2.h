/**
 * @file i2c_chipcap2.h
 * @author Matic Kukovec (https://github.com/matkuki/ExCo/)
 * @brief ChipCap2 I2C temperature and humidity sensor (header)
 * @version 0.1
 * @date 2025-04-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef I2C_CHIPCAP2_H
#define I2C_CHIPCAP2_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ChipCap2 constants
 */
#define COMBINE(a, b) (((a) << 1) | (b))
#define CC2_I2C_DEVICE_ADDRESS 0b0101000
#define CC2_I2C_DATA_FETCH COMBINE(CC2_I2C_DEVICE_ADDRESS, 0b1)
#define CC2_I2C_MEASUREMENT_REQ COMBINE(CC2_I2C_DEVICE_ADDRESS, 0b0)

/**
 * @brief Base struct for storing humidity/temperature data
 */
typedef struct {
    float value;
    unsigned char high_byte;
    unsigned char low_byte;
} i2c_chipcap2_mixed_number_t;

/**
 * @brief ChipCap2 device struct
 */
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} i2c_chipcap2_t;
typedef i2c_chipcap2_t *i2c_chipcap2_handle_t;

/**
 * @brief ChipCap2 struct for holding measurement data
 */
typedef struct {
    i2c_chipcap2_mixed_number_t humidity;
    i2c_chipcap2_mixed_number_t temperature;
} i2c_chipcap2_data_t;

/**
 * @brief ChipCap2 sensor initialization
 *
 * @param bus_handle I2C master bus handle
 * @param i2c_device I2C device configuration
 * @param chipcap2_handle ChipCap2 device handle
 * @return esp_err_t
 */
esp_err_t i2c_chipcap2_init(i2c_master_bus_handle_t bus_handle,
                            const i2c_device_config_t *i2c_device);

/**
 * @brief Send
 *
 * @param chipcap2_handle ChipCap2 device handle
 * @return esp_err_t
 */
esp_err_t i2c_chipcap2_measurement_request(void);

/**
 * @brief Send a Data-Fetch command to the ChipCap2 sensor and read back the
 * data to a buffer
 *
 * @param chipcap2_handle ChipCap2 device handle
 * @param buffer An array buffer of 4 bytes for the read data
 * @return esp_err_t
 */
esp_err_t i2c_chipcap2_data_fetch(uint8_t *buffer);

/**
 * @brief Read measurement data
 *
 * @param out_data Pointer to ChipCap2 measurement data
 * @return esp_err_t
 */
esp_err_t i2c_chipcap2_read(i2c_chipcap2_data_t *out_data);

#ifdef __cplusplus
}
#endif

#endif  // I2C_CHIPCAP2_H
