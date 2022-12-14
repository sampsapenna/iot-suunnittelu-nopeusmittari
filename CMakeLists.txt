cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico-sdk/pico_sdk_init.cmake)
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
    hw_config.c
    # simple.c
    nopeusmittari.c
)

target_link_libraries(nopeusmittari pico_stdlib)

add_subdirectory("no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI" build)

add_subdirectory("grove-lcd-pico")
target_include_directories(rgb_lcd PUBLIC "grove-lcd-pico")
target_link_directories(rgb_lcd PUBLIC "grove-lcd-pico")

add_subdirectory("dht20-pico")
target_include_directories(dht20 PUBLIC "dht20-pico")
target_link_directories(dht20 PUBLIC "dht20-pico")

target_link_libraries(nopeusmittari
    hardware_i2c
    rgb_lcd
    pico_binary_info
    dht20
    FatFs_SPI
    hardware_clocks
    hardware_adc
)

pico_enable_stdio_usb(nopeusmittari 1)
pico_enable_stdio_uart(nopeusmittari 0)

pico_add_extra_outputs(nopeusmittari)
