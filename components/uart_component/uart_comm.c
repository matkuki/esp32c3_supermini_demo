/**
 * @file uart_comm.c
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief UART serial communication helper
 * @version 0.1
 * @date 2025-04-11
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "uart_comm.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define TAG "UART_COMM_RS485"

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins
// using Kconfig.
#define ECHO_TEST_TXD (CONFIG_ECHO_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_ECHO_UART_RXD)

// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define ECHO_TEST_RTS (CONFIG_ECHO_UART_RTS)

// CTS is not used in RS485 Half-Duplex Mode
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (127)
#define BAUD_RATE (CONFIG_ECHO_UART_BAUD_RATE)

// Read packet timeout
#define PACKET_READ_TICS (100 / portTICK_PERIOD_MS)
#define ECHO_TASK_STACK_SIZE (CONFIG_ECHO_TASK_STACK_SIZE)
#define ECHO_TASK_PRIO (10)
#define ECHO_UART_PORT (CONFIG_ECHO_UART_PORT_NUM)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged
// state on receive pin
#define ECHO_READ_TOUT (3)  // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

void uart_comm_init(void) {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    ESP_ERROR_CHECK(
        uart_driver_install(ECHO_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0));

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT, &uart_config));

    ESP_LOGI(TAG, "UART set pins, mode and install driver.");

    // Set UART pins as per KConfig settings
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT, ECHO_TEST_TXD, ECHO_TEST_RXD,
                                 ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Set RS485 half duplex mode
    ESP_ERROR_CHECK(uart_set_mode(ECHO_UART_PORT, UART_MODE_RS485_HALF_DUPLEX));

    // Set read timeout of UART TOUT feature
    ESP_ERROR_CHECK(uart_set_rx_timeout(ECHO_UART_PORT, ECHO_READ_TOUT));
}

void uart_comm_send(const char* str, uint8_t length) {
    if (uart_write_bytes(ECHO_UART_PORT, str, length) != length) {
        ESP_LOGE(TAG, "Send data critical failure.");
        // Handle a failure
        abort();
    }
}

void uart_comm_vsend(const char* format, ...) {
    // Character buffer, you can increase the size if needed
    char buffer[128];

    // Initialize the variable arguments
    va_list args;
    // Initialize internal variable argument things
    va_start(args, format);

    // Format the string
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    // Zero the remaining unused bytes (if any)
    if (len > 0 && len < sizeof(buffer)) {
        memset(buffer + len, 0, sizeof(buffer) - len);
    }

    // Clean up internal variable argument things
    va_end(args);

    // Send the buffer over the UART
    uart_comm_send(buffer, len);
}