#include "utils.h"
#include "mat.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define ALIGN 16

void *fast_malloc(size_t size) {
    void *mem = malloc(size + sizeof(void *) + (ALIGN - 1));
    void *ptr = (void **)((uintptr_t)(mem + (ALIGN - 1) + sizeof(void *)) &
                          ~(ALIGN - 1));
    ((void **)ptr)[-1] = mem;

    // Return user pointer
    return ptr;
}

void fast_free(void *ptr) {
    // Sneak *behind* user pointer to find address returned by malloc
    // Use that address to free
    free(((void **)ptr)[-1]);
}