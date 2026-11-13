#include <stdio.h>
#include <time.h>

#include "improc.h"
#include "mat.h"

#define IMAGE_SIZE 512
#define ITERATIONS 100

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void run(const char *name, int code, struct Mat *dst, struct Mat *src) {
    double start = now_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        cvt_color(dst, src, code);
    }
    printf("%-10s %10.4f ms/iter\n", name, (now_ms() - start) / ITERATIONS);
}

int main(void) {
    struct Mat *src = alloc_mat_synthetic(IMAGE_SIZE, IMAGE_SIZE);
    struct Mat *dst = alloc_mat(IMAGE_SIZE, IMAGE_SIZE, CV_8UC1);

    run("scalar", COLOR_RGB2GRAY, dst, src);

    free_mat(&dst);
    free_mat(&src);
    return 0;
}
