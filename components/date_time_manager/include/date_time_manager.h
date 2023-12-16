
/*  DS3231_DateTime_Manager

        Algumas observacoes do driver: 
        ->  Antes de instalar o driver, é necessario iniciar o bus I2C "ESP_ERROR_CHECK(i2cdev_init());"
        ->  Após instalar o driver, para que a tarefa seja notificada sobre a atualizacao do relogio, é necessario adicionar a notificacao para o id da tarefa via "DS3231_DateTime_Manager_task_notify_add"
        ->  apos adicionar a notificacao, tal tarefa pode aguardar uma notificacao por meio de ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification
        ->  É possivel configurar a atualizacao da hora de maneira manual ou automatica via internet (segunda opca nao pronta)
        ->  É possivel modificar o periodo do alarme depois de instalado o driver
*/
#ifndef __DATE_TIME_MANAGER_H__
#define __DATE_TIME_MANAGER_H__

#include <i2cdev.h>
#include <ds3231.h>

#include <stdio.h>
#include <string.h>

#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <freertos/FreeRTOS.h>

#include <driver/gpio.h>
#include <esp_log.h>

typedef enum {
    NTP_AUTO = 0,
    MANUAL_USER
} update_method_t;

typedef enum {
    EVERY_SECOND = 0,
    EVERY_MIN
} alarm_rate_t;

esp_err_t date_time_manager_init();

esp_err_t date_time_manager_deinit();

esp_err_t date_time_manager_start();

esp_err_t date_time_manager_set_mode(update_method_t update_method);

update_method_t date_time_manager_get_mode();

esp_err_t date_time_manager_start_alarm(alarm_rate_t alarm_rate);

esp_err_t date_time_manager_stop_alarm();

esp_err_t date_time_manager_set_date_time(struct tm *dateTime);

esp_err_t date_time_manager_get_date_time(struct tm *dateTime);

esp_err_t date_time_manager_task_notify_add(TaskHandle_t *taskToUpdateHandle);

esp_err_t date_time_manager_task_notify_remove(TaskHandle_t *taskToUpdateHandle);

#endif