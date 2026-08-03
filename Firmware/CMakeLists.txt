cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# 1. Project Setup & Compiler Standards
# ==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Thiết lập Tên Project
set(CMAKE_PROJECT_NAME "water_tank_monitor")

# Toolchain ARM GCC (Chắc chắn file này tồn tại trong thư mục cmake/)
include("${CMAKE_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake" OPTIONAL)

# Tự động xuất compile_commands.json cho clangd / VS Code IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME} C ASM)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ==============================================================================
# 2. Executable & Target Definition
# ==============================================================================
add_executable(${CMAKE_PROJECT_NAME})

# ==============================================================================
# 3. Source Discovery 
# ==============================================================================
# Tầng Core (CubeMX sinh ra)
file(GLOB_RECURSE CORE_SRC
    "${CMAKE_SOURCE_DIR}/Core/Src/*.c"
)

# Tầng Application (App logic, State Machine, Controller)
file(GLOB_RECURSE APP_SRC
    "${CMAKE_SOURCE_DIR}/App/*.c"
)

# Tầng Drivers BSP (Relay, Buzzer, LCD, Water Sensor, v.v.)
file(GLOB_RECURSE BSP_SRC
    "${CMAKE_SOURCE_DIR}/Drivers/BSP/*.c"
)

# Tầng Middleware (Filter, Logger, DSP, v.v.)
file(GLOB_RECURSE MIDDLEWARE_SRC
    "${CMAKE_SOURCE_DIR}/Middleware/*.c"
)

# Tầng HAL Driver & CMSIS 
set(HAL_SRC

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_i2c.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c

${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
)

# Startup Assembly
set(STARTUP_SRC "${CMAKE_SOURCE_DIR}/Core/Startup/startup_stm32f103xb.s")
if(NOT EXISTS ${STARTUP_SRC})
    # Fallback nếu file startup ở ngay thư mục gốc
    set(STARTUP_SRC "${CMAKE_SOURCE_DIR}/startup_stm32f103xb.s")
endif()

# Gắn toàn bộ sources vào Executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CORE_SRC}
    ${APP_SRC}
    ${BSP_SRC}
    ${MIDDLEWARE_SRC}
    ${HAL_SRC}
    ${STARTUP_SRC}
)

# ==============================================================================
# 4. Include Directories
# ==============================================================================
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/App
    ${CMAKE_SOURCE_DIR}/Middleware
    ${CMAKE_SOURCE_DIR}/Drivers/BSP
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

# ==============================================================================
# 5. Compile Definitions (Preprocessor Macros)
# ==============================================================================
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    STM32F103xB
    USE_HAL_DRIVER
    HSI_VALUE=8000000
    $<$<CONFIG:Debug>:DEBUG>
)


# ==============================================================================
# 7. Post-Build Commands (Xuất HEX, BIN và kiểm tra Memory Size)
# ==============================================================================
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMENT "Building HEX and BIN binary outputs..."
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Memory usage summary:"
)

# Dọn dẹp các file sinh ra khi gõ 'make clean' hoặc 'ninja clean'
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    ADDITIONAL_CLEAN_FILES "${CMAKE_PROJECT_NAME}.map;${CMAKE_PROJECT_NAME}.hex;${CMAKE_PROJECT_NAME}.bin"
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary
            $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
)