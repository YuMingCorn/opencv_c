# Raspberry Pi 32-bit (ARMv7-A with NEON, hard float). Compile check only.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")
