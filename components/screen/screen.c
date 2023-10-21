#include "screen.h"

#define CONFIG_EXAMPLE_I2C_ADDR 0x27

// ESP8266
// #define CONFIG_EXAMPLE_I2C_MASTER_SDA 4
// #define CONFIG_EXAMPLE_I2C_MASTER_SCL 5

// ESP32
#define CONFIG_EXAMPLE_I2C_MASTER_SDA 21
#define CONFIG_EXAMPLE_I2C_MASTER_SCL 22

static i2c_dev_t pcf8574;


static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data) {
    return pcf8574_port_write(&pcf8574, data);
}


// create describer of lcd
hd44780_t lcd = {
    .write_cb = write_lcd_data,  // use callback to send data to LCD by I2C GPIO expander
    .font = HD44780_FONT_5X8,
    .lines = 2,
    .pins = {
        .rs = 0,
        .e = 2,
        .d4 = 4,
        .d5 = 5,
        .d6 = 6,
        .d7 = 7,
        .bl = 3
    }
};

 
void upload_character(){
    static const uint8_t char_data[] = { 
        // 0x04, 0x0A, 0x0A, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E, //temperatura
        0x04, 0x0A, 0x0A, 0x0E, 0x1F, 0x1F, 0x0E, 0x00, //temperatura
        0x00, 0x18, 0x1B, 0x04, 0x04, 0x04, 0x03, 0x00, //celsius
        // 0x00, 0x00, 0x04, 0x0E, 0x1B, 0x11, 0x11, 0x0E, //umidade
        0x00, 0x00, 0x04, 0x0A, 0x11, 0x11, 0x0E, 0x00, //umidade
        0x00, 0x0E, 0x04, 0x1F, 0x01, 0x00, 0x00, 0x00, //torneira   
        0x01, 0x01, 0x05, 0x05, 0x15, 0x15, 0x15, 0x00, //wifi
        0x00, 0x04, 0x0E, 0x0E, 0x0E, 0x1F, 0x04, 0x00,  //alarme
    };

    hd44780_upload_character(&lcd, 0, char_data);
    hd44780_upload_character(&lcd, 1, char_data + 8);
    hd44780_upload_character(&lcd, 2, char_data + (2*8));
    hd44780_upload_character(&lcd, 3, char_data + (3*8));
    hd44780_upload_character(&lcd, 4, char_data + (4*8));
    hd44780_upload_character(&lcd, 5, char_data + (5*8));
    hd44780_upload_character(&lcd, 6, char_data + (6*8));

    // hd44780_gotoxy(&lcd, 0, 0);
    // hd44780_puts(&lcd, "\x08 Hello world!");
    // hd44780_gotoxy(&lcd, 0, 1);
    // hd44780_puts(&lcd, "\x09 ");
}


void init_lcd() {
    memset(&pcf8574, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(
        pcf8574_init_desc(
            &pcf8574, 
            CONFIG_EXAMPLE_I2C_ADDR,
            0,
            CONFIG_EXAMPLE_I2C_MASTER_SDA,
            CONFIG_EXAMPLE_I2C_MASTER_SCL
        )
    );

    ESP_ERROR_CHECK(hd44780_init(&lcd));
    hd44780_switch_backlight(&lcd, true);

    upload_character();
}


void clear_lcd() {
    hd44780_clear(&lcd);
}


void blink_cursor(int row, int col) {
    hd44780_gotoxy(&lcd, col, row);
    hd44780_control(&lcd, false, true, true);
}


void write_lcd(char *s, int row, int col, bool clear, bool blink){
    if(clear){
        clear_lcd();
    }

    hd44780_gotoxy(&lcd, col, row);

    if(blink){
        hd44780_control(&lcd, false, false, true);
    }

    // ADDR_SCREEN, COLUMN, ROW
    hd44780_puts(&lcd, s);
    // ESP_LOGI("LCD", "ESCREVEU NA TELA");
}