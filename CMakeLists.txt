# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
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
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(D-MAG-Firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

file(GLOB DISPLAYLIB_16_SRC
    "lib/displaylib_16/*.cpp"
)

add_executable(D-MAG-Firmware 
    src/D-MAG-Firmware.cpp
    src/Hardware.cpp
    src/Graphics.cpp
    src/FileSystem.cpp
    src/UserInterface.cpp
    src/Datalogger.cpp
    src/Math.cpp
    src/Formats.cpp
    src/Config.cpp
    ${DISPLAYLIB_16_SRC}
    )
add_subdirectory(lib/fatfs/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI)
add_subdirectory(lib/ds3231)
add_subdirectory(lib/pico-ads1115)

pico_set_program_name(D-MAG-Firmware "D-MAG-Firmware")

set(FW_VERSION "1.9.3 DEV")
pico_set_program_version(D-MAG-Firmware ${FW_VERSION})
target_compile_definitions(D-MAG-Firmware PRIVATE
    FIRMWARE_VERSION="${FW_VERSION}"
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(D-MAG-Firmware 0)
pico_enable_stdio_usb(D-MAG-Firmware 1)

# Add the standard library to the build
target_link_libraries(D-MAG-Firmware
        pico_stdlib
        )

# Add the standard include files to the build
target_include_directories(D-MAG-Firmware PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include/
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_sources(D-MAG-Firmware INTERFACE

)

# Add any user requested libraries
target_link_libraries(D-MAG-Firmware 
        hardware_i2c
        hardware_pwm
        hardware_spi
        hardware_adc
        hardware_pio
        hardware_flash
        hardware_sync
        pico_multicore
        FatFs_SPI
        pico-ds3231
        pico-ads1115
        )
        
target_link_options(D-MAG-Firmware PRIVATE -Wl,--print-memory-usage)

# Remove the FatFS library's rtc.c from compilation, since we're overriding with our own RTC function.
set_source_files_properties(
    lib/fatfs/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI/src/rtc.c
    PROPERTIES HEADER_FILE_ONLY TRUE
)

pico_add_extra_outputs(D-MAG-Firmware)

