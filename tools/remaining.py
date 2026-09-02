#!/usr/bin/env python3
"""List the exported FUNCTIONS that are not yet matched.

Why this exists: 716 symbols are exported but 41 of them are data, and the
obvious way to tell them apart — "can `true_extent` find a body?" — is wrong.
`tools/audit.py`'s walker will happily disassemble a data symbol and report a
plausible instruction count, so a naive script invents targets that do not
exist. `SPRITE_ClipRect` (0x004bdea0) is the known case: it is the full-screen
clip rectangle `{0, 0, 640, 480}`, and a tally script once put it on the
roadmap as an 83-instruction function.

This tool classifies each export by the PE SECTION its RVA falls in, so a
symbol in `.rdata`/`.data` is never offered as a target, and cross-checks the
first bytes against a plausible function prologue.

    python3 tools/remaining.py            # unmatched code exports, smallest first
    python3 tools/remaining.py --all      # include the ones already marked
    python3 tools/remaining.py --data     # show the data exports instead
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from audit import true_extent, load_exe  # noqa: E402

IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_MEM_EXECUTE = 0x20000000


def sections_with_names():
    """[(name, va, vsize, rawptr, rawsize, characteristics)] from the PE header."""
    d = open(os.path.join(ROOT, "original", "legoland.exe"), "rb").read()
    e = struct.unpack_from("<I", d, 0x3C)[0]
    coff = e + 4
    nsec = struct.unpack_from("<H", d, coff + 2)[0]
    optsz = struct.unpack_from("<H", d, coff + 16)[0]
    base = coff + 20 + optsz
    out = []
    for i in range(nsec):
        o = base + i * 40
        name = d[o:o + 8].rstrip(b"\0").decode("latin1")
        vsz, va, rawsz, rawptr = struct.unpack_from("<IIII", d, o + 8)
        chars = struct.unpack_from("<I", d, o + 36)[0]
        out.append((name, va, vsz, rawptr, rawsz, chars))
    return out


def section_of(secs, rva):
    for name, va, vsz, rp, rs, chars in secs:
        if va <= rva < va + max(vsz, rs):
            return name, chars
    return None, 0


def marked():
    """{va: is_wip} over the COMMITTED files (never the working tree)."""
    out = {}
    files = subprocess.run(["git", "ls-files", "LEGOLAND/*.c"],
                           capture_output=True, text=True, cwd=ROOT).stdout.split()
    for f in files:
        for m in re.finditer(r"^//\s*(WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)",
                             open(os.path.join(ROOT, f)).read(), re.M):
            va = int(m.group(2), 16)
            out[va] = bool(m.group(1)) and out.get(va, True)
    return out


def main():
    show_all = "--all" in sys.argv
    show_data = "--data" in sys.argv
    secs = sections_with_names()
    d, plain = load_exe()
    done = marked()

    code, data = [], []
    for ln in open(os.path.join(ROOT, "symbols", "legoland.exports.txt")):
        p = ln.split()
        if len(p) < 3 or not p[-1].startswith("0x"):
            continue
        name, rva = p[0], int(p[-1], 16)
        sec, chars = section_of(secs, rva)
        if chars & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE):
            code.append((name, rva, sec))
        else:
            data.append((name, rva, sec))

    if show_data:
        print(f"{len(data)} DATA exports (never decompilation targets):")
        for name, rva, sec in sorted(data, key=lambda r: r[1]):
            print(f"  0x{rva + 0x400000:08x}  {sec:8s}  {name}")
        return 0

    rows = []
    for name, rva, sec in code:
        va = rva + 0x400000
        # WIP-marked exports are still "to finish", so keep them unless the
        # caller asked for everything (which also shows the finished ones).
        if va in done and not done[va] and not show_all:
            continue
        n, b = true_extent(d, plain, rva)
        rows.append((n or 0, va, name, "WIP" if done.get(va) else ("done" if va in done else "")))
    rows.sort()

    exact = sum(1 for _, rva, _ in code if rva + 0x400000 in done
                and not done[rva + 0x400000])
    print(f"{exact} of {len(code)} code exports exact ({100.0 * exact / len(code):.1f}%); "
          f"{len(code) - exact} to finish  [{len(data)} data exports excluded]\n")
    for n, va, name, state in rows:
        print(f"  {n:5d}  0x{va:08x}  {name}{'   [' + state + ']' if state else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
