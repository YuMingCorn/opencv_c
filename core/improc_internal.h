#ifndef IMPROC_INTERNAL_H_
#define IMPROC_INTERNAL_H_

// Internal helpers shared by the core sources and the tests. Not part of the
// public API: improc.h does not include this header.

#include <stddef.h>
#include <stdint.h>

// ---- Checked size arithmetic -----------------------------------------------

// Stores a * b in *out and returns 1. Returns 0 when the product does not fit
// size_t (16 bits on AVR, 32 bits on Cortex-M and armhf). *out is untouched on
// failure.
int improc_mul_size(size_t a, size_t b, size_t *out);

// Stores a + b in *out and returns 1. Returns 0 on size_t overflow.
int improc_add_size(size_t a, size_t b, size_t *out);

// Scratch-size forms. They return a * b (or a + b) and fail ASSERT on overflow,
// the same way ops fail ASSERT when fast_malloc returns NULL. A wrapped size
// therefore never turns into an undersized buffer.
size_t improc_scratch_mul(size_t a, size_t b);
size_t improc_scratch_add(size_t a, size_t b);

// Fails ASSERT when any of the count blocks is NULL (an allocation failed).
// It releases the other blocks first, so a handler that longjmps does not leak
// them. All ops use it, so allocation failure behaves the same everywhere.
void improc_scratch_check(void *const blocks[], size_t count);

// ---- 128-bit helpers for the exact Otsu comparison (hist.c) ----------------

struct improc_u128 {
    uint64_t hi;
    uint64_t lo;
};

// 64 x 64 -> 128 bit product from four 32-bit multiplies. Portable C: no
// __int128, so it builds on AVR and Cortex-M0+.
struct improc_u128 improc_mul_u64(uint64_t a, uint64_t b);

// Returns 1 when a > b.
int improc_u128_greater(struct improc_u128 a, struct improc_u128 b);

#endif // IMPROC_INTERNAL_H_
