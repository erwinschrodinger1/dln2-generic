cmake_minimum_required(VERSION 3.16)

# Detect platform
if(DEFINED ESP_PLATFORM)
    set(PLATFORM "ESP32")
elseif(CMAKE_TOOLCHAIN_FILE MATCHES "arm-none-eabi" OR 
       CMAKE_C_COMPILER MATCHES "arm-none-eabi" OR
       DEFINED STM32)
    set(PLATFORM "STM32")
endif()

# Allow manual override
set(PLATFORM ${PLATFORM} CACHE STRING "Target platform (ESP32 or STM32)")

if(NOT PLATFORM)
    message(FATAL_ERROR "Platform not detected. Set -DPLATFORM=ESP32 or -DPLATFORM=STM32")
endif()

message(STATUS "Building dln2-generic for: ${PLATFORM}")

# Common application sources
set(driver_sources
#     src/drivers/gpio_driver.c
)

set(APP_SOURCES
    src/app/driver.c
    src/app/dln2.c
    src/app/dln2-gpio.c
    src/app/dln2-pin.c
    src/app/dln2-i2c-master.c
#     src/app/dln2-spi.c
     src/app/dln2-adc.c
#     src/app/dln2-pwm.c
)

set(TUSB_SOURCES
    src/tusb/usb_descriptors.c
)

# ESP-IDF component build
if(ESP_PLATFORM)
    idf_component_register(
        SRCS ${APP_SOURCES} ${DRIVER_SOURCES}
        INCLUDE_DIRS 
            "src/drivers"
            "src/app"
            "src/utils"
        REQUIRES usb tinyusb   
    )

    idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)

    target_include_directories(${tusb_lib} PUBLIC "${COMPONENT_DIR}/src/tusb")
target_sources(${tusb_lib} PUBLIC ${TUSB_SOURCES})

else()
  target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/src/drivers
            ${CMAKE_CURRENT_LIST_DIR}/src/app
            ${CMAKE_CURRENT_LIST_DIR}/src/utils
            ${CMAKE_CURRENT_LIST_DIR}/src/tusb
    )

target_sources(${PROJECT_NAME} PRIVATE ${APP_SOURCES} ${DRIVER_SOURCES} ${TUSB_SOURCES})
    
endif()

