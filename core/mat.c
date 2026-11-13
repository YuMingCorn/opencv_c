#include "mat.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Returns 1 and stores a*b in *out, or 0 on size_t overflow.
static int mul_size(size_t a, size_t b, size_t *out) {
    if (a != 0 && b > SIZE_MAX / a) {
        return 0;
    }
    *out = a * b;
    return 1;
}

static struct Mat *mat_create(int rows, int cols, int type) {
    if (rows < 0 || cols < 0) {
        return NULL;
    }
    type              = CV_MAT_TYPE(type);
    size_t pixel_size = CV_PIXEL_SIZE(type);
    size_t row_bytes  = 0;
    size_t total      = 0;
    if (!mul_size(pixel_size, (size_t)cols, &row_bytes) ||
        !mul_size(row_bytes, (size_t)rows, &total)) {
        return NULL;
    }

    struct Mat *mat = (struct Mat *)malloc(sizeof(struct Mat));
    if (mat == NULL) {
        return NULL;
    }
    mat->flags = (size_t)(type & CV_MAT_TYPE_MASK) | MAGIC_VAL;
    mat->dims  = 2;
    mat->rows  = rows;
    mat->cols  = cols;
    mat->size  = (size_t *)fast_malloc(2 * sizeof(size_t));
    mat->step  = (size_t *)fast_malloc(2 * sizeof(size_t));
    mat->data  = (uchar *)fast_malloc(total);
    if (mat->size == NULL || mat->step == NULL || mat->data == NULL) {
        fast_free(mat->size);
        fast_free(mat->step);
        fast_free(mat->data);
        free(mat);
        return NULL;
    }
    mat->size[0] = (size_t)rows;
    mat->size[1] = (size_t)cols;
    mat->step[0] = row_bytes;
    mat->step[1] = pixel_size;
    return mat;
}

struct Mat *alloc_mat(int rows, int cols, int type) {
    struct Mat *mat = mat_create(rows, cols, type);
    ASSERT(mat != NULL);
    return mat;
}

struct Mat *alloc_mat_synthetic(int rows, int cols) {
    struct Mat *mat = alloc_mat(rows, cols, CV_8UC3);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            uchar *p = MAT_AT_ELEM_PTR(*mat, uchar, i, j);
            p[0]     = (uchar)(j * 255 / (cols > 1 ? cols - 1 : 1));
            p[1]     = (uchar)(i * 255 / (rows > 1 ? rows - 1 : 1));
            p[2]     = (uchar)((i + j) * 255 /
                               (rows + cols > 2 ? rows + cols - 2 : 1));
        }
    }
    return mat;
}

void free_mat(struct Mat **mat) {
    if (mat && *mat) {
        fast_free((*mat)->size);
        fast_free((*mat)->step);
        fast_free((*mat)->data);
        free(*mat);
        *mat = NULL;
    }
    return;
}

void *mat_at(const struct Mat *mat, int row, int col) {
    ASSERT(mat && row >= 0 && row < mat->rows && col >= 0 && col < mat->cols);
    return (void *)(mat->data + mat->step[0] * row + mat->step[1] * col);
}

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

size_t mat_depth(const struct Mat *self) { return CV_MAT_DEPTH(self->flags); }

size_t mat_channel_size(const struct Mat *self) {
    return CV_CH_SIZE(self->flags);
}

size_t mat_channel_num(const struct Mat *self) {
    return CV_MAT_CN(self->flags);
}

size_t mat_pixel_size(const struct Mat *self) {
    return CV_PIXEL_SIZE(self->flags);
}

size_t mat_type(const struct Mat *self) { return CV_MAT_TYPE(self->flags); }