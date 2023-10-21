#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <esp_err.h>
#include <esp_log.h>

#include <string.h>
#include <hd44780.h>
#include <pcf8574.h>


/**
 * Content descriptor struct
 */
typedef struct {
    const char *s;
    int row;
    int col;
    bool clear;
    bool blink;
} ContentMessage;


/**
 * @brief Inicialization screen
 *
 * @return `ESP_OK` on success
 */
void init_lcd();


/**
 * @brief Install button
 *
 * @param s Pointer to string
 * @param col Integer of column
 * @param row Integer of row
 * @param clear Bool flag to clear screen
 * @param blink Bool flag to blink character
 * @return `ESP_OK` on success
 */
void write_lcd(char *s, int row, int col, bool clear, bool blink);

void clear_lcd();

void blink_cursor(int row, int col);

#endif /* __COMPONENTS_BUTTON_H__ */