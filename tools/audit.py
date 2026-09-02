#!/usr/bin/env python3
"""Audit annotated functions against their TRUE extent.

Why this exists: tools/matchfull.py truncates the original to the COMPILED
length, so a reconstruction that stops early (e.g. reproduces only the prologue
up to an early-return `ret`) can still report 100%. This tool independently
establishes where each original function really ends and requires our compiled
body to cover it.

Finding the end is the subtle part. Using "the next exported symbol" as the
boundary over-counts, because many functions are NOT exported — a `ret` followed
immediately by a new function then looks like a mid-function early return. So we
use control flow instead: walk from the entry, tracking the furthest branch
target seen; a `ret` ends the function only when nothing before it jumps past it.

    python3 tools/audit.py LEGOLAND/foo.c [LEGOLAND/bar.c ...]
"""
import os
import re
import subprocess
import sys

import capstone

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from match import load_exe, rva2off, obj_function_code, norm  # noqa: E402


BRANCHY = ("call", "loop", "loope", "loopne", "loopz", "loopnz")


def norm2(insn):
    """norm() plus two fixes for branch targets.

    (a) Capstone renders a small relative target as bare decimal (e.g. `jmp 8`),
        and match.py's norm() only rewrites 0x-prefixed ones, so an otherwise
        identical branch compares unequal.
    (b) `loop`/`loope`/`loopne` are relative branches too, but their mnemonics
        do not start with "j", so neither norm() nor the old rule here touched
        them: an EXACT function containing a `loop` still reported mismatches,
        because the original's absolute target (0x464ddf) and our COMDAT's
        (0x34f) normalise differently. This cost ZBufferHelper its [OK] despite
        a byte-for-byte identical body.

    Normalise any pure-numeric branch operand to <t>. (match.py is shared;
    fixing it there needs coordination, so we compensate locally.)
    """
    if insn.mnemonic.startswith(BRANCHY) or insn.mnemonic.startswith("j"):
        op = insn.op_str.strip()
        if re.match(r"^(0x[0-9a-f]+|\d+)$", op):
            return insn.mnemonic + " <t>"
    return norm(insn)

CL = "/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl"
MARK = re.compile(r"//\s*(WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)")
NAME = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


_EXPORT_RVAS = None


def export_rvas():
    """Sorted RVAs of every exported symbol (symbols/legoland.exports.txt).

    Used ONLY as an upper bound for deciding that a forward `jmp` leaves the
    function; never as the function's end (see the module docstring)."""
    global _EXPORT_RVAS
    if _EXPORT_RVAS is None:
        out = []
        try:
            for ln in open(os.path.join(ROOT, "symbols", "legoland.exports.txt")):
                parts = ln.split()
                if len(parts) >= 3 and parts[-1].startswith("0x"):
                    out.append(int(parts[-1], 16))
        except OSError:
            pass
        _EXPORT_RVAS = sorted(set(out))
    return _EXPORT_RVAS


def _table_targets(d, secs, op_str, lo, hi):
    """Case targets of an indirect `jmp dword ptr [reg*4 + TABLE]` in the exe.

    /Gy puts the table in .rdata; entries are VAs. Read consecutive dwords while
    they land inside [lo, hi) — the function's plausible span — and stop at the
    first that does not."""
    m = re.search(r"\*4\s*\+\s*0x([0-9a-f]+)\]", op_str)
    if not m:
        return []
    toff = rva2off(secs, int(m.group(1), 16) - 0x400000)
    if toff is None:
        return []
    out = []
    for i in range(0, 4 * 512, 4):
        if toff + i + 4 > len(d):
            break
        v = int.from_bytes(d[toff + i:toff + i + 4], "little")
        if not (lo <= v < hi):
            break
        out.append(v)
    return out


def true_extent(d, secs, rva):
    """(instruction count, byte length) of the original function at rva.

    Walk from the entry tracking the furthest forward branch target seen —
    including the case blocks of a `switch` jump table. The function ends at
    the first `ret` or unconditional direct `jmp` that nothing jumps past:
    after such an instruction nothing later is reachable by fall-through, and
    by construction nothing branches past it. This covers plain returns,
    early returns jumped past by guards, void tail calls (`jmp` out of the
    function, with or without a trailing jump table), and an out-of-line
    block that ends in a backward `jmp` into the body (LoadObjectLibrary).
    """
    off = rva2off(secs, rva)
    if off is None:
        return None, None
    va = rva + 0x400000
    insns = list(md.disasm(d[off:off + 0x4000], va))
    exps = export_rvas()
    nxt = None
    for e in exps:
        if e > rva:
            nxt = e + 0x400000
            break
    addr_index = {x.address: k for k, x in enumerate(insns)}

    def external(tgt):
        """A branch target outside this function: before the entry, at/after the
        next exported symbol, or a 16-aligned address reached only across nop
        padding (an unexported neighbour that /Gy aligned)."""
        if tgt < va or (nxt is not None and tgt >= nxt):
            return True
        k = addr_index.get(tgt)
        if k is not None and k > 0 and tgt % 16 == 0 and insns[k - 1].mnemonic == "nop":
            return True
        return False

    furthest = va
    for i, x in enumerate(insns):
        if nxt is not None and x.address >= nxt:
            # Ran past the next EXPORTED symbol without meeting a terminator.
            # A function cannot contain instructions there, so bound it and
            # drop any alignment padding.
            body = insns[:i]
            while body and body[-1].mnemonic in ("nop", "int3"):
                body.pop()
            return (len(body), sum(k.size for k in body)) if body else (None, None)
        if x.mnemonic in ("nop", "int3") and i and x.address >= furthest:
            # A padding run that ends on a 16-byte boundary (or at the next
            # export) is the gap between functions, so the body ended at the
            # previous instruction. This is the only terminator a function
            # whose last statement calls a NORETURN routine has:
            # RenderTiledSprite (0x00488c50) ends in exit(1) with no ret and no
            # jmp, and its successor is unexported, so neither the control-flow
            # walk nor the export bound could stop. Guarded by the same
            # "nothing jumps past it" rule as ret and jmp, so padding that some
            # earlier branch targets is not mistaken for the end.
            j = i
            while j < len(insns) and insns[j].mnemonic in ("nop", "int3"):
                j += 1
            end = insns[j].address if j < len(insns) else x.address + x.size
            if end % 16 == 0 or end == nxt:
                return i, sum(k.size for k in insns[:i])
        if x.mnemonic.startswith("j"):
            m = re.match(r"^0x([0-9a-f]+)$", x.op_str.strip())
            if m:
                tgt = int(m.group(1), 16)
                if x.mnemonic == "jmp" and x.address >= furthest:
                    return i + 1, sum(k.size for k in insns[:i + 1])
                if not external(tgt):
                    furthest = max(furthest, tgt)
            elif x.mnemonic == "jmp":
                for t in _table_targets(d, secs, x.op_str, va, va + 0x4000):
                    furthest = max(furthest, t)
        if x.mnemonic == "ret" and x.address >= furthest:
            return i + 1, sum(k.size for k in insns[:i + 1])
    return None, None


def compiled_body(insns, n_ins):
    """Trim a compiled COMDAT to the ORIGINAL's extent (n_ins instructions).

    /Gy emits switch jump tables straight after the code, and disassembling
    them yields junk "instructions"; trailing alignment padding follows. The
    original's extent is known exactly, so take that many instructions and
    then require (in main) that no direct branch among them escapes past the
    trimmed end — the compiled body may not hide reachable code beyond what
    the original has. Returns (body, escapes)."""
    if n_ins is None:
        return end_of_body(insns), False
    body = insns[:n_ins]
    end = body[-1].address + body[-1].size if body else 0
    code_len = sum(x.size for x in insns)
    escapes = False
    for x in body:
        if x.mnemonic.startswith("j"):
            m = re.match(r"^(?:0x([0-9a-f]+)|(\d+))$", x.op_str.strip())
            if m:
                tgt = int(m.group(1), 16) if m.group(1) else int(m.group(2))
                # match.py patches every relocated field to the 0x00990099
                # sentinel, so a call/jmp to another symbol decodes to a target
                # far outside the COMDAT; only in-section targets can escape.
                if 0 <= tgt < code_len and tgt >= end:
                    escapes = True
    return body, escapes


def end_of_body(insns):
    """Fallback trim when the original extent is unknown: first `ret` (or
    external `jmp`, zero rel32) that no earlier branch jumps past."""
    furthest = 0
    for i, x in enumerate(insns):
        if x.mnemonic.startswith("j"):
            m = re.match(r"^(?:0x([0-9a-f]+)|(\d+))$", x.op_str.strip())
            if m:
                tgt = int(m.group(1), 16) if m.group(1) else int(m.group(2))
                if (x.mnemonic == "jmp" and x.size == 5 and tgt == x.address + x.size
                        and x.address >= furthest):
                    return insns[:i + 1]
                furthest = max(furthest, tgt)
        if x.mnemonic == "ret" and x.address >= furthest:
            return insns[:i + 1]
    while insns and insns[-1].mnemonic in ("nop", "int3"):
        insns.pop()
    return insns


def annotated(path):
    lines = open(path).read().splitlines()
    out = []
    for i, ln in enumerate(lines):
        m = MARK.search(ln)
        if not m:
            continue
        for j in range(i + 1, min(i + 4, len(lines))):
            nm = NAME.search(lines[j])
            if nm and not lines[j].lstrip().startswith("//"):
                out.append((nm.group(1), m.group(2), bool(m.group(1))))
                break
    return out


def main():
    d, secs = load_exe()
    env = dict(os.environ, ALPHATEAM_VC6_ROOT=os.path.join(ROOT, "toolchain"))
    bad = 0
    for f in sys.argv[1:]:
        print(f"### {os.path.basename(f)}")
        obj = "/tmp/_audit_%d.obj" % os.getpid()
        r = subprocess.run([CL, "/nologo", "/c", "/Fo" + obj, "/O2", "/Gy", "/Gd", f],
                           capture_output=True, text=True, env=env, cwd=ROOT)
        if r.returncode != 0:
            print("  COMPILE FAILED"); bad += 1; continue
        for name, addr, wip in annotated(f):
            rva = int(addr, 16) - 0x400000
            n_ins, n_bytes = true_extent(d, secs, rva)
            comp, escapes = compiled_body(list(md.disasm(obj_function_code(obj, name), 0)), n_ins)
            c_bytes = sum(i.size for i in comp)
            off = rva2off(secs, rva)
            ob = list(md.disasm(d[off:off + max(64, n_bytes or 64)], rva + 0x400000))[:n_ins or 0]
            mism = sum(1 for i in range(max(len(ob), len(comp)))
                       if i >= len(ob) or i >= len(comp) or norm2(ob[i]) != norm2(comp[i]))
            ok = (n_ins is not None and len(comp) == n_ins and c_bytes == n_bytes
                  and mism == 0 and not escapes)
            tag = "WIP  " if wip else ("OK   " if ok else "REJECT")
            if not wip and not ok:
                bad += 1
            print(f"  [{tag}] {addr} {name:28s} ours={len(comp):4d}i/{c_bytes:4d}B  "
                  f"orig={n_ins}i/{n_bytes}B  mismatch={mism}{'  ESCAPES' if escapes else ''}")
    print(f"\n{'FAIL' if bad else 'PASS'}: {bad} function(s) failed the extent gate")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
