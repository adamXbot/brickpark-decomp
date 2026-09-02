#!/usr/bin/env python3
"""List the UNEXPORTED functions the matched code calls but nobody has matched.

Why this exists: `tools/remaining.py` measures progress against the 675 exported
functions, and that number (95%+) is easy to mistake for "the game is nearly
decompiled". It is not. Exports are only the symbols the linker happened to
expose; the game has roughly twice as many functions, and the matched files
reach the rest through `extern` declarations carrying the callee's address in a
trailing comment:

    extern void  SortBlokeIn3D(Bloke* b);            /* 0x0043ffd0 */

Every such address with no `// FUNCTION:` / `// WIP-FUNCTION:` marker anywhere
is behaviour the reconstruction NAMES but does not yet reproduce — and for the
browser runtime those are exactly the gaps that matter.

    python3 tools/callees.py              # unmatched callees, largest first
    python3 tools/callees.py --by-file    # group by the file that declares them
    python3 tools/callees.py --min 100    # only ones at least this many instructions
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from audit import true_extent, load_exe  # noqa: E402

# A prototype line ending in a comment that carries the callee's address.
DECL = re.compile(
    r'^\s*(?:extern\s+)?[A-Za-z_][^;{}]*\b(\w+)\s*\([^;{}]*\)\s*;.*?(0x00[0-9a-fA-F]{6})')
MARK = re.compile(r'^//\s*(WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)', re.M)


def main():
    by_file = '--by-file' in sys.argv
    floor = 0
    if '--min' in sys.argv:
        floor = int(sys.argv[sys.argv.index('--min') + 1])

    d, secs = load_exe()
    marked, declared = set(), {}
    for path in sorted(glob.glob(os.path.join(ROOT, 'LEGOLAND', '*.c'))):
        text = open(path).read()
        for m in MARK.finditer(text):
            marked.add(int(m.group(2), 16))
        base = os.path.basename(path)
        for line in text.splitlines():
            m = DECL.search(line)
            if m:
                declared.setdefault(int(m.group(2), 16), set()).add((m.group(1), base))

    rows = []
    for va, names in declared.items():
        if va in marked:
            continue
        n, b = true_extent(d, secs, va - 0x400000)
        if n and n >= floor:
            nm = sorted(n for n, _ in names)[0]
            rows.append((n, va, nm, sorted({f for _, f in names})))

    total = sum(r[0] for r in rows)
    print(f"{len(marked)} addresses carry a marker.")
    print(f"{len(declared)} distinct addresses are called through an extern declaration.")
    print(f"{len(rows)} of those are UNMATCHED: {total} instructions of behaviour "
          f"the reconstruction names but does not reproduce.\n")

    if by_file:
        groups = {}
        for n, va, nm, files in rows:
            for f in files:
                groups.setdefault(f, []).append((n, va, nm))
        for f in sorted(groups, key=lambda k: -sum(r[0] for r in groups[k])):
            g = sorted(groups[f], reverse=True)
            print(f"{f}  ({len(g)} callees, {sum(r[0] for r in g)} instructions)")
            for n, va, nm in g[:12]:
                print(f"    {n:5d}  0x{va:08x}  {nm}")
            if len(g) > 12:
                print(f"    ... and {len(g) - 12} more")
            print()
        return 0

    for n, va, nm, files in sorted(rows, reverse=True):
        print(f"  {n:5d}  0x{va:08x}  {nm:34s} {','.join(files)}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
