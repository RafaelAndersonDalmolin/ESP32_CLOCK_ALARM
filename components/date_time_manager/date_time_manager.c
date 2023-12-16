#include "date_time_manager.h"

#define ESP_INTR_FLAG_DEFAULT      0   /**< Default flag for GPIO interrupt configuration. */

/* @brief tag used for ESP serial console messages */
static const char* DTMANAGER= "date time manager";

//Variavel para armazenamento atual da data e hora 
static struct tm CurrentDateTime = {
    .tm_sec = 00, //representa os segundos de 0 a 59
    .tm_min = 00, //representa os minutos de 0 a 59
    .tm_hour = 00, //representa as horas de 0 a 23
    .tm_wday = 0, //dia da semana de 0 (domingo) até 6 (sábado)
    .tm_mday = 1, //dia do mês de 1 a 31
    .tm_mon = 0, //representa os meses do ano de 0 a 11
    .tm_year = 100, //representa o ano a partir de 1900
    .tm_yday = 1, // dia do ano de 1 a 365
    .tm_isdst = 0 //indica horário de verão se for diferente de zero
};

//configuration DateTime Manager
static SemaphoreHandle_t dateTimeMutex = NULL; //!< Device mutex dateTime 
static TimerHandle_t Timer_Alarm = NULL;       //!< timer para debouncing
static update_method_t update_method = MANUAL_USER;
static bool installed = false;

//config device ds3231 I2C
static i2c_dev_t ds3231;

//task identifiers
static TaskHandle_t *taskHandle[CONFIG_SIZE_IDENTIFIERS];
static short int size_taskHandle = 0;    

void IRAM_ATTR date_time_manager_isr_alarm(void* arg){
    //desabilitar interrupcao do pino
    esp_err_t err = gpio_isr_handler_remove((gpio_num_t) arg);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* Verifica se o semáforo acordou uma tarefa de maior prioridade que a tarefa sendo executada atualmente */ 
    if( xTimerStartFromISR(Timer_Alarm, &xHigherPriorityTaskWoken ) == pdPASS ){
        /* The start command was not executed successfully.  Take appropriate
        action here. */
        //portYIELD();
    }
}

void date_time_manager_cb_timer_alarm(TimerHandle_t xTimer){

    esp_err_t err = ESP_OK;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    gpio_num_t gpio_alarm = (gpio_num_t) pvTimerGetTimerID(xTimer);
    
    if(gpio_get_level(gpio_alarm) == 0){
      
        if (xSemaphoreTake(dateTimeMutex, portMAX_DELAY)){
            err = ds3231_get_time(&ds3231, &CurrentDateTime);
            if (err != ESP_OK){
                ESP_LOGI(DTMANAGER, "Could not get CurrentDateTime!");
            }
            xSemaphoreGive(dateTimeMutex);
            
            if((err == ESP_OK) && (size_taskHandle != 0)){
                for (int i = 0; i < CONFIG_SIZE_IDENTIFIERS; i++) {
                    if(taskHandle[i] != NULL){
                        // xTaskNotifyGive(*(taskHandle[i]));
                        vTaskNotifyGiveFromISR(*(taskHandle[i]), &xHigherPriorityTaskWoken);                        
                    }
                }
            }
        }
        //portYIELD();
    }
    err = ds3231_clear_alarm_flags(&ds3231, DS3231_ALARM_BOTH);
    err = gpio_isr_handler_add(gpio_alarm, date_time_manager_isr_alarm, (void*) gpio_alarm);
}

esp_err_t date_time_manager_set_date_time(struct tm *dateTime){
    return ds3231_set_time(&ds3231, dateTime);
}

esp_err_t date_time_manager_get_date_time(struct tm *dateTime){
    if (xSemaphoreTake(dateTimeMutex, portMAX_DELAY)) {
        dateTime->tm_sec =  CurrentDateTime.tm_sec; //representa os segundos de 0 a 59
        dateTime->tm_min = CurrentDateTime.tm_min;  //representa os minutos de 0 a 59
        dateTime->tm_hour = CurrentDateTime.tm_hour; //representa as horas de 0 a 23
        dateTime->tm_wday = CurrentDateTime.tm_wday;  //dia da semana de 0 (domingo) até 6 (sábado)
        dateTime->tm_mday = CurrentDateTime.tm_mday;  //dia do mês de 1 a 31
        dateTime->tm_mon = CurrentDateTime.tm_mon;  //representa os meses do ano de 0 a 11
        dateTime->tm_year = CurrentDateTime.tm_year; //representa o ano a partir de 1900
        dateTime->tm_yday = CurrentDateTime.tm_yday;  // dia do ano de 1 a 365
        dateTime->tm_isdst = CurrentDateTime.tm_isdst; //indica horário de verão se for diferente de zero
        xSemaphoreGive(dateTimeMutex);
    }
    return ESP_OK;
}

esp_err_t date_time_manager_task_notify_add(TaskHandle_t *taskToUpdateHandle){
    
    for (int i = 0; i < CONFIG_SIZE_IDENTIFIERS; i++) {
        if(taskHandle[i] == NULL){
            taskHandle[i] = taskToUpdateHandle;
            size_taskHandle ++;
            return ESP_OK;
        }
    }
    printf("falha ao add taskHandle_t\n");
    return ESP_FAIL;
}

esp_err_t date_time_manager_task_notify_remove(TaskHandle_t *taskToUpdateHandle){
    
    for (int i = 0; i < CONFIG_SIZE_IDENTIFIERS; i++) {
        printf("taskHandle_t for\n");
        if(taskHandle[i] != NULL){
            if(taskHandle[i] == taskToUpdateHandle){
                taskHandle[i] = NULL;
                size_taskHandle --;
                printf("taskHandle_t remove\n");
                return ESP_OK;
            }
        }
    }
    printf("falha ao remove taskHandle_t\n");
    return ESP_FAIL;
}

esp_err_t init_config_ds3231(){
    //inclui zeros na regiao de memoria da variavel do device i2c
        memset(&ds3231, 0, sizeof(i2c_dev_t));
        //inicia descritor do RTC ds3231
        return ds3231_init_desc(&ds3231);
}

static esp_err_t config_gpio_alarm(gpio_num_t gpio){
    
    esp_err_t err = ESP_OK;
    uint64_t pin_bit_mask = (1ULL<<gpio);
    gpio_mode_t mode = GPIO_MODE_INPUT;
    gpio_int_type_t intr_type = GPIO_INTR_NEGEDGE; 
    
    gpio_pullup_t pull_up = GPIO_PULLUP_ENABLE;
    gpio_pulldown_t pull_down = GPIO_PULLDOWN_DISABLE;

    gpio_config_t io_conf;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = pin_bit_mask;
    // set input
    io_conf.mode = mode;
    //GPIO interrupt type
    io_conf.intr_type = intr_type;
    //GPIO pull-up
    io_conf.pull_up_en = pull_up;
    //GPIO pull-down
    io_conf.pull_down_en = pull_down;

    //configure GPIO with the given settings
    err = gpio_config(&io_conf);
    if(err!= ESP_OK){
        ESP_LOGI(DTMANAGER, "invalid argument for alarm interrupt pin!");
        return err;
    }

    err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if(err!= ESP_OK){
        if(err == ESP_ERR_INVALID_STATE){
            ESP_LOGI(DTMANAGER, "interrupt service already installed!");
        }
        else{
            ESP_LOGI(DTMANAGER, "interrupt service was not installed!");
            return err;
        }  
    }

    err = gpio_isr_handler_add(CONFIG_GPIO_ALARM, date_time_manager_isr_alarm, (void*) CONFIG_GPIO_ALARM);
    if(err!= ESP_OK){
        ESP_LOGI(DTMANAGER, "invalid argument for handler add pin!");
        return err;
    }

    return err;
}

esp_err_t date_time_manager_start_alarm(alarm_rate_t alarm_rate){

    esp_err_t err = config_gpio_alarm(CONFIG_GPIO_ALARM);
    //configura pino para interrupcao gerada pelo alarme
    if(err != ESP_OK){
        ESP_LOGI(DTMANAGER, "invalid argument for alarm interrupt pin!");
        return ESP_ERR_INVALID_ARG;
    }

    //configura alarme periodico por segundo
    if(alarm_rate == EVERY_SECOND){
        err = ds3231_disable_alarm_ints(&ds3231, DS3231_ALARM_2);
        err = ds3231_set_alarm(&ds3231, DS3231_ALARM_1, &CurrentDateTime, DS3231_ALARM1_EVERY_SECOND, 0, 0);
        err = ds3231_clear_alarm_flags(&ds3231, DS3231_ALARM_BOTH);
        err = ds3231_enable_alarm_ints(&ds3231, DS3231_ALARM_1);
    }
    //configura alarme periodico EVERY_MIN
    else{
        err = ds3231_disable_alarm_ints(&ds3231, DS3231_ALARM_1);
        err = ds3231_set_alarm(&ds3231, DS3231_ALARM_2, 0, 0, &CurrentDateTime, DS3231_ALARM2_EVERY_MIN);
        err = ds3231_clear_alarm_flags(&ds3231, DS3231_ALARM_BOTH);
        err = ds3231_enable_alarm_ints(&ds3231, DS3231_ALARM_2);
    }
    return err;
}

esp_err_t date_time_manager_init(){
    
    if(installed == false){
        //iniciando vetor de identificadores das tarefas
        for (int i = 0; i < CONFIG_SIZE_IDENTIFIERS; i++) {
            taskHandle[i] = NULL;
        }

        //iniciando device ds3231 i2c, a partir deste momento ja esta pronto para se comunicar via i2c
        if(init_config_ds3231() == ESP_FAIL){
            ESP_LOGI(DTMANAGER, "The  device i2c ds3231 not configuring!");
            return ESP_FAIL;
        }

        dateTimeMutex = xSemaphoreCreateMutex();
        if(dateTimeMutex == NULL ){
            ESP_LOGI(DTMANAGER, "The semaphore was not created!");
            ESP_ERROR_CHECK(ds3231_free_desc(&ds3231));
            return ESP_FAIL;
        }

        Timer_Alarm = xTimerCreate("debouncing alarm", pdMS_TO_TICKS(CONFIG_TIME_DEBOUNCING_US), pdFALSE,(void *) CONFIG_GPIO_ALARM, date_time_manager_cb_timer_alarm);
        if(Timer_Alarm == NULL ){
            ESP_LOGI(DTMANAGER, "The timer debounce was not created!");
            vSemaphoreDelete(dateTimeMutex);
            ESP_ERROR_CHECK(ds3231_free_desc(&ds3231));
            return ESP_FAIL;
        }  

        //adicionar true em install
        installed = true;
        return ESP_OK;
    }
    else{
        ESP_LOGI(DTMANAGER, "date and time manager already installed!");
        return ESP_ERR_INVALID_STATE;
    }
}

esp_err_t date_time_manager_deinit(){

    for (int i = 0; i < CONFIG_SIZE_IDENTIFIERS; i++) {
        printf("taskHandle_t uninstall\n");
        if(taskHandle[i] != NULL){
            taskHandle[i] = NULL;
            size_taskHandle --;
            printf("taskHandle_t remove\n");
        }
    }
    printf("remove all taskHandle_t\n");

    return ESP_OK;
}

