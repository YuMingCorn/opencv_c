#ifndef KODAK_H_
#define KODAK_H_

#include "mat.h"

// One operation under test. run() receives the decoded Kodak image (CV_8UC3
// RGB) and returns a newly allocated Mat that the harness frees. Output sizes
// derive from the input (nothing assumes 768x512; six images are 512x768).
//
// The harness hashes every output (rows, cols, type, data) and compares it with
// the manifest. With --dump, an 8-bit output is written as <image>_<op>.png.
// Any other depth (CV_32S, CV_32F, ...) is written as text to
// values_<module>.txt, one line per image and op. CMake defines KODAK_MODULE
// as the module name string.
struct KodakOp {
    const char *name;
    struct Mat *(*run)(const struct Mat *rgb);
};

// Defined by tests/kodak_ops_<module>.c.
extern const struct KodakOp kodak_ops[];
extern const int            kodak_op_count;

#endif // KODAK_H_
