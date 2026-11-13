#!/usr/bin/env python3
"""Compare opencv_c Kodak dumps with OpenCV results.

    test_kodak_phase1 --dir <kodak> --manifest tests/golden/kodak_phase1.txt --dump <dump>
    python3 tools/compare_opencv.py --dump <dump>

Every tools/opencv_ref/*.py module exposes

    OPS = {op_name: (ref_fn, max_abs_diff, note)}

ref_fn(ctx) returns the OpenCV result for the image held by ctx. Each reference
is fed our own dumped input (ctx.get("src"), ctx.get("gray"), ...), so a
difference in one op never leaks into the next. Arrays keep the channel order
of the C code (channel 0 of a Mat is channel 0 of the array), not OpenCV's BGR
order.

Exit status is 1 when any op exceeds its tolerance on any image.
"""

import argparse
import glob
import importlib.util
import os
import re
import sys

import cv2
import numpy as np


class Context:
    """One Kodak image: loads <image>_<op>.png from the dump directory."""

    def __init__(self, dump_dir, image):
        self.dump_dir = dump_dir
        self.image = image
        self._cache = {}

    def path(self, op):
        return os.path.join(self.dump_dir, f"{self.image}_{op}.png")

    def has(self, op):
        return os.path.exists(self.path(op))

    def get(self, op):
        if op not in self._cache:
            arr = cv2.imread(self.path(op), cv2.IMREAD_UNCHANGED)
            if arr is None:
                raise FileNotFoundError(self.path(op))
            # The PNG channel order is the Mat channel order; cv2 returns BGR(A).
            if arr.ndim == 3 and arr.shape[2] == 3:
                arr = cv2.cvtColor(arr, cv2.COLOR_BGR2RGB)
            elif arr.ndim == 3 and arr.shape[2] == 4:
                arr = cv2.cvtColor(arr, cv2.COLOR_BGRA2RGBA)
            self._cache[op] = arr
        return self._cache[op]


def load_ref_modules(ref_dir):
    ops = {}
    for path in sorted(glob.glob(os.path.join(ref_dir, "*.py"))):
        name = os.path.splitext(os.path.basename(path))[0]
        if name.startswith("_"):
            continue
        spec = importlib.util.spec_from_file_location(f"opencv_ref_{name}", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for op, entry in module.OPS.items():
            if op in ops:
                sys.exit(f"duplicate op {op!r} in {path}")
            ops[op] = (name,) + tuple(entry)
    return ops


def find_images(dump_dir):
    names = []
    for path in glob.glob(os.path.join(dump_dir, "kodim*_src.png")):
        match = re.match(r"(kodim\d+)_src\.png$", os.path.basename(path))
        if match:
            names.append(match.group(1))
    return sorted(names)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--dump", required=True, help="directory written by --dump")
    parser.add_argument("--ref", default=os.path.join(here, "opencv_ref"),
                        help="directory with reference modules")
    parser.add_argument("--ops", nargs="*", help="only these ops")
    args = parser.parse_args()

    ops = load_ref_modules(args.ref)
    images = find_images(args.dump)
    if not images:
        sys.exit(f"no kodim*_src.png found in {args.dump}")
    selected = args.ops or sorted(ops)
    unknown = [op for op in selected if op not in ops]
    if unknown:
        sys.exit(f"unknown ops: {', '.join(unknown)}")

    print(f"OpenCV {cv2.__version__}, {len(images)} images, {len(selected)} ops")
    print(f"{'op':<16}{'module':<9}{'max diff':>9}{'tol':>5}{'diff px %':>11}  result")
    failed = False
    for op in selected:
        module, ref_fn, tolerance, note = ops[op]
        worst = 0
        differing = 0
        total = 0
        error = None
        for image in images:
            ctx = Context(args.dump, image)
            if not ctx.has(op):
                error = f"{image}: no dump for {op}"
                break
            ours = ctx.get(op)
            ref = ref_fn(ctx)
            if ours.shape != ref.shape:
                error = f"{image}: shape {ours.shape} vs OpenCV {ref.shape}"
                break
            diff = np.abs(ours.astype(np.int32) - ref.astype(np.int32))
            worst = max(worst, int(diff.max()))
            differing += int(np.count_nonzero(diff))
            total += diff.size
        if error:
            failed = True
            print(f"{op:<16}{module:<9}{'-':>9}{tolerance:>5}{'-':>11}  ERROR {error}")
            continue
        ok = worst <= tolerance
        failed |= not ok
        percent = 100.0 * differing / total if total else 0.0
        print(f"{op:<16}{module:<9}{worst:>9}{tolerance:>5}{percent:>11.4f}  "
              f"{'ok' if ok else 'FAIL'}  {note}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
