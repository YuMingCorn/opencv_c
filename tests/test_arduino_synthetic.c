// Runs the Synthetic.ino pipeline on the host and checks the expected hashes
// stored in examples/Synthetic/synthetic_pipeline.h, so the numbers a board
// prints can be compared with the PC.
#include <inttypes.h>

#include "../examples/Synthetic/synthetic_pipeline.h"
#include "test_util.h"

static void test_synthetic_pipeline_hashes(void) {
    struct SynthResult r;
    CHECK_EQ_INT(synth_run(&r, NULL), 0);
    for (int i = 0; i < SYNTH_STEP_COUNT; i++) {
        printf("  step %d hash %08" PRIx32 "%08" PRIx32 "\n", i, r.hash_hi[i],
               r.hash_lo[i]);
        CHECK_EQ_INT(r.hash_hi[i], synth_expected_hash_hi[i]);
        CHECK_EQ_INT(r.hash_lo[i], synth_expected_hash_lo[i]);
    }
    printf("  otsu %d\n", r.otsu);
    CHECK_EQ_INT(r.otsu, SYNTH_EXPECTED_OTSU);
    CHECK(synth_matches(&r));
}

int main(void) {
    RUN_TEST(test_synthetic_pipeline_hashes);
    return test_finish();
}
