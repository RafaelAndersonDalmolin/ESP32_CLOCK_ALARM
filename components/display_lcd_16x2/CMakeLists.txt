idf_component_register(
    SRCS display_lcd_16x2.c
    INCLUDE_DIRS "include"
    REQUIRES hd44780 pcf8574
)