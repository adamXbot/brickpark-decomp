#!/usr/bin/env python3
r"""Oracle for the LLIDB element database loader (scope PORT-C).

Entry points under test:

    LLIDB_LoadICM      0x0047aff0  LEGOLAND/data2.c
    LLIDB_GetCount     0x0047b2d0  LEGOLAND/llidb.c
    LLIDB_GetElement   0x0047b2e0  LEGOLAND/llidb.c
    LLIDB_FindElement  0x0047b330  LEGOLAND/llidb.c
    ElemID             0x0047b3f0  LEGOLAND/mapinit.c

Oracle: tools/tilemap.py `load_icm`, which is the clean-room reading of
LEGOLAND.ICM:

    u32 count
    count * 20-byte element records (raw; only +8, the type/flags word,
                                    survives the load)
    count * { u32 len, len bytes }  x2     label then value (image filename)

and the flag fixup the loader applies: `type_flags &= ~0xa` on the way in and
`&= ~1` after the strings, i.e. the element's type word is the file's word with
bits 0, 1 and 3 cleared. Lookups are BY NAME and case-insensitive
(LLIDB_FindElement uses the game's own stricmp).

Nothing but counts, a digest and a few probe names leaves this script. The
digest is FNV-1a-64 over, for each element in INDEX order:

    "<index>;<LABEL>;<VALUE>;<type_flags>;"

-- index order matters: the save format refers to elements by index, and the
page table (256 elements per 0x1400-byte page) has to come out in file order.

    python3 tools/oracle_icm.py --header <out.h>
"""
import argparse
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import tilemap  # noqa: E402

FNV_BASIS = 0xcbf29ce484222325
FNV_PRIME = 0x100000001b3
MASK64 = (1 << 64) - 1

ICM = os.path.join(ROOT, "gamedata", "main", "Legoland.icm")

# LLElem is 20 bytes on the wire; if that ever stops being true the loader's
# bulk `_read(page, n * sizeof(LLElem))` reads the wrong number of bytes.
LLELEM_SIZE = 20


def fnv_str(s, h=FNV_BASIS):
    for b in s.encode("latin1"):
        h = ((h ^ b) * FNV_PRIME) & MASK64
    return h


def load(path=ICM):
    """tilemap.load_icm plus the raw type word, so the flag fixup is checkable."""
    b = open(path, "rb").read()
    count = struct.unpack_from("<I", b, 0)[0]
    elems = []
    o = 4 + count * LLELEM_SIZE
    for i in range(count):
        raw = struct.unpack_from("<I", b, 4 + i * LLELEM_SIZE + 8)[0]
        ln = struct.unpack_from("<I", b, o)[0]; o += 4
        label = b[o:o + ln].decode("latin1"); o += ln
        lv = struct.unpack_from("<I", b, o)[0]; o += 4
        value = b[o:o + lv].decode("latin1"); o += lv
        elems.append({"index": i, "label": label, "value": value,
                      "raw_flags": raw,
                      # LLIDB_LoadICM: &= ~0xa, then &= ~1
                      "type_flags": raw & ~0xb & 0xffffffff})
    return {"count": count, "elems": elems, "leftover": len(b) - o,
            "file_size": len(b)}


def digest(elems):
    h = FNV_BASIS
    for e in elems:
        h = fnv_str("%d;%s;%s;%d;" % (e["index"], e["label"].upper(),
                                      e["value"].upper(), e["type_flags"]), h)
    return h


# Probes for LLIDB_FindElement / ElemID. Names come from the database itself so
# nothing is hard-coded, with the case deliberately mangled to prove the lookup
# is case-insensitive, plus one name that is not there.
def pick_probes(elems):
    if not elems:
        return []
    idx = [0, 1, len(elems) // 3, len(elems) // 2, len(elems) - 1]
    out = []
    for k, i in enumerate(idx):
        e = elems[i]
        name = e["label"]
        mangled = "".join(c.lower() if (j + k) % 2 else c.upper()
                          for j, c in enumerate(name))
        out.append({"query": mangled, "found": 1, "index": e["index"],
                    "label": e["label"], "value": e["value"],
                    "type_flags": e["type_flags"]})
    out.append({"query": "NO SUCH ELEMENT IN THE ICM", "found": 0,
                "index": 0, "label": "", "value": "", "type_flags": 0})
    return out


def c_str(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header")
    ap.add_argument("--json")
    args = ap.parse_args()

    d = load()
    # Cross-check against tools/tilemap.py's own reader: same count, same
    # labels, same type nibble. A difference here is a finding.
    tm = tilemap.load_icm(ICM)
    agree = (len(tm["elems"]) == d["count"] and
             all(a["label"] == b["label"] and a["value"] == b["value"] and
                 a["type"] == (b["type_flags"] & 0xfff0)
                 for a, b in zip(tm["elems"], d["elems"])))

    probes = pick_probes(d["elems"])
    by_type = {}
    for e in d["elems"]:
        by_type[e["type_flags"] & 0xfff0] = \
            by_type.get(e["type_flags"] & 0xfff0, 0) + 1

    out = {"count": d["count"], "file_size": d["file_size"],
           "trailing_bytes": d["leftover"],
           "digest": "0x%016x" % digest(d["elems"]),
           "capacity": (d["count"] + 0xff) & ~0xff,
           "pages": ((d["count"] + 0xff) & ~0xff) >> 8,
           "agrees_with_tilemap_load_icm": agree,
           "elements_by_type": {("0x%04x" % k): v
                                for k, v in sorted(by_type.items())}}

    if args.header:
        with open(args.header, "w") as fh:
            w = fh.write
            w("/* GENERATED by tools/oracle_icm.py -- do not edit. */\n")
            w("#ifndef ORACLE_ICM_H\n#define ORACLE_ICM_H\n\n")
            w('#define ORACLE_ICM_FILE     "LEGOLAND.ICM"\n')
            w("#define ORACLE_ICM_COUNT    %d\n" % d["count"])
            w("#define ORACLE_ICM_CAPACITY %d\n" % out["capacity"])
            w("#define ORACLE_ICM_PAGES    %d\n" % out["pages"])
            w("#define ORACLE_ICM_LLELEM_SIZE %d\n" % LLELEM_SIZE)
            w("#define ORACLE_ICM_DIGEST   0x%016xull\n" % digest(d["elems"]))
            w("#define ORACLE_ICM_TRAILING %d\n\n" % d["leftover"])
            w("/* LLIDB_FindElement / ElemID probes; `query` is the name with\n"
              "   its case mangled, to prove the lookup is case-insensitive. */\n")
            w("static const struct {\n"
              "    const char*  query;\n"
              "    int          found;\n"
              "    int          index;\n"
              "    const char*  label;\n"
              "    const char*  value;\n"
              "    unsigned int type_flags;\n"
              "} oracle_icm_probes[] = {\n")
            for p in probes:
                w('    { "%s", %d, %d, "%s", "%s", 0x%08xu },\n'
                  % (c_str(p["query"]), p["found"], p["index"],
                     c_str(p["label"]), c_str(p["value"]), p["type_flags"]))
            w("};\n#define ORACLE_ICM_PROBE_N %d\n\n" % len(probes))
            w("#endif\n")

    if args.json:
        with open(args.json, "w") as fh:
            json.dump(out, fh, indent=1)
    if not args.header and not args.json:
        json.dump(out, sys.stdout, indent=1)
        print()


if __name__ == "__main__":
    main()
