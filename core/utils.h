#ifndef UTILS_H_
#define UTILS_H_

#include <stddef.h>

// ---- Assertion hook --------------------------------------------------------

// Called when ASSERT fails. The handler may longjmp. If it returns, the default
// action runs. With IMPROC_ASSERT_TERSE, expr and file are NULL.
typedef void (*improc_assert_fn)(const char *expr, const char *file, int line);

// Installs handler. NULL restores the default action.
void improc_set_assert_handler(improc_assert_fn handler);

// Calls the handler if one is set. Then prints the message and exits with
// EXIT_FAILURE, or calls abort() when IMPROC_NO_STDIO is defined. Never
// returns.
_Noreturn void improc_assert_fail(const char *expr, const char *file, int line);

#ifdef IMPROC_ASSERT_TERSE
#define ASSERT(expr)                                                           \
    ((expr) ? (void)0 : improc_assert_fail(NULL, NULL, __LINE__))
#else
#define ASSERT(expr)                                                           \
    ((expr) ? (void)0 : improc_assert_fail(#expr, __FILE__, __LINE__))
#endif

// ---- Allocator hook --------------------------------------------------------

typedef void *(*improc_alloc_fn)(size_t size);
typedef void (*improc_free_fn)(void *ptr);

// Replaces the allocator behind fast_malloc/fast_free. Both are NULL or both
// are set. NULL, NULL restores the default (malloc/free, or no allocator when
// IMPROC_NO_MALLOC is defined). Call it only while no fast_malloc block is
// live.
void improc_set_allocator(improc_alloc_fn alloc_fn, improc_free_fn free_fn);

// Scratch-memory policy. Every op that needs scratch (box_filter,
// gaussian_blur, sobel, erode/dilate/morph_*, resize, equalize_hist,
// otsu_threshold) takes it from fast_malloc and releases it before it returns.
// It fails ASSERT, in the same way in every module, when
//   - fast_malloc returns NULL (out of memory, or no allocator), or
//   - a scratch size does not fit size_t (the checked size arithmetic in
//     improc_internal.h), so a wrapped size never becomes an undersized buffer.
// The blocks that did succeed are freed before ASSERT, so a handler that
// longjmps leaks nothing. A target that cannot afford the abort installs an
// assert handler (improc_set_assert_handler) that recovers. calc_hist and
// otsu_threshold_hist take a caller-owned histogram and need no scratch.
// alloc_mat is the same. mat_create and the image IO functions return NULL
// instead, because bad input there is an expected error.

// Returns a block aligned to 16 bytes from the current allocator. Returns NULL
// on failure, on size overflow, or when there is no allocator.
void *fast_malloc(size_t size);

// Releases a block from fast_malloc. NULL is accepted.
void fast_free(void *ptr);

#endif // UTILS_H_
