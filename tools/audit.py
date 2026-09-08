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

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from match import (load_exe, rva2off, obj_function_code, norm, md,  # noqa: E402
                   true_extent, compiled_body, end_of_body, export_rvas, CL_WRAPPER)

# The extent walker (true_extent / compiled_body / end_of_body) and the
# branch-target normalisation used to live here while tools/match.py was
# frozen; they were ported into match.py on 2026-09-03 and are imported so the
# two gates can never disagree again. norm2 is kept as a name for callers.
norm2 = norm

CL = CL_WRAPPER
MARK = re.compile(r"//\s*(WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)")
NAME = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def annotated(path):
    """(name, address, is_wip) per marker; name is None if no signature follows.

    The signature is looked for on the following lines, skipping comment and
    blank lines. A marker whose signature was not found within a three-line
    window used to be dropped SILENTLY, so the body disappeared from the gate
    while coverage.py still counted it from the marker — unref4.c's
    JcDeco_CalcCursor, whose note ran to five lines, was invisible this way.
    Unbound markers are returned with name None so main() can report them.
    """
    lines = open(path).read().splitlines()
    out = []
    for i, ln in enumerate(lines):
        m = MARK.search(ln)
        if not m:
            continue
        found = False
        for j in range(i + 1, len(lines)):
            stripped = lines[j].lstrip()
            if stripped.startswith("//") or stripped.startswith("*") or not stripped:
                continue
            nm = NAME.search(lines[j])
            if nm:
                out.append((nm.group(1), m.group(2), bool(m.group(1))))
                found = True
            break
        if not found:
            out.append((None, m.group(2), bool(m.group(1))))
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
            if name is None:
                print("  [NOMARK] %s  marker with no signature after it" % addr)
                bad += 1
                continue
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
