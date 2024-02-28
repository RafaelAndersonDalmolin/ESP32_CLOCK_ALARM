#include "display_lcd_16x2.h"

static i2c_dev_t pcf8574;

static esp_err_t write_lcd_data(const hd44780_t *lcd, uint8_t data) {
    return pcf8574_port_write(&pcf8574, data);
}

// create describer of lcd
hd44780_t display_lcd = {
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

    hd44780_upload_character(&display_lcd, 0, char_data);
    hd44780_upload_character(&display_lcd, 1, char_data + 8);
    hd44780_upload_character(&display_lcd, 2, char_data + (2*8));
    hd44780_upload_character(&display_lcd, 3, char_data + (3*8));
    hd44780_upload_character(&display_lcd, 4, char_data + (4*8));
    hd44780_upload_character(&display_lcd, 5, char_data + (5*8));
    hd44780_upload_character(&display_lcd, 6, char_data + (6*8));

}

void display_lcd_16x2_init(){
    memset(&pcf8574, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(pcf8574_init_desc(&pcf8574));
    ESP_ERROR_CHECK(hd44780_init(&display_lcd));
    hd44780_switch_backlight(&display_lcd, true);
    upload_character();
}

void display_lcd_16x2_clear() {
    hd44780_clear(&display_lcd);
}

void display_lcd_16x2_blink_cursor(int row, int col) {
    hd44780_gotoxy(&display_lcd, col, row);
    hd44780_control(&display_lcd, false, true, true);
}

void display_lcd_16x2_write(char *s, int row, int col, bool clear, bool blink){
    if(clear){
        hd44780_clear(&display_lcd);
    }

    hd44780_gotoxy(&display_lcd, col, row);
    
    if(blink){
        hd44780_control(&display_lcd, false, false, true);
    }

    // ADDR_SCREEN, COLUMN, ROW
    hd44780_puts(&display_lcd, s);
    // ESP_LOGI("LCD", "ESCREVEU NA TELA");
}