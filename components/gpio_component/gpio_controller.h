/**
 * @file gpio_controller.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief GPIO functionality
 * @version 0.1
 * @date 2025-04-12
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#define GPIO_BUTTON_STATE_PRESSED 0
#define GPIO_BUTTON_STATE_RELEASED 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the global GPIO functionality
 *
 */
void gpio_controller_init(QueueHandle_t* gpio_event_queue_reference);

/**
 * @brief
 *
 * @return int The state of the pin (0-pressed, 1-released)
 */
int gpio_controller_get_button_state(void);

#ifdef __cplusplus
}
#endif

#endif  // GPIO_CONTROLLER_H