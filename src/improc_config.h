// Build settings for the Arduino / PlatformIO library. CMake builds pass these
// as -D flags; Arduino cannot, so this header supplies the same defaults.
// To override one, define it in the build flags (for example PlatformIO
// build_flags), so the library sources and the sketch see the same value.
// A #define in the sketch before #include <improc.h> does not work: the
// library .c files are compiled separately and would keep the default.
// IMPROC_NO_STDIO, IMPROC_NO_MALLOC and IMPROC_ASSERT_TERSE are tested with
// #ifdef, so -DX=0 still turns them on. Meaning of each option:
// docs/porting.md.
#ifndef IMPROC_CONFIG_H_
#define IMPROC_CONFIG_H_

// The Arduino library never contains image file IO (core/mat_io.c, stb).
#ifndef IMPROC_CORE_ONLY
#define IMPROC_CORE_ONLY 1
#endif

#if defined(__AVR__)
// 8-bit AVR: no float code, no stdio in ASSERT, short ASSERT records.
#ifndef IMPROC_HAS_FLOAT
#define IMPROC_HAS_FLOAT 0
#endif
#ifndef IMPROC_NO_STDIO
#define IMPROC_NO_STDIO 1
#endif
#ifndef IMPROC_ASSERT_TERSE
#define IMPROC_ASSERT_TERSE 1
#endif
#else
// ESP32, ARM Cortex-M and others: float is compiled (soft float on cores
// without an FPU); unused functions are dropped by the linker.
#ifndef IMPROC_HAS_FLOAT
#define IMPROC_HAS_FLOAT 1
#endif
#endif

#endif // IMPROC_CONFIG_H_
