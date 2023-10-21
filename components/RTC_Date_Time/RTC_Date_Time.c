#include "RTC_Date_Time.h"

#define ESP_INTR_FLAG_DEFAULT      0   /**< Default flag for GPIO interrupt configuration. */
#define TIME_DEBOUNCING_US         15  /**< Debouncing time in microseconds (15ms). */
#define SIZE_IDENTIFIERS           5   /**< Maximum number of identifiers that can be installed. */

//Variavel para armazenamento atual da data e hora 
struct tm CurrentDateTime = {
    .tm_sec = 00, //representa os segundos de 0 a 59
    .tm_min = 26, //representa os minutos de 0 a 59
    .tm_hour = 22, //representa as horas de 0 a 23
    .tm_wday = 0, //dia da semana de 0 (domingo) até 6 (sábado)
    .tm_mday = 18, //dia do mês de 1 a 31
    .tm_mon = 9, //representa os meses do ano de 0 a 11
    .tm_year = 123, //representa o ano a partir de 1900
    .tm_yday = 232, // dia do ano de 1 a 365
    .tm_isdst = 0 //indica horário de verão se for diferente de zero
};

//configuration DS3231 DateTime Manager
ds3231_clock_t device_data_time = {
    .dateTimeMutex = NULL,
    .Timer_Alarm = NULL,
    .gpio_interrupt = 0,
    .update_method = MANUAL_USER,
    .installed = false
};

//config device ds3231 I2C
i2c_dev_t ds3231;

//task identifiers
TaskHandle_t *taskHandle[SIZE_IDENTIFIERS];
short int size_taskHandle = 0;    

void IRAM_ATTR rtcAlarmISR(void* arg){

    //desabilitar interrupcao do pino
    ESP_ERROR_CHECK(gpio_isr_handler_remove((gpio_num_t) arg));
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* Verifica se o semáforo acordou uma tarefa de maior prioridade que a tarefa sendo executada atualmente */ 
    if( xTimerStartFromISR(device_data_time.Timer_Alarm, &xHigherPriorityTaskWoken ) == pdPASS ){
        /* The start command was not executed successfully.  Take appropriate
        action here. */
    }
}

void vTimer_alam_RTC(TimerHandle_t xTimer){
    // BaseType_t HigherPriorityTaskWoken = pdFALSE;
    gpio_num_t gpio_alarm = (gpio_num_t) pvTimerGetTimerID(xTimer);
    if(gpio_get_level(gpio_alarm) == 0){
      
        if (xSemaphoreTake(device_data_time.dateTimeMutex, portMAX_DELAY)){
            if (ds3231_get_time(&ds3231, &CurrentDateTime) != ESP_OK){
                printf("Could not get CurrentDateTime\n");
            }
            xSemaphoreGive(device_data_time.dateTimeMutex);
            if(size_taskHandle != 0){
                for (int i = 0; i < SIZE_IDENTIFIERS; i++) {
                    if(taskHandle[i] != NULL){
                        xTaskNotifyGive(*(taskHandle[i])); // Send notification to Task 1
                        // vTaskNotifyGiveFromISR(*(taskHandle[i]), &HigherPriorityTaskWoken); // Send notification to Task 1
                        
                    }
                }
            }
        }
        //portYIELD();
    }
    ESP_ERROR_CHECK(ds3231_clear_alarm_flags(&ds3231, DS3231_ALARM_BOTH));
    ESP_ERROR_CHECK(gpio_isr_handler_add(gpio_alarm, rtcAlarmISR, (void*) gpio_alarm));
    //portYIELD();
}

esp_err_t rtc_config_gpio(gpio_num_t gpio){
    
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
    return gpio_config(&io_conf);
}

esp_err_t DS3231_DateTime_Manager_set_alarm(alarm_rate_t alarm_rate){

    //configura alarme periodico por segundo
    if(alarm_rate == EVERY_SECOND){
        ESP_ERROR_CHECK(ds3231_disable_alarm_ints(&ds3231, DS3231_ALARM_2));
        ESP_ERROR_CHECK(ds3231_set_alarm(&ds3231, DS3231_ALARM_1, &CurrentDateTime, DS3231_ALARM1_EVERY_SECOND, 0, 0));
        ESP_ERROR_CHECK(ds3231_enable_alarm_ints(&ds3231, DS3231_ALARM_1));
        ESP_ERROR_CHECK(ds3231_clear_alarm_flags(&ds3231, DS3231_ALARM_1));
    }
    //configura alarme periodico EVERY_MIN
    else{
        ESP_ERROR_CHECK(ds3231_disable_alarm_ints(&ds3231, DS3231_ALARM_1));
        ESP_ERROR_CHECK(ds3231_set_alarm(&ds3231, DS3231_ALARM_2, 0, 0, &CurrentDateTime, DS3231_ALARM2_EVERY_MIN));
        ESP_ERROR_CHECK(ds3231_enable_alarm_ints(&ds3231, DS3231_ALARM_2));
        ESP_ERROR_CHECK(ds3231_clear_alarm_flags(&ds3231, DS3231_ALARM_2));
    }
    return ESP_OK;
}

esp_err_t DS3231_DateTime_Manager_set_time(struct tm *DateTime){
    ESP_ERROR_CHECK(ds3231_set_time(&ds3231, DateTime));
    return ESP_OK;
}

esp_err_t DS3231_DateTime_Manager_get_date_time(struct tm *time){
    if (xSemaphoreTake(device_data_time.dateTimeMutex, portMAX_DELAY)) {
        time->tm_sec =  CurrentDateTime.tm_sec; //representa os segundos de 0 a 59
        time->tm_min = CurrentDateTime.tm_min;  //representa os minutos de 0 a 59
        time->tm_hour = CurrentDateTime.tm_hour; //representa as horas de 0 a 23
        time->tm_wday = CurrentDateTime.tm_wday;  //dia da semana de 0 (domingo) até 6 (sábado)
        time->tm_mday = CurrentDateTime.tm_mday;  //dia do mês de 1 a 31
        time->tm_mon = CurrentDateTime.tm_mon;  //representa os meses do ano de 0 a 11
        time->tm_year = CurrentDateTime.tm_year; //representa o ano a partir de 1900
        time->tm_yday = CurrentDateTime.tm_yday;  // dia do ano de 1 a 365
        time->tm_isdst = CurrentDateTime.tm_isdst; //indica horário de verão se for diferente de zero
        xSemaphoreGive(device_data_time.dateTimeMutex);
    }
    return ESP_OK;
}

esp_err_t DS3231_DateTime_Manager_task_notify_add(TaskHandle_t *taskToUpdateHandle){
    
    for (int i = 0; i < SIZE_IDENTIFIERS; i++) {
        if(taskHandle[i] == NULL){
            taskHandle[i] = taskToUpdateHandle;
            size_taskHandle ++;
            return ESP_OK;
        }
    }
    printf("falha ao add taskHandle_t\n");
    return ESP_FAIL;
}

esp_err_t DS3231_DateTime_Manager_task_notify_remove(TaskHandle_t *taskToUpdateHandle){
    
    for (int i = 0; i < SIZE_IDENTIFIERS; i++) {
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

esp_err_t init_ds3231(gpio_num_t i2c_master_sda, gpio_num_t i2c_master_scl){
    //inclui zeros na regiao de memoria da variavel do device i2c
        memset(&ds3231, 0, sizeof(i2c_dev_t));
        //inicia descritor do RTC ds3231
        ESP_ERROR_CHECK(ds3231_init_desc(&ds3231, 0, i2c_master_sda, i2c_master_scl));

        //configura hora e data inicial
        //Na primeira inicializacao setar uma data e hora random, depois nao configurar para nao perder a hora atual
        //ESP_ERROR_CHECK(DS3231_DateTime_Manager_set_time(&CurrentDateTime));

        return ESP_OK;
}

esp_err_t DS3231_DateTime_Manager_install(gpio_num_t i2c_master_sda, gpio_num_t i2c_master_scl, gpio_num_t gpio_interrupt, alarm_rate_t alarm_rate){
    
    if(device_data_time.installed == false){
        //iniciando vetor de identificadores das tarefas
        for (int i = 0; i < SIZE_IDENTIFIERS; i++) {
            taskHandle[i] = NULL;
        }

        //iniciando device ds3231 i2c, a partir deste momento ja esta pronto para se comunicar via i2c
        ESP_ERROR_CHECK(init_ds3231(i2c_master_sda, i2c_master_scl));

        device_data_time.dateTimeMutex = xSemaphoreCreateMutex();
        if(device_data_time.dateTimeMutex == NULL ){
            ESP_ERROR_CHECK(ds3231_free_desc(&ds3231));
            return ESP_FAIL;
        }

        device_data_time.Timer_Alarm = xTimerCreate("timer alarm RTC", pdMS_TO_TICKS(TIME_DEBOUNCING_US), pdFALSE,(void *) gpio_interrupt, vTimer_alam_RTC);
        if(device_data_time.Timer_Alarm == NULL ){
            ESP_ERROR_CHECK(ds3231_free_desc(&ds3231));
            vSemaphoreDelete(device_data_time.dateTimeMutex);
            return ESP_FAIL;
        }  

        //configura pino para interrupcao gerada pelo alarme
        ESP_ERROR_CHECK(rtc_config_gpio(gpio_interrupt));
        //instala servico de interrupcao
        //check de retorno para caso servico ja esta instalado
        // ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));

        ESP_ERROR_CHECK(DS3231_DateTime_Manager_set_alarm(alarm_rate));

        //adiciona interrupcao ao pino configurado
        ESP_ERROR_CHECK(gpio_isr_handler_add(gpio_interrupt, rtcAlarmISR, (void*) gpio_interrupt));
        //adicionar true em install
        device_data_time.installed = true;
        return ESP_OK;
    }
    else{
        printf("Driver de data e tempo ja instalado\n");
        return ESP_ERR_INVALID_STATE;
    }
}

esp_err_t DS3231_DateTime_Manager_uninstall(){

    for (int i = 0; i < SIZE_IDENTIFIERS; i++) {
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

