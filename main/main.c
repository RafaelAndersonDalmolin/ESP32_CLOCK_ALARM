#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <stdio.h>
// #include <string.h>
#include <sys/time.h>

#include "menu.h"
#include "button.h"
// #include "RTC_Date_Time.h"
#include <FreeRTOSConfig.h>

#define BUTTON_VOLTAR    GPIO_NUM_35     //Voltar
#define BUTTON_MODO      GPIO_NUM_34     //Mode/Ok
#define BUTTON_BAIXO	 GPIO_NUM_39     //DOWM
#define BUTTON_CIMA 	 GPIO_NUM_36     //UP                                                                                               

#define I2C_MASTER_SDA  GPIO_NUM_21
#define I2C_MASTER_SCL  GPIO_NUM_22
#define GPIO_ALARM      GPIO_NUM_18


const char* BUTTON = "BUTTON";
const char* SCREEN = "SCREEN";
const char* MENU = "MENU";

TaskHandle_t taskToUpdateHandle;

void menu(void *params) {
    init_lcd();
    menu_option();
}

int app_main() {

    // second button voltar connected between GPIO and VCC
    // pressed logic level 1, no autorepeat, no press long
    gpio_num_t gpio = BUTTON_VOLTAR;                     //!< GPIO
    bool internal_resistors = false;                //!< Enable internal pull-up/pull-down
    uint8_t pressed_level = 1;                      //!< Logic level of pressed button
    bool autorepeat = false;                         //!< Enable autorepeat
    bool pressed_long = false;                      //!< Enable pressed long
    gpio_num_t shared_queue_gpio = -1;              //!< shared queue with GPIO

    if (button_install(gpio,internal_resistors,pressed_level,autorepeat,pressed_long,shared_queue_gpio) != ESP_OK){
        ESP_LOGI(BUTTON, "Erro ao iniciar BUTTON_VOLTAR!");
        return 1;
    }

    // third button modo/ok connected between GPIO and VCC
    // pressed logic level 1, autorepeat, no press long
    gpio = BUTTON_MODO;                                //!< GPIO
    internal_resistors = false;                     //!< Enable internal pull-up/pull-down
    pressed_level = 1;                              //!< Logic level of pressed button
    autorepeat = false;                             //!< Enable autorepeat
    pressed_long = true;                            //!< Enable pressed long
    shared_queue_gpio = BUTTON_VOLTAR;                   //!< shared queue with GPIO

    if (button_install(gpio,internal_resistors,pressed_level,autorepeat,pressed_long,shared_queue_gpio) != ESP_OK){
        ESP_LOGI(BUTTON, "Erro ao iniciar BUTTON_MODO!");
        return 1;
    }

    // fourth button donw connected between GPIO and VCC
    // pressed logic level 1, no autorepeat, press long
    gpio = BUTTON_BAIXO;                                //!< GPIO
    internal_resistors = false;                     //!< Enable internal pull-up/pull-down
    pressed_level = 1;                              //!< Logic level of pressed button
    autorepeat = true;                             //!< Enable autorepeat
    pressed_long = false;                            //!< Enable pressed long
    shared_queue_gpio = BUTTON_VOLTAR;                   //!< shared queue with GPIO

    if (button_install(gpio,internal_resistors,pressed_level,autorepeat,pressed_long,shared_queue_gpio) != ESP_OK){
        ESP_LOGI(BUTTON, "Erro ao iniciar BUTTON_BAIXO!");
        return 1;
    }
    
    // fifth button up connected between GPIO and VCC
    // pressed logic level 1, autorepeat, no press long
    gpio = BUTTON_CIMA;                                //!< GPIO
    internal_resistors = false;                     //!< Enable internal pull-up/pull-down
    pressed_level = 1;                              //!< Logic level of pressed button
    autorepeat = true;                             //!< Enable autorepeat
    pressed_long = false;                            //!< Enable pressed long
    shared_queue_gpio = BUTTON_VOLTAR;                   //!< shared queue with GPIO

    if (button_install(gpio,internal_resistors,pressed_level,autorepeat,pressed_long,shared_queue_gpio) != ESP_OK){
        ESP_LOGI(BUTTON, "Erro ao iniciar BUTTON_CIMA!");
        return 1;
    }

    ESP_ERROR_CHECK(i2cdev_init());

    if (xTaskCreate(menu, "menu", configMINIMAL_STACK_SIZE * 10, NULL, 4, NULL) != pdPASS) {
        ESP_LOGI("menu", "The task was not created!");
    }

    ESP_ERROR_CHECK(DS3231_DateTime_Manager_install(I2C_MASTER_SDA, I2C_MASTER_SCL, GPIO_ALARM, EVERY_SECOND));
    
    return 0;
}
