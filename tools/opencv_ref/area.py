"""OpenCV references for the INTER_AREA Kodak ops (tests/kodak_ops_area.c).

OPS maps an op name to (ref_fn, max_abs_diff, note). Tolerances come from the
Phase 2 plan.
"""

import cv2


def _area(rows_fn, cols_fn):
    def ref(c):
        src = c.get("src")
        rows, cols = src.shape[:2]
        return cv2.resize(src, (cols_fn(cols), rows_fn(rows)), interpolation=cv2.INTER_AREA)

    return ref


OPS = {
    "area_half": (_area(lambda r: r // 2, lambda w: w // 2), 0, "INTER_AREA"),
    "area_third": (_area(lambda r: r // 3, lambda w: w // 3), 1, "INTER_AREA"),
    "area_quarter": (_area(lambda r: r // 4, lambda w: w // 4), 1, "INTER_AREA"),
    "area_nonint": (_area(lambda r: r * 5 // 12 + 1, lambda w: w * 3 // 7), 1, "INTER_AREA"),
    "area_up_1_5": (_area(lambda r: r * 3 // 2, lambda w: w * 3 // 2), 1, "INTER_AREA"),
}
