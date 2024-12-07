#ifndef MAT_H_
#define MAT_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define MAGIC_VAL 0x42FF0000
#define CV_8U     0 // 8-bit unsigned
#define CV_8S     1 // 8-bit signed
#define CV_16U    2 // 16-bit unsigned
#define CV_16S    3 // 16-bit signed
#define CV_32S    4 // 32-bit signed
#define CV_32F    5 // 32-bit float
#define CV_64F    6 // 64-bit float
#define CV_16F    7 // 16-bit float

#define CV_CN_SHIFT 3   // channel shift
#define CV_CN_MAX   512 // max channel num (0 ~ 511)

#define CV_DEPTH_MAX (1 << CV_CN_SHIFT) // (b 0111, d 8)

/**
    Mask: 4095 (binary 1111 1111 1111)
 */
#define CV_MAT_TYPE_MASK   (CV_DEPTH_MAX * CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags) ((flags)&CV_MAT_TYPE_MASK)

/**
    Mask to extract depth information (binary 0111, decimal 7)
 */
#define CV_MAT_DEPTH_MASK   (CV_DEPTH_MAX - 1)
#define CV_MAT_DEPTH(flags) ((flags)&CV_MAT_DEPTH_MASK)

/**
    Channel mask: 4088 (binary 1111 1111 1000)
*/
#define CV_MAT_CN_MASK ((CV_CN_MAX - 1) << CV_CN_SHIFT)

/**
    Shift right to isolate channel bits, then add 1 to get channel count
*/
#define CV_MAT_CN(flags) ((((flags)&CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)

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
#define CV_MAKETYPE(depth, cn) (CV_MAT_DEPTH(depth) + (((cn)-1) << CV_CN_SHIFT))

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

struct Mat {
    /*! includes several bit-fields:
         - the magic signature
         - continuity flag
         - depth
         - number of channels (0~)
     */
    size_t flags;
    //! the matrix dimensionality, >= 2
    size_t dims;
    //! number of rows, colums
    int rows;
    int cols;
    //! pointer to the array of size and step for n-dim array
    size_t *size;
    size_t *step;
    //! pointer to the data
    uchar *data;
};

struct Mat *alloc_mat(int rows, int cols, int type);

// TODO: Not implementation
struct Mat *alloc_mat_from_data(int rows, int cols, uchar *data);

struct Mat *alloc_mat_from_buffer(uchar *buffer, size_t buffer_size);
struct Mat *alloc_mat_from_file(char const *filename);

void mat_write(char const *filename, struct Mat *mat);

void free_mat(struct Mat **mat);

// mat index
void *mat_at(struct Mat *mat, int row, int col);

void mat_info(struct Mat *mat);

// Mat attribute
size_t mat_depth(struct Mat *self);
size_t mat_channel_size(struct Mat *self);
size_t mat_channel_num(struct Mat *self);
size_t mat_pixel_size(struct Mat *self);
size_t mat_type(struct Mat *self);

#endif // MAT_H_