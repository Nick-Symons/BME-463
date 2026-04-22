cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_afib_detector C CXX ASM)

pico_sdk_init()

add_executable(pico_afib_detector
    main.c
    adc_ecg.c
    filters.c
    qrs.c
    features.c
    classify.c
)

target_link_libraries(pico_afib_detector
    pico_stdlib
    hardware_adc
)

pico_enable_stdio_usb(pico_afib_detector 1)
pico_enable_stdio_uart(pico_afib_detector 0)

pico_add_extra_outputs(pico_afib_detector)