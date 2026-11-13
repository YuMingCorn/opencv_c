#include "utils.h"

#include "improc_internal.h"

#include <stdint.h>
#include <stdlib.h>

#ifndef IMPROC_NO_STDIO
#include <stdio.h>
#endif

#define ALIGN 16

static improc_assert_fn assert_handler = NULL;

void improc_set_assert_handler(improc_assert_fn handler) {
    assert_handler = handler;
}

_Noreturn void improc_assert_fail(const char *expr, const char *file,
                                  int line) {
    if (assert_handler != NULL) {
        assert_handler(expr, file, line);
    }
#ifdef IMPROC_NO_STDIO
    abort();
#else
    if (expr != NULL) {
        fprintf(stderr, "Assertion failed: %s, file: %s:%d\n", expr, file,
                line);
    } else {
        fprintf(stderr, "Assertion failed at line %d\n", line);
    }
    exit(EXIT_FAILURE);
#endif
}

#ifndef IMPROC_NO_MALLOC
static improc_alloc_fn alloc_hook = malloc;
static improc_free_fn  free_hook  = free;
#else
static improc_alloc_fn alloc_hook = NULL;
static improc_free_fn  free_hook  = NULL;
#endif

void improc_set_allocator(improc_alloc_fn alloc_fn, improc_free_fn free_fn) {
    ASSERT((alloc_fn == NULL) == (free_fn == NULL));
    if (alloc_fn != NULL) {
        alloc_hook = alloc_fn;
        free_hook  = free_fn;
        return;
    }
#ifndef IMPROC_NO_MALLOC
    alloc_hook = malloc;
    free_hook  = free;
#else
    alloc_hook = NULL;
    free_hook  = NULL;
#endif
}

void *fast_malloc(size_t size) {
    if (alloc_hook == NULL) {
        return NULL;
    }
    size_t extra = sizeof(void *) + (ALIGN - 1);
    if (size > SIZE_MAX - extra) {
        return NULL;
    }
    unsigned char *mem = (unsigned char *)alloc_hook(size + extra);
    if (mem == NULL) {
        return NULL;
    }
    void *ptr = (void *)(((uintptr_t)(mem + extra)) & ~(uintptr_t)(ALIGN - 1));
    ((void **)ptr)[-1] = mem;

    // Return user pointer
    return ptr;
}

void fast_free(void *ptr) {
    if (ptr == NULL || free_hook == NULL) {
        return;
    }
    // Sneak *behind* user pointer to find address returned by the allocator
    // Use that address to free
    free_hook(((void **)ptr)[-1]);
}

int improc_mul_size(size_t a, size_t b, size_t *out) {
    if (a != 0 && b > SIZE_MAX / a) {
        return 0;
    }
    *out = a * b;
    return 1;
}

int improc_add_size(size_t a, size_t b, size_t *out) {
    if (b > SIZE_MAX - a) {
        return 0;
    }
    *out = a + b;
    return 1;
}

size_t improc_scratch_mul(size_t a, size_t b) {
    size_t out = 0;
    ASSERT(improc_mul_size(a, b, &out));
    return out;
}

size_t improc_scratch_add(size_t a, size_t b) {
    size_t out = 0;
    ASSERT(improc_add_size(a, b, &out));
    return out;
}

void improc_scratch_check(void *const blocks[], size_t count) {
    int ok = 1;
    for (size_t i = 0; i < count; i++) {
        ok = ok && blocks[i] != NULL;
    }
    if (!ok) {
        for (size_t i = 0; i < count; i++) {
            fast_free(blocks[i]);
        }
    }
    ASSERT(ok);
}
