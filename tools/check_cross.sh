#!/usr/bin/env bash
# Cross-compile check for improc_core.
#
# For every toolchain whose compiler is installed: configure (MinSizeRel,
# IMPROC_WERROR=ON), build improc_core, print the object sizes and scan the
# undefined symbols for calls the target must not make. Missing compilers are
# skipped. Exits non-zero when any target fails.
#
# Usage: tools/check_cross.sh [--require-all] [build-root]
#        (build-root default: <repo>/build-cross)
#   --require-all  a missing compiler is a failure instead of a skip (CI).
#                  IMPROC_REQUIRE_ALL=1 in the environment does the same.

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REQUIRE_ALL="${IMPROC_REQUIRE_ALL:-0}"
if [ "${1:-}" = "--require-all" ]; then
    REQUIRE_ALL=1
    shift
fi
BUILD_ROOT="${1:-$ROOT/build-cross}"
TOOLCHAINS="$ROOT/cmake/toolchains"

# name | toolchain file | compiler | binutils prefix | forbidden (nostdio|nomalloc|none) | extra cmake args | CFLAGS
# CFLAGS go in through the environment: -DCMAKE_C_FLAGS would replace the
# toolchain flags (-mcpu, -mthumb, ...).
TARGETS=(
    "aarch64|aarch64-linux-gnu|aarch64-linux-gnu-gcc|aarch64-linux-gnu-|none||"
    "armhf|armhf-linux-gnueabihf|arm-linux-gnueabihf-gcc|arm-linux-gnueabihf-|none||"
    "cortex-m4f|cortex-m4f|arm-none-eabi-gcc|arm-none-eabi-|nostdio||"
    "cortex-m7|cortex-m7|arm-none-eabi-gcc|arm-none-eabi-|nostdio||"
    "cortex-m0plus|cortex-m0plus|arm-none-eabi-gcc|arm-none-eabi-|nostdio||-fstack-usage"
    "cortex-m0plus-nomalloc|cortex-m0plus|arm-none-eabi-gcc|arm-none-eabi-|nomalloc|-DIMPROC_NO_MALLOC=ON|"
    "avr-atmega328p|avr-atmega328p|avr-gcc|avr-|nomalloc||"
    "esp32|esp32|xtensa-esp32-elf-gcc|xtensa-esp32-elf-|nostdio||"
)

# Symbols the core must never reference on a target without stdio or threads.
FORBID_PTHREAD='pthread_.*'
# GCC lowers literal printf/fprintf calls to puts/putchar/fwrite/fputc, so the
# lowered forms and the stdio objects themselves are listed too.
FORBID_STDIO='printf|fprintf|vprintf|vfprintf|snprintf|sprintf|puts|fputs|fputc|putchar|fwrite|fopen|exit|_Exit|stdout|stderr|_impure_ptr|__iob'
FORBID_MALLOC='malloc|calloc|realloc|free'

declare -a SUMMARY=()
declare -a SIZE_TABLE=()
failed=0

# Prints the undefined symbols of an archive, one per line.
undefined_symbols() {
    "$1"nm -u "$2" | awk 'NF == 2 && $1 == "U" { print $2 }' | sort -u
}

# Prints the 5 largest stack frames from the .su files under a build dir.
print_stack_usage() {
    echo "  five largest stack frames (-fstack-usage):"
    find "$1" -name '*.su' -print0 | xargs -0 cat 2>/dev/null |
        awk -F'\t' '{ split($1, a, ":"); print $2 "\t" a[1] ":" a[4] }' |
        sort -rn | head -5 | sed 's/^/    /'
}

run_target() {
    local name="$1" toolchain="$2" compiler="$3" prefix="$4" forbid="$5" extra="$6" cflags="$7"
    local build="$BUILD_ROOT/$name"

    if ! command -v "$compiler" >/dev/null 2>&1; then
        if [ "$REQUIRE_ALL" = "1" ]; then
            echo "== $name: FAIL ($compiler not installed, --require-all)"
            SUMMARY+=("$name FAIL(missing compiler)")
            failed=1
        else
            echo "== $name: SKIP ($compiler not installed)"
            SUMMARY+=("$name SKIP")
        fi
        return
    fi

    echo "== $name ($("$compiler" -dumpfullversion 2>/dev/null || "$compiler" -dumpversion))"
    rm -rf "$build"

    # shellcheck disable=SC2086
    if ! CFLAGS="$cflags" cmake -S "$ROOT" -B "$build" -G Ninja \
            --toolchain "$TOOLCHAINS/$toolchain.cmake" \
            -DCMAKE_BUILD_TYPE=MinSizeRel -DIMPROC_WERROR=ON $extra \
            >"$build.configure.log" 2>&1; then
        echo "  configure FAILED, see $build.configure.log"
        tail -15 "$build.configure.log" | sed 's/^/    /'
        SUMMARY+=("$name FAIL(configure)")
        failed=1
        return
    fi

    if ! cmake --build "$build" --target improc_core >"$build.build.log" 2>&1; then
        echo "  build FAILED"
        grep -E 'error|warning' "$build.build.log" | sed 's/^/    /' | head -20
        SUMMARY+=("$name FAIL(build)")
        failed=1
        return
    fi

    local lib="$build/libimproc_core.a"
    echo "  sizes:"
    "$prefix"size -t "$lib" | sed 's/^/    /'
    SIZE_TABLE+=("$name|$("$prefix"size -t "$lib" | awk 'END { print $1 "|" $2 "|" $3 }')")

    local pattern="$FORBID_PTHREAD"
    case "$forbid" in
        nostdio) pattern="$pattern|$FORBID_STDIO" ;;
        nomalloc) pattern="$pattern|$FORBID_STDIO|$FORBID_MALLOC" ;;
    esac
    local bad
    bad="$(undefined_symbols "$prefix" "$lib" | grep -E "^($pattern)\$" || true)"
    if [ -n "$bad" ]; then
        echo "  forbidden undefined symbols:"
        echo "$bad" | sed 's/^/    /'
        SUMMARY+=("$name FAIL(symbols)")
        failed=1
        return
    fi

    case "$cflags" in
        *-fstack-usage*) print_stack_usage "$build" ;;
    esac
    SUMMARY+=("$name PASS")
}

mkdir -p "$BUILD_ROOT"
for entry in "${TARGETS[@]}"; do
    IFS='|' read -r name toolchain compiler prefix forbid extra cflags <<<"$entry"
    run_target "$name" "$toolchain" "$compiler" "$prefix" "$forbid" "$extra" "$cflags"
done

echo
echo "== size table (text|data|bss bytes of libimproc_core.a, MinSizeRel)"
for row in "${SIZE_TABLE[@]}"; do
    echo "  $row"
done
echo
echo "== summary"
for row in "${SUMMARY[@]}"; do
    echo "  $row"
done

exit "$failed"
