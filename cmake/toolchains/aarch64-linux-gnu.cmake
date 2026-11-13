# Raspberry Pi 64-bit (Cortex-A53/A72, AArch64 Linux).
# Tests run under qemu-user. QEMU timing says nothing about real speed.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_C_FLAGS_INIT "-march=armv8-a")

set(CMAKE_CROSSCOMPILING_EMULATOR qemu-aarch64 -L /usr/aarch64-linux-gnu)
