#!/usr/bin/env python3
"""Dump the export table of a PE (name, ordinal, RVA).

legoland.exe exports 716 readable names (675 functions and 41 data symbols),
which are the backbone of the decompilation effort.  This writes a
sorted `name<TAB>ordinal<TAB>0xRVA` list for use as reccmp annotation seeds and
a Ghidra bulk-rename source.
"""
import struct
import sys


def sections(d):
    e = struct.unpack_from("<I", d, 0x3C)[0]
    coff = e + 4
    nsec = struct.unpack_from("<H", d, coff + 2)[0]
    optsz = struct.unpack_from("<H", d, coff + 16)[0]
    base = coff + 20 + optsz
    out = []
    for i in range(nsec):
        o = base + i * 40
        vsz, va, rawsz, rawptr = struct.unpack_from("<IIII", d, o + 8)
        out.append((va, vsz, rawptr, rawsz))
    return e, coff, out


def rva2off(secs, rva):
    for va, vsz, rp, rs in secs:
        if va <= rva < va + max(vsz, rs):
            return rp + (rva - va)
    return None


def exports(path):
    d = open(path, "rb").read()
    e, coff, secs = sections(d)
    opt = coff + 20
    exp_rva, exp_sz = struct.unpack_from("<II", d, opt + 96)
    if not exp_rva:
        return []
    eo = rva2off(secs, exp_rva)
    ordbase = struct.unpack_from("<I", d, eo + 16)[0]
    naddr = struct.unpack_from("<I", d, eo + 20)[0]
    nnames = struct.unpack_from("<I", d, eo + 24)[0]
    addr_rva = struct.unpack_from("<I", d, eo + 28)[0]
    name_rva = struct.unpack_from("<I", d, eo + 32)[0]
    ord_rva = struct.unpack_from("<I", d, eo + 36)[0]
    ao = rva2off(secs, addr_rva)
    no = rva2off(secs, name_rva)
    oo = rva2off(secs, ord_rva)
    res = []
    for i in range(nnames):
        nrva = struct.unpack_from("<I", d, no + i * 4)[0]
        so = rva2off(secs, nrva)
        end = d.find(b"\0", so)
        name = d[so:end].decode("latin1")
        ordi = struct.unpack_from("<H", d, oo + i * 2)[0]
        frva = struct.unpack_from("<I", d, ao + ordi * 4)[0]
        res.append((name, ordi + ordbase, frva))
    return res


if __name__ == "__main__":
    path = sys.argv[1]
    exp = exports(path)
    for name, ordi, rva in sorted(exp):
        print(f"{name}\t{ordi}\t0x{rva:08x}")
    print(f"# {len(exp)} exports", file=sys.stderr)
