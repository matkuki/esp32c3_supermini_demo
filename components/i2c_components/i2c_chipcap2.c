/**
 * @file i2c_chipcap2.c
 * @author @author Matic Kukovec (https://github.com/matkuki/ExCo/)
 * @brief ChipCap2 I2C temperature and humidity sensor (implementation)
 * @version 0.1
 * @date 2025-04-10
 *
 */

#include "i2c_chipcap2.h"

#include <math.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char TAG[] = "i2c-chipcap2";
static i2c_chipcap2_handle_t chipcap2_handle;
static uint8_t chipcap2_read_buffer[4] = {0};
static i2c_chipcap2_data_t chipcap2_data = {0};

/**
 * @brief Helper for reseting the ChipCap2 measurement data
 *
 */
static void i2c_chipcap2_reset_data(void) {
    chipcap2_data.humidity.value = 0.0;
    chipcap2_data.humidity.high_byte = 0;
    chipcap2_data.humidity.low_byte = 0;
    chipcap2_data.temperature.value = 0.0;
    chipcap2_data.temperature.high_byte = 0;
    chipcap2_data.temperature.low_byte = 0;
}

esp_err_t i2c_chipcap2_init(i2c_master_bus_handle_t bus_handle,
                            const i2c_device_config_t* i2c_config) {
    esp_err_t ret = ESP_OK;
    chipcap2_handle =
        (i2c_chipcap2_handle_t)calloc(1, sizeof(*chipcap2_handle));
    ESP_GOTO_ON_FALSE(chipcap2_handle, ESP_ERR_NO_MEM, err, TAG,
                      "no memory for i2c eeprom device");

    if (chipcap2_handle->i2c_dev == NULL) {
        ESP_GOTO_ON_ERROR(i2c_master_bus_add_device(bus_handle, i2c_config,
                                                    &chipcap2_handle->i2c_dev),
                          err, TAG, "i2c new bus failed");
    }

    return ESP_OK;

err:
    if (chipcap2_handle && chipcap2_handle->i2c_dev) {
        i2c_master_bus_rm_device(chipcap2_handle->i2c_dev);
    }
    free(chipcap2_handle);
    return ret;
}

esp_err_t i2c_chipcap2_measurement_request(void) {
    ESP_RETURN_ON_FALSE(chipcap2_handle, ESP_ERR_NO_MEM, TAG,
                        "no mem for buffer");
    const uint8_t buffer[1] = {0};
    return i2c_master_transmit(chipcap2_handle->i2c_dev, buffer, 1, -1);
}

esp_err_t i2c_chipcap2_data_fetch(uint8_t* buffer) {
    ESP_RETURN_ON_FALSE(chipcap2_handle, ESP_ERR_NO_MEM, TAG,
                        "no mem for buffer");

    // Reset the ChipCap2 read buffer
    for (int i = 0; i < 4; i++) {
        chipcap2_read_buffer[i] = 0;
    }

    return i2c_master_receive(chipcap2_handle->i2c_dev, buffer, sizeof(buffer),
                              -1);
}

esp_err_t i2c_chipcap2_read(i2c_chipcap2_data_t* out_data) {
    esp_err_t result = ESP_OK;

    // Do a ChipCap2 measurement
    result = i2c_chipcap2_measurement_request();
    if (result != ESP_OK) {
        return result;
    }

    // Measurement delay
    vTaskDelay(30 / portTICK_PERIOD_MS);

    // Fetch both the humidity and temperature data
    result = i2c_chipcap2_data_fetch(chipcap2_read_buffer);
    if (result != ESP_OK) {
        return result;
    }

    i2c_chipcap2_reset_data();

    unsigned char rh_byte_1 = chipcap2_read_buffer[0];
    unsigned char rh_byte_2 = chipcap2_read_buffer[1];
    unsigned char temp_byte_1 = chipcap2_read_buffer[2];
    unsigned char temp_byte_2 = chipcap2_read_buffer[3];

    // The upper two bits of the first result byte are STATUS BITS
    rh_byte_1 &= 0b00111111;
    // Humidity calculated according to the datasheet
    chipcap2_data.humidity.value =
        ((((float)(rh_byte_1 * 256) + (float)rh_byte_2) / 16384.0) * 100.0);
    // Store the bytes
    chipcap2_data.humidity.high_byte = rh_byte_1;
    chipcap2_data.humidity.low_byte = rh_byte_2;

    // Lower two bits of the second temperature byte are unused
    temp_byte_2 &= 0b11111100;
    // Temperature calculated according to the datasheet
    chipcap2_data.temperature.value =
        ((((float)(temp_byte_1 * 64) + ((float)temp_byte_2 / 4)) / 16384.0) *
         165.0) -
        40;
    // Store the bytes
    chipcap2_data.temperature.high_byte = temp_byte_1;
    chipcap2_data.temperature.low_byte = temp_byte_2;

    // Copy data to the caller
    *out_data = chipcap2_data;

    return result;
}
