#!/usr/bin/env python3
"""Per-function VC6 match check (decomp.me-style scratch verification).

Compiles a decomp C file with the VC6 SP3 toolchain (the compiler LEGOLAND was
built with), extracts the target function's machine code from the COFF object,
and diffs it instruction-by-instruction against the original function in
`original/legoland.exe`. Reports a match percentage and a side-by-side diff.

This verifies matching WITHOUT a full-exe rebuild: exactly the loop used to grow
a matching decompilation one function at a time.

    tools/match.py LEGOLAND/foo.c FuncName 0x00441ec0 [--flags "/O2 /Gy"]

The C file must define exactly the target function (plus any needed type/struct
decls). Address is the original function's VA (RVA + 0x400000) or bare RVA.
"""
import argparse
import os
import re
import struct
import subprocess
import sys

import capstone

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
IMAGE_BASE = 0x400000
CL = os.path.join(ROOT, "toolchain", "..", "tools", "wibo-msvc", "cl")
# The symlinked toolchain points into the sibling project; use its cl wrapper.
CL_WRAPPER = "/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl"


# --- original binary access ------------------------------------------------- #
def load_exe():
    d = open(os.path.join(ROOT, "original", "legoland.exe"), "rb").read()
    e = struct.unpack_from("<I", d, 0x3C)[0]
    coff = e + 4
    nsec = struct.unpack_from("<H", d, coff + 2)[0]
    optsz = struct.unpack_from("<H", d, coff + 16)[0]
    base = coff + 20 + optsz
    secs = []
    for i in range(nsec):
        o = base + i * 40
        vsz, va, rawsz, rawptr = struct.unpack_from("<IIII", d, o + 8)
        secs.append((va, vsz, rawptr, rawsz))
    return d, secs


def rva2off(secs, rva):
    for va, vsz, rp, rs in secs:
        if va <= rva < va + max(vsz, rs):
            return rp + (rva - va)
    return None


# --- COFF .obj: extract the named function's code --------------------------- #
def obj_function_code(obj_path, func):
    d = open(obj_path, "rb").read()
    nsec, symptr, nsym = struct.unpack_from("<HxxxxII", d, 2)
    # section headers start at 20
    sects = []
    for i in range(nsec):
        o = 20 + i * 40
        name = d[o:o + 8]
        rawsz, rawptr = struct.unpack_from("<II", d, o + 16)
        sects.append((name, rawptr, rawsz))
    # string table follows symbol table (18 bytes each)
    strtab = symptr + nsym * 18
    def symname(rec):
        if rec[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", rec, 4)[0]
            end = d.find(b"\0", strtab + off)
            return d[strtab + off:end].decode("latin1")
        return rec.split(b"\0")[0].decode("latin1")
    # section relocations (offset -> apply a sentinel so global refs disassemble
    # as an absolute address, matching the linked exe after normalisation).
    def patched_section(secidx):
        o = 20 + secidx * 40
        rawsz, rawptr, relptr = struct.unpack_from("<III", d, o + 16)
        nrel = struct.unpack_from("<H", d, o + 32)[0]
        code = bytearray(d[rawptr:rawptr + rawsz])
        for r in range(nrel):
            ro = relptr + r * 10
            va, sym, typ = struct.unpack_from("<IIH", d, ro)
            if va + 4 <= len(code):
                struct.pack_into("<I", code, va, 0x00990099)  # 6-hex sentinel
        return code
    # find the symbol for func (VC6 prepends '_' to cdecl C names)
    targets = {func, "_" + func}
    for i in range(nsym):
        o = symptr + i * 18
        rec = d[o:o + 18]
        name = symname(rec)
        value, secnum, typ, sclass = struct.unpack_from("<IhHB", rec, 8)
        if name in targets and secnum > 0:
            code = patched_section(secnum - 1)
            return bytes(code[value:])           # to end of (per-func) COMDAT section
    # fallback: first .text section as a whole
    for si, (nm, rawptr, rawsz) in enumerate(sects):
        if nm.startswith(b".text"):
            return bytes(patched_section(si))
    raise SystemExit(f"function {func} not found in {obj_path}")


# --- disassembly + normalisation -------------------------------------------- #
def norm(insn):
    """Normalise an instruction for comparison: keep mnemonic + operand shape,
    blank out absolute addresses/displacements that differ due to relocation."""
    op = insn.op_str
    # relative call/jmp targets -> placeholder
    if insn.mnemonic == "call" or insn.mnemonic.startswith("j"):
        op = re.sub(r"0x[0-9a-f]+", "<t>", op)
    # absolute [disp] memory operands (globals, relocated) -> placeholder,
    # but keep small struct offsets [reg + 0xNN] intact
    op = re.sub(r"\[0x[0-9a-f]{5,}\]", "[<abs>]", op)
    op = re.sub(r"\b0x[0-9a-f]{5,}\b", "<imm>", op)
    return insn.mnemonic + " " + op


def disasm(code, base=0):
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    out = []
    for insn in md.disasm(code, base):
        out.append(insn)
        if insn.mnemonic == "ret":
            break
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src"); ap.add_argument("func"); ap.add_argument("addr")
    ap.add_argument("--flags", default="/O2 /Gy /Gd")
    args = ap.parse_args()
    rva = int(args.addr, 16)
    if rva >= IMAGE_BASE:
        rva -= IMAGE_BASE

    obj = "/tmp/_match.obj"
    cmd = [CL_WRAPPER, "/nologo", "/c", "/FoZZ"] + args.flags.split() + [args.src]
    cmd[cmd.index("/FoZZ")] = "/Fo" + obj
    env = dict(os.environ, ALPHATEAM_VC6_ROOT=os.path.join(ROOT, "toolchain"))
    r = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if r.returncode != 0:
        print(r.stdout); print(r.stderr, file=sys.stderr)
        raise SystemExit("compile failed")

    comp = disasm(obj_function_code(obj, args.func))
    d, secs = load_exe()
    off = rva2off(secs, rva)
    orig = disasm(d[off:off + max(64, len(comp) * 8)])

    n = max(len(comp), len(orig))
    match = 0
    print(f"{'ORIGINAL @ 0x%08x' % (rva + IMAGE_BASE):42s} | RECOMPILED ({args.func})")
    print("-" * 90)
    for i in range(n):
        o = orig[i] if i < len(orig) else None
        c = comp[i] if i < len(comp) else None
        ok = o and c and norm(o) == norm(c)
        if ok:
            match += 1
        os_ = f"{o.mnemonic} {o.op_str}" if o else ""
        cs_ = f"{c.mnemonic} {c.op_str}" if c else ""
        mark = " " if ok else "X"
        print(f"{mark} {os_:40s} | {cs_}")
    pct = 100.0 * match / max(1, n)
    print("-" * 90)
    print(f"MATCH: {match}/{n} instructions = {pct:.1f}%")
    return 0 if match == n else 1


if __name__ == "__main__":
    sys.exit(main())
