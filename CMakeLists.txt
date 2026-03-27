cmake_minimum_required(VERSION 3.13)

# Importa o SDK da Raspberry Pi Pico
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Importa o FreeRTOS
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializa o Pico SDK
pico_sdk_init()

add_definitions(-DREGION_AU915) 
add_definitions(-DACTIVE_REGION=LORAMAC_REGION_AU915)
add_definitions(-DLORAMAC_CLASS_A)

add_subdirectory(lib/pico-lorawan)

add_library(freertos_config INTERFACE)

# Pluviometro 
add_library(Pluviometro src/pluviometro.cpp)

# Configuração dos Includes do Pluviometro
target_include_directories(Pluviometro PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/includes
)

# Includes do FreeRTOS Config
target_include_directories(freertos_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Links do Pluviometro
target_link_libraries(Pluviometro
    pico_stdlib
    hardware_pwm
    hardware_gpio
)

add_executable(main 
    src/main.cpp
    src/lorawan_task.c
    src/spi_diagnostic.c  # TEMPORÁRIO — remover após diagnóstico
)

# Includes do Main
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}  
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/includes 
    # Includes da Stack LoRaWAN
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico-lorawan/src
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico-lorawan/src/mac
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico-lorawan/src/system
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico-lorawan/src/radio
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico-lorawan/src/boards
)

target_link_libraries(main
    pico_stdlib
    pico_multicore
    pico_lorawan
    Pluviometro
    hardware_spi
    hardware_i2c
    hardware_pwm
    hardware_adc
    hardware_interp
    FreeRTOS-Kernel-Heap4
    freertos_config
)

# Habilita stdio via USB e UART
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

# Gera arquivos extras
pico_add_extra_outputs(main)