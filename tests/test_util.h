#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

#include <stdio.h>
#include <string.h>

// CTest reports this exit code as "Skipped" through SKIP_RETURN_CODE.
#define TEST_EXIT_SKIP 77

static int test_failures_total   = 0;
static int test_failures_current = 0;
static int test_passed           = 0;
static int test_skipped          = 0;
static int test_skip_current     = 0;

#define TEST_FAIL_(...)                                                        \
    do {                                                                       \
        test_failures_current++;                                               \
        printf("  %s:%d: ", __FILE__, __LINE__);                               \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
    } while (0)

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            TEST_FAIL_("CHECK failed: %s", #expr);                             \
        }                                                                      \
    } while (0)

#define CHECK_EQ_INT(actual, expected)                                         \
    do {                                                                       \
        long long actual_    = (long long)(actual);                            \
        long long expected_  = (long long)(expected);                          \
        if (actual_ != expected_) {                                            \
            TEST_FAIL_("%s == %lld, expected %lld", #actual, actual_,          \
                       expected_);                                             \
        }                                                                      \
    } while (0)

#define CHECK_EQ_BYTES(actual, expected, count)                                \
    do {                                                                       \
        const unsigned char *actual_   = (const unsigned char *)(actual);      \
        const unsigned char *expected_ = (const unsigned char *)(expected);    \
        size_t count_                  = (size_t)(count);                      \
        for (size_t i_ = 0; i_ < count_; i_++) {                               \
            if (actual_[i_] != expected_[i_]) {                                \
                TEST_FAIL_("%s[%zu] == %d, expected %d", #actual, i_,          \
                           (int)actual_[i_], (int)expected_[i_]);              \
                break;                                                         \
            }                                                                  \
        }                                                                      \
    } while (0)

// Call from a test body, then return. A skip never counts as a pass.
#define TEST_SKIP(reason)                                                      \
    do {                                                                       \
        test_skip_current = 1;                                                 \
        printf("  skip reason: %s\n", (reason));                               \
    } while (0)

#define RUN_TEST(fn)                                                           \
    do {                                                                       \
        test_failures_current = 0;                                             \
        test_skip_current     = 0;                                             \
        fn();                                                                  \
        if (test_failures_current > 0) {                                       \
            test_failures_total += test_failures_current;                      \
            printf("[FAIL] %s\n", #fn);                                        \
        } else if (test_skip_current) {                                        \
            test_skipped++;                                                    \
            printf("[SKIP] %s\n", #fn);                                        \
        } else {                                                               \
            test_passed++;                                                     \
            printf("[PASS] %s\n", #fn);                                        \
        }                                                                      \
    } while (0)

// Return this from main(). Any failure wins; otherwise, a program in which
// nothing actually passed is reported as skipped.
static inline int test_finish(void) {
    printf("%d passed, %d skipped, %d failed checks\n", test_passed,
           test_skipped, test_failures_total);
    if (test_failures_total > 0) {
        return 1;
    }
    return (test_passed == 0) ? TEST_EXIT_SKIP : 0;
}

#endif // TEST_UTIL_H_
