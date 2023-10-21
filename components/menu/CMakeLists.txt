idf_component_register(
    SRCS menu.c
    INCLUDE_DIRS .
    REQUIRES hd44780 pcf8574 screen button RTC_Date_Time
)