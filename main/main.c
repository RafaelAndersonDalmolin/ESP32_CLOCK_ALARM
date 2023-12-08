#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <FreeRTOSConfig.h>

#include "button.h"
#include "display_lcd_16x2.h"
#include "menu.h"
#include "RTC_Date_Time.h"
#include "wifi_manager.h"

/* @brief tag used for ESP serial console messages */
static const char* MAIN= "MAIN";

void monitoring_task(void *pvParameter)
{
	for(;;){
		ESP_LOGI(MAIN, "free heap: %d",esp_get_free_heap_size());
		vTaskDelay( pdMS_TO_TICKS(1000) );
	}
}

void starting_buttons(){
    esp_err_t err = ESP_FAIL;
    uint8_t count = 0;

    while((err != ESP_OK) && (count < 5)){
        // first button, WILDCARD button connected between GPIO and GND, (PULL DOWN)
        err = button_install(CONFIG_BUTTON_ACTIVATE_DEACTIVATE,false,0,false,false,-1);
        if( err != ESP_OK){
            ESP_LOGI(MAIN, "error installing button %d!",CONFIG_BUTTON_ACTIVATE_DEACTIVATE);
            count++;
            continue;
        }

        // second button, BACK button connected between GPIO and GND, (PULL DOWN)
        err = button_install(CONFIG_BUTTON_BACK,false,0,false,false,CONFIG_BUTTON_ACTIVATE_DEACTIVATE);
        if( err != ESP_OK){
            ESP_LOGI(MAIN, "error installing button %d!",CONFIG_BUTTON_BACK);
            count++;
            continue;
        }

        // third button, MODE button connected between GPIO and GND, (PULL DOWN)
        err = button_install(CONFIG_BUTTON_MODE,false,0,false,true,CONFIG_BUTTON_ACTIVATE_DEACTIVATE);
        if( err != ESP_OK){
            ESP_LOGI(MAIN, "error installing button %d!",CONFIG_BUTTON_MODE);
            count++;
            continue;
        }

        // fourth button, BACK button connected between GPIO and GND, (PULL DOWN)
        err = button_install(CONFIG_BUTTON_DOWN,false,0,true,false,CONFIG_BUTTON_ACTIVATE_DEACTIVATE);
        if( err != ESP_OK){
            ESP_LOGI(MAIN, "error installing button %d!",CONFIG_BUTTON_DOWN);
            count++;
            continue;
        }

        // fifth button, BACK button connected between GPIO and GND, (PULL DOWN)
        err = button_install(CONFIG_BUTTON_UP,false,0,true,false,CONFIG_BUTTON_ACTIVATE_DEACTIVATE);
        if( err != ESP_OK){
            ESP_LOGI(MAIN, "error installing button %d!",CONFIG_BUTTON_UP);
            count++;
            continue;
        }
    }
}

int app_main() {
    
    starting_buttons();

    ESP_ERROR_CHECK(i2cdev_init());

    display_lcd_16x2_init();    

    menu_init();

    ESP_ERROR_CHECK(DS3231_DateTime_Manager_install(EVERY_SECOND));

    /* start the wifi manager */
	wifi_manager_start();

    /* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);

    return 0;
}
