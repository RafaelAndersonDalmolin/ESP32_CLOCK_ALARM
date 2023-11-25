idf_component_register(
    SRCS "RTC_Date_Time.c"
    INCLUDE_DIRS "include"
    REQUIRES i2cdev ds3231 
)