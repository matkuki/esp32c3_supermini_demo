/**
 * @file led.h
 * @author Matic Kukovec (https://github.com/matkuki)
 * @brief LED control for the ESP32-C3 SuperMini board
 * @version 0.1
 * @date 2025-04-11
 *
 */

#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LED GPIO
 *
 */
void led_initialize(void);

/**
 * @brief Toggle the state of the LED
 *
 */
void led_toggle(void);

/**
 * @brief Turn the LED on
 *
 */
void led_on(void);

/**
 * @brief Turn the LED off
 *
 */
void led_off(void);

/**
 * @brief Predefined delay function for the LED
 *
 */
void led_delay(void);

#ifdef __cplusplus
}
#endif

#endif  // LED_H
