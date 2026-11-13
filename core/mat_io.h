#ifndef MAT_IO_H_
#define MAT_IO_H_

#include <stddef.h>

#include "mat.h"

// Hosted-only image IO (stb). Not part of improc_core.

// Decodes an in-memory PNG/JPEG. Returns NULL on failure.
struct Mat *alloc_mat_from_buffer(const uchar *buffer, size_t buffer_size);

// Decodes an image file. Returns NULL on failure.
struct Mat *alloc_mat_from_file(char const *filename);

// Returns 0 on success, -1 on failure. Format is chosen by extension (.png,
// .jpg, .jpeg).
int mat_write(const char *filename, const struct Mat *mat);

// Prints size and type of mat to stdout.
void mat_info(const struct Mat *mat);

#endif // MAT_IO_H_
