#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Button states
 */
typedef enum {
    BUTTON_RELEASED = 0,   /**< Button released state. */
    BUTTON_PRESSED          /**< Button pressed state. */
} ButtonState;

/**
 * @brief Button actions
 */
typedef enum {
    BUTTON_CLICKED = 2,     /**< Button clicked action. */
    BUTTON_PRESSED_LONG     /**< Button pressed long action. */
} ButtonAction;

/**
 * @brief Notification descriptor struct
 */
typedef struct {
    gpio_num_t button_num;    /**< Button number identifier. */
    ButtonAction action;        /**< Button action, see ::ButtonAction. */
} ButtonNotification;

/**
 * @brief Button descriptor struct
 */
typedef struct {
    gpio_num_t gpio;                    /**< GPIO pin associated with the button. */
    uint8_t pressed_level;              /**< Logic level of a pressed button. */
    bool autorepeat;                    /**< Enable autorepeat feature. */
    bool pressed_long;                  /**< Enable pressed long feature. */
    gpio_num_t shared_queue_gpio;       /**< GPIO pin for shared queue. Use -1 to create an exclusive queue. */
    bool free;                          /**< Flag indicating if the struct is free for use. */

    struct {
        ButtonState state;                        /**< Internal button state. */
        uint64_t pressed_time;                    /**< Time when the button was pressed. */
        uint64_t repeating_time;                  /**< Time for autorepeat interval. */
        QueueHandle_t button_action_queue;         /**< Actions queue for button. */
        bool shared_queue;                        /**< Flag indicating if the queue is shared to avoid race conditions. */
        SemaphoreHandle_t mutex;                  /**< Mutex to ensure safe access to the queue. */
        TimerHandle_t timer;                      /**< Timer for exclusive use by the button. */
    } internal;  
} ButtonConfig;


/**
 * @brief Install button
 *
 * @param gpio GPIO pin associated with the button
 * @param internal_resistors Whether to use internal pull-up/pull-down resistors
 * @param pressed_level Logic level of a pressed button
 * @param autorepeat Enable autorepeat feature
 * @param pressed_long Enable pressed long feature
 * @param shared_queue_gpio GPIO pin for shared queue. Use -1 to create an exclusive queue.
 * @return `ESP_OK` on success
 */
esp_err_t button_install(gpio_num_t gpio, bool internal_resistors, uint8_t pressed_level, bool autorepeat, bool pressed_long, gpio_num_t shared_queue_gpio);

/**
 * @brief Uninstall button
 *
 * @param gpio GPIO pin associated with the button
 * @return `ESP_OK` on success
 */
esp_err_t button_uninstall(gpio_num_t gpio);

/**
 * @brief Find the queue associated with a GPIO pin
 *
 * @param gpio GPIO pin to find the queue for
 * @return The queue handle if found, or NULL if not found
 */
QueueHandle_t find_queue(gpio_num_t gpio);

#ifdef __cplusplus
}
#endif

#endif /* __COMPONENTS_BUTTON_H__ */