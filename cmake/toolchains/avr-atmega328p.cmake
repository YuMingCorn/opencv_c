# Arduino Uno (ATmega328P, 8-bit AVR, 16-bit int). Compile and size check only.
# No allocator and terse asserts keep the code small.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_C_FLAGS_INIT "-mmcu=atmega328p -ffunction-sections -fdata-sections")

set(IMPROC_ENABLE_FLOAT OFF CACHE BOOL "")
set(IMPROC_NO_MALLOC ON CACHE BOOL "")
set(IMPROC_ASSERT_TERSE ON CACHE BOOL "")
