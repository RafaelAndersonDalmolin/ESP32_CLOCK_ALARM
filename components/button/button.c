/**
 * @file button.c
 * @brief Implementation of the button driver.
 */

#include "button.h"

#define ESP_INTR_FLAG_DEFAULT      0   /**< Default flag for GPIO interrupt configuration. */

static ButtonConfig buttons[CONFIG_SIZE_BUTTONS];  /**< Array of button configurations. */
volatile int size_b = 0;                   /**< Number of installed buttons. */

static const char *BUTTON_TAG = "Driver Button"; /**< Tag for button driver log. */

/**
 * @brief GPIO interrupt handler for buttons.
 *
 * This interrupt handler is triggered when a button is pressed. It disables the interrupt for the pin
 * and starts a timer for debouncing.
 *
 * @param btn Pointer to the button configuration.
 */
void IRAM_ATTR HandlerISR(void* btn){
    ButtonConfig* button = (ButtonConfig*) btn;
    gpio_isr_handler_remove(button->gpio);

    BaseType_t xHigherPriorityTaskWoken = pdFAIL;
    /* Verifica se o semáforo acordou uma tarefa de maior prioridade que a tarefa sendo executada atualmente */ 
    if( xTimerStartFromISR( button->internal.timer, &xHigherPriorityTaskWoken ) == pdFAIL ){
        // Se acordou, chama interrupção de software para troca de contexto
        portYIELD();
    }
}

/**
 * @brief Insert an item into the button action queue.
 *
 * Checks if the button's action queue is not full and inserts an item containing the button number and the action to be performed.
 *
 * @param btn Pointer to the button configuration.
 * @param action Action to be inserted into the queue.
 */
void insert_item(ButtonConfig* btn, ButtonAction action){

    if( uxQueueMessagesWaitingFromISR(btn->internal.button_action_queue) < CONFIG_SIZE_QUEUE){
        ButtonNotification* notification = (ButtonNotification*) malloc(sizeof(ButtonNotification));
        notification->button_num = btn->gpio;
        notification->action = action;

        xSemaphoreTake(btn->internal.mutex, portMAX_DELAY);
        if(xQueueSend(btn->internal.button_action_queue,(void*) &notification, 0) == pdFAIL){
            ESP_LOGI(BUTTON_TAG, "The GPIO %d event item was not sucessfully posted!", (uint32_t) btn->gpio);
        }
        xSemaphoreGive(btn->internal.mutex);
    }                        
}

/**
 * @brief Timer callback function for button action handling.
 *
 * This function is called when the timer used for debouncing and button press/repeat control expires.
 * It checks the button state and performs the appropriate action.
 *
 * @param xTimer Handle of the timer used for the button.
 */
void vTimerCallback(TimerHandle_t xTimer){
    
    ButtonConfig* btn = (ButtonConfig*) pvTimerGetTimerID(xTimer);

    if (gpio_get_level(btn->gpio) == btn->pressed_level){	
        // primeira vez que entra na chamada
        if (btn->internal.state == BUTTON_RELEASED){
            //muda periodo do timer
            xTimerChangePeriod( xTimer, pdMS_TO_TICKS(CONFIG_POLL_TIMEOUT_US), 0 );
            //muda estado para pressionado
            // pressing just started, reset pressing/repeating time
            btn->internal.state = BUTTON_PRESSED;
            btn->internal.pressed_time = 0;
            btn->internal.repeating_time = 0;       
            return;
        }
        else{
            // increment pressing time
            btn->internal.pressed_time += CONFIG_POLL_TIMEOUT_US;

            if(btn->autorepeat &&  (btn->internal.pressed_time >= CONFIG_AUTOREPEAT_ACTIVE_US)){
                // increment repeating time
                btn->internal.repeating_time += CONFIG_POLL_TIMEOUT_US;
                if (btn->internal.repeating_time >= CONFIG_AUTOREPEAT_ACTION_US){
                    //reset repeating time
                    btn->internal.repeating_time = 0;
                    insert_item(btn,BUTTON_CLICKED);
                }
            }
            else if(btn->pressed_long && (btn->internal.pressed_time >= CONFIG_LONG_PRESS_ACTION_US)){
                //stop timer
                xTimerStop( xTimer, 0 );
                //muda estado para liberado
                btn->internal.state = BUTTON_RELEASED;
                insert_item(btn,BUTTON_PRESSED_LONG);
                //ativa interrupacao do pino
                gpio_isr_handler_add(btn->gpio, HandlerISR, (void *) btn);
            }
            return;
        }
    }
    //estao do botao em pressionado, porem, nivel logico do botao nao é pressionado
    else if (btn->internal.state == BUTTON_PRESSED){ 
         //muda estado para liberado
        btn->internal.state = BUTTON_RELEASED;
        insert_item(btn,BUTTON_CLICKED);
    }
    //para timer
    xTimerStop( xTimer, 0 );
    //ativa interrupacao do pinos
    gpio_isr_handler_add(btn->gpio, HandlerISR, (void *) btn);
}

/**
 * @brief Find the queue associated with a specific GPIO pin.
 *
 * Searches the button array for the queue associated with the given GPIO pin.
 * If found, returns a reference to the queue.
 *
 * @param gpio GPIO pin to be searched.
 * @return Reference to the button's queue if found. Otherwise, returns NULL.
 */
QueueHandle_t find_queue(gpio_num_t gpio){
    for(size_t i = 0; i < CONFIG_SIZE_BUTTONS; i++){
        if(!buttons[i].free){
            if(buttons[i].gpio == gpio){
                return buttons[i].internal.button_action_queue;
            }
        }
    } 
    ESP_LOGI(BUTTON_TAG, "The GPIO %d queue was not find!", (uint32_t) gpio);
    return NULL;   
}

/**
 * @brief Initialize all button configurations.
 *
 * Initializes all button configuration structs as free for use.
 */
void initializeButtons(){
    for (size_t i = 0; i < CONFIG_SIZE_BUTTONS; i++) {
        buttons[i].free = true;
    }
}

/**
 * @brief Configure the GPIO for the button.
 *
 * Configures the button's GPIO according to the options provided in the button configuration.
 *
 * @param gpio GPIO pin for the button.
 * @param internal_resistors If true, enable internal resistors.
 * @param pressed_level Logical level of the button when pressed.
 * @return ESP_OK if the configuration is successful, otherwise returns ESP_FAIL.
 */
esp_err_t config_gpio(gpio_num_t gpio, bool internal_resistors, uint8_t pressed_level){
    
    uint64_t pin_bit_mask = (1ULL<<gpio);
    gpio_mode_t mode = GPIO_MODE_INPUT;
    gpio_int_type_t intr_type = (pressed_level ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE); 
    
    gpio_pullup_t pull_up =  GPIO_PULLUP_DISABLE;
    gpio_pulldown_t pull_down = GPIO_PULLDOWN_DISABLE;

    if(internal_resistors){
        pull_up = (pressed_level ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE);
        pull_down = (pressed_level ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE);
    }

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

/**
 * @brief Install a button.
 *
 * Installs a button based on the configurations provided in the button configuration.
 * Creates an action queue and a timer for button control.
 *
 * @param gpio GPIO pin for the button.
 * @param internal_resistors If true, enable internal resistors.
 * @param pressed_level Logical level of the button when pressed.
 * @param autorepeat If true, activate the button's autorepeat mode.
 * @param pressed_long If true, activate the button's long press detection.
 * @param shared_queue_gpio GPIO pin of the shared queue, if applicable.
 * @return ESP_OK if the installation is successful, otherwise returns ESP_FAIL.
 */
esp_err_t button_install(gpio_num_t gpio, bool internal_resistors, uint8_t pressed_level, bool autorepeat, bool pressed_long, gpio_num_t shared_queue_gpio){
    
    /* disable the default button logging ESP_LOG_NONE*/
	esp_log_level_set(BUTTON_TAG, ESP_LOG_INFO);

    //capacidade do vetor de buttons é maior que numero de buttons instalados?
    if (CONFIG_SIZE_BUTTONS > size_b){
        //guarda posicao livre no vetor de buttons
        size_t free_position = 0;
        //ref. queue compartilhada
        static QueueHandle_t gpio_evt_queue = NULL;
        //fila do botao é uma fila compartilhada
        bool shared_queue;
        // mutex para safe race of the queue
        SemaphoreHandle_t filaMutex = NULL;              

        //inicia configuracoes do gpio, baseado no descritor do button
        if (config_gpio(gpio,internal_resistors,pressed_level) != ESP_OK){
            ESP_LOGI(BUTTON_TAG, "Erro ao definir configuracoes do pino %d!", (uint32_t) gpio);
            return ESP_FAIL;
        }
 
        //numero de buttons instalados é igual a zero?
        //Primeiro button a ser instalado!
        if(!size_b){
            //inicializa todas structs como livres para uso
            initializeButtons(); 

            //install gpio isr service
            ESP_LOGI(BUTTON_TAG," install service ISR");
            if( gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT) != ESP_OK ){
                    ESP_LOGI(BUTTON_TAG, "ISR Service was not install and must not be used!");
                    return ESP_FAIL;
            }    
        }
        //Ao menos um botao ja foi instalado!
        else{
            //percorre o vetor de buttons
            for(size_t i = 0; i < CONFIG_SIZE_BUTTONS; i++){    
                //verifica se a posicao no vetor esta livre para um button
                //sempre guarda o ultima posicao de livre do vetor
                if(buttons[i].free){
                    free_position = i;
                }
                //se posicao nao esta livre, contem um button para acessar na memoria.
                else{
                    //verificar se button passado ja esta instalado
                    if(buttons[i].gpio == gpio){
                        ESP_LOGI(BUTTON_TAG, "Esse button ja foi instalado anteriormente!");
                        return ESP_FAIL;
                    }
                    //aproveita o loop para encontrar ref. para a queue, caso seja compartilhada.
                    //se for diferente de zero, nao é queue exclusiva, entao, buscamos pelo pino que compartilhara a queue.
                    //Caso nao encontre o pino a compartilhar, fica NULL na ref. para queue
                    else if(shared_queue_gpio == buttons[i].gpio){
                        gpio_evt_queue = buttons[i].internal.button_action_queue;
                        filaMutex = buttons[i].internal.mutex;
                        buttons[i].internal.shared_queue = true;
                        shared_queue = true;
                    }
                }
            }
        }

        //Por ser o primeiro botao que estamos instalando, nao existe uma fila para compartilhar.
        //modo fila exclusiva, o parametro passado é igual a -1
        if(shared_queue_gpio == -1){
            gpio_evt_queue = xQueueCreate(CONFIG_SIZE_QUEUE, sizeof(ButtonNotification*));
            if(gpio_evt_queue == NULL ){
                ESP_LOGI(BUTTON_TAG, "Queue was not created and must not be used!");
                return ESP_FAIL;
            }

            filaMutex = xSemaphoreCreateMutex();
            if(filaMutex == NULL ){
                ESP_LOGI(BUTTON_TAG, "Semaphore was not created and must not be used!");
                vQueueDelete(gpio_evt_queue);
                return ESP_FAIL;
            }
            shared_queue = false;
        }
        //caso numero de um pino que se deseja compartilhar a fila seja passado, e, nao seja encontrado. retorna um aviso e falha
        // modo queue compartilhada.
        else if(gpio_evt_queue == NULL){
            ESP_LOGI(BUTTON_TAG, "Queue shared was not found!");
            return ESP_FAIL; 
        }

        //create timer
        //argumentos necessarios para criar um timer para o button
        buttons[free_position].internal.timer = xTimerCreate("timer_callback", pdMS_TO_TICKS(CONFIG_TIME_DEBOUNCING_US),pdTRUE,( void * ) &buttons[free_position],vTimerCallback );
        if(buttons[free_position].internal.timer  == NULL ){
                ESP_LOGI(BUTTON_TAG, "Timer was not created and must not be used!");
                vQueueDelete(gpio_evt_queue);
                vSemaphoreDelete(filaMutex);
                return ESP_FAIL;
        }  
        //atribui parametros recebidos do descritor,
        //atribuicao deve ocorrer antes de add a ISR ao pino, pode ser que ocorra uma ISR antes de finalizar a funcao.
        buttons[free_position].gpio = gpio;
        buttons[free_position].pressed_level = pressed_level;

        //usuario nao pode instalar button com duas funcoes especiais ao mesmo tempo.
        if(autorepeat == true && pressed_long == true){
            ESP_LOGI(BUTTON_TAG, "Um botao nao pode atuar em duas funcoes especiais ao mesmo tempo!");
            buttons[free_position].autorepeat = false;
            buttons[free_position].pressed_long = false;
        }
        else{
            buttons[free_position].autorepeat = autorepeat;
            buttons[free_position].pressed_long = pressed_long;
        }

        buttons[free_position].internal.state = BUTTON_RELEASED;
        buttons[free_position].internal.button_action_queue = gpio_evt_queue;
        buttons[free_position].internal.mutex = filaMutex;
        buttons[free_position].internal.shared_queue = shared_queue;

        //initialize isr handler for specific gpio pin
        ESP_LOGI(BUTTON_TAG, "Initializing gpio_isr_handler pino %d", (uint32_t) buttons[free_position].gpio);
        if( gpio_isr_handler_add( buttons[free_position].gpio, HandlerISR, (void*) &buttons[free_position]) != ESP_OK ){
                ESP_LOGI(BUTTON_TAG, "ISR handler add was not install and must not be used!");
                return ESP_FAIL;
        } 

        buttons[free_position].free = false;
        size_b++;
        ESP_LOGI(BUTTON_TAG, "***********************************************************");
        ESP_LOGI(BUTTON_TAG, "ref. fila utilizada  %p",  buttons[free_position].internal.button_action_queue);
        ESP_LOGI(BUTTON_TAG, "ref. mutex utilizada  %p",  buttons[free_position].internal.mutex);
        ESP_LOGI(BUTTON_TAG, "ref. timer utilizada  %p",  buttons[free_position].internal.timer);
        ESP_LOGI(BUTTON_TAG, "ref. button utilizada  %p",  &buttons[free_position]);
        ESP_LOGI(BUTTON_TAG, "***********************************************************");
        return ESP_OK;
    }
     //vetor de buttons esta cheio, nao há espaco para instalar mais buttons
    else{
        ESP_LOGI(BUTTON_TAG, "Quantidade maxima de buttons excedida!");
        ESP_LOGI(BUTTON_TAG, "O Button nao pode ser instalado!");
        return ESP_FAIL;
    }
}

/**
 * @brief Uninstall a button.
 *
 * Uninstalls a button associated with the given GPIO pin, freeing allocated resources.
 *
 * @param gpio GPIO pin of the button to be uninstalled.
 * @return ESP_OK if the uninstallation is successful, otherwise returns ESP_FAIL.
 */
esp_err_t button_uninstall(gpio_num_t gpio){

    // mudar pino para nivel logico baixo
    if(size_b != 0){
        for(size_t i = 0; i < CONFIG_SIZE_BUTTONS; i++){    
            //verifica se a posicao no vetor esta livre para um button
            if(buttons[i].free == false){
                if(buttons[i].gpio == gpio){
                    gpio_isr_handler_remove((uint64_t) gpio);
                                 
                    if(xTimerDelete(buttons[i].internal.timer,portMAX_DELAY) == pdFAIL){
                        ESP_LOGI(BUTTON_TAG, "Nao foi possivel deletar o timer");
                        return ESP_FAIL;
                    }

                    if(buttons[i].internal.shared_queue == false){                     
                        vQueueDelete(buttons[i].internal.button_action_queue);
                        vSemaphoreDelete(buttons[i].internal.mutex);
                    }
                    else{
                        bool *shared_queue;
                        int count = 0;
                        for(size_t j = 0; j < CONFIG_SIZE_BUTTONS; j++){
                            if(buttons[j].free == false){
                                if(buttons[j].internal.button_action_queue == buttons[i].internal.button_action_queue){
                                    shared_queue = &(buttons[j].internal.shared_queue);
                                    count ++;
                                }
                            }
                        }
                        if(count == 1){
                            *shared_queue = false;
                        }
                    }
                    gpio_config_t io_conf;

                    //bit mask of the pins that you want to set
                    io_conf.pin_bit_mask = (1ULL<<gpio);
                    // set input
                    io_conf.mode = GPIO_MODE_DEF_OUTPUT;
                    //GPIO interrupt type
                    io_conf.intr_type = GPIO_INTR_DISABLE;
                    //GPIO pull-up
                    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
                    //GPIO pull-down
                    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
                    //configure GPIO with the given settings
                     
                    if (gpio_config(&io_conf) != ESP_OK){
                        ESP_LOGI(BUTTON_TAG, "Erro ao mudar config button!");
                        return ESP_FAIL;
                    }
                    // Marca o botão como livre
                    buttons[i].free = true;
                    size_b--;
                    return ESP_OK;
                }
            }
        }
        ESP_LOGI(BUTTON_TAG, "Botão com GPIO %d não encontrado!", gpio);
        return ESP_FAIL;
    }
    else{
        ESP_LOGI(BUTTON_TAG, "Nao existem botoes instalados!");
            return ESP_FAIL;
    }
}