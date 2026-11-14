// Compiles core/convert_f32.c for the Arduino build (see improc.h).
// CMake builds this file with -ffp-contract=off; Arduino cannot set per-file
// flags, so the pragma below does the same. Without it, FPUs with a fused
// multiply-add (ESP32, Cortex-M4F/M7) could round differently from other
// targets and break the bit-identical output.
#include "improc_config.h"

#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#endif

#include "../core/convert_f32.c"
