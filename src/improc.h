// Entry header of the Arduino / PlatformIO library: #include <improc.h>.
// The sources live in core/; the files in src/ only include them, so the
// CMake build and the Arduino build compile the same code.
#ifndef IMPROC_ARDUINO_H_
#define IMPROC_ARDUINO_H_

#include "improc_config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/improc.h"

#ifdef __cplusplus
}
#endif

#endif // IMPROC_ARDUINO_H_
