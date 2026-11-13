"""OpenCV references for the morphology Kodak ops (tests/kodak_ops_morph.c).

All tolerances are 0. cv2.erode/dilate use an all-ones rectangular element and
the default border, which ignores outside pixels, as core/morph.c does.
"""

import cv2
import numpy as np


def _kernel(k):
    return np.ones((k, k), np.uint8)


def _erode(k, src="src"):
    return lambda c: cv2.erode(c.get(src), _kernel(k))


def _dilate(k, src="src"):
    return lambda c: cv2.dilate(c.get(src), _kernel(k))


def _morphology(op, k):
    return lambda c: cv2.morphologyEx(c.get("src"), op, _kernel(k))


OPS = {
    "erode3": (_erode(3), 0, "cv2.erode 3x3"),
    "dilate3": (_dilate(3), 0, "cv2.dilate 3x3"),
    "gray_erode7": (_erode(7, "gray_src"), 0, "cv2.erode 7x7 on our gray"),
    # lane-b-open-close-ops
}
