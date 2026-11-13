"""OpenCV references for the histogram Kodak ops (tests/kodak_ops_hist.c).

The histogram and the Otsu threshold are not images, so the harness writes them
to values_hist.txt (lines "<image> <op> <rows> <cols> <type> <values...>"). The
compare driver only handles image ops. The values are therefore checked inside
the reference of "otsu_binary", which runs once per image: calc_hist must equal
cv2.calcHist exactly, and the Otsu threshold must equal the one cv2 returns for
THRESH_OTSU. A mismatch aborts the run with a message.

Run this file directly for a per-image table:

    python3 tools/opencv_ref/hist.py --dump <dump dir>
"""

import argparse
import glob
import os
import re
import sys

import cv2
import numpy as np


def read_values(dump_dir):
    """Returns {(image, op): list of ints} from values_hist.txt."""
    values = {}
    with open(os.path.join(dump_dir, "values_hist.txt")) as f:
        for line in f:
            parts = line.split()
            values[(parts[0], parts[1])] = [int(v) for v in parts[5:]]
    return values


def cv_hist(gray):
    return cv2.calcHist([gray], [0], None, [256], [0, 256]).astype(np.int64).ravel()


def cv_otsu(gray):
    return int(cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)[0])


def check_values(ctx):
    """Asserts hist and Otsu of ctx.image against OpenCV; returns (hist_diff, ours, ref)."""
    gray = ctx.get("gray")
    values = read_values(ctx.dump_dir)
    ours_hist = np.array(values[(ctx.image, "hist")], dtype=np.int64)
    hist_diff = int(np.abs(ours_hist - cv_hist(gray)).max())
    ours = values.get((ctx.image, "otsu"), [None])[0]  # None until the otsu op exists
    ref = cv_otsu(gray)
    return hist_diff, ours, ref


def _equalize(ctx):
    hist_diff, _, _ = check_values(ctx)
    if hist_diff != 0:
        sys.exit(f"{ctx.image}: calc_hist differs from cv2.calcHist (max {hist_diff})")
    return cv2.equalizeHist(ctx.get("gray"))


OPS = {
    "equalize": (_equalize, 1, "equalizeHist on our gray; also checks hist values"),
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dump", required=True)
    args = parser.parse_args()
    here = os.path.dirname(os.path.abspath(__file__))
    sys.path.insert(0, os.path.join(here, ".."))
    from compare_opencv import Context, find_images

    print(f"{'image':<9}{'hist diff':>10}{'otsu':>6}{'cv2':>6}")
    failed = False
    for image in find_images(args.dump):
        ctx = Context(args.dump, image)
        hist_diff, ours, ref = check_values(ctx)
        bad = hist_diff != 0 or ours not in (None, ref)
        failed |= bad
        print(f"{image:<9}{hist_diff:>10}{str(ours):>6}{ref:>6}{'  DIFF' if bad else ''}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
