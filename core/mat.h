#ifndef MAT_H_
#define MAT_H_

#include <stddef.h>
#include <stdint.h>

#define MAGIC_VAL     0x42FF0000UL
#define MAT_FLAG_HEAP 0x1000UL // set by alloc_mat; outside CV_MAT_TYPE_MASK
#define CV_8U         0        // 8-bit unsigned
#define CV_8S         1        // 8-bit signed
#define CV_16U        2        // 16-bit unsigned
#define CV_16S        3        // 16-bit signed
#define CV_32S        4        // 32-bit signed
#define CV_32F        5        // 32-bit float
#define CV_64F        6        // 64-bit float
#define CV_16F        7        // 16-bit float

#define CV_CN_SHIFT 3   // channel shift
#define CV_CN_MAX   512 // max channel num (0 ~ 511)

#define CV_DEPTH_MAX (1 << CV_CN_SHIFT) // (b 0111, d 8)

/**
    Mask: 4095 (binary 1111 1111 1111)
 */
#define CV_MAT_TYPE_MASK   (CV_DEPTH_MAX * CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags) ((flags) & CV_MAT_TYPE_MASK)

/**
    Mask to extract depth information (binary 0111, decimal 7)
 */
#define CV_MAT_DEPTH_MASK   (CV_DEPTH_MAX - 1)
#define CV_MAT_DEPTH(flags) ((flags) & CV_MAT_DEPTH_MASK)

/**
    Channel mask: 4088 (binary 1111 1111 1000)
*/
#define CV_MAT_CN_MASK ((CV_CN_MAX - 1) << CV_CN_SHIFT)

/**
    Shift right to isolate channel bits, then add 1 to get channel count
*/
#define CV_MAT_CN(flags) ((((flags) & CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)

/**
   Size of each channel item,
   0x28442211 = 0010 1000 0100 0100 0010 0010 0001 0001
   Depth      =    7    6    5    4    3    2    1    0
   array of sizeof(arr_type_elem)
*/
#define CV_CH_SIZE(type) ((0x28442211 >> (CV_MAT_DEPTH(type) * 4)) & 15)

/**
    Total size (per pixel) = channels * size of each channel element
 */
#define CV_PIXEL_SIZE(type) (CV_MAT_CN(type) * CV_CH_SIZE(type))

/**
    Stores depth in the lowest 3 bits and channel count shifted to higher bits.
    This allows us to represent image type in a single integer containing both
    depth and channel information.
*/
#define CV_MAKETYPE(depth, cn)                                                 \
    (CV_MAT_DEPTH(depth) + (((cn) - 1) << CV_CN_SHIFT))

/**
 create cv::type
 */
#define CV_8UC1 CV_MAKETYPE(CV_8U, 1) // 0  (0000 | 0000)
#define CV_8UC2 CV_MAKETYPE(CV_8U, 2) // 8  (0000 | 1000)
#define CV_8UC3 CV_MAKETYPE(CV_8U, 3) // 16 (0001 | 0000)
#define CV_8UC4 CV_MAKETYPE(CV_8U, 4) // 24 (0010 | 0000)

#define CV_8SC1 CV_MAKETYPE(CV_8S, 1)
#define CV_8SC2 CV_MAKETYPE(CV_8S, 2)
#define CV_8SC3 CV_MAKETYPE(CV_8S, 3)
#define CV_8SC4 CV_MAKETYPE(CV_8S, 4)

#define CV_16UC1 CV_MAKETYPE(CV_16U, 1)
#define CV_16UC2 CV_MAKETYPE(CV_16U, 2)
#define CV_16UC3 CV_MAKETYPE(CV_16U, 3)
#define CV_16UC4 CV_MAKETYPE(CV_16U, 4)

#define CV_16SC1 CV_MAKETYPE(CV_16S, 1)
#define CV_16SC2 CV_MAKETYPE(CV_16S, 2)
#define CV_16SC3 CV_MAKETYPE(CV_16S, 3)
#define CV_16SC4 CV_MAKETYPE(CV_16S, 4)

#define CV_32FC1 CV_MAKETYPE(CV_32F, 1)
#define CV_32FC2 CV_MAKETYPE(CV_32F, 2)
#define CV_32FC3 CV_MAKETYPE(CV_32F, 3)
#define CV_32FC4 CV_MAKETYPE(CV_32F, 4)

#define CV_64FC1 CV_MAKETYPE(CV_64F, 1)
#define CV_64FC2 CV_MAKETYPE(CV_64F, 2)
#define CV_64FC3 CV_MAKETYPE(CV_64F, 3)
#define CV_64FC4 CV_MAKETYPE(CV_64F, 4)

/*! The Macros will cause the index speed to be slow
    because it needs to move the pointer from original position each time
*/
#define MAT_AT(mat, row, col)                                                  \
    ((mat).data + (mat).step[0] * (row) + (mat).step[1] * (col))

#define MAT_AT_ELEM_PTR(mat, type, row, col) ((type *)(MAT_AT(mat, row, col)))

#define MAT_AT_ELEM(mat, type, row, col) (*((type *)(MAT_AT(mat, row, col))))

typedef unsigned char uchar;

// Total bytes of a rows x cols image of the given type. size_t arithmetic.
#define MAT_DATA_SIZE(rows, cols, type)                                        \
    ((size_t)(rows) * (size_t)(cols) * CV_PIXEL_SIZE(type))

struct Mat {
    /*! includes several bit-fields:
         - the magic signature
         - MAT_FLAG_HEAP when alloc_mat owns the data
         - depth
         - number of channels (0~)
     */
    uint32_t flags;
    //! the matrix dimensionality, >= 2
    int dims;
    //! number of rows, colums
    int rows;
    int cols;
    //! size and step per dimension, stored in the header
    size_t size[2];
    size_t step[2];
    //! pointer to the data
    uchar *data;
};

// Builds a header over caller-owned memory. Nothing is allocated, and the
// header must not be passed to free_mat. Returns 0, or -1 when rows or cols is
// negative, the size overflows size_t, cols * channels exceeds INT_MAX, or
// data_size is smaller than the image. ASSERTs mat != NULL and that data is
// aligned to the channel size.
int mat_init(struct Mat *mat, int rows, int cols, int type, void *data,
             size_t data_size);

// One pixel of the alloc_mat_synthetic pattern for an image of rows x cols.
// 0 <= i < rows and 0 <= j < cols. Exposed so tests can reach extreme sizes
// without allocating them.
void mat_synthetic_pixel(int rows, int cols, int i, int j, uchar out[3]);

#ifndef IMPROC_NO_MALLOC
// Allocates a Mat that owns its data. Returns NULL on invalid size or
// allocation failure.
struct Mat *mat_create(int rows, int cols, int type);

// Same as mat_create, but ASSERTs instead of returning NULL.
struct Mat *alloc_mat(int rows, int cols, int type);

// CV_8UC3 test pattern: R ramps by column, G by row, B along the diagonal
struct Mat *alloc_mat_synthetic(int rows, int cols);

void free_mat(struct Mat **mat);
#endif

// mat index
static inline uchar *mat_row(const struct Mat *m, int row) {
    return m->data + m->step[0] * (size_t)row;
}

void *mat_at(const struct Mat *mat, int row, int col);

// Mat attribute
size_t mat_depth(const struct Mat *self);
size_t mat_channel_size(const struct Mat *self);
size_t mat_channel_num(const struct Mat *self);
size_t mat_pixel_size(const struct Mat *self);
size_t mat_type(const struct Mat *self);

#endif // MAT_H_