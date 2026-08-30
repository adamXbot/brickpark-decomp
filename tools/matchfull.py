#!/usr/bin/env python3
"""Full-body per-function VC6 match check.

Like tools/match.py, but compares the WHOLE function instead of stopping at the
first `ret` — required for multi-return functions (switches, early returns),
where match.py only verifies up to the first `ret`. Uses its own object path so
it never collides with match.py's shared /tmp/_match.obj.

    tools/matchfull.py LEGOLAND/foo.c FuncName 0x00441ec0 [--flags "/O2 /Gy /Gd"]
"""
import argparse
import os
import subprocess
import sys

import capstone

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from match import load_exe, rva2off, obj_function_code, norm  # noqa: E402

IMAGE_BASE = 0x400000
CL = "/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl"


def disasm_full(code, base=0):
    """Disassemble the whole function (past internal `ret`s), then trim trailing
    COMDAT/alignment padding (int3/nop). VC6 /Gy puts switch jump tables in
    .rdata, so the code stream has no embedded data to misread."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    out = list(md.disasm(code, base))
    while out and out[-1].mnemonic in ("nop", "int3"):
        out.pop()
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src"); ap.add_argument("func"); ap.add_argument("addr")
    ap.add_argument("--flags", default="/O2 /Gy /Gd")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()
    rva = int(args.addr, 16)
    if rva >= IMAGE_BASE:
        rva -= IMAGE_BASE

    obj = "/tmp/_matchfull.obj"
    cmd = [CL, "/nologo", "/c", "/Fo" + obj] + args.flags.split() + [args.src]
    env = dict(os.environ, ALPHATEAM_VC6_ROOT=os.path.join(ROOT, "toolchain"))
    r = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if r.returncode != 0:
        print(r.stdout); print(r.stderr, file=sys.stderr); raise SystemExit("compile failed")

    comp = disasm_full(obj_function_code(obj, args.func))
    d, secs = load_exe()
    off = rva2off(secs, rva)
    orig = disasm_full(d[off:off + max(64, len(comp) * 10)])
    orig = orig[:len(comp)] if len(orig) > len(comp) else orig

    # align on normalised text with difflib so a single insertion/deletion
    # doesn't cascade the whole tail into "mismatch".
    import difflib
    on = [norm(o) for o in orig]
    cn = [norm(c) for c in comp]
    sm = difflib.SequenceMatcher(a=on, b=cn, autojunk=False)
    n = max(len(on), len(cn))
    m = sum(b - a for tag, a, b, c, dd in sm.get_opcodes() if tag == "equal")
    if not args.quiet:
        for tag, a1, a2, b1, b2 in sm.get_opcodes():
            if tag == "equal":
                continue
            for k in range(max(a2 - a1, b2 - b1)):
                o = orig[a1 + k] if a1 + k < a2 else None
                c = comp[b1 + k] if b1 + k < b2 else None
                print(f"X {(o.mnemonic+' '+o.op_str) if o else '':40s} | {(c.mnemonic+' '+c.op_str) if c else ''}")
    pct = 100.0 * m / max(1, n)
    print(f"FULL MATCH: {m}/{n} = {pct:.1f}%  ({args.func})")
    return 0 if m == n else 1


if __name__ == "__main__":
    sys.exit(main())
