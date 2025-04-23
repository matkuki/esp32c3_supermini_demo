/**
 * @file i2c_controller.c
 * @author Matic Kukovec (https://github.com/matkuki/ExCo/)
 * @brief Global I2C controller
 * @version 0.1
 * @date 2025-04-12
 *
 */

#include "i2c_controller.h"

#include <stdio.h>
#include <string.h>

#include "i2c_chipcap2.h"
#include "sdkconfig.h"

#define SCL_IO_PIN CONFIG_I2C_MASTER_SCL
#define SDA_IO_PIN CONFIG_I2C_MASTER_SDA
#define MASTER_FREQUENCY CONFIG_I2C_MASTER_FREQUENCY
#define PORT_NUMBER -1

void i2c_controller_init(void) {
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = PORT_NUMBER,
        .scl_io_num = SCL_IO_PIN,
        .sda_io_num = SDA_IO_PIN,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t bus_handle;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    // ChipCap2 sensor initialization
    i2c_device_config_t i2c_chipcap2_dev_conf = {
        .scl_speed_hz = MASTER_FREQUENCY,
        .device_address = CC2_I2C_DEVICE_ADDRESS,
    };

    ESP_ERROR_CHECK(i2c_chipcap2_init(bus_handle, &i2c_chipcap2_dev_conf));
}