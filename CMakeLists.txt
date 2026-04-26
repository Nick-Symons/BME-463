# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(BME463_Receiver C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(BME463_Receiver
    main.h
    main.c
    Active_Lab.h
    Drivers/DAC_I2C_Driver.h
    Drivers/DAC_I2C_Driver.c
    Drivers/Filters.h
    Drivers/Filters.c
    Drivers/SevSeg_Driver.h
    Drivers/SevSeg_Driver.c
    Drivers/Serial.h
    Drivers/Serial.c
    Labs/Final_Project.c
    Labs/lab03.c
    Labs/lab04.c
    Labs/lab05.c
    Labs/lab06.c
    Labs/lab08.c
    Labs/lab09.c
    Labs/lab10.c
    Labs/lab11.c
    Labs/lab12.c
    mini_rpnet_weights.h
    AF_Detector/Filters.c
    AF_Detector/Functions.h
    AF_Detector/Noise_Detection.cpp
    AF_Detector/PWaveScore.cpp
    AF_Detector/QRS_Detection.cpp
    AF_Detector/Single_Lead_AFib_Detection.cpp
    )

pico_set_program_name(BME463_Receiver "BME463_Receiver")
pico_set_program_version(BME463_Receiver "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(BME463_Receiver 0)
pico_enable_stdio_usb(BME463_Receiver 1)

# Add the standard library to the build
target_link_libraries(BME463_Receiver
        hardware_spi
        hardware_i2c
        hardware_adc
        hardware_timer
        pico_stdlib
)


# Add the standard include files to the build
target_include_directories(BME463_Receiver PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(BME463_Receiver 
        hardware_spi
        hardware_i2c
        hardware_clocks
        )

pico_add_extra_outputs(BME463_Receiver)

