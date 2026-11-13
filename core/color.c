#include "color.h"

#include <stdint.h>

// Largest value is (77 + 150 + 29) * 255 + 128 = 256 * 255 + 128 = 65408, which
// fits a 16-bit unsigned, so this is safe where int is 16 bits.
static inline uchar luma(unsigned r, unsigned g, unsigned b) {
    return (uchar)((77u * r + 150u * g + 29u * b + 128u) >> 8);
}

static void rgb2gray_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3) {
        d[x] = luma(s[0], s[1], s[2]);
    }
}

static void bgr2gray_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3) {
        d[x] = luma(s[2], s[1], s[0]);
    }
}

static void rgba2gray_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, s += 4) {
        d[x] = luma(s[0], s[1], s[2]);
    }
}

static void bgra2gray_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, s += 4) {
        d[x] = luma(s[2], s[1], s[0]);
    }
}

static void gray2rgb_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, d += 3) {
        d[0] = d[1] = d[2] = s[x];
    }
}

static void gray2rgba_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, d += 4) {
        d[0] = d[1] = d[2] = s[x];
        d[3]               = 255;
    }
}

static void rgb2bgr_row(const uchar *restrict s, uchar *restrict d, int width) {
    for (int x = 0; x < width; x++, s += 3, d += 3) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
    }
}

static void rgba2bgra_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, s += 4, d += 4) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
        d[3] = s[3];
    }
}

static void rgb2rgba_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3, d += 4) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = 255;
    }
}

static void rgba2rgb_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 4, d += 3) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
    }
}

static void rgb2bgra_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3, d += 4) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
        d[3] = 255;
    }
}

static void rgba2bgr_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 4, d += 3) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
    }
}

// ---- HSV (H 0..179, S and V 0..255) ----------------------------------------
// RGB to HSV uses two reciprocal tables in Q12 so the row kernel has no
// division. sdiv_table[i] = round(255 * 4096 / i) and
// hdiv_table[i] = round(30 * 4096 / i), with entry 0 set to 0 (a zero
// divisor then yields 0 after the rounding term). The tables are static
// const; on AVR they occupy RAM (2 KB) unless the port moves them to
// PROGMEM.

static const uint32_t sdiv_table[256] = {
    0u, 1044480u, 522240u, 348160u, 261120u, 208896u,
    174080u, 149211u, 130560u, 116053u, 104448u, 94953u,
    87040u, 80345u, 74606u, 69632u, 65280u, 61440u,
    58027u, 54973u, 52224u, 49737u, 47476u, 45412u,
    43520u, 41779u, 40172u, 38684u, 37303u, 36017u,
    34816u, 33693u, 32640u, 31651u, 30720u, 29842u,
    29013u, 28229u, 27486u, 26782u, 26112u, 25475u,
    24869u, 24290u, 23738u, 23211u, 22706u, 22223u,
    21760u, 21316u, 20890u, 20480u, 20086u, 19707u,
    19342u, 18991u, 18651u, 18324u, 18008u, 17703u,
    17408u, 17123u, 16846u, 16579u, 16320u, 16069u,
    15825u, 15589u, 15360u, 15137u, 14921u, 14711u,
    14507u, 14308u, 14115u, 13926u, 13743u, 13565u,
    13391u, 13221u, 13056u, 12895u, 12738u, 12584u,
    12434u, 12288u, 12145u, 12006u, 11869u, 11736u,
    11605u, 11478u, 11353u, 11231u, 11111u, 10995u,
    10880u, 10768u, 10658u, 10550u, 10445u, 10341u,
    10240u, 10141u, 10043u, 9947u, 9854u, 9761u,
    9671u, 9582u, 9495u, 9410u, 9326u, 9243u,
    9162u, 9082u, 9004u, 8927u, 8852u, 8777u,
    8704u, 8632u, 8561u, 8492u, 8423u, 8356u,
    8290u, 8224u, 8160u, 8097u, 8034u, 7973u,
    7913u, 7853u, 7795u, 7737u, 7680u, 7624u,
    7569u, 7514u, 7461u, 7408u, 7355u, 7304u,
    7253u, 7203u, 7154u, 7105u, 7057u, 7010u,
    6963u, 6917u, 6872u, 6827u, 6782u, 6739u,
    6695u, 6653u, 6611u, 6569u, 6528u, 6487u,
    6447u, 6408u, 6369u, 6330u, 6292u, 6254u,
    6217u, 6180u, 6144u, 6108u, 6073u, 6037u,
    6003u, 5968u, 5935u, 5901u, 5868u, 5835u,
    5803u, 5771u, 5739u, 5708u, 5677u, 5646u,
    5615u, 5585u, 5556u, 5526u, 5497u, 5468u,
    5440u, 5412u, 5384u, 5356u, 5329u, 5302u,
    5275u, 5249u, 5222u, 5196u, 5171u, 5145u,
    5120u, 5095u, 5070u, 5046u, 5022u, 4998u,
    4974u, 4950u, 4927u, 4904u, 4881u, 4858u,
    4836u, 4813u, 4791u, 4769u, 4748u, 4726u,
    4705u, 4684u, 4663u, 4642u, 4622u, 4601u,
    4581u, 4561u, 4541u, 4522u, 4502u, 4483u,
    4464u, 4445u, 4426u, 4407u, 4389u, 4370u,
    4352u, 4334u, 4316u, 4298u, 4281u, 4263u,
    4246u, 4229u, 4212u, 4195u, 4178u, 4161u,
    4145u, 4128u, 4112u, 4096u,
};

static const uint32_t hdiv_table[256] = {
    0u, 122880u, 61440u, 40960u, 30720u, 24576u,
    20480u, 17554u, 15360u, 13653u, 12288u, 11171u,
    10240u, 9452u, 8777u, 8192u, 7680u, 7228u,
    6827u, 6467u, 6144u, 5851u, 5585u, 5343u,
    5120u, 4915u, 4726u, 4551u, 4389u, 4237u,
    4096u, 3964u, 3840u, 3724u, 3614u, 3511u,
    3413u, 3321u, 3234u, 3151u, 3072u, 2997u,
    2926u, 2858u, 2793u, 2731u, 2671u, 2614u,
    2560u, 2508u, 2458u, 2409u, 2363u, 2318u,
    2276u, 2234u, 2194u, 2156u, 2119u, 2083u,
    2048u, 2014u, 1982u, 1950u, 1920u, 1890u,
    1862u, 1834u, 1807u, 1781u, 1755u, 1731u,
    1707u, 1683u, 1661u, 1638u, 1617u, 1596u,
    1575u, 1555u, 1536u, 1517u, 1499u, 1480u,
    1463u, 1446u, 1429u, 1412u, 1396u, 1381u,
    1365u, 1350u, 1336u, 1321u, 1307u, 1293u,
    1280u, 1267u, 1254u, 1241u, 1229u, 1217u,
    1205u, 1193u, 1182u, 1170u, 1159u, 1148u,
    1138u, 1127u, 1117u, 1107u, 1097u, 1087u,
    1078u, 1069u, 1059u, 1050u, 1041u, 1033u,
    1024u, 1016u, 1007u, 999u, 991u, 983u,
    975u, 968u, 960u, 953u, 945u, 938u,
    931u, 924u, 917u, 910u, 904u, 897u,
    890u, 884u, 878u, 871u, 865u, 859u,
    853u, 847u, 842u, 836u, 830u, 825u,
    819u, 814u, 808u, 803u, 798u, 793u,
    788u, 783u, 778u, 773u, 768u, 763u,
    759u, 754u, 749u, 745u, 740u, 736u,
    731u, 727u, 723u, 719u, 714u, 710u,
    706u, 702u, 698u, 694u, 690u, 686u,
    683u, 679u, 675u, 671u, 668u, 664u,
    661u, 657u, 654u, 650u, 647u, 643u,
    640u, 637u, 633u, 630u, 627u, 624u,
    621u, 617u, 614u, 611u, 608u, 605u,
    602u, 599u, 597u, 594u, 591u, 588u,
    585u, 582u, 580u, 577u, 574u, 572u,
    569u, 566u, 564u, 561u, 559u, 556u,
    554u, 551u, 549u, 546u, 544u, 541u,
    539u, 537u, 534u, 532u, 530u, 527u,
    525u, 523u, 521u, 518u, 516u, 514u,
    512u, 510u, 508u, 506u, 504u, 502u,
    500u, 497u, 495u, 493u, 492u, 490u,
    488u, 486u, 484u, 482u,
};


// Every intermediate is int32_t: hn is in [-255, 1275] and hdiv up to 122880,
// so hn * hdiv stays below 2^28. Negative values are never shifted.
static inline void rgb2hsv_px(int32_t r, int32_t g, int32_t b, uchar *d) {
    int32_t v    = r;
    int32_t minc = r;
    if (g > v) {
        v = g;
    }
    if (b > v) {
        v = b;
    }
    if (g < minc) {
        minc = g;
    }
    if (b < minc) {
        minc = b;
    }
    int32_t diff = v - minc;
    int32_t s    = (int32_t)(((uint32_t)diff * sdiv_table[v] + 2048u) >> 12);
    int32_t hn;
    if (v == r) {
        hn = g - b;
    } else if (v == g) {
        hn = b - r + 2 * diff;
    } else {
        hn = r - g + 4 * diff;
    }
    int32_t x = hn * (int32_t)hdiv_table[diff] + 2048;
    int32_t h;
    if (x >= 0) {
        h = x >> 12;
    } else {
        h = -((-x + 4095) >> 12);
    }
    if (h < 0) {
        h += 180;
    }
    d[0] = (uchar)h;
    d[1] = (uchar)s;
    d[2] = (uchar)v;
}

// Products reach 255 * 7650 = 1950750, which needs 32 bits (AVR int is 16).
static inline void hsv2rgb_px(uint32_t h, uint32_t s, uint32_t v, int swap,
                              uchar *d) {
    h %= 180u;
    uint32_t sec = h / 30u;
    uint32_t f   = h - 30u * sec;
    uint32_t p   = (v * (255u - s) + 127u) / 255u;
    uint32_t q   = (v * (7650u - s * f) + 3825u) / 7650u;
    uint32_t t   = (v * (7650u - s * (30u - f)) + 3825u) / 7650u;
    uint32_t r, g, b;
    switch (sec) {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    default:
        r = v, g = p, b = q;
        break;
    }
    d[swap ? 2 : 0] = (uchar)r;
    d[1]            = (uchar)g;
    d[swap ? 0 : 2] = (uchar)b;
}

static void rgb2hsv_row(const uchar *restrict s, uchar *restrict d, int width) {
    for (int x = 0; x < width; x++, s += 3, d += 3) {
        rgb2hsv_px(s[0], s[1], s[2], d);
    }
}

static void bgr2hsv_row(const uchar *restrict s, uchar *restrict d, int width) {
    for (int x = 0; x < width; x++, s += 3, d += 3) {
        rgb2hsv_px(s[2], s[1], s[0], d);
    }
}

static void hsv2rgb_row(const uchar *restrict s, uchar *restrict d, int width) {
    for (int x = 0; x < width; x++, s += 3, d += 3) {
        hsv2rgb_px(s[0], s[1], s[2], 0, d);
    }
}

static void hsv2bgr_row(const uchar *restrict s, uchar *restrict d, int width) {
    for (int x = 0; x < width; x++, s += 3, d += 3) {
        hsv2rgb_px(s[0], s[1], s[2], 1, d);
    }
}

typedef void (*color_row_fn)(const uchar *restrict, uchar *restrict, int);

struct ColorConversion {
    int          code;
    int          scn;
    int          dcn;
    color_row_fn kernel;
};

static const struct ColorConversion conversions[] = {
    {COLOR_RGB2GRAY, 3, 1, rgb2gray_row},
    {COLOR_BGR2GRAY, 3, 1, bgr2gray_row},
    {COLOR_RGBA2GRAY, 4, 1, rgba2gray_row},
    {COLOR_BGRA2GRAY, 4, 1, bgra2gray_row},
    {COLOR_GRAY2RGB, 1, 3, gray2rgb_row},
    {COLOR_GRAY2RGBA, 1, 4, gray2rgba_row},
    {COLOR_RGB2BGR, 3, 3, rgb2bgr_row},
    {COLOR_RGBA2BGRA, 4, 4, rgba2bgra_row},
    {COLOR_RGB2RGBA, 3, 4, rgb2rgba_row},
    {COLOR_RGBA2RGB, 4, 3, rgba2rgb_row},
    {COLOR_RGB2BGRA, 3, 4, rgb2bgra_row},
    {COLOR_RGBA2BGR, 4, 3, rgba2bgr_row},
    {COLOR_RGB2HSV, 3, 3, rgb2hsv_row},
    {COLOR_BGR2HSV, 3, 3, bgr2hsv_row},
    {COLOR_HSV2RGB, 3, 3, hsv2rgb_row},
    {COLOR_HSV2BGR, 3, 3, hsv2bgr_row},
};

void cvt_color(struct Mat *dst, const struct Mat *src, int code) {
    const struct ColorConversion *conv = NULL;
    for (size_t i = 0; i < sizeof(conversions) / sizeof(conversions[0]); i++) {
        if (conversions[i].code == code) {
            conv = &conversions[i];
            break;
        }
    }
    ASSERT(conv != NULL);
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(mat_type(src) == (size_t)CV_MAKETYPE(CV_8U, conv->scn));
    ASSERT(mat_type(dst) == (size_t)CV_MAKETYPE(CV_8U, conv->dcn));
    ASSERT(src->rows == dst->rows && src->cols == dst->cols);
    if (src->rows == 0 || src->cols == 0) {
        return;
    }
    for (int y = 0; y < src->rows; y++) {
        conv->kernel(mat_row(src, y), mat_row(dst, y), src->cols);
    }
}
