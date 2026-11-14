#!/usr/bin/env bash
# Checks the Arduino / PlatformIO layout (src/) without arduino-cli:
#   1. every installed cross compiler builds src/*.c with only -Isrc and no -D
#      flags, the way the Arduino builder does;
#   2. a C++ file that includes <improc.h> compiles and links on the host
#      (HOST_CC and HOST_CXX pick the host compilers, default gcc and g++);
#   3. on FPU targets, the float wrapper contains no fused multiply-add.
# Missing compilers are reported as SKIP. Exit status 1 on any failure.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${OUT:-$ROOT/build-arduino-src}"
mkdir -p "$OUT"
fail=0

# Every core source except the file IO (mat_io.c) needs a src/ wrapper.
for core_src in "$ROOT"/core/*.c; do
    module="$(basename "$core_src" .c)"
    [ "$module" = mat_io ] && continue
    if [ ! -f "$ROOT/src/improc_$module.c" ]; then
        echo "  missing wrapper src/improc_$module.c for core/$module.c"
        fail=1
    fi
done

# name | compiler | flags | objdump | fma regex
TARGETS=(
    "cortex-m4f|arm-none-eabi-gcc|-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16|arm-none-eabi-objdump|vfma|vfms|vfnm"
    "cortex-m0plus|arm-none-eabi-gcc|-mcpu=cortex-m0plus -mthumb|arm-none-eabi-objdump|"
    "avr-atmega328p|avr-gcc|-mmcu=atmega328p|avr-objdump|"
    "esp32|xtensa-esp32-elf-gcc|-mlongcalls|xtensa-esp32-elf-objdump|madd\\.s|msub\\.s"
    "host-cc|${HOST_CC:-gcc}||objdump|"
)

for entry in "${TARGETS[@]}"; do
    IFS='|' read -r name cc flags objdump fma <<<"$entry"
    if ! command -v "$cc" >/dev/null 2>&1; then
        echo "  $name SKIP ($cc not installed)"
        continue
    fi
    dir="$OUT/$name"
    mkdir -p "$dir"
    ok=1
    for src in "$ROOT"/src/*.c; do
        obj="$dir/$(basename "$src" .c).o"
        # Arduino cores use gnu11/gnu17; -Wall -Wextra surfaces anything a
        # user enabling warnings would see.
        # shellcheck disable=SC2086
        if ! "$cc" -std=gnu11 -Os -Wall -Wextra -Werror $flags -I"$ROOT/src" \
            -c "$src" -o "$obj"; then
            ok=0
        fi
    done
    if [ "$ok" = 1 ] && [ -n "$fma" ]; then
        if "$objdump" -d "$dir/improc_convert_f32.o" | grep -Eq "$fma"; then
            echo "  $name: fused multiply-add found in improc_convert_f32.o"
            ok=0
        fi
    fi
    if [ "$ok" = 1 ]; then
        echo "  $name PASS"
    else
        echo "  $name FAIL"
        fail=1
    fi
done

# C++ consumer on the host: include the entry header like a sketch does and
# run the Synthetic.ino pipeline against its expected hashes.
HOST_CXX="${HOST_CXX:-g++}"
if command -v "$HOST_CXX" >/dev/null 2>&1 && [ -d "$OUT/host-cc" ]; then
    dir="$OUT/host-cxx"
    mkdir -p "$dir"
    cat >"$dir/sketch.cpp" <<'EOF'
#include <improc.h>
#include "synthetic_pipeline.h"
int main() {
    struct SynthResult r;
    return (synth_run(&r, 0) == 0 && synth_matches(&r)) ? 0 : 1;
}
EOF
    if "$HOST_CXX" -std=c++17 -Wall -Wextra -Werror -pedantic -I"$ROOT/src" \
        -I"$ROOT/examples/Synthetic" -c "$dir/sketch.cpp" -o "$dir/sketch.o" &&
        "$HOST_CXX" "$dir/sketch.o" "$OUT/host-cc"/*.o -lm -o "$dir/sketch" &&
        "$dir/sketch"; then
        echo "  host-c++ PASS"
    else
        echo "  host-c++ FAIL"
        fail=1
    fi
else
    echo "  host-c++ SKIP ($HOST_CXX or host-cc objects missing)"
fi

exit "$fail"
