#include "improc.h"
#include "kodak.h"

#if IMPROC_HAS_FLOAT

// u8 -> f32 (x / 255) -> u8 (x * 255). The result must equal the source image,
// so the golden hash of this op equals the hash of "src" for every image.
static struct Mat *op_roundtrip_identity(const struct Mat *rgb) {
    struct Mat *mid = alloc_mat(rgb->rows, rgb->cols, CV_32FC3);
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    convert_to(mid, rgb, 1.0f / 255.0f, 0.0f);
    convert_to(out, mid, 255.0f, 0.0f);
    free_mat(&mid);
    return out;
}

// The float intermediate itself, to pin its exact bits across platforms.
static struct Mat *op_unit_f32(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_32FC3);
    convert_to(out, rgb, 1.0f / 255.0f, 0.0f);
    return out;
}

const struct KodakOp kodak_ops[] = {
    {"roundtrip_identity", op_roundtrip_identity},
    {"unit_f32", op_unit_f32},
};

#else

// Float support is off: the manifest cannot match, so the test is not
// registered for such builds in practice (IMPROC_CORE_ONLY skips tests).
static struct Mat *op_unavailable(const struct Mat *rgb) {
    return alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
}

const struct KodakOp kodak_ops[] = {{"unavailable", op_unavailable}};

#endif

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
