#include "mat_io.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Takes ownership of the stb-allocated data and always releases it.
static struct Mat *mat_from_stb(uchar *data, int w, int h, int channels) {
    if (channels < 1 || channels > 4) {
        stbi_image_free(data);
        return NULL;
    }
    struct Mat *mat = mat_create(h, w, CV_MAKETYPE(CV_8U, channels));
    if (mat != NULL) {
        memcpy(mat->data, data, (size_t)h * mat->step[0]);
    }
    stbi_image_free(data);
    return mat;
}

struct Mat *alloc_mat_from_buffer(const uchar *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size > INT_MAX) {
        return NULL;
    }
    int    width = 0, height = 0, channels = 0;
    uchar *data = stbi_load_from_memory(buffer, (int)buffer_size, &width,
                                        &height, &channels, 0);
    if (data == NULL) {
        fprintf(stderr, "Failed to load image from buffer\n");
        return NULL;
    }
    return mat_from_stb(data, width, height, channels);
}

struct Mat *alloc_mat_from_file(const char *filename) {
    int    width = 0, height = 0, channels = 0;
    uchar *data = stbi_load(filename, &width, &height, &channels, 0);
    if (data == NULL) {
        fprintf(stderr, "Failed to load image %s\n", filename);
        return NULL;
    }
    return mat_from_stb(data, width, height, channels);
}

// Case-insensitive match of the extension after the last '.' in the file name.
static int has_extension(const char *filename, const char *ext) {
    const char *name = filename;
    for (const char *p = filename; *p; p++) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    const char *dot = NULL;
    for (const char *p = name; *p; p++) {
        if (*p == '.') {
            dot = p;
        }
    }
    if (dot == NULL) {
        return 0;
    }
    const char *a = dot + 1;
    while (*a && *ext) {
        char c = *a;
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        if (c != *ext) {
            return 0;
        }
        a++;
        ext++;
    }
    return *a == '\0' && *ext == '\0';
}

int mat_write(const char *filename, const struct Mat *mat) {
    ASSERT(mat != NULL && mat_depth(mat) == CV_8U);
    int ok = 0;
    int cn = (int)mat_channel_num(mat);
    if (has_extension(filename, "png")) {
        ok = stbi_write_png(filename, mat->cols, mat->rows, cn, mat->data,
                            (int)mat->step[0]);
    } else if (has_extension(filename, "jpg") ||
               has_extension(filename, "jpeg")) {
        ok = stbi_write_jpg(filename, mat->cols, mat->rows, cn, mat->data, 100);
    } else {
        fprintf(stderr, "Unsupported image extension: %s\n", filename);
        return -1;
    }
    if (!ok) {
        fprintf(stderr, "Failed to write image %s\n", filename);
        return -1;
    }
    return 0;
}

void mat_info(const struct Mat *mat) {
    printf("Size:              : [%d x %d]\n", (int)mat->rows, (int)mat->cols);
    printf("Channel num        : %d\n", (int)mat_channel_num(mat));
    printf("Channel size(byte) : %d\n", (int)mat_channel_size(mat));
    printf("Pixel   size(btye) : %d\n", (int)mat_pixel_size(mat));
}
