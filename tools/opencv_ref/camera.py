"""OpenCV references for the camera-format Kodak ops (tests/kodak_ops_camera.c).

The C test packs each Kodak RGB image into RGB565 or YUYV bytes before
decoding; the helpers below pack the same bytes from our dumped source.
"""

import cv2
import numpy as np


def _bgr565(c):
    # cv2 RGB2BGR565 truncates exactly like the C packer and stores the word
    # little-endian in a 2-channel array.
    return cv2.cvtColor(c.get("src"), cv2.COLOR_RGB2BGR565)


def _bgr565_be_gray(c):
    # Our BE op reads high byte first; swapping back gives OpenCV's order.
    return cv2.cvtColor(_bgr565(c), cv2.COLOR_BGR5652GRAY)


def _yuyv(c):
    src = c.get("src")
    out = np.empty(src.shape[:2] + (2,), np.uint8)
    out[:, 0::2, 0] = src[:, 0::2, 0]  # Y0 = p0.R
    out[:, 0::2, 1] = src[:, 0::2, 1]  # U  = p0.G
    out[:, 1::2, 0] = src[:, 1::2, 0]  # Y1 = p1.R
    out[:, 1::2, 1] = src[:, 1::2, 2]  # V  = p1.B
    return out


OPS = {
    "bgr565_rgb": (lambda c: cv2.cvtColor(_bgr565(c), cv2.COLOR_BGR5652RGB), 0,
                   "COLOR_BGR5652RGB, exact"),
    "bgr565be_gray": (_bgr565_be_gray, 1,
                      "COLOR_BGR5652GRAY (we use the 77/150/29 gray formula)"),
    "yuyv_rgb": (lambda c: cv2.cvtColor(_yuyv(c), cv2.COLOR_YUV2RGB_YUYV), 0,
                 "COLOR_YUV2RGB_YUYV, exact"),
    "yuyv_gray": (lambda c: cv2.cvtColor(_yuyv(c), cv2.COLOR_YUV2GRAY_YUYV), 0,
                  "COLOR_YUV2GRAY_YUYV, exact"),
}
