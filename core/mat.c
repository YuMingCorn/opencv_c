#include "mat.h"

#include "utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Mat *alloc_mat(int rows, int cols, int type) {
    struct Mat *mat = (struct Mat *)malloc(sizeof(struct Mat));
    ASSERT(mat != NULL);
    type       = CV_MAT_TYPE(type);
    mat->flags = (type & CV_MAT_TYPE_MASK) | MAGIC_VAL;
    mat->dims  = 2;
    mat->rows  = rows;
    mat->cols  = cols;

    mat->size = (size_t *)fast_malloc(2 * sizeof(size_t));
    mat->step = (size_t *)fast_malloc(2 * sizeof(size_t));
    assert(mat->size != NULL && mat->step != NULL);

    mat->size[0] = rows;
    mat->size[1] = cols;

    size_t pixel_size = CV_PIXEL_SIZE(mat->flags);
    mat->step[0]      = pixel_size * cols;
    mat->step[1]      = pixel_size;
    mat->data         = (uchar *)fast_malloc(pixel_size * rows * cols);

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

void *mat_at(struct Mat *mat, int row, int col) {
    ASSERT(mat && row >= 0 && row < mat->rows && col >= 0 && col < mat->cols);
    return (void *)(mat->data + mat->step[0] * row + mat->step[1] * col);
}

struct Mat *alloc_mat_from_buffer(uchar *buffer, size_t buffer_size) {
    int width = 0, height = 0, channels = 0;
    // It's may be will cause some problem
    uchar *data = (uchar *)stbi_load_from_memory(buffer, (int)buffer_size,
                                                 &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load image from buffer\n");
        free(data);
        return NULL;
    }
    int type = 0;
    if (channels == 1) {
        type = CV_8UC1;
    } else if (channels == 3) {
        type = CV_8UC3;
    } else {
        fprintf(stderr, "Unsupported channel count: %d\n", channels);
        stbi_image_free(data);
        return NULL;
    }
    struct Mat *mat = (struct Mat *)malloc(sizeof(struct Mat));
    ASSERT(mat != NULL);
    mat->rows  = height;
    mat->cols  = width;
    mat->flags = (type & CV_MAT_TYPE_MASK) | MAGIC_VAL;
    mat->dims  = 2;

    mat->size = (size_t *)fast_malloc(2 * sizeof(size_t));
    mat->step = (size_t *)fast_malloc(2 * sizeof(size_t));

    mat->size[0] = mat->rows;
    mat->size[1] = mat->cols;

    size_t pixel_size = (size_t)CV_PIXEL_SIZE(type);
    mat->step[0]      = pixel_size * mat->cols;
    mat->step[1]      = pixel_size;

    // copy and free
    mat->data       = (uchar *)fast_malloc(pixel_size * mat->rows * mat->cols);
    int total_bytes = (int)pixel_size * mat->rows * mat->cols;
    for (int i = 0; i < total_bytes; i++) {
        mat->data[i] = data[i];
    }
    stbi_image_free(data);
    return mat;
}

struct Mat *alloc_mat_from_file(const char *filename) {
    int    width = 0, height = 0, channels = 0;
    uchar *data = stbi_load(filename, &width, &height, &channels, 0);
    if (data == NULL) {
        printf("Error in loading the image\n");
        return NULL;
    } else {
        printf("Image successfully loading from %s\n", filename);
    }

    struct Mat *mat = (struct Mat *)malloc(sizeof(struct Mat));
    ASSERT(mat != NULL);

    int type = 0;
    if (channels == 1) {
        type = CV_8UC1;
    } else if (channels == 3) {
        type = CV_8UC3;
    } else {
        fprintf(stderr, "Unsupported channel count: %d\n", channels);
        stbi_image_free(data);
        fast_free(mat);
        return NULL;
    }
    mat->rows  = height;
    mat->cols  = width;
    mat->flags = (type & CV_MAT_TYPE_MASK) | MAGIC_VAL;
    mat->dims  = 2;

    mat->size = (size_t *)fast_malloc(2 * sizeof(size_t));
    mat->step = (size_t *)fast_malloc(2 * sizeof(size_t));

    mat->size[0] = mat->rows;
    mat->size[1] = mat->cols;

    size_t pixel_size = (size_t)CV_PIXEL_SIZE(type);
    mat->step[0]      = pixel_size * mat->cols;
    mat->step[1]      = pixel_size;

    // copy and free
    mat->data       = (uchar *)fast_malloc(pixel_size * mat->rows * mat->cols);
    int total_bytes = (int)pixel_size * mat->rows * mat->cols;
    for (int i = 0; i < total_bytes; i++) {
        mat->data[i] = data[i];
    }
    stbi_image_free(data);
    return mat;
}

void mat_write(const char *filename, struct Mat *mat) {
    if (stbi_write_jpg(filename, mat->cols, mat->rows,
                       (int)mat_channel_num(mat), mat->data, 100)) {
        printf("Image successfully written to %s\n", filename);
    } else {
        fprintf(stderr, "Failed to write image %s\n", filename);
    }
}

void mat_info(struct Mat *mat) {
    printf("Size:              : [%d x %d]\n", (int)mat->rows, (int)mat->cols);
    printf("Channel num        : %d\n", (int)mat_channel_num(mat));
    printf("Channel size(byte) : %d\n", (int)mat_channel_size(mat));
    printf("Pixel   size(btye) : %d\n", (int)mat_pixel_size(mat));
}

size_t mat_depth(struct Mat *self) { return CV_MAT_DEPTH(self->flags); }

size_t mat_channel_size(struct Mat *self) { return CV_CH_SIZE(self->flags); }

size_t mat_channel_num(struct Mat *self) { return CV_MAT_CN(self->flags); }

size_t mat_pixel_size(struct Mat *self) { return CV_PIXEL_SIZE(self->flags); }

size_t mat_type(struct Mat *self) { return CV_MAT_TYPE(self->flags); }