idf_component_register(
    SRCS "date_time_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES i2cdev ds3231 
)