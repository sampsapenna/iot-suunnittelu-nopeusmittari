add_library(rgb_lcd
    STATIC
    rgb_lcd.c
)

target_link_libraries(rgb_lcd pico_stdlib hardware_i2c)
