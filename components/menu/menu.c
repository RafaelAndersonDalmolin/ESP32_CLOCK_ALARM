#include "menu.h"

#define VIEW_ONOFF_MANUAL 1
#define VIEW_MENU 1
#define VIEW_AGENDAMENTO 1

#define BUTTON_VOLTAR   GPIO_NUM_35     // Voltar
#define BUTTON_MODO     GPIO_NUM_34     // Mode/Ok
#define BUTTON_BAIXO    GPIO_NUM_39     // DOWM
#define BUTTON_CIMA     GPIO_NUM_36     // UP

#define TEMPERATURA_SYMBOL  '\x08'
#define CELSIUS_SYMBOL      '\x09'
#define UMIDADE_SYMBOL      '\x0A'
#define LIGADO_SYMBOL       '\x0B'
#define WIFI_SYMBOL         '\x0C'
#define ALARME_SYMBOL       '\x0D'

extern const char *MENU;
ButtonNotification *receive_action;

TaskHandle_t myTaskHandle;

int current_frame = 0;
int current_nested_frame = 0;

int option = 1;

struct tm datetime_aux = {
    .tm_sec = 0, //representa os segundos de 0 a 59
    .tm_min = 0, //representa os minutos de 0 a 59
    .tm_hour = 22, //representa as horas de 0 a 23
    .tm_wday = 3, //dia da semana de 0 (domingo) até 6 (sábado)
    .tm_mday = 27, //dia do mês de 1 a 31
    .tm_mon = 9, //representa os meses do ano de 0 a 11
    .tm_year = 123, //representa o ano a partir de 1900
    .tm_yday = 0, // dia do ano de 1 a 365
    .tm_isdst = 0 //indica horário de verão se for diferente de zero
};

int current_day = 3;
char string_time[16];

typedef struct {
    int linha;
    int coluna;
    char *weekday;
} Posicao;
Posicao pos[7];

char *week[] = {"DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SAB"};

int row = 0;
int col = 0;

void frame_set_time() {
    ESP_LOGI(MENU, "FRAME SET DATETIME");

    QueueHandle_t queue_button = find_queue((uint32_t)BUTTON_VOLTAR);

    bool next_frame = true;
    current_nested_frame = 1;

    write_lcd("DATA", 0, 0, true, false);
    write_lcd(week[datetime_aux.tm_wday], 0, 7, false, false);
    write_lcd("HORA", 0, 11, false, false);

    DS3231_DateTime_Manager_get_date_time(&datetime_aux);

    sprintf(string_time, "%02d-%02d-", datetime_aux.tm_mday, datetime_aux.tm_mon);
    write_lcd(string_time, 1, 0, false, false);

    sprintf(string_time, "%04d", (datetime_aux.tm_year + 1900));
    write_lcd(string_time, 1, 6, false, false);

    sprintf(string_time, "%02d:%02d", datetime_aux.tm_hour, datetime_aux.tm_min);
    write_lcd(string_time, 1, 11, false, false);

    while (next_frame) {
        switch (current_nested_frame) {
            case 1:  // dia
                row = 1;
                col = 0;
                while (true) {
                    sprintf(string_time, "%02d", datetime_aux.tm_mday);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_aux.tm_mday == 31) {
                                datetime_aux.tm_mday = 1;
                            } else {
                                datetime_aux.tm_mday++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_aux.tm_mday == 1) {
                                datetime_aux.tm_mday = 31;
                            } else {
                                datetime_aux.tm_mday--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case 2:  // mes
                row = 1;
                col = 3;
                while (true) {
                    sprintf(string_time, "%02d", datetime_aux.tm_mon);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_aux.tm_mon == 12) {
                                datetime_aux.tm_mon = 1;
                            } else {
                                datetime_aux.tm_mon++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_aux.tm_mon == 1) {
                                datetime_aux.tm_mon = 12;
                            } else {
                                datetime_aux.tm_mon--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }

                break;

            case 3:  // ano
                row = 1;
                col = 6;
                while (true) {
                    sprintf(string_time, "%04d", datetime_aux.tm_year + 1900);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            datetime_aux.tm_year++;
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            datetime_aux.tm_year--;
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("    ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case 4:  // semana
                row = 0;
                col = 7;
                while (true) {
                    datetime_aux.tm_wday = current_day;
                    write_lcd(week[datetime_aux.tm_wday], row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (current_day == 6) {
                                current_day = 0;
                            } else {
                                current_day++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (current_day == 0) {
                                current_day = 6;
                            } else {
                                current_day--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("   ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case 5:  // configurar hora
                row = 1;
                col = 11;
                while (true) {
                    sprintf(string_time, "%02d", datetime_aux.tm_hour);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_aux.tm_hour == 23) {
                                datetime_aux.tm_hour = 0;
                            } else {
                                datetime_aux.tm_hour++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_aux.tm_hour == 0) {
                                datetime_aux.tm_hour = 23;
                            } else {
                                datetime_aux.tm_hour--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case 6:  // configura minuto
                row = 1;
                col = 14;
                while (true) {
                    sprintf(string_time, "%02d", datetime_aux.tm_min);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_aux.tm_min == 60) {
                                datetime_aux.tm_min = 0;
                            } else {
                                datetime_aux.tm_min++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_aux.tm_min == 0) {
                                datetime_aux.tm_min = 60;
                            } else {
                                datetime_aux.tm_min--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            DS3231_DateTime_Manager_set_time(&datetime_aux);
                            option=2;
                            next_frame = false;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;
        }
    }
}

// tela inicial
void frame_initial() {
    ESP_LOGI(MENU, "FRAME INICIAL");
    
    DS3231_DateTime_Manager_task_notify_add(&myTaskHandle);

    ESP_ERROR_CHECK(DS3231_DateTime_Manager_get_date_time(&datetime_aux));
    QueueHandle_t queue_button = find_queue((uint32_t)BUTTON_VOLTAR);

    current_nested_frame = 1;
    write_lcd(week[datetime_aux.tm_wday], 0, 11, true, false);

    sprintf(string_time, "%02d-%02d-", datetime_aux.tm_mday, datetime_aux.tm_mon);
    write_lcd(string_time, 0, 0, false, false);

    sprintf(string_time, "%04d", (datetime_aux.tm_year + 1900));
    write_lcd(string_time, 0, 6, false, false);

    sprintf(string_time, "%02d:%02d:%02d", datetime_aux.tm_hour, datetime_aux.tm_min, datetime_aux.tm_sec);
    write_lcd(string_time, 1, 0, false, false);

    uint32_t ret_notify = 0;
    ESP_LOGI(MENU, "ANTES DE WHILE");
    
    while (true) {

        if(xQueueReceive(queue_button, &receive_action, 0)){
            
            // avanca para o frame de configurar timer
            if ((receive_action->button_num == BUTTON_MODO) & (receive_action->action == BUTTON_CLICKED)) {
                option = 3;
                break;
            }
            // retorna para o frame de configurar hora
            else if((receive_action->button_num == BUTTON_MODO) & (receive_action->action == BUTTON_PRESSED_LONG)) { 
                option = 1;
                break;
            }
        }

        ret_notify = (uint32_t) ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
        if(ret_notify != 0){
            ESP_ERROR_CHECK(DS3231_DateTime_Manager_get_date_time(&datetime_aux));
            sprintf(string_time, "%02d:%02d:%02d", datetime_aux.tm_hour, datetime_aux.tm_min, datetime_aux.tm_sec);
            write_lcd(string_time, 1, 0, false, false);
        }

    }
}


void frame_set_alarme() {
    ESP_LOGI(MENU, "SET ALARME");
    QueueHandle_t queue_button = find_queue((uint32_t)BUTTON_VOLTAR);
    DS3231_DateTime_Manager_task_notify_add(&myTaskHandle);

    bool next_frame = true;

    current_nested_frame = 1;
    struct tm datetime_onoff;
    datetime_onoff.tm_hour = 0;
    datetime_onoff.tm_min = 0;
    datetime_onoff.tm_sec = 0;

    write_lcd("TIMER", 0, 0, true, false);
    sprintf(string_time, "%02d:%02d:%02d", datetime_onoff.tm_hour, datetime_onoff.tm_min, datetime_onoff.tm_sec);
    write_lcd(string_time, 1, 0, false, false);

    while (next_frame) {
        switch (current_nested_frame) {
            case 1:  // configurar hora
                row = 1;
                col = 0;
                while (true) {
                    sprintf(string_time, "%02d", datetime_onoff.tm_hour);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_onoff.tm_hour == 23) {
                                datetime_onoff.tm_hour = 0;
                            } else {
                                datetime_onoff.tm_hour++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_onoff.tm_hour == 0) {
                                datetime_onoff.tm_hour = 23;
                            } else {
                                datetime_onoff.tm_hour--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            DS3231_DateTime_Manager_task_notify_remove(&myTaskHandle);
                            option--;
                            next_frame = false;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case 2:  // configura minuto
                row = 1;
                col = 3;
                while (true) {
                    sprintf(string_time, "%02d", datetime_onoff.tm_min);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_onoff.tm_min == 60) {
                                datetime_onoff.tm_min = 0;
                            } else {
                                datetime_onoff.tm_min++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_onoff.tm_min == 0) {
                                datetime_onoff.tm_min = 60;
                            } else {
                                datetime_onoff.tm_min--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case 3:  // configura segundos
                row = 1;
                col = 6;
                while (true) {
                    sprintf(string_time, "%02d", datetime_onoff.tm_sec);
                    write_lcd(string_time, row, col, false, false);

                    if (xQueueReceive(queue_button, &receive_action, pdMS_TO_TICKS(500))) {
                        if (receive_action->button_num == BUTTON_CIMA) {
                            if (datetime_onoff.tm_sec == 60) {
                                datetime_onoff.tm_sec = 0;
                            } else {
                                datetime_onoff.tm_sec++;
                            }
                        } else if (receive_action->button_num == BUTTON_BAIXO) {
                            if (datetime_onoff.tm_sec == 0) {
                                datetime_onoff.tm_sec = 60;
                            } else {
                                datetime_onoff.tm_sec--;
                            }
                        } else if (receive_action->button_num == BUTTON_MODO) {
                            current_nested_frame++;
                            break;
                        } else if (receive_action->button_num == BUTTON_VOLTAR) {
                            current_nested_frame--;
                            break;
                        }
                    } else {
                        write_lcd("  ", row, col, false, false);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;
            
            case 4:  // contagem regressiva
                sprintf(string_time, "%c", ALARME_SYMBOL);
                write_lcd(string_time, 1, 15, false, false);

                while (datetime_onoff.tm_hour >= 0) {

                    sprintf(string_time, "%02d:%02d:", datetime_onoff.tm_hour, datetime_onoff.tm_min);
                    write_lcd(string_time, 1, 0, false, false);

                    sprintf(string_time, "%02d", datetime_onoff.tm_sec);
                    write_lcd(string_time, 1, 6, false, false);

                    // Aguarda 1 segundo
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                    // Diminui 1 segundo
                    datetime_onoff.tm_sec--;

                    // Se os segundos chegarem a zero, ajusta minutos e segundos
                    if (datetime_onoff.tm_sec < 0) {
                        datetime_onoff.tm_sec = 59;
                        datetime_onoff.tm_min--;
                    }

                    // Se os minutos chegarem a zero, ajusta horas e minutos
                    if (datetime_onoff.tm_min < 0) {
                        datetime_onoff.tm_min = 59;
                        datetime_onoff.tm_hour--;
                    }

                    // Se o tempo acabou, interrompe o loop
                    if (datetime_onoff.tm_hour == 0 && datetime_onoff.tm_min == 0 && datetime_onoff.tm_sec == 0){
                        DS3231_DateTime_Manager_task_notify_remove(&myTaskHandle);
                        option=2;
                        next_frame = false;
                        break;
                    }
                }
                break;
        }
    }
}

void menu_option() {
    myTaskHandle = xTaskGetHandle("menu");
    
    while (true) {
        switch (option) {
            case 1:
                frame_set_time();
                break;

            case 2:
                frame_initial();
                break;

            case 3:
                frame_set_alarme();
                break;

            default:
                printf("Invalid option. Please try again.\n");
                break;
        }
    }
}
