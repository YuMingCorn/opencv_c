#include "mat.h"

#include <limits.h>
#include <stdint.h>

#include "utils.h"

#ifndef IMPROC_NO_MALLOC
// Returns 1 and stores a*b in *out, or 0 on size_t overflow.
static int mul_size(size_t a, size_t b, size_t *out) {
    if (a != 0 && b > SIZE_MAX / a) {
        return 0;
    }
    *out = a * b;
    return 1;
}

struct Mat *mat_create(int rows, int cols, int type) {
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

    struct Mat *mat = (struct Mat *)fast_malloc(sizeof(struct Mat));
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
        fast_free(mat);
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
        fast_free(*mat);
        *mat = NULL;
    }
    return;
}
#endif // IMPROC_NO_MALLOC

void *mat_at(const struct Mat *mat, int row, int col) {
    ASSERT(mat && row >= 0 && row < mat->rows && col >= 0 && col < mat->cols);
    return (void *)(mat->data + mat->step[0] * row + mat->step[1] * col);
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
