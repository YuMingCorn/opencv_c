#ifndef TEST_ALLOC_UTIL_H_
#define TEST_ALLOC_UTIL_H_

#include <stdlib.h>

#include "utils.h"

// Counting allocator for tests. Tracks the bytes that fast_malloc asks the
// allocator for (alignment overhead included) and the number of calls.
struct AllocStats {
    size_t current;
    size_t peak;
    long   allocs;
    long   frees;
};

static struct AllocStats alloc_stats;

// Each block carries a 16-byte header that holds the requested size.
#define ALLOC_HEADER 16

static void *counting_alloc(size_t size) {
    unsigned char *mem = (unsigned char *)malloc(size + ALLOC_HEADER);
    if (mem == NULL) {
        return NULL;
    }
    *(size_t *)mem = size;
    alloc_stats.allocs++;
    alloc_stats.current += size;
    if (alloc_stats.current > alloc_stats.peak) {
        alloc_stats.peak = alloc_stats.current;
    }
    return mem + ALLOC_HEADER;
}

static void counting_free(void *ptr) {
    unsigned char *mem = (unsigned char *)ptr - ALLOC_HEADER;
    alloc_stats.frees++;
    alloc_stats.current -= *(size_t *)mem;
    free(mem);
}

// Zeroes the counters and routes fast_malloc through the counting allocator.
static inline void alloc_counter_start(void) {
    alloc_stats = (struct AllocStats){0, 0, 0, 0};
    improc_set_allocator(counting_alloc, counting_free);
}

// Restores the default allocator. The counters stay readable.
static inline void alloc_counter_stop(void) {
    improc_set_allocator(NULL, NULL);
}

#endif // TEST_ALLOC_UTIL_H_
