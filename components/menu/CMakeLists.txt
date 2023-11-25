idf_component_register(
    SRCS menu.c
    INCLUDE_DIRS "include"
    REQUIRES hd44780 pcf8574 display_lcd_16x2 button RTC_Date_Time
)