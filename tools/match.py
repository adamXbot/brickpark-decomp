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

Extent rules (ported from tools/audit.py, 2026-09-03). The original function's
TRUE extent is found by control flow — the first `ret` or unconditional direct
`jmp` that no earlier branch jumps past, following `switch` jump tables in
.rdata and stopping at 16-aligned padding for bodies that end in a `noreturn`
call — and the compiled COMDAT is trimmed to that many instructions. A void
tail-`jmp` wrapper, a body ending in `exit()`, and a recursive function (whose
un-relocated self-call disassembles as a bare numeric target) therefore all
compare correctly. The gate is the same as audit.py's: same instruction count,
same byte length, zero normalised mismatches, and no branch in our body that
targets past the trimmed end (`ESCAPES`).
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
# The cl wrapper lives in the sibling adamXbot/alphateam checkout (its
# tools/setup_toolchain_macos.sh populates our toolchain/). Override with
# LEGOLAND_CL when it lives elsewhere.
CL_WRAPPER = os.environ.get(
    "LEGOLAND_CL",
    os.path.join(os.path.dirname(ROOT), "alphateam", "tools", "wibo-msvc", "cl"))

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


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
    # A relocation against an ABSOLUTE symbol is resolved by the linker to the
    # symbol's value plus the field's addend, not to an address, so it is
    # resolved the same way here. VC6 emits one: every SEH frame's fs:[0]
    # (`mov eax, fs:[0]` / `mov fs:[0], esp`) is a relocation against
    # `__except_list`, an UNDEFINED external in the object (section 0) that
    # the CRT library defines as the absolute 0. Without this the sentinel
    # made the operand fs:[<abs>] against the linked original's fs:[0].
    KNOWN_ABSOLUTE = {"__except_list": 0}
    def absolute_value(symidx):
        if symidx >= nsym:
            return None
        rec = d[symptr + symidx * 18:symptr + symidx * 18 + 18]
        value, secnum = struct.unpack_from("<Ih", rec, 8)
        if secnum == -1:
            return value
        if secnum == 0:
            return KNOWN_ABSOLUTE.get(symname(rec))
        return None
    def patched_section(secidx):
        o = 20 + secidx * 40
        rawsz, rawptr, relptr = struct.unpack_from("<III", d, o + 16)
        nrel = struct.unpack_from("<H", d, o + 32)[0]
        code = bytearray(d[rawptr:rawptr + rawsz])
        for r in range(nrel):
            ro = relptr + r * 10
            va, sym, typ = struct.unpack_from("<IIH", d, ro)
            if va + 4 <= len(code):
                absval = absolute_value(sym)
                if absval is not None:
                    addend = struct.unpack_from("<I", code, va)[0]
                    struct.pack_into("<I", code, va, (absval + addend) & 0xffffffff)
                else:
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
BRANCHY = ("call", "loop", "loope", "loopne", "loopz", "loopnz")
_NUM = re.compile(r"^(0x[0-9a-f]+|\d+)$")


def _branch_target(insn):
    """Numeric target of a direct branch/call, or None (indirect / not a branch).
    Capstone prints targets below 10 as bare decimal (`jmp 8`), so both
    spellings are accepted."""
    if not (insn.mnemonic.startswith("j") or insn.mnemonic.startswith(BRANCHY)):
        return None
    op = insn.op_str.strip()
    m = _NUM.match(op)
    if not m:
        return None
    return int(op, 16) if op.startswith("0x") else int(op)


def norm(insn):
    """Normalise an instruction for comparison: keep mnemonic + operand shape,
    blank out absolute addresses/displacements that differ due to relocation.

    Every direct branch/call target — `jcc`/`jmp`/`call` and the `loop`
    family, hex or bare decimal — becomes `<t>`: a recursive self-call inside
    its own COMDAT is not relocated and decodes as a small numeric target where
    the original shows an absolute address, and `loop` targets differ the same
    way. Both are exact code that a mnemonic-only rule mis-scored."""
    if _branch_target(insn) is not None:
        return insn.mnemonic + " <t>"
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
    """First-`ret` disassembly. Kept for callers that want the legacy prefix
    view; the matcher itself uses the extent-bounded walk below."""
    out = []
    for insn in md.disasm(code, base):
        out.append(insn)
        if insn.mnemonic == "ret":
            break
    return out


# --- true extent of the original function ----------------------------------- #
_EXPORT_RVAS = None


def export_rvas():
    """Sorted RVAs of every exported symbol (symbols/legoland.exports.txt).

    Used ONLY as an upper bound for deciding that a forward `jmp` leaves the
    function; never as the function's end — many functions are NOT exported,
    so a `ret` followed by a new function would look like an early return."""
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
    toff = rva2off(secs, int(m.group(1), 16) - IMAGE_BASE)
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


def _loop_entry(insns, addr_index, i, tgt):
    """True if the forward `jmp` at insns[i] (target tgt) is a rotated loop's
    entry: walking on from tgt, some direct branch met before the first `ret`
    targets the skipped region (jmp_end, tgt) -- the loop body. A tail-jmp
    wrapper never satisfies this: nothing reached from another function's
    entry branches back into the padding before it."""
    lo = insns[i].address + insns[i].size
    k = addr_index.get(tgt)
    if k is None:
        return False
    for j in range(k, min(k + 4000, len(insns))):
        y = insns[j]
        if y.mnemonic == "ret":
            return False
        if y.mnemonic.startswith("j"):
            m = re.match(r"^0x([0-9a-f]+)$", y.op_str.strip())
            if m:
                t = int(m.group(1), 16)
                if lo <= t < tgt:
                    return True
    return False


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
    va = rva + IMAGE_BASE
    insns = list(md.disasm(d[off:off + 0x4000], va))
    exps = export_rvas()
    nxt = None
    for e in exps:
        if e > rva:
            nxt = e + IMAGE_BASE
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
                    if tgt > x.address and not external(tgt) and _loop_entry(insns, addr_index, i, tgt):
                        # A forward jmp over a block that a later conditional
                        # branch jumps BACK into is a rotated loop's entry
                        # (`jmp cond; body: ...; cond: ...; jcc body`), not the
                        # end of the function. MatMul (0x00426120) and
                        # Coaster3D_BuildPieceGeometry (0x004284d0) were both
                        # truncated at such a jmp, reported ESCAPES against the
                        # truncated extent, and could never print [OK].
                        furthest = max(furthest, tgt)
                        continue
                    return i + 1, sum(k.size for k in insns[:i + 1])
                if not external(tgt):
                    furthest = max(furthest, tgt)
            elif x.mnemonic == "jmp":
                for t in _table_targets(d, secs, x.op_str, va, va + 0x4000):
                    furthest = max(furthest, t)
        if x.mnemonic == "ret" and x.address >= furthest:
            return i + 1, sum(k.size for k in insns[:i + 1])
    return None, None


def original_body(d, secs, rva):
    """The original function's instructions bounded by true_extent(), plus
    (n_ins, n_bytes). Falls back to end_of_body() when the extent cannot be
    established."""
    off = rva2off(secs, rva)
    if off is None:
        raise SystemExit("address 0x%08x is outside every section" % (rva + IMAGE_BASE))
    n_ins, n_bytes = true_extent(d, secs, rva)
    if n_ins is None:
        insns = end_of_body(list(md.disasm(d[off:off + 0x4000], rva + IMAGE_BASE)))
        return insns, None, None
    insns = list(md.disasm(d[off:off + n_bytes], rva + IMAGE_BASE))[:n_ins]
    return insns, n_ins, n_bytes


def compiled_body(insns, n_ins):
    """Trim a compiled COMDAT to the ORIGINAL's extent (n_ins instructions).

    /Gy emits switch jump tables straight after the code, and disassembling
    them yields junk "instructions"; trailing alignment padding follows. The
    original's extent is known exactly, so take that many instructions and
    then require that no direct branch among them escapes past the trimmed
    end — the compiled body may not hide reachable code beyond what the
    original has. Returns (body, escapes)."""
    if n_ins is None:
        return end_of_body(insns), False
    body = insns[:n_ins]
    end = body[-1].address + body[-1].size if body else 0
    code_len = sum(x.size for x in insns)
    escapes = False
    for x in body:
        if x.mnemonic.startswith("j"):
            tgt = _branch_target(x)
            # match.py patches every relocated field to the 0x00990099
            # sentinel, so a call/jmp to another symbol decodes to a target
            # far outside the COMDAT; only in-section targets can escape.
            if tgt is not None and 0 <= tgt < code_len and tgt >= end:
                escapes = True
    return body, escapes


def end_of_body(insns):
    """Fallback trim when the original extent is unknown: first `ret` (or
    external `jmp`, zero rel32) that no earlier branch jumps past."""
    furthest = 0
    for i, x in enumerate(insns):
        if x.mnemonic.startswith("j"):
            tgt = _branch_target(x)
            if tgt is not None:
                if (x.mnemonic == "jmp" and x.size == 5 and tgt == x.address + x.size
                        and x.address >= furthest):
                    return insns[:i + 1]
                furthest = max(furthest, tgt)
        if x.mnemonic == "ret" and x.address >= furthest:
            return insns[:i + 1]
    while insns and insns[-1].mnemonic in ("nop", "int3"):
        insns.pop()
    return insns


# --- compile ---------------------------------------------------------------- #
def compile_obj(src, flags, obj):
    cmd = [CL_WRAPPER, "/nologo", "/c", "/Fo" + obj] + flags.split() + [src]
    env = dict(os.environ, ALPHATEAM_VC6_ROOT=os.path.join(ROOT, "toolchain"))
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=ROOT)
    if r.returncode != 0:
        print(r.stdout); print(r.stderr, file=sys.stderr)
        raise SystemExit("compile failed")


def compare(orig, comp, n_ins, n_bytes, escapes):
    """Index-for-index comparison. Returns (match, n, gate_ok, reasons)."""
    n = max(len(comp), len(orig))
    match = sum(1 for i in range(n)
                if i < len(orig) and i < len(comp) and norm(orig[i]) == norm(comp[i]))
    c_bytes = sum(x.size for x in comp)
    reasons = []
    if n_ins is not None and len(comp) != n_ins:
        reasons.append(f"{len(comp)} vs {n_ins} instructions")
    if n_bytes is not None and c_bytes != n_bytes:
        reasons.append(f"{c_bytes} vs {n_bytes} bytes")
    if escapes:
        reasons.append("ESCAPES")
    if n_ins is None:
        reasons.append("extent unknown")
    return match, n, (match == n and not reasons), reasons


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src"); ap.add_argument("func"); ap.add_argument("addr")
    ap.add_argument("--flags", default="/O2 /Gy /Gd")
    ap.add_argument("--obj", default=None,
                    help="object path (default: unique per pid, so parallel runs don't collide)")
    args = ap.parse_args()
    rva = int(args.addr, 16)
    if rva >= IMAGE_BASE:
        rva -= IMAGE_BASE

    obj = args.obj or ("/tmp/_match_%d.obj" % os.getpid())
    compile_obj(args.src, args.flags, obj)
    try:
        code = obj_function_code(obj, args.func)
    finally:
        if args.obj is None:
            try:
                os.remove(obj)
            except OSError:
                pass

    d, secs = load_exe()
    orig, n_ins, n_bytes = original_body(d, secs, rva)
    comp, escapes = compiled_body(list(md.disasm(code, 0)), n_ins)

    match, n, gate_ok, reasons = compare(orig, comp, n_ins, n_bytes, escapes)
    print(f"{'ORIGINAL @ 0x%08x' % (rva + IMAGE_BASE):42s} | RECOMPILED ({args.func})")
    print("-" * 90)
    for i in range(n):
        o = orig[i] if i < len(orig) else None
        c = comp[i] if i < len(comp) else None
        ok = o and c and norm(o) == norm(c)
        os_ = f"{o.mnemonic} {o.op_str}" if o else ""
        cs_ = f"{c.mnemonic} {c.op_str}" if c else ""
        mark = " " if ok else "X"
        print(f"{mark} {os_:40s} | {cs_}")
    pct = 100.0 * match / max(1, n)
    print("-" * 90)
    extent = f"orig={n_ins}i/{n_bytes}B" if n_ins is not None else "orig=?"
    gate = "extent ok" if gate_ok else "; ".join(reasons) or "mismatch"
    print(f"MATCH: {match}/{n} instructions = {pct:.1f}%  [{extent}; {gate}]")
    return 0 if gate_ok else 1


if __name__ == "__main__":
    sys.exit(main())
