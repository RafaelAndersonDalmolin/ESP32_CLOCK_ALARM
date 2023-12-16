#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <FreeRTOSConfig.h>

#include "button.h"
#include "display_lcd_16x2.h"
#include "menu.h"
#include "date_time_manager.h"
#include "wifi_manager.h"


#include <time.h>
#include "esp_sntp.h"
#include "esp_log.h"



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

// Função de callback para lidar com a atualização do RTC externo
void sntp_time_sync_notification_cb(struct timeval *tv) {
    struct tm timeinfo;
    char strftime_buf[64];

    // Set timezone to Standard Time and print local time
    setenv("TZ", "BRT3", 1);
    tzset();
    localtime_r(&tv->tv_sec, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(MAIN, "The current date/time in Chapeco-SC is: %s", strftime_buf);
    // Aqui você terá que implementar a lógica para enviar a struct tm para o RTC externo
    // Substitua rtc_set_time com a função correta para atualizar o RTC externo
    ESP_ERROR_CHECK(date_time_manager_set_date_time(&timeinfo));

}

void initialize_sntp(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org"); // Servidor NTP a ser utilizado
    sntp_set_time_sync_notification_cb(sntp_time_sync_notification_cb); // Define a função de callback
    sntp_init();
}

int app_main() {
    
    starting_buttons();

    ESP_ERROR_CHECK(i2cdev_init());

    display_lcd_16x2_init();    

    ESP_ERROR_CHECK(date_time_manager_init());
    // date_time_manager_set_mode(); manual ou sntp
    // date_time_manager_start_alarm(EVERY_SECOND);

    /* start the wifi manager */
	wifi_manager_start();

    /* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
    //   
    initialize_sntp();

    vTaskDelay(20000 / portTICK_PERIOD_MS);

    menu_init();


    return 0;
}
