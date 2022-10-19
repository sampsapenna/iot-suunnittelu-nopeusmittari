cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico-sdk/external/pico_sdk_import.cmake)

project(iot_suunnittelun_perusteet_nopeusmittari C CXX ASM)
set(CMAKE_C_STANDARD 11)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(nopeusmittari
    nopeusmittari.c
)

add_subdirectory("Grove_LCD_RGB_Backlight")

target_include_directories(rgb_lcd PUBLIC "Grove_LCD_RGB_Backlight")
target_link_directories(rgb_lcd PUBLIC "Grove_LCD_RGB_Backlight")

target_link_libraries(nopeusmittari pico_stdlib hardware_i2c rgb_lcd pico_binary_info)

pico_enable_stdio_usb(nopeusmittari 1)
pico_enable_stdio_uart(nopeusmittari 0)

pico_add_extra_outputs(nopeusmittari)
