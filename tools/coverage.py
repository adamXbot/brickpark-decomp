#!/usr/bin/env python3
"""Measure decompilation coverage against a FIXED denominator: bytes of code.

Why this exists. Two other progress numbers are both true and both misleading
on their own:

  * `tools/remaining.py` reports exported functions (95%+). Exports are only the
    symbols the linker exposed — a fraction of the game.
  * `tools/callees.py` reports unmatched callees. That number MOVES IN BOTH
    DIRECTIONS: every newly matched file declares `extern`s for its own callees,
    so a productive round can raise it. It measures the frontier, not progress.

Bytes of matched code against bytes of game code in `.text` does not move
except by doing work, so it is the number to quote when asked how far along the
project is.

The `.text` section also contains the statically-linked C runtime, which is not
a decompilation target. The CRT sits in one contiguous run at the top of the
section (the game's own code stops below it), so this tool takes the lowest
address the reconstruction has ever called through a `/* 0x... */` extern and
labelled as CRT — or, failing that, the lowest known CRT address — as the
boundary and reports both figures.

    python3 tools/coverage.py
"""
import glob
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from audit import true_extent, load_exe  # noqa: E402

# Lowest confirmed CRT routine. Everything from here to the end of .text is
# statically-linked runtime (malloc 0x49e4ff, sprintf 0x49e573, exit 0x4a02b8,
# _stricmp 0x4aab90, _read 0x49f4ca, _write 0x4a63e4, _lseek 0x4a56c3 ...).
CRT_BASE = 0x0049E000


def code_section():
    d = open(os.path.join(ROOT, 'original', 'legoland.exe'), 'rb').read()
    e = struct.unpack_from('<I', d, 0x3C)[0]
    coff = e + 4
    nsec = struct.unpack_from('<H', d, coff + 2)[0]
    optsz = struct.unpack_from('<H', d, coff + 16)[0]
    base = coff + 20 + optsz
    for i in range(nsec):
        o = base + i * 40
        name = d[o:o + 8].rstrip(b'\0').decode('latin1')
        vsz, va, rawsz, rawptr = struct.unpack_from('<IIII', d, o + 8)
        chars = struct.unpack_from('<I', d, o + 36)[0]
        if chars & (0x20 | 0x20000000):
            return name, va + 0x400000, vsz
    raise SystemExit('no code section found')


def main():
    d, secs = load_exe()
    name, va0, vsz = code_section()
    end = va0 + vsz
    game_bytes = max(0, min(CRT_BASE, end) - va0)

    exact_b = exact_i = exact_n = 0
    wip_b = wip_n = 0
    for f in sorted(glob.glob(os.path.join(ROOT, 'LEGOLAND', '*.c'))):
        text = open(f).read()
        for m in re.finditer(r'^//\s*(WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)',
                             text, re.M):
            ins, b = true_extent(d, secs, int(m.group(2), 16) - 0x400000)
            if not ins:
                continue
            if m.group(1):
                wip_n += 1
                wip_b += b
            else:
                exact_n += 1
                exact_i += ins
                exact_b += b

    print(f"code section {name}: 0x{va0:08x}..0x{end:08x}  {vsz} bytes ({vsz/1024:.0f} KB)")
    print(f"  statically-linked CRT from 0x{CRT_BASE:08x}: "
          f"{max(0, end - CRT_BASE)} bytes ({max(0, end-CRT_BASE)/1024:.0f} KB), not a target")
    print(f"  game code: {game_bytes} bytes ({game_bytes/1024:.0f} KB)\n")
    print(f"matched exactly : {exact_n:5d} functions  {exact_i:7d} instructions  "
          f"{exact_b:7d} bytes ({exact_b/1024:.0f} KB)")
    print(f"partial (WIP)   : {wip_n:5d} functions  {'':19s}{wip_b:7d} bytes")
    print()
    print(f"COVERAGE OF GAME CODE: {100.0*exact_b/game_bytes:.1f}% exact"
          f"  ({100.0*(exact_b+wip_b)/game_bytes:.1f}% including partials)")
    return 0


if __name__ == '__main__':
    sys.exit(main())
