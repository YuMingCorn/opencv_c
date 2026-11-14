// improc example: runs a small pipeline on a built-in synthetic image and
// prints how long each step takes and a hash of each result.
//
//   synthetic RGB 160x120 -> gray -> 5x5 Gaussian -> Otsu -> binary
//
// The PC produces the same hashes (tests/test_arduino_synthetic.c), so
// "PASS" means this board computed byte-identical output. No camera or other
// hardware is needed. RAM: about 115 KB of images plus a few KB of scratch,
// so use an ESP32, ESP32-S3 or a Cortex-M board with enough RAM (an Arduino
// Uno with 2 KB is too small).
//
// Open the Serial Monitor at 115200 baud.

#include <improc.h>

#include "synthetic_pipeline.h"

static unsigned long step_us[SYNTH_STEP_COUNT];
static unsigned long start_us;

static void mark(int step) {
    unsigned long now = micros();
    step_us[step]     = now - start_us;
    start_us          = now;
}

static const char *const step_names[SYNTH_STEP_COUNT] = {
    "source (synthetic RGB)", "RGB -> gray", "Gaussian 5x5",
    "Otsu + threshold"};

static void print_hex32(uint32_t v) {
    for (int shift = 28; shift >= 0; shift -= 4) {
        Serial.print((unsigned)((v >> shift) & 0xFu), HEX);
    }
}

static void run_once() {
    struct SynthResult r;
    start_us = micros();
    if (synth_run(&r, mark) != 0) {
        Serial.println("out of memory: this board has too little RAM");
        return;
    }
    for (int i = 0; i < SYNTH_STEP_COUNT; i++) {
        Serial.print(step_names[i]);
        Serial.print(": ");
        Serial.print(step_us[i]);
        Serial.print(" us, hash ");
        print_hex32(r.hash_hi[i]);
        print_hex32(r.hash_lo[i]);
        Serial.println();
    }
    Serial.print("Otsu threshold: ");
    Serial.println(r.otsu);
    Serial.println(synth_matches(&r) ? "PASS: identical to the PC"
                                     : "FAIL: output differs from the PC");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("improc Synthetic example");
    run_once();
}

void loop() {
    delay(5000);
    run_once();
}
