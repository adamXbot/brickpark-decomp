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


def norm2(insn):
    """norm() plus a fix for branch targets printed WITHOUT an 0x prefix.

    Capstone renders a small relative target as bare decimal (e.g. `jmp 8`), and
    match.py's norm() only rewrites 0x-prefixed ones, so an otherwise identical
    branch compares unequal. Normalise any pure-numeric branch operand to <t>.
    (match.py is shared; fixing it there needs coordination, so we compensate
    locally.)
    """
    if insn.mnemonic == "call" or insn.mnemonic.startswith("j"):
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


def true_extent(d, secs, rva):
    """(instruction count, byte length) of the original function at rva.

    Ends at the first `ret` that no earlier branch jumps past, OR at an
    unconditional `jmp` that leaves the function (a void tail call: target
    before the entry, or at/after the next exported symbol) that nothing
    jumps past. Without the second rule a ret-less tail-call wrapper such as
    UnLoad_PopUpInfo (0x00471450) runs on into the following routine.
    """
    off = rva2off(secs, rva)
    if off is None:
        return None, None
    va = rva + 0x400000
    exps = export_rvas()
    nxt = None
    for e in exps:
        if e > rva:
            nxt = e + 0x400000
            break
    insns = list(md.disasm(d[off:off + 0x4000], va))
    furthest = va
    for i, x in enumerate(insns):
        if x.mnemonic.startswith("j"):
            m = re.match(r"^0x([0-9a-f]+)$", x.op_str.strip())
            if m:
                tgt = int(m.group(1), 16)
                external = tgt < va or (nxt is not None and tgt >= nxt)
                if x.mnemonic == "jmp" and external and x.address >= furthest:
                    return i + 1, sum(k.size for k in insns[:i + 1])
                if not external:
                    furthest = max(furthest, tgt)
        if x.mnemonic == "ret" and x.address >= furthest:
            return i + 1, sum(k.size for k in insns[:i + 1])
    return None, None


def end_of_body(insns):
    """Trim a compiled COMDAT to the function body.

    /Gy emits switch jump tables straight after the code, and disassembling them
    yields junk "instructions"; trailing alignment padding follows. Use the same
    rule as the original side: the body ends at the first `ret` that no earlier
    branch jumps past, or at an unconditional `jmp` to an EXTERNAL symbol that
    nothing jumps past. In an unlinked .obj an external jmp carries a
    relocation and a zero rel32, so its decoded target is the very next byte;
    an optimiser never emits an internal jump to the next instruction, so that
    signature is unambiguous.
    """
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
            comp = end_of_body(list(md.disasm(obj_function_code(obj, name), 0)))
            c_bytes = sum(i.size for i in comp)
            off = rva2off(secs, rva)
            ob = list(md.disasm(d[off:off + max(64, n_bytes or 64)], rva + 0x400000))[:n_ins or 0]
            mism = sum(1 for i in range(max(len(ob), len(comp)))
                       if i >= len(ob) or i >= len(comp) or norm2(ob[i]) != norm2(comp[i]))
            ok = (n_ins is not None and len(comp) == n_ins and c_bytes == n_bytes and mism == 0)
            tag = "WIP  " if wip else ("OK   " if ok else "REJECT")
            if not wip and not ok:
                bad += 1
            print(f"  [{tag}] {addr} {name:28s} ours={len(comp):4d}i/{c_bytes:4d}B  "
                  f"orig={n_ins}i/{n_bytes}B  mismatch={mism}")
    print(f"\n{'FAIL' if bad else 'PASS'}: {bad} function(s) failed the extent gate")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
