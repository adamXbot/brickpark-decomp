#!/usr/bin/env python3
"""Run the per-function VC6 match check across every annotated function and
report a tally — the LEGOLAND decomp's accuracy report.

Scans LEGOLAND/*.c for reccmp-style markers:

    // FUNCTION: LEGOLAND 0x00441ec0
    <return type> Name(args...)

compiles each file with the VC6 SP3 toolchain, and diffs each annotated
function against original/legoland.exe over its full extent (see tools/match.py).
Run it ALONE: nothing else may be compiling while it runs.
"""
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "LEGOLAND")
MARKER = re.compile(r"//\s*FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)")
NAME = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def annotated(path):
    lines = open(path).read().splitlines()
    out = []
    for i, ln in enumerate(lines):
        m = MARKER.search(ln)
        if not m:
            continue
        # the function name is on the next non-blank, non-comment line
        for j in range(i + 1, min(i + 4, len(lines))):
            nm = NAME.search(lines[j])
            if nm and not lines[j].lstrip().startswith("//"):
                out.append((nm.group(1), m.group(1)))
                break
    return out


def main():
    files = sorted(glob.glob(os.path.join(SRC, "*.c")))
    total = ok = 0
    rows = []
    for f in files:
        for name, addr in annotated(f):
            total += 1
            r = subprocess.run(
                [sys.executable, os.path.join(HERE, "match.py"),
                 os.path.relpath(f, ROOT), name, addr],
                capture_output=True, text=True, cwd=ROOT)
            m = re.search(r"MATCH:\s*(\d+)/(\d+)\s*instructions\s*=\s*([\d.]+)%(?:\s*\[([^\]]*)\])?", r.stdout)
            pct = float(m.group(3)) if m else 0.0
            # match.py bounds both bodies by the original's true extent and
            # prints "[orig=NNi/NNNB; extent ok]" only when the instruction
            # count, byte length, and every instruction agree and no branch
            # escapes the extent. A bare 100% without that token is not a match.
            good = pct >= 100.0 and bool(m) and "extent ok" in (m.group(4) or "")
            ok += good
            rows.append((name, addr, pct, good, os.path.basename(f)))
    for name, addr, pct, good, fn in rows:
        print(f"  [{'OK ' if good else '  '}] {pct:6.1f}%  {addr}  {name}  ({fn})")
    print(f"\n{ok}/{total} functions matching at 100%")
    return 0 if ok == total else 1


if __name__ == "__main__":
    sys.exit(main())
