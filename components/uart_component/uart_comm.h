/**
 * @file uart_comm.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief UART serial communication helper
 * @version 0.1
 * @date 2025-04-11
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART RS485 initialization
 *
 */
void uart_comm_init(void);

/**
 * @brief Send (transmit) a string over the UART module
 *
 * @param str String buffer to send over UART
 * @param length The length of the string buffer
 */
void uart_comm_send(const char* str, uint8_t length);

/**
 * @brief Send (transmit) a formatted string over the UART module (uses
 * vsnprintf internally)
 *
 */
void uart_comm_vsend(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif  // UART_COMM_H
