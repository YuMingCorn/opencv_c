"""OpenCV references for the colour Kodak ops (tests/kodak_ops_color2.c).

OPS maps an op name to (ref_fn, max_abs_diff, note). Arrays keep Mat channel
order. HSV2RGB is fed our own dumped RGB2HSV output.
"""

import cv2

# Our U is Cb and V is Cr; OpenCV orders YCrCb, so channels 1 and 2 swap.
_SWAP = [0, 2, 1]

OPS = {
    "rgb2hsv": (lambda c: cv2.cvtColor(c.get("src"), cv2.COLOR_RGB2HSV), 0,
                "COLOR_RGB2HSV, exact"),
    "hsv2rgb_roundtrip": (lambda c: cv2.cvtColor(c.get("rgb2hsv"), cv2.COLOR_HSV2RGB), 1,
                          "COLOR_HSV2RGB on our HSV"),
    "rgb2yuv": (lambda c: cv2.cvtColor(c.get("src"), cv2.COLOR_RGB2YCrCb)[..., _SWAP], 1,
                "COLOR_RGB2YCrCb, chroma swapped"),
    "yuv2rgb_roundtrip": (
        lambda c: cv2.cvtColor(c.get("rgb2yuv")[..., _SWAP].copy(), cv2.COLOR_YCrCb2RGB), 1,
        "COLOR_YCrCb2RGB on our YUV, chroma swapped"),
}
