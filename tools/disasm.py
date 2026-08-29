#!/usr/bin/env python3
"""Tiny capstone-based function disassembler for legoland.exe.

Given an RVA (or an export name), linearly disassemble until a likely function
end (ret padded by int3/nop, or a hard cap) and annotate call targets with
export names when known.  Used to read the game's own routines while
reverse-engineering the asset formats.
"""
import struct
import sys

import capstone

IMAGE_BASE = 0x400000


def load(path):
    d = open(path, "rb").read()
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


def exports(path):
    from dump_exports import exports as _e
    return {rva: name for name, ordi, rva in _e(path)}


def disasm(path, rva, count=400):
    d, secs = load(path)
    names = exports(path)
    off = rva2off(secs, rva)
    code = d[off:off + count * 8]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = False
    n = 0
    for insn in md.disasm(code, IMAGE_BASE + rva):
        tgt = ""
        if insn.mnemonic == "call" or insn.mnemonic.startswith("j"):
            op = insn.op_str
            if op.startswith("0x"):
                t = int(op, 16) - IMAGE_BASE
                if t in names:
                    tgt = f"   ; -> {names[t]}"
        print(f"  0x{insn.address:08x}: {insn.mnemonic:7s} {insn.op_str}{tgt}")
        n += 1
        if insn.mnemonic == "ret" and n > 3:
            # keep going a little in case of multiple rets, but cap
            pass
        if n >= count:
            break


if __name__ == "__main__":
    path = sys.argv[1]
    arg = sys.argv[2]
    cnt = int(sys.argv[3]) if len(sys.argv) > 3 else 400
    if arg.startswith("0x"):
        rva = int(arg, 16)
    else:
        exp = exports(path)
        rva = next(r for r, n in exp.items() if n == arg)
    disasm(path, rva, cnt)
