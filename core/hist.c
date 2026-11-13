#include "hist.h"

#include <string.h>

#include "improc_internal.h"
#include "pixel.h"

static void check_u8c1(const struct Mat *m) {
    ASSERT(m != NULL);
    ASSERT(mat_type(m) == (size_t)CV_8UC1);
    ASSERT(m->rows >= 0 && m->cols >= 0);
    ASSERT((uint64_t)(uint32_t)m->rows * (uint64_t)(uint32_t)m->cols <=
           (uint64_t)HIST_MAX_PIXELS);
}

void calc_hist(uint32_t hist[256], const struct Mat *src) {
    ASSERT(hist != NULL);
    check_u8c1(src);
    memset(hist, 0, 256 * sizeof hist[0]);
    for (int y = 0; y < src->rows; y++) {
        const uchar *row = mat_row(src, y);
        for (int x = 0; x < src->cols; x++) {
            hist[row[x]]++;
        }
    }
}

void equalize_hist(struct Mat *dst, const struct Mat *src) {
    check_u8c1(src);
    ASSERT(dst != NULL);
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(mat_type(dst) == (size_t)CV_8UC1);
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);
    if (src->rows == 0 || src->cols == 0) {
        return;
    }

    // Scratch: histogram (1024 bytes) followed by the LUT (256 bytes).
    uint32_t *hist = fast_malloc(HIST_EQUALIZE_SCRATCH);
    improc_scratch_check((void *const[]){hist}, 1);
    uchar *lut = (uchar *)(hist + 256);
    calc_hist(hist, src);

    uint32_t total = (uint32_t)src->rows * (uint32_t)src->cols;
    int      m     = 0;
    while (hist[m] == 0) {
        m++;
    }
    uint32_t den = total - hist[m];
    if (den == 0) {
        for (int i = 0; i < 256; i++) {
            lut[i] = (uchar)i;
        }
    } else {
        uint32_t sum = 0;
        for (int v = 0; v <= m; v++) {
            lut[v] = 0;
        }
        for (int v = m + 1; v < 256; v++) {
            sum += hist[v];
            // sum <= 2^24 - 1, so sum * 255 + den / 2 < 2^32.
            lut[v] = (uchar)((sum * 255u + den / 2u) / den);
        }
    }
    mat_lut(dst, src, lut);
    fast_free(hist);
}

// 64 x 64 -> 128 bit product from four 32-bit multiplies. Portable C: no
// __int128, so it builds on AVR and Cortex-M0+. Declared in improc_internal.h.
struct improc_u128 improc_mul_u64(uint64_t a, uint64_t b) {
    uint64_t a_lo = a & 0xFFFFFFFFu, a_hi = a >> 32;
    uint64_t b_lo = b & 0xFFFFFFFFu, b_hi = b >> 32;
    uint64_t ll = a_lo * b_lo;
    uint64_t lh = a_lo * b_hi;
    uint64_t hl = a_hi * b_lo;
    uint64_t hh = a_hi * b_hi;
    // mid < 3 * 2^32, no overflow.
    uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFFu) + (hl & 0xFFFFFFFFu);
    struct improc_u128 r;
    r.lo = (ll & 0xFFFFFFFFu) | (mid << 32);
    r.hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
    return r;
}

int improc_u128_greater(struct improc_u128 a, struct improc_u128 b) {
    return a.hi > b.hi || (a.hi == b.hi && a.lo > b.lo);
}

uchar otsu_threshold_hist(const uint32_t hist[256]) {
    ASSERT(hist != NULL);
    uint64_t total = 0;
    uint64_t sum   = 0;
    for (int i = 0; i < 256; i++) {
        total += hist[i];
        sum += (uint64_t)hist[i] * (uint64_t)i;
    }
    ASSERT(total <= (uint64_t)HIST_MAX_PIXELS);

    // f(t) = s0^2 / w0 + s1^2 / w1 = best_int + best_num / best_den.
    int      found    = 0;
    uchar    best_t   = 0;
    uint64_t best_int = 0;
    uint64_t best_num = 0;
    uint64_t best_den = 1;
    uint64_t w0       = 0;
    uint64_t s0       = 0;
    for (int t = 0; t < 255; t++) {
        w0 += hist[t];
        s0 += (uint64_t)hist[t] * (uint64_t)t;
        uint64_t w1 = total - w0;
        if (w0 == 0) {
            continue;
        }
        if (w1 == 0) {
            break;
        }
        uint64_t s1 = sum - s0;
        // s0, s1 < 2^32, so the squares fit uint64_t.
        uint64_t sq0   = s0 * s0;
        uint64_t sq1   = s1 * s1;
        uint64_t q0    = sq0 / w0;
        uint64_t r0    = sq0 % w0;
        uint64_t q1    = sq1 / w1;
        uint64_t r1    = sq1 % w1;
        uint64_t den   = w0 * w1;           // < 2^46
        uint64_t num   = r0 * w1 + r1 * w0; // < 2^47
        uint64_t whole = q0 + q1 + num / den;
        num %= den;

        int better = 0;
        if (!found || whole > best_int) {
            better = 1;
        } else if (whole == best_int) {
            // num / den > best_num / best_den
            better = improc_u128_greater(improc_mul_u64(num, best_den),
                                         improc_mul_u64(best_num, den));
        }
        if (better) {
            found    = 1;
            best_t   = (uchar)t;
            best_int = whole;
            best_num = num;
            best_den = den;
        }
    }
    return found ? best_t : 0;
}

uchar otsu_threshold(const struct Mat *src) {
    check_u8c1(src); // before the allocation: a failed ASSERT may longjmp
    uint32_t *hist = fast_malloc(HIST_OTSU_SCRATCH);
    improc_scratch_check((void *const[]){hist}, 1);
    calc_hist(hist, src);
    uchar t = otsu_threshold_hist(hist);
    fast_free(hist);
    return t;
}
