#include "utils.h"
#include <stdint.h>
#include <stdlib.h>

#define ALIGN 16

void *fast_malloc(size_t size) {
    size_t extra = sizeof(void *) + (ALIGN - 1);
    if (size > SIZE_MAX - extra) {
        return NULL;
    }
    unsigned char *mem = (unsigned char *)malloc(size + extra);
    if (mem == NULL) {
        return NULL;
    }
    void *ptr = (void *)(((uintptr_t)(mem + extra)) & ~(uintptr_t)(ALIGN - 1));
    ((void **)ptr)[-1] = mem;

    // Return user pointer
    return ptr;
}

void fast_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    // Sneak *behind* user pointer to find address returned by malloc
    // Use that address to free
    free(((void **)ptr)[-1]);
}
