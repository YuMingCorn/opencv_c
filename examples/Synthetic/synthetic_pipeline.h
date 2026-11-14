// Pipeline shared by Synthetic.ino and the host test
// tests/test_arduino_synthetic.c, so a board and a PC run the same code and
// compare against the same expected hashes.
//
//   synthetic RGB 160x120 -> gray -> 5x5 Gaussian -> Otsu -> binary
//
// The hash is FNV-1a 64 over rows, cols and type (little-endian uint32) and
// then the pixel bytes, the same hash as the Kodak tests.
#ifndef SYNTHETIC_PIPELINE_H_
#define SYNTHETIC_PIPELINE_H_

#include "improc.h"

#define SYNTH_ROWS 120
#define SYNTH_COLS 160

enum SynthStep {
    SYNTH_STEP_SOURCE,
    SYNTH_STEP_GRAY,
    SYNTH_STEP_BLUR,
    SYNTH_STEP_BINARY,
    SYNTH_STEP_COUNT
};

// Expected values, produced on the PC (Windows, Linux, qemu-aarch64 agree).
static const uint32_t synth_expected_hash_hi[SYNTH_STEP_COUNT] = {
    0x79f4763eu, 0xcbd0b7d9u, 0x26f5ae5fu, 0xe095efb4u};
static const uint32_t synth_expected_hash_lo[SYNTH_STEP_COUNT] = {
    0x350d17cau, 0x212d14f0u, 0x706f05ecu, 0x779a31abu};
#define SYNTH_EXPECTED_OTSU 127

struct SynthResult {
    uint32_t hash_hi[SYNTH_STEP_COUNT];
    uint32_t hash_lo[SYNTH_STEP_COUNT];
    uchar    otsu;
};

// The hash is computed in 64 bits and stored as two 32-bit halves, because
// printf on some Arduino cores cannot print 64-bit integers.
static inline uint64_t synth_fnv_bytes(uint64_t h, const uchar *p, size_t n) {
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

static inline uint64_t synth_fnv_u32(uint64_t h, uint32_t v) {
    uchar b[4];
    b[0] = (uchar)(v & 0xFFu);
    b[1] = (uchar)((v >> 8) & 0xFFu);
    b[2] = (uchar)((v >> 16) & 0xFFu);
    b[3] = (uchar)(v >> 24);
    return synth_fnv_bytes(h, b, 4);
}

static inline uint64_t synth_hash(const struct Mat *m) {
    uint64_t h = 14695981039346656037ull;
    h          = synth_fnv_u32(h, (uint32_t)m->rows);
    h          = synth_fnv_u32(h, (uint32_t)m->cols);
    h          = synth_fnv_u32(h, (uint32_t)mat_type(m));
    for (int y = 0; y < m->rows; y++) {
        h = synth_fnv_bytes(h, mat_row(m, y), (size_t)m->step[0]);
    }
    return h;
}

static inline void synth_store(struct SynthResult *r, int step,
                               const struct Mat *m) {
    uint64_t h       = synth_hash(m);
    r->hash_hi[step] = (uint32_t)(h >> 32);
    r->hash_lo[step] = (uint32_t)(h & 0xFFFFFFFFu);
}

// Runs the pipeline. mark(step) is called right after each step (pass NULL
// when no timing is needed). Returns 0, or -1 when an image cannot be
// allocated. The images come from mat_create, which returns NULL when memory
// runs out (alloc_mat would stop with ASSERT instead). The Gaussian's small
// scratch buffer still follows the library rule and ASSERTs on failure.
static inline int synth_run(struct SynthResult *r, void (*mark)(int step)) {
    struct Mat *rgb  = mat_create(SYNTH_ROWS, SYNTH_COLS, CV_8UC3);
    struct Mat *gray = mat_create(SYNTH_ROWS, SYNTH_COLS, CV_8UC1);
    struct Mat *blur = mat_create(SYNTH_ROWS, SYNTH_COLS, CV_8UC1);
    struct Mat *bin  = mat_create(SYNTH_ROWS, SYNTH_COLS, CV_8UC1);
    int         rc   = -1;
    if (rgb && gray && blur && bin) {
        for (int i = 0; i < SYNTH_ROWS; i++) {
            uchar *row = mat_row(rgb, i);
            for (int j = 0; j < SYNTH_COLS; j++) {
                mat_synthetic_pixel(SYNTH_ROWS, SYNTH_COLS, i, j, row + 3 * j);
            }
        }
        if (mark) {
            mark(SYNTH_STEP_SOURCE);
        }
        cvt_color(gray, rgb, COLOR_RGB2GRAY);
        if (mark) {
            mark(SYNTH_STEP_GRAY);
        }
        gaussian_blur(blur, gray, 5);
        if (mark) {
            mark(SYNTH_STEP_BLUR);
        }
        r->otsu = otsu_threshold(blur);
        threshold(bin, blur, r->otsu, 255, THRESH_BINARY);
        if (mark) {
            mark(SYNTH_STEP_BINARY);
        }
        synth_store(r, SYNTH_STEP_SOURCE, rgb);
        synth_store(r, SYNTH_STEP_GRAY, gray);
        synth_store(r, SYNTH_STEP_BLUR, blur);
        synth_store(r, SYNTH_STEP_BINARY, bin);
        rc = 0;
    }
    if (bin) {
        free_mat(&bin);
    }
    if (blur) {
        free_mat(&blur);
    }
    if (gray) {
        free_mat(&gray);
    }
    if (rgb) {
        free_mat(&rgb);
    }
    return rc;
}

// 1 when every hash and the Otsu threshold match the expected values.
static inline int synth_matches(const struct SynthResult *r) {
    for (int i = 0; i < SYNTH_STEP_COUNT; i++) {
        if (r->hash_hi[i] != synth_expected_hash_hi[i] ||
            r->hash_lo[i] != synth_expected_hash_lo[i]) {
            return 0;
        }
    }
    return r->otsu == SYNTH_EXPECTED_OTSU;
}

#endif // SYNTHETIC_PIPELINE_H_
