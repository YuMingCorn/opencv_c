# ESP32 (Xtensa LX6, single-precision FPU), e.g. AI-Thinker ESP32-CAM.
# Builds improc_core only. The compiler ships with the esp32 Arduino core
# (~/.arduino15/packages/esp32/tools/esp-x32/<version>/bin) and with ESP-IDF;
# put its bin directory on PATH. Arduino projects use the library in src/
# instead; this file is for size and cross-compile checks.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER xtensa-esp32-elf-gcc)
set(CMAKE_C_FLAGS_INIT "-mlongcalls -ffunction-sections -fdata-sections")

set(IMPROC_ENABLE_FLOAT ON CACHE BOOL "")
