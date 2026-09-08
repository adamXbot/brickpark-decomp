#!/usr/bin/env python3
"""Whole-binary inventory of the game functions in `original/legoland.exe`.

Why this exists: `tools/coverage.py` reports that roughly 30% of the game code
in `.text` (everything below the CRT boundary) carries no marker at all, but
the only roadmap the project has, `tools/callees.py`, lists the callees our
`extern` declarations name and cannot see past them. The rest is reached
through callback tables, through callers that are themselves unmatched, and
through code nothing calls. This tool enumerates EVERY function start in the
game-code range from several sources iterated to a fixpoint, bounds each with
`match.true_extent`, and reports the ones without a `// FUNCTION:` /
`// WIP-FUNCTION:` marker: size, how each was reached, its nearest matched
neighbour, and a grouping into candidate scopes.

    python3 tools/inventory.py                   # full report to stdout
    python3 tools/inventory.py --json out.json   # also dump everything as JSON
    python3 tools/inventory.py --group 1200      # target instructions per group
    python3 tools/inventory.py --self-check      # check docs/RIDE_CALLBACKS.md

Sources (an address records every source that reached it):

  export  `symbols/legoland.exports.txt`, code section only (`remaining.py`'s
          section test keeps the 41 data exports out).
  marker  every `// FUNCTION:` / `// WIP-FUNCTION:` in `LEGOLAND/*.c`.
  call    a direct `call rel32` from inside a known function's extent.
  tail    a direct `jmp rel32` that leaves its function's extent (tail call).
  imm     a 32-bit immediate operand in a known function's code that lands on
          a plausible function start: the pointers the `*_GetInterfaces`
          providers store into object-definition slots, and callbacks passed
          as arguments. Most of the game's "callback tables" live here, not
          in `.data`.
  table   a 4-byte-aligned dword in `.data`/`.rdata` (or in a `.text` gap no
          extent covers) that lands on a plausible function start.
  crt     a `call`/`jmp rel32` in the statically linked CRT (above the
          boundary) whose target is game code: the entry code's call to the
          game's main function, and anything registered with it.
  sweep   16-aligned code straight after a known extent's padding that no
          reference reaches at all: dead code the linker kept. Tried only
          after every reference-based source is exhausted, so a `sweep`
          function is one that nothing in the binary names.

A `call` target is accepted unconditionally (a direct call is proof). The weak
sources must land on a 16-byte boundary (every one of the 3,100+ starts the
strong sources establish is 16-aligned), outside every known extent, with a
bounded extent whose first instruction is not padding. Jump tables are
excluded twice over: their targets sit inside the extent of the `switch` that
owns them, and every table a `jmp dword ptr [reg*4+T]` names is masked out of
the pointer scan; a run of consecutive in-range dwords containing any
unaligned value (a jump table or SEH scope table of a function not yet known)
is rejected whole. A `.data` dword whose bytes and neighbours are all
printable may be text; a function that has no other evidence is flagged.

Extents come from `match.true_extent`. That walker disassembles a 16 KB
window and returns nothing for a function longer than that; `long_extent`
below applies the SAME rules over a 128 KB window and is used only then
(one game function, 0x004453a0, needs it). `__try/__except` bodies need no
help here: since 2026-09-08 the walker reads the SEH scope table itself. Extents that overlap another known
start are reported, never silently trimmed.

Read-only: never compiles, never edits anything, safe to run alongside other
sessions. It does not run `tools/verify.py`, `progress.py` or `coverage.py`
but recomputes coverage.py's matched-bytes figure by the same method so the
reconciliation is against the same number.
"""
import argparse
import bisect
import glob
import json
import os
import re
import struct
import sys

import capstone
from capstone import x86

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from match import (IMAGE_BASE, load_exe, rva2off, true_extent, md,  # noqa: E402
                   _branch_target, _table_targets, _loop_entry, export_rvas)
from audit import annotated  # noqa: E402
from callees import DECL  # noqa: E402
from coverage import CRT_BASE  # noqa: E402
from remaining import sections_with_names  # noqa: E402

GAME_LO = 0x00401000
GAME_HI = CRT_BASE            # 0x0049e000: statically linked CRT above this
PADDING = (0xCC, 0x90)        # int3 / nop between /Gy-aligned functions
LONG_WINDOW = 0x20000

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True

# Display priority of a function's sources, strongest evidence first.
SOURCE_RANK = {'marker': 0, 'export': 1, 'call': 2, 'crt': 3, 'tail': 4,
               'imm': 5, 'table': 6, 'sweep': 7, 'extern': 8}

# Mnemonics VC6 never emits for C; a body containing them is data being read
# as code. `int3` is padding and handled separately.
WEIRD = {'in', 'out', 'insb', 'insd', 'outsb', 'outsd', 'hlt', 'iret', 'iretd',
         'cli', 'sti', 'arpl', 'bound', 'les', 'lds', 'lfs', 'lgs', 'lss', 'enter',
         'into', 'aaa', 'aad', 'aam', 'aas', 'daa', 'das', 'salc', 'lahf', 'sahf',
         'xlatb', 'cmc', 'wait', 'fwait', 'sldt', 'lgdt', 'lidt', 'lmsw', 'clts',
         'invd', 'wbinvd', 'rsm', 'ud2', 'ljmp', 'lcall', 'retf', 'int', 'int1',
         'icebp', 'sysenter', 'ltr', 'str', 'verr', 'verw', 'lar', 'lsl', 'smsw',
         'sgdt', 'sidt'}
# Not in the set although rare: matched bodies use rdtsc (Coaster3D_DrawModel),
# pushal/popal (ZBufferHelper) and wait (Raster_SetFloatMode).


def weird_count(insns):
    n = 0
    for x in insns:
        if x.mnemonic in WEIRD or (x.mnemonic == 'add' and x.op_str == 'byte ptr [eax], al'):
            n += 1
    return n


def long_extent(d, secs, rva, window=LONG_WINDOW):
    """`match.true_extent` verbatim, with the disassembly window as a
    parameter. Used only when true_extent returns (None, None) because the
    function is longer than its 16 KB window. Delete this when match.py grows
    the parameter."""
    off = rva2off(secs, rva)
    if off is None:
        return None, None
    va = rva + IMAGE_BASE
    insns = list(md.disasm(d[off:off + window], va))
    exps = export_rvas()
    nxt = None
    for e in exps:
        if e > rva:
            nxt = e + IMAGE_BASE
            break
    addr_index = {x.address: k for k, x in enumerate(insns)}

    def external(tgt):
        if tgt < va or (nxt is not None and tgt >= nxt):
            return True
        k = addr_index.get(tgt)
        if k is not None and k > 0 and tgt % 16 == 0 and insns[k - 1].mnemonic == 'nop':
            return True
        return False

    furthest = va
    for i, x in enumerate(insns):
        if nxt is not None and x.address >= nxt:
            body = insns[:i]
            while body and body[-1].mnemonic in ('nop', 'int3'):
                body.pop()
            return (len(body), sum(k.size for k in body)) if body else (None, None)
        if x.mnemonic in ('nop', 'int3') and i and x.address >= furthest:
            j = i
            while j < len(insns) and insns[j].mnemonic in ('nop', 'int3'):
                j += 1
            end = insns[j].address if j < len(insns) else x.address + x.size
            if end % 16 == 0 or end == nxt:
                return i, sum(k.size for k in insns[:i])
        if x.mnemonic.startswith('j'):
            m = re.match(r'^0x([0-9a-f]+)$', x.op_str.strip())
            if m:
                tgt = int(m.group(1), 16)
                if x.mnemonic == 'jmp' and x.address >= furthest:
                    if tgt > x.address and not external(tgt) and _loop_entry(insns, addr_index, i, tgt):
                        furthest = max(furthest, tgt)
                        continue
                    return i + 1, sum(k.size for k in insns[:i + 1])
                if not external(tgt):
                    furthest = max(furthest, tgt)
            elif x.mnemonic == 'jmp':
                for t in _table_targets(d, secs, x.op_str, va, va + window):
                    furthest = max(furthest, t)
        if x.mnemonic == 'ret' and x.address >= furthest:
            return i + 1, sum(k.size for k in insns[:i + 1])
    return None, None


class Fn:
    __slots__ = ('va', 'n_ins', 'n_bytes', 'sources', 'name', 'marker', 'file',
                 'kind', 'declared', 'jump_tables', 'note', 'group', 'long', 'live')

    def __init__(self, va):
        self.va = va
        self.n_ins = self.n_bytes = 0
        self.sources = []        # [(kind, ref)] in discovery order
        self.name = None         # marker or export name
        self.marker = None       # 'FUNCTION' | 'WIP-FUNCTION' | None
        self.file = None         # LEGOLAND/<file>.c carrying the marker
        self.kind = 'function'   # 'function' | 'thunk' | 'unknown-extent'
        self.declared = []       # [(name, file)] from extern declarations
        self.jump_tables = []    # [(table_va, n_entries)]
        self.note = []
        self.group = None
        self.long = False        # extent needed long_extent
        self.live = False        # reachable from a root (marker/export/table/crt)

    @property
    def end(self):
        return self.va + self.n_bytes

    @property
    def matched(self):
        return self.marker is not None

    @property
    def kinds(self):
        return {k for k, _ in self.sources}



class Inventory:
    def __init__(self):
        self.d, self.secs = load_exe()
        self.named_secs = sections_with_names()
        text = [s for s in self.named_secs if s[0] == '.text'][0]
        self.text_end = text[1] + IMAGE_BASE + text[2]
        self.fns = {}                 # va -> Fn
        self._starts = []             # sorted VAs of functions with extents
        self._max_bytes = 0           # longest extent seen, bounds containing()
        self._ext_cache = {}
        self.queue = []
        self.candidates = {}          # va -> [(kind, ref)] weak evidence, pending
        self.declared = {}            # va -> [(name, file)]
        self.rejected = {}            # va -> (reason, [(kind, ref)])
        self.jump_table_bytes = set() # dword addresses inside known jump tables
        self.jump_tables = {}         # table va -> entries
        self.data_refs = {}           # game-range address read as MEMORY by known code -> [from]
        self.jcc_escapes = []         # (from_va, target) conditional branch leaving an extent
        self.textish = set()          # .data dwords that could equally be text
        self.scanned = set()          # dword addresses already scanned for pointers
        self.external_calls = {}      # target -> count, calls outside the game range
        self.stats = {'found': {}}

    # --- helpers ------------------------------------------------------------ #
    def extent(self, va):
        """(n_ins, n_bytes, needed_long_window) for the function at va."""
        if va not in self._ext_cache:
            n, b = true_extent(self.d, self.secs, va - IMAGE_BASE)
            lng = False
            if not n:
                n, b = long_extent(self.d, self.secs, va - IMAGE_BASE)
                lng = bool(n)
            self._ext_cache[va] = (n, b, lng)
        return self._ext_cache[va]

    def byte(self, va):
        off = rva2off(self.secs, va - IMAGE_BASE)
        return None if off is None else self.d[off]

    def code(self, va, n):
        off = rva2off(self.secs, va - IMAGE_BASE)
        return self.d[off:off + n]

    def insns(self, fn):
        return list(MD.disasm(self.code(fn.va, fn.n_bytes), fn.va))[:fn.n_ins]

    def containing(self, va):
        """The known function whose extent strictly contains va, or None."""
        i = bisect.bisect_right(self._starts, va) - 1
        for j in range(i, -1, -1):
            fn = self.fns[self._starts[j]]
            if fn.va < va < fn.end:
                return fn
            if fn.va + self._max_bytes <= va:
                break
        return None

    def add(self, va, kind, ref):
        """Register va as a function reached via (kind, ref); queue it once."""
        fn = self.fns.get(va)
        if fn is None:
            fn = self.fns[va] = Fn(va)
            n, b, lng = self.extent(va)
            if kind != 'marker' and self.is_thunk_at(va):
                # Import thunk: the walker has no terminator for an indirect
                # jmp and would run on into the next thunk. A marker on such
                # a body (GetTicks, sysstubs.c) keeps it a function.
                n, b = 1, list(MD.disasm(self.code(va, 8), va))[0].size
                fn.kind = 'thunk'
            if n:
                fn.n_ins, fn.n_bytes, fn.long = n, b, lng
                bisect.insort(self._starts, va)
                self._max_bytes = max(self._max_bytes, b)
            else:
                fn.kind = 'unknown-extent'
            self.queue.append(fn)
            self.stats['found'][kind] = self.stats['found'].get(kind, 0) + 1
        if (kind, ref) not in fn.sources:
            fn.sources.append((kind, ref))
        return fn

    def plausible(self, va):
        """Reason a weak-source candidate is rejected, or None if it is a
        plausible function start."""
        if va in self.fns:
            return None
        if va % 16:
            return 'unaligned'
        c = self.containing(va)
        if c is not None:
            return 'inside 0x%08x' % c.va
        n, b, _ = self.extent(va)
        if not n:
            return 'no extent'
        first = self.byte(va)
        if first in PADDING:
            return 'padding'
        # A value inside a data table. A jump table's span is known exactly
        # (a function may start right after it: 0x00484790 follows
        # GetTileInDir's table with no padding). A byte index table that
        # known code reads has no known length, so a candidate after one in
        # the same gap with no padding between is inside it: 0x00480020 is
        # two CRT _ctype entries that a .data dword spells, lying in
        # LegoLandWindowProc's index table.
        i = bisect.bisect_right(self._starts, va) - 1
        gap_lo = self.fns[self._starts[i]].end if i >= 0 else GAME_LO
        for t, cnt in self.jump_tables.items():
            if t <= va < t + 4 * cnt:
                return 'inside the jump table at 0x%08x' % t
        reads = [a for a in self.data_refs if gap_lo <= a < va]
        if reads:
            a = max(reads)
            if not any(x in PADDING for x in self.code(a, va - a)):
                return 'inside a data table at 0x%08x' % a
        body = list(MD.disasm(self.code(va, b), va))[:n]
        if weird_count(body):
            return 'not code'
        return None

    def is_thunk_at(self, va):
        first = list(MD.disasm(self.code(va, 8), va))[:1]
        return bool(first) and first[0].mnemonic == 'jmp' and first[0].op_str.startswith('dword ptr [')

    def text_like(self, addr):
        """A .data dword whose own bytes and 4 neighbours each side are all
        printable or NUL may be a string rather than a pointer."""
        off = rva2off(self.secs, addr - IMAGE_BASE)
        own, around = self.d[off:off + 4], self.d[off - 4:off] + self.d[off + 4:off + 8]
        return all(32 <= x < 127 or x == 0 for x in own) and all(32 <= x < 127 for x in around)

    # --- sources ------------------------------------------------------------ #
    def seed_exports(self):
        n = 0
        for ln in open(os.path.join(ROOT, 'symbols', 'legoland.exports.txt')):
            p = ln.split()
            if len(p) < 3 or not p[-1].startswith('0x'):
                continue
            name, rva = p[0], int(p[-1], 16)
            chars = 0
            for sname, sva, vsz, rp, rs, ch in self.named_secs:
                if sva <= rva < sva + max(vsz, rs):
                    chars = ch
            if not chars & (0x20 | 0x20000000):
                continue                      # data export, never a target
            va = rva + IMAGE_BASE
            if not GAME_LO <= va < GAME_HI:
                continue
            fn = self.add(va, 'export', name)
            fn.name = fn.name or name
            n += 1
        self.stats['export'] = n

    def seed_markers(self):
        n = exact = wip = 0
        for path in sorted(glob.glob(os.path.join(ROOT, 'LEGOLAND', '*.c'))):
            base = os.path.basename(path)
            for name, addr, is_wip in annotated(path):
                va = int(addr, 16)
                fn = self.add(va, 'marker', name)
                fn.name = name
                fn.file = base
                fn.marker = 'WIP-FUNCTION' if is_wip else 'FUNCTION'
                n += 1
                wip += is_wip
                exact += not is_wip
            for line in open(path).read().splitlines():
                m = DECL.search(line)
                if m:
                    self.declared.setdefault(int(m.group(2), 16), []).append((m.group(1), base))
        self.stats['marker'] = n
        self.stats['marker_exact'] = exact
        self.stats['marker_wip'] = wip

    def descend(self, fn):
        """Direct calls, tail jumps, immediates, data anchors and jump tables
        in fn's code."""
        if fn.kind != 'function' or not fn.n_ins:
            return
        lo, hi = fn.va, fn.end
        for x in self.insns(fn):
            branchy = x.mnemonic == 'call' or x.mnemonic.startswith('j') \
                or x.mnemonic.startswith('loop')
            if branchy:
                t = _branch_target(x)
                if t is None:
                    if x.mnemonic == 'jmp':
                        m = re.search(r'\*4\s*\+\s*0x([0-9a-f]+)\]', x.op_str)
                        if m:
                            tva = int(m.group(1), 16)
                            n = len(_table_targets(self.d, self.secs, x.op_str, lo, hi))
                            fn.jump_tables.append((tva, n))
                            self.jump_tables[tva] = n
                            for i in range(n):
                                self.jump_table_bytes.add(tva + 4 * i)
                    continue
                if x.mnemonic == 'call':
                    if GAME_LO <= t < GAME_HI:
                        self.add(t, 'call', fn.va)
                    else:
                        self.external_calls[t] = self.external_calls.get(t, 0) + 1
                elif not lo <= t < hi:
                    if x.mnemonic == 'jmp':
                        if GAME_LO <= t < GAME_HI:
                            self.candidates.setdefault(t, []).append(('tail', fn.va))
                        else:
                            self.external_calls[t] = self.external_calls.get(t, 0) + 1
                    else:
                        self.jcc_escapes.append((fn.va, t))
                continue
            for op in x.operands:
                if op.type == x86.X86_OP_IMM:
                    v = op.imm & 0xffffffff
                    if GAME_LO <= v < GAME_HI and not lo <= v < hi:
                        self.candidates.setdefault(v, []).append(('imm', fn.va))
                elif op.type == x86.X86_OP_MEM:
                    disp = op.mem.disp & 0xffffffff
                    if GAME_LO <= disp < GAME_HI and not lo <= disp < hi:
                        # Absolute or indexed data read inside .text: a
                        # lookup table the compiler placed after the code.
                        self.data_refs.setdefault(disp, []).append(fn.va)

    def scan_dwords(self, sname, lo, hi, kind='table'):
        """Aligned dwords in [lo, hi) that point into the game code."""
        found = 0
        off = rva2off(self.secs, lo - IMAGE_BASE)
        n = (hi - lo) // 4
        vals = struct.unpack_from('<%dI' % n, self.d, off)
        run = []
        for i, v in enumerate(vals + (0,)):
            addr = lo + 4 * i
            if addr in self.scanned:
                v = 0
            self.scanned.add(addr)
            if GAME_LO <= v < GAME_HI and addr not in self.jump_table_bytes:
                run.append((addr, v))
                continue
            if run:
                if any(t % 16 for _, t in run):
                    # A jump table or SEH scope table of a function nothing
                    # has reached yet: such targets are never all 16-aligned.
                    self.stats['table_runs_rejected'] = self.stats.get('table_runs_rejected', 0) + 1
                    for a, t in run:
                        lst = self.rejected.setdefault(t, ('unaligned run', []))[1]
                        if (kind, a) not in lst:
                            lst.append((kind, a))
                else:
                    for a, t in run:
                        if sname in ('.data', '.rdata') and self.text_like(a):
                            # Could be a string; still a candidate (records
                            # with inline names hold real pointers), but the
                            # report flags a function this is the only
                            # evidence for.
                            self.textish.add(a)
                        self.candidates.setdefault(t, []).append((kind, a))
                        found += 1
                run = []
        return found

    def export_directory(self):
        """VA range of the PE export directory: its address table holds RVAs
        that can coincide with VAs of game functions."""
        e = struct.unpack_from('<I', self.d, 0x3C)[0]
        rva, size = struct.unpack_from('<II', self.d, e + 4 + 20 + 96)
        return rva + IMAGE_BASE, rva + IMAGE_BASE + size

    def scan_tables(self):
        found = 0
        xlo, xhi = self.export_directory()
        for sname, sva, vsz, rp, rs, ch in self.named_secs:
            if sname in ('.data', '.rdata'):
                lo, hi = sva + IMAGE_BASE, sva + IMAGE_BASE + (rs // 4) * 4
                if lo <= xlo < hi:
                    found += self.scan_dwords(sname, lo, xlo & ~3)
                    found += self.scan_dwords(sname, (xhi + 3) & ~3, hi)
                else:
                    found += self.scan_dwords(sname, lo, hi)
        self.stats['table_dwords'] = self.stats.get('table_dwords', 0) + found

    def scan_text_gaps(self):
        """Pointer tables the compiler left inside .text (in gaps no extent
        covers)."""
        found = 0
        for lo, hi, cls in self.residue():
            if cls not in ('padding', 'code-like') and hi - lo >= 8:
                a = (lo + 3) & ~3
                found += self.scan_dwords('.text', a, a + ((hi - a) // 4) * 4)
        self.stats['text_dwords'] = self.stats.get('text_dwords', 0) + found

    def scan_crt(self):
        """call/jmp rel32 sites in the CRT whose target is game code."""
        lo, hi = GAME_HI, self.text_end
        off = rva2off(self.secs, lo - IMAGE_BASE)
        blob = self.d[off:off + (hi - lo)]
        n = 0
        for i in range(len(blob) - 4):
            if blob[i] in (0xE8, 0xE9):
                rel = struct.unpack_from('<i', blob, i + 1)[0]
                t = (lo + i + 5 + rel) & 0xffffffff
                if GAME_LO <= t < GAME_HI:
                    self.candidates.setdefault(t, []).append(('crt', lo + i))
                    n += 1
        self.stats['crt_sites'] = n

    def promote_candidates(self):
        """Accept every plausible weak-source candidate; returns how many new
        functions that added."""
        before = len(self.fns)
        for va in sorted(self.candidates):
            refs = self.candidates.pop(va)
            why = self.plausible(va)
            if why is None:
                for kind, ref in refs:
                    self.add(va, kind, ref)
            else:
                prev = self.rejected.get(va)
                merged = list(prev[1]) if prev else []
                for r in refs:
                    if r not in merged:
                        merged.append(r)
                self.rejected[va] = (why, merged)
        return len(self.fns) - before

    def gaps(self):
        out = []
        cur = GAME_LO
        for va in self._starts:
            fn = self.fns[va]
            if va > cur:
                out.append((cur, va))
            cur = max(cur, fn.end)
        if cur < GAME_HI:
            out.append((cur, GAME_HI))
        return out

    def anchors_in(self, lo, hi):
        """Data anchors (jump tables, memory reads by known code) in [lo, hi)."""
        out = []
        for t, n in self.jump_tables.items():
            if lo <= t < hi or lo < t + 4 * n <= hi:
                out.append(('jump table', t))
        for a in self.data_refs:
            if lo <= a < hi:
                out.append(('data read', a))
        return out

    def sweep(self):
        """Contiguous unreferenced code: from every gap, try each 16-aligned
        address that follows padding or a known extent, and accept it when
        it walks to a bounded, clean body inside the gap."""
        added = 0
        for lo, hi in self.gaps():
            va = lo
            while va < hi:
                while va < hi and self.byte(va) in PADDING:
                    va += 1
                if va >= hi:
                    break
                if self.is_thunk_at(va):
                    # A block of import thunks nothing calls directly (the
                    # linker keeps every thunk of an import library it used).
                    va = max(self.add(va, 'sweep', None).end, va + 1)
                    added += 1
                    continue
                ok = va % 16 == 0 and (va == lo or self.byte(va - 1) in PADDING + (0xC3,)
                                       or self.containing(va - 1) is not None
                                       or (va - 1) in self.fns)
                if ok:
                    n, b, _ = self.extent(va)
                    ok = bool(n) and va + b <= hi
                    if ok:
                        body = list(MD.disasm(self.code(va, b), va))[:n]
                        ok = len(body) == n and weird_count(body) == 0 \
                            and not self.anchors_in(va, va + b)
                if ok:
                    self.add(va, 'sweep', None)
                    added += 1
                    va += b
                    continue
                # Skip to the next 16-aligned address that follows a padding byte.
                nxt = (va | 15) + 1
                while nxt < hi and self.byte(nxt - 1) not in PADDING:
                    nxt += 16
                va = nxt
        return added

    # --- driver ------------------------------------------------------------- #
    def build(self):
        self.seed_exports()
        self.seed_markers()
        self.scan_crt()
        rounds = 0
        scanned = False
        swept = False
        while True:
            rounds += 1
            while self.queue:
                self.descend(self.queue.pop())
            if not scanned:
                self.scan_tables()
                scanned = True
            new = self.promote_candidates()
            if new or self.queue:
                continue
            new = 0
            self.scan_text_gaps()
            new += self.promote_candidates()
            if new or self.queue:
                continue
            if not swept:
                self.stats['sweep_rounds'] = self.stats.get('sweep_rounds', 0) + 1
            new = self.sweep()
            swept = True
            if not new and not self.queue:
                break
        self.stats['rounds'] = rounds
        for va, names in self.declared.items():
            fn = self.fns.get(va)
            if fn is not None:
                for nm, base in names:
                    if (nm, base) not in fn.declared:
                        fn.declared.append((nm, base))
                if ('extern', None) not in fn.sources:
                    fn.sources.append(('extern', None))
        self.finalise()

    def finalise(self):
        """Overlaps, pruning of weak candidates swallowed by later extents,
        per-source counts."""
        strong = {'marker', 'export', 'call'}
        pruned = []
        for va in sorted(self.fns):
            fn = self.fns[va]
            c = self.containing(va)
            if c is None:
                continue
            if fn.kinds & strong:
                fn.note.append('starts inside the extent of 0x%08x (%s, %di/%dB)'
                               % (c.va, c.name or '?', c.n_ins, c.n_bytes))
                c.note.append('extent covers the start of 0x%08x (%s)'
                              % (fn.va, fn.name or ','.join(sorted(fn.kinds))))
            else:
                pruned.append((fn, c))
        for fn, c in pruned:
            del self.fns[fn.va]
            self._starts.remove(fn.va)
            self.rejected[fn.va] = ('inside 0x%08x' % c.va, fn.sources)
        for va in [va for va in self.rejected if va in self.fns]:
            del self.rejected[va]          # rejected by one source, established by another
        self.stats['pruned'] = len(pruned)
        # Liveness: a root is anything a marker, export, pointer table or the
        # CRT names; everything a live function calls, tail-jumps to or takes
        # the address of is live. The rest is code nothing live names.
        live = set()
        for va, fn in self.fns.items():
            for k, ref in fn.sources:
                if k in ('marker', 'export', 'crt') or (k == 'table' and not GAME_LO <= ref < GAME_HI):
                    live.add(va)
                    break
        changed = True
        while changed:
            changed = False
            for va, fn in self.fns.items():
                if va not in live and any(k in ('call', 'tail', 'imm') and ref in live
                                          for k, ref in fn.sources):
                    live.add(va)
                    changed = True
        for va, fn in self.fns.items():
            fn.live = va in live
        counts, only = {}, {}
        for fn in self.fns.values():
            for k in fn.kinds:
                counts[k] = counts.get(k, 0) + 1
            kinds = fn.kinds - {'extern'}
            if len(kinds) == 1:
                k = kinds.pop()
                only[k] = only.get(k, 0) + 1
        self.stats['reached_by'] = counts
        self.stats['only_source'] = only

    # --- reporting ---------------------------------------------------------- #
    def matched_addrs(self):
        return sorted(va for va, fn in self.fns.items() if fn.matched)

    def nearest_matched(self, va, marked):
        i = bisect.bisect_left(marked, va)
        best = None
        for j in (i - 1, i):
            if 0 <= j < len(marked):
                m = marked[j]
                if best is None or abs(m - va) < abs(best - va):
                    best = m
        return self.fns[best] if best is not None else None

    def best_source(self, fn):
        best = None
        for kind, ref in fn.sources:
            r = SOURCE_RANK[kind]
            if kind in ('call', 'tail', 'imm') and ref in self.fns and not self.fns[ref].matched:
                r += 0.5          # via an unmatched function ranks below via a matched one
            if best is None or r < best[0]:
                best = (r, kind, ref)
        return best[1], best[2]

    def weak(self, fn):
        """True when every source is a .data dword that could be text."""
        refs = [(k, r) for k, r in fn.sources if k != 'extern']
        return bool(refs) and all(k == 'table' and r in self.textish for k, r in refs)

    def callers(self, fn):
        return sorted({ref for kind, ref in fn.sources if kind in ('call', 'tail', 'imm')})

    def reach_text(self, fn):
        """Strongest source, worded for the report."""
        kind, ref = self.best_source(fn)
        more = len(self.callers(fn)) - 1
        tail = ' (+%d more)' % more if more > 0 and kind in ('call', 'tail', 'imm') else ''
        if kind == 'marker':
            return 'marker in %s' % fn.file
        if kind == 'export':
            return 'exported'
        if kind in ('call', 'tail', 'imm'):
            src = self.fns.get(ref)
            who = self.label(src) if src else '0x%08x' % ref
            verb = {'call': 'called by', 'tail': 'tail-jumped from', 'imm': 'pointer in'}[kind]
            return '%s %s%s' % (verb, who, tail)
        if kind == 'table':
            return 'table at 0x%08x in %s' % (ref, self.section_name(ref))
        if kind == 'crt':
            return 'called from CRT 0x%08x' % ref
        if kind == 'sweep':
            return 'unreferenced (swept)'
        return 'declared extern'

    def section_name(self, va):
        for sname, sva, vsz, rp, rs, ch in self.named_secs:
            if sva <= va - IMAGE_BASE < sva + max(vsz, rs):
                return sname
        return '?'

    def label(self, fn):
        if fn.name:
            return '%s (%s)' % (fn.name, fn.file) if fn.file else '%s (export)' % fn.name
        if fn.declared:
            return '%s [declared in %s]' % (fn.declared[0][0], fn.declared[0][1])
        return '0x%08x [unmatched]' % fn.va

    def residue(self):
        """Game-code byte ranges no known extent covers, classified."""
        out = []
        for lo, hi in self.gaps():
            b = self.code(lo, hi - lo)
            anchors = self.anchors_in(lo, hi)
            if all(x in PADDING for x in b):
                cls = 'padding'
            elif any(k == 'jump table' for k, _ in anchors):
                cls = 'jump-table'
            elif anchors:
                cls = 'data-in-text'
            elif not any(b):
                cls = 'zero'
            else:
                insns = list(MD.disasm(b, lo))
                covered = sum(x.size for x in insns)
                clean = covered >= len(b) - 15 and weird_count(insns) == 0
                cls = 'code-like' if clean else 'data-like'
                if hi == GAME_HI and not clean:
                    cls = 'crt-data'      # the CRT's tables begin below CRT_BASE
            out.append((lo, hi, cls))
        return out

    def group(self, target):
        """Pack unmatched functions into candidate scopes by address."""
        rows = [fn for fn in self.fns.values()
                if not fn.matched and fn.kind == 'function']
        rows.sort(key=lambda f: f.va)
        groups, cur, total = [], [], 0
        for fn in rows:
            gap = fn.va - cur[-1].end if cur else 0
            if fn.n_ins > target * 1.25:
                # Too big for any scope: stands alone so the packing around
                # it is not distorted.
                if cur:
                    groups.append(cur)
                groups.append([fn])
                cur, total = [], 0
                continue
            if cur and (total + fn.n_ins > target * 1.25 and total >= target * 0.8
                        or gap > 0x4000 and total >= target * 0.5):
                groups.append(cur)
                cur, total = [], 0
            cur.append(fn)
            total += fn.n_ins
        if cur:
            groups.append(cur)
        for gi, g in enumerate(groups, 1):
            for fn in g:
                fn.group = gi
        return groups

    def group_summary(self, g):
        callers, tables, files = {}, {}, {}
        marked = self.matched_addrs()
        for fn in g:
            for kind, ref in fn.sources:
                if kind in ('call', 'imm', 'tail') and ref in self.fns:
                    src = self.fns[ref]
                    if src.matched:
                        key = src.file
                    elif src.group == fn.group:
                        key = 'within group'
                    else:
                        key = 'group %d' % src.group if src.group else 'other unmatched'
                    callers[key] = callers.get(key, 0) + 1
                elif kind == 'table':
                    tables[ref & ~0xff] = tables.get(ref & ~0xff, 0) + 1
                elif kind == 'crt':
                    callers['CRT'] = callers.get('CRT', 0) + 1
            if not fn.live:
                callers['dead'] = callers.get('dead', 0) + 1
            near = self.nearest_matched(fn.va, marked)
            if near is not None:
                files[near.file] = files.get(near.file, 0) + 1
        top = lambda dct, n: ', '.join('%s x%d' % (k, v) if v > 1 else str(k)
                                       for k, v in sorted(dct.items(), key=lambda kv: -kv[1])[:n])
        return top(callers, 6) or 'tables only', top(files, 3), ', '.join('0x%08x' % t for t in sorted(tables)[:3])


def self_check(inv):
    """Every callback address in docs/RIDE_CALLBACKS.md must be in the inventory."""
    path = os.path.join(ROOT, 'docs', 'RIDE_CALLBACKS.md')
    text = open(path).read()
    addrs = set()
    for m in re.finditer(r'\b(?:8c|90|94|98|9c|a0|a4|a8|ac|b0|b8|bc|c0) ([0-9a-f]{6})\b', text):
        addrs.add(int(m.group(1), 16))
    missing = sorted(a for a in addrs if a not in inv.fns)
    print('self-check: %d callback addresses in RIDE_CALLBACKS.md, %d in the inventory, %d missing'
          % (len(addrs), len(addrs) - len(missing), len(missing)))
    for a in missing:
        why = inv.rejected.get(a)
        print('  MISSING 0x%08x  %s' % (a, why[0] if why else 'never proposed'))
    via, matched = {}, 0
    for a in addrs:
        fn = inv.fns.get(a)
        if fn:
            k = min((SOURCE_RANK[k], k) for k, _ in fn.sources)[1]
            via[k] = via.get(k, 0) + 1
            matched += fn.matched
            if 'imm' not in fn.kinds:
                print('  0x%08x is not reached by any pointer store (%s)' % (a, ','.join(sorted(fn.kinds))))
    print('  strongest source: %s; %d of them carry a marker'
          % (', '.join('%s %d' % kv for kv in sorted(via.items())), matched))
    return 0 if not missing else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--json', metavar='PATH', help='write the full inventory as JSON')
    ap.add_argument('--group', type=int, default=1200, metavar='INSNS',
                    help='target instructions per candidate scope (default 1200)')
    ap.add_argument('--self-check', action='store_true',
                    help='verify every docs/RIDE_CALLBACKS.md address is inventoried')
    ap.add_argument('--min-gap', type=int, default=16, metavar='BYTES',
                    help='smallest residue gap to list (default 16)')
    args = ap.parse_args()

    inv = Inventory()
    inv.build()
    fns = inv.fns
    marked = inv.matched_addrs()
    game_bytes = GAME_HI - GAME_LO

    matched_b = sum(fn.n_bytes for fn in fns.values() if fn.matched)
    exact_b = sum(fn.n_bytes for fn in fns.values() if fn.marker == 'FUNCTION')
    unmatched = [fn for fn in fns.values() if not fn.matched]
    thunks = [fn for fn in unmatched if fn.kind == 'thunk']
    unknown = [fn for fn in unmatched if fn.kind == 'unknown-extent']
    targets = [fn for fn in unmatched if fn.kind == 'function']
    referenced = [fn for fn in targets if fn.live]
    dead = [fn for fn in targets if not fn.live]
    unm_b = sum(fn.n_bytes for fn in targets)
    unm_i = sum(fn.n_ins for fn in targets)
    ref_b = sum(fn.n_bytes for fn in referenced)
    ref_i = sum(fn.n_ins for fn in referenced)
    dead_b = sum(fn.n_bytes for fn in dead)
    dead_i = sum(fn.n_ins for fn in dead)
    thunk_b = sum(fn.n_bytes for fn in thunks)
    gap_b = game_bytes - matched_b
    res = inv.residue()
    res_b = sum(hi - lo for lo, hi, _ in res)
    res_by = {}
    for lo, hi, cls in res:
        n, b = res_by.get(cls, (0, 0))
        res_by[cls] = (n + 1, b + hi - lo)
    groups = inv.group(args.group)
    s = inv.stats
    # The accounting below assumes extents neither overlap nor cross the
    # boundary; measure both so the identity is checked, not assumed.
    overlap_b = past_end = 0
    prev_end = GAME_LO
    for va in inv._starts:
        fn = fns[va]
        overlap_b += max(0, prev_end - va)
        past_end += max(0, fn.end - GAME_HI)
        prev_end = max(prev_end, fn.end)
    total = matched_b + unm_b + thunk_b + res_b - overlap_b - past_end
    assert total == game_bytes, (total, game_bytes, overlap_b, past_end)

    print('LEGOLAND game code 0x%08x..0x%08x: %d bytes (%.0f KB); the CRT above it is not a target'
          % (GAME_LO, GAME_HI, game_bytes, game_bytes / 1024))
    print('inputs: %d code exports | %d markers (%d exact, %d wip) | %d pointer dwords in .data/.rdata + %d in .text gaps (%d unaligned runs rejected) | %d CRT call sites into game code | fixpoint in %d rounds'
          % (s['export'], s['marker'], s['marker_exact'], s['marker_wip'], s.get('table_dwords', 0),
             s.get('text_dwords', 0), s.get('table_runs_rejected', 0), s.get('crt_sites', 0), s['rounds']))
    fd = s['found']
    order = ('export', 'marker', 'call', 'crt', 'tail', 'imm', 'table', 'sweep')
    print('first found by: ' + ', '.join('%s %d' % (k, fd.get(k, 0)) for k in order))
    rb = s['reached_by']
    print('reached by:     ' + ', '.join('%s %d' % (k, rb.get(k, 0)) for k in order + ('extern',)))
    only = s['only_source']
    print('reached ONLY by: ' + ', '.join('%s %d' % (k, only.get(k, 0)) for k in order))
    print('functions: %d = %d matched + %d unmatched (%d targets: %d live + %d dead; %d import thunks; %d unknown extent); %d weak candidates pruned inside later extents; %d addresses rejected'
          % (len(fns), len(marked), len(unmatched), len(targets), len(referenced), len(dead),
             len(thunks), len(unknown), s['pruned'], len(inv.rejected)))
    lng = [fn for fn in fns.values() if fn.long]
    if lng:
        print('extents beyond true_extent\'s 16 KB window (bounded by long_extent): %s'
              % ' '.join('0x%08x (%di/%dB)' % (f.va, f.n_ins, f.n_bytes) for f in lng))
    print()
    print('bytes: matched %d (exact %d + wip %d) = %.1f%% of game code (coverage.py method)'
          % (matched_b, exact_b, matched_b - exact_b, 100.0 * matched_b / game_bytes))
    print('       unmatched by that measure %d (%.1f%%), accounted for as:' % (gap_b, 100.0 * gap_b / game_bytes))
    print('         live unmatched functions        %6d bytes  %5d insns  %4d functions  (%.1f%% of the gap)'
          % (ref_b, ref_i, len(referenced), 100.0 * ref_b / gap_b))
    print('         dead unmatched functions        %6d bytes  %5d insns  %4d functions  (%.1f%%)'
          % (dead_b, dead_i, len(dead), 100.0 * dead_b / gap_b))
    print('         import thunks                   %6d bytes  %19d  (%.1f%%)' % (thunk_b, len(thunks), 100.0 * thunk_b / gap_b))
    print('         residue                         %6d bytes  %19d gaps  (%.1f%%): %s'
          % (res_b, len(res), 100.0 * res_b / gap_b,
             ', '.join('%s %d in %d' % (cls, b, n) for cls, (n, b) in sorted(res_by.items(), key=lambda kv: -kv[1][1]))))
    print()

    print('UNMATCHED FUNCTIONS (%d), largest first' % len(targets))
    print('%6s %6s  %-10s  %-52s  %s' % ('insns', 'bytes', 'address', 'reached by', 'nearest matched'))
    for fn in sorted(targets, key=lambda f: (-f.n_ins, f.va)):
        near = inv.nearest_matched(fn.va, marked)
        nl = '%s (%s) %+d' % (near.name, near.file, fn.va - near.va) if near else '-'
        extra = ''
        if fn.name:
            extra = '  export ' + fn.name
        elif fn.declared:
            extra = '  declared %s in %s' % (fn.declared[0][0], fn.declared[0][1])
        if not fn.live:
            extra += '  DEAD'
        if fn.long:
            extra += '  LONG'
        if inv.weak(fn):
            extra += '  WEAK (pointer bytes are printable)'
        if fn.note:
            extra += '  !' + '; '.join(fn.note)
        print('%6d %6d  0x%08x  %-52s  %s%s' % (fn.n_ins, fn.n_bytes, fn.va, inv.reach_text(fn), nl, extra))
    print()

    if unknown:
        print('UNKNOWN EXTENT (%d): neither walker found a terminator' % len(unknown))
        for fn in sorted(unknown, key=lambda f: f.va):
            print('  0x%08x  %s' % (fn.va, inv.reach_text(fn)))
        print()

    overl = [fn for fn in fns.values() if fn.note]
    tails = [(va, refs) for va, (why, refs) in sorted(inv.rejected.items())
             if why == 'unaligned' and any(k == 'tail' for k, _ in refs)]
    if overl or tails or overlap_b or past_end:
        print('EXTENT ANOMALIES (%d functions, %d unaligned tail-jump targets, %d overlap bytes, %d bytes past CRT_BASE)'
              % (len(overl), len(tails), overlap_b, past_end))
        for fn in sorted(overl, key=lambda f: f.va):
            print('  0x%08x %-30s %s' % (fn.va, fn.name or '', '; '.join(fn.note)))
        for va, refs in tails:
            # A jmp out of an extent to an unaligned address is the classic
            # sign of an extent the walker cut short (an SEH handler block,
            # a shared tail).
            print('  jmp to unaligned 0x%08x from %s' % (va, ' '.join('0x%08x' % r for k, r in refs if k == 'tail')))
        print()
    if inv.jcc_escapes:
        print('CONDITIONAL BRANCHES LEAVING AN EXTENT (%d)' % len(inv.jcc_escapes))
        for src, t in sorted(set(inv.jcc_escapes))[:40]:
            print('  0x%08x -> 0x%08x' % (src, t))
        print()

    print('RESIDUE: %d bytes in %d gaps no extent covers; gaps >= %d bytes:' % (res_b, len(res), args.min_gap))
    for lo, hi, cls in res:
        if hi - lo >= args.min_gap:
            i = bisect.bisect_right(inv._starts, lo) - 1
            prev = fns[inv._starts[i]] if i >= 0 else None
            anchors = inv.anchors_in(lo, hi)
            an = ('  anchors: ' + ', '.join('%s 0x%08x' % a for a in anchors[:3])) if anchors else ''
            print('  0x%08x..0x%08x %6d  %-12s after %s%s' % (lo, hi, hi - lo, cls, inv.label(prev) if prev else '-', an))
    print()

    susp = []
    for va, (why, refs) in sorted(inv.rejected.items()):
        kinds = {k for k, _ in refs}
        if kinds & {'imm', 'tail', 'crt', 'table'} and va % 16 == 0 and inv.byte(va - 1) in PADDING \
                and why.startswith('inside'):
            susp.append((va, why, refs))
    print('REJECTED CANDIDATES: %d addresses; by reason: %s'
          % (len(inv.rejected), ', '.join('%s %d' % kv for kv in sorted(
              __import__('collections').Counter(w for w, _ in inv.rejected.values()).items()))))
    if susp:
        print('  16-aligned, padding-preceded, yet inside a known extent -- possible walker over-runs:')
        for va, why, refs in susp:
            print('    0x%08x %s  via %s' % (va, why, ' '.join('%s@0x%08x' % (k, r) for k, r in refs[:3])))
    print()

    crt = sorted(t for t in inv.external_calls if GAME_HI <= t < 0x0049e4ff)
    if crt:
        print('BOUNDARY CHECK: %d direct-call targets between CRT_BASE and malloc (0x0049e4ff), first %s'
              % (len(crt), ' '.join('0x%08x' % t for t in crt[:6])))
        print()

    print('CANDIDATE SCOPES (%d groups, target %d instructions, packed by address)' % (len(groups), args.group))
    for gi, g in enumerate(groups, 1):
        callers, files, tables = inv.group_summary(g)
        print('  group %2d  0x%08x..0x%08x  %3d functions  %5d insns  %6d bytes  largest %d'
              % (gi, g[0].va, g[-1].end, len(g), sum(f.n_ins for f in g), sum(f.n_bytes for f in g), max(f.n_ins for f in g)))
        print('            among: %s' % files)
        print('            reached from: %s%s' % (callers, ('; tables ' + tables) if tables else ''))
    print()

    rc = 0
    if args.self_check:
        rc = self_check(inv)

    if args.json:
        out = {
            'range': [GAME_LO, GAME_HI], 'stats': inv.stats,
            'bytes': {'game': game_bytes, 'matched': matched_b, 'exact': exact_b, 'gap': gap_b,
                      'live_unmatched': ref_b, 'dead': dead_b, 'thunks': thunk_b, 'residue': res_b},
            'functions': [],
            'residue': [{'lo': lo, 'hi': hi, 'bytes': hi - lo, 'class': cls} for lo, hi, cls in res],
            'rejected': {('0x%08x' % va): {'why': why, 'refs': [[k, r] for k, r in refs]}
                         for va, (why, refs) in inv.rejected.items()},
            'groups': [{'index': gi, 'lo': g[0].va, 'hi': g[-1].end, 'functions': [f.va for f in g],
                        'insns': sum(f.n_ins for f in g), 'bytes': sum(f.n_bytes for f in g)}
                       for gi, g in enumerate(groups, 1)],
        }
        for va in sorted(fns):
            fn = fns[va]
            near = inv.nearest_matched(va, marked) if not fn.matched else None
            out['functions'].append({
                'va': va, 'name': fn.name, 'marker': fn.marker, 'file': fn.file, 'kind': fn.kind,
                'insns': fn.n_ins, 'bytes': fn.n_bytes, 'matched': fn.matched, 'live': fn.live,
                'long_extent': fn.long, 'weak': inv.weak(fn),
                'sources': [[k, r] for k, r in fn.sources],
                'reached_by': inv.reach_text(fn), 'callers': inv.callers(fn),
                'declared': fn.declared, 'jump_tables': fn.jump_tables, 'note': fn.note,
                'nearest_matched': ({'va': near.va, 'name': near.name, 'file': near.file} if near else None),
                'group': fn.group,
            })
        with open(args.json, 'w') as f:
            json.dump(out, f, indent=1)
        print('wrote %s' % args.json)
    return rc


if __name__ == '__main__':
    sys.exit(main())
