# Cortex-M0+ bare metal (RP2040, STM32G0, ...). No FPU, so float is off.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS_INIT
    "-mcpu=cortex-m0plus -mthumb -ffunction-sections -fdata-sections --specs=nano.specs")

set(IMPROC_ENABLE_FLOAT OFF CACHE BOOL "")
