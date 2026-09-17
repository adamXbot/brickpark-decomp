#!/usr/bin/env python3
"""LEGOLAND .res archive directory parser (Graphics1.res / Graphics2.res).

A .res is a single archive: u32 directory-offset at +0, member data packed from
+4, and a hierarchical name directory at the tail.  Leaf (FILE) records are:

    ff ff ff ff | u32 X | u32 zero=0 | u32 size | u32 data_offset | name\\0

Folder records use a small count in the first dword instead of 0xffffffff.
This module recovers every leaf with a robust scan for that record shape and
exposes `list` and `extract` sub-commands.

`list` tags each COMP member with its size, the LLS FORMAT word at +0x0c
(`fmt8` = 8-bit paletted frames, `fmt16` = 0x10, the 16-bpp RLE frames; see
tools/comp.py) and its animation frame count, with `+base` when flags bit 0
puts a base image record in front of them.

Usage:
    python3 tools/resfile.py list    <archive.res> [--all]
    python3 tools/resfile.py extract <archive.res> <name|index> <out_file>
    python3 tools/resfile.py extractall <archive.res> <out_dir>
"""
import os
import struct
import sys


def parse_leaves(data):
    """Return list of dicts {name, offset, size, is_comp} for every leaf record.

    Robust scan: an FILE record begins with 0xffffffff, has a zero dword at +8,
    an in-range size and data_offset, and an ASCII NUL-terminated name at +20.
    """
    n = len(data)
    leaves = []
    seen = set()
    i = 0
    while True:
        j = data.find(b"\xff\xff\xff\xff", i)
        if j < 0:
            break
        i = j + 1
        if j + 20 > n:
            continue
        X, zero, size, off = struct.unpack_from("<IIII", data, j + 4)
        if zero != 0:
            continue
        if size == 0 or size > 64_000_000:
            continue
        if off < 4 or off + size > n:
            continue
        k = j + 20
        e = data.find(b"\x00", k)
        if e < 0 or e == k or e - k > 128:
            continue
        name = data[k:e]
        if not all(32 <= c < 127 for c in name):
            continue
        try:
            nm = name.decode("ascii")
        except UnicodeDecodeError:
            continue
        if off in seen:
            continue
        seen.add(off)
        leaves.append({
            "name": nm,
            "offset": off,
            "size": size,
            "is_comp": data[off:off + 4] == b"COMP",
        })
    leaves.sort(key=lambda r: r["offset"])
    return leaves


def load(path):
    return open(path, "rb").read()


def cmd_list(argv):
    show_all = "--all" in argv
    argv = [a for a in argv if not a.startswith("--")]
    data = load(argv[0])
    leaves = parse_leaves(data)
    rows = leaves if show_all else [r for r in leaves if r["is_comp"]]
    for idx, r in enumerate(rows):
        tag = ""
        if r["is_comp"]:
            w, h, fmt, count, _, flags = struct.unpack_from(
                "<3IHHI", data, r["offset"] + 4)
            base = "+base" if flags & 1 else ""
            tag = f"COMP {w}x{h} fmt{fmt} frames={count}{base}"
        print(f"{idx:4d}  off=0x{r['offset']:08x}  size={r['size']:>9d}  "
              f"{r['name']:<40s} {tag}")
    print(f"# {len(rows)} entries "
          f"({sum(1 for r in leaves if r['is_comp'])} COMP of {len(leaves)} leaves)")


def _find(leaves, key):
    if key.isdigit():
        comp = [r for r in leaves if r["is_comp"]]
        return comp[int(key)]
    for r in leaves:
        if r["name"] == key:
            return r
    raise SystemExit(f"not found: {key}")


def cmd_extract(argv):
    data = load(argv[0])
    leaves = parse_leaves(data)
    r = _find(leaves, argv[1])
    open(argv[2], "wb").write(data[r["offset"]:r["offset"] + r["size"]])
    print(f"wrote {argv[2]}  ({r['name']}, {r['size']} bytes @ 0x{r['offset']:08x})")


def cmd_extractall(argv):
    data = load(argv[0])
    out = argv[1]
    os.makedirs(out, exist_ok=True)
    leaves = parse_leaves(data)
    for r in leaves:
        p = os.path.join(out, r["name"].replace("/", "_"))
        open(p, "wb").write(data[r["offset"]:r["offset"] + r["size"]])
    print(f"extracted {len(leaves)} members to {out}")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        raise SystemExit(1)
    cmd = sys.argv[1]
    rest = sys.argv[2:]
    {"list": cmd_list, "extract": cmd_extract,
     "extractall": cmd_extractall}[cmd](rest)


if __name__ == "__main__":
    main()
