"""OpenCV references for the phase 1 Kodak ops (tests/kodak_ops_phase1.c).

OPS maps an op name to (ref_fn, max_abs_diff, note). Tolerances come from the
Phase 2 plan. third_nearest is not compared: its ratio is not an integer for a
512-row image, so no OpenCV mode defines the expected result.
"""

import cv2
import numpy as np


def _flip(code):
    return lambda c: cv2.flip(c.get("src"), code)


def _gauss(k):
    return lambda c: cv2.GaussianBlur(c.get("src"), (k, k), 0)


def _sobel(dx, dy):
    def ref(c):
        grad = cv2.Sobel(c.get("gray"), cv2.CV_16S, dx, dy, ksize=3)
        return cv2.convertScaleAbs(grad)

    return ref


def _resize(rows_fn, cols_fn, interpolation):
    def ref(c):
        src = c.get("src")
        rows, cols = src.shape[:2]
        return cv2.resize(src, (cols_fn(cols), rows_fn(rows)), interpolation=interpolation)

    return ref


_INVERT_LUT = np.arange(255, -1, -1, dtype=np.uint8)

OPS = {
    "gray": (lambda c: cv2.cvtColor(c.get("src"), cv2.COLOR_RGB2GRAY), 1, "COLOR_RGB2GRAY"),
    "bgr": (lambda c: cv2.cvtColor(c.get("src"), cv2.COLOR_RGB2BGR), 0, "COLOR_RGB2BGR"),
    "rgba": (lambda c: cv2.cvtColor(c.get("src"), cv2.COLOR_RGB2RGBA), 0, "COLOR_RGB2RGBA"),
    "thresh128": (lambda c: cv2.threshold(c.get("src"), 128, 255, cv2.THRESH_BINARY)[1], 0,
                  "THRESH_BINARY"),
    "invert": (lambda c: cv2.LUT(c.get("src"), _INVERT_LUT), 0, "LUT 255-i"),
    "bright_384_10": (lambda c: cv2.convertScaleAbs(c.get("src"), alpha=1.5, beta=10), 1,
                      "convertScaleAbs(1.5, 10)"),
    "box5": (lambda c: cv2.blur(c.get("src"), (5, 5)), 0, "cv2.blur 5x5, 0 expected"),
    "gauss3": (_gauss(3), 0, "GaussianBlur 3x3, 0 expected"),
    "gauss5": (_gauss(5), 0, "GaussianBlur 5x5, 0 expected"),
    "gauss7": (_gauss(7), 0, "GaussianBlur 7x7, 0 expected"),
    "sobel_x_abs": (_sobel(1, 0), 0, "Sobel CV_16S ksize 3 on our gray"),
    "sobel_y_abs": (_sobel(0, 1), 0, "Sobel CV_16S ksize 3 on our gray"),
    "flip_h": (_flip(1), 0, "flip 1"),
    "flip_v": (_flip(0), 0, "flip 0"),
    "flip_both": (_flip(-1), 0, "flip -1"),
    "half_linear": (_resize(lambda r: r // 2, lambda w: w // 2, cv2.INTER_LINEAR), 1,
                    "INTER_LINEAR"),
    "nonint_linear": (_resize(lambda r: r * 5 // 12 + 1, lambda w: w * 3 // 7, cv2.INTER_LINEAR),
                      1, "INTER_LINEAR"),
    "double_nearest": (_resize(lambda r: r * 2, lambda w: w * 2, cv2.INTER_NEAREST_EXACT), 0,
                       "INTER_NEAREST_EXACT, integer ratio"),
}
