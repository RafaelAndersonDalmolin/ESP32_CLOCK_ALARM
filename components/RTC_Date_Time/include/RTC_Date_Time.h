
/*  DS3231_DateTime_Manager

    Algumas observacoes do driver: 
        ->  Antes de instalar o driver, é necessario iniciar o bus I2C "ESP_ERROR_CHECK(i2cdev_init());"
        ->  Após instalar o driver, para que a tarefa seja notificada sobre a atualizacao do relogio, é necessario adicionar a notificacao para o id da tarefa via "DS3231_DateTime_Manager_task_notify_add"
        ->  apos adicionar a notificacao, tal tarefa pode aguardar uma notificacao por meio de ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification
        ->  É possivel configurar a atualizacao da hora de maneira manual ou automatica via internet (segunda opca nao pronta)
        ->  É possivel modificar o periodo do alarme depois de instalado o driver
*/
#ifndef __RTC_Date_Time_H__
#define __RTC_Date_Time_H__

#include "driver/gpio.h"
#include <i2cdev.h>
#include <ds3231.h>
#include <stdio.h>
#include <string.h>
#include <freertos/task.h>
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/FreeRTOS.h"

typedef enum {
    NTP_AUTO = 0,
    MANUAL_USER
} update_method_t;

typedef enum {
    EVERY_SECOND = 0,
    EVERY_MIN
} alarm_rate_t;


typedef struct{
    SemaphoreHandle_t dateTimeMutex; //!< Device mutex dateTime 
    TimerHandle_t Timer_Alarm; //!< //timer para debouncing
    gpio_num_t gpio_interrupt;
    update_method_t update_method;
    bool installed;
} ds3231_clock_t;


esp_err_t DS3231_DateTime_Manager_install(gpio_num_t i2c_master_sda, gpio_num_t i2c_master_scl, gpio_num_t gpio_interrupt, alarm_rate_t alarm_rate);

esp_err_t DS3231_DateTime_Manager_uninstall();

esp_err_t DS3231_DateTime_Manager_set_alarm(alarm_rate_t alarm_rate);

esp_err_t DS3231_DateTime_Manager_set_time(struct tm *DateTime);

esp_err_t DS3231_DateTime_Manager_get_date_time(struct tm *time);

esp_err_t DS3231_DateTime_Manager_task_notify_add(TaskHandle_t *taskToUpdateHandle);

esp_err_t DS3231_DateTime_Manager_task_notify_remove(TaskHandle_t *taskToUpdateHandle);

#endif