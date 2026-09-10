#!/usr/bin/env python3
r"""Oracle for LoadPos, the .pos/.3d position-and-orientation loader (PORT-C).

Entry points under test:

    LoadPos              0x0043f660  LEGOLAND/loaders.c
    BuildYRotationMatrix 0x00443360  LEGOLAND/math3d.c
    MatrixMultiply       0x00443270  LEGOLAND/math3d.c
    CopyMatrix           0x00443490  LEGOLAND/math3d.c
    UnloadPos            0x0043f7d0  LEGOLAND/math3d.c

Oracle: tools/geom.py, whose docstring is the recovery of this very function
("The .3d record layout was recovered by disassembling the game's own loader
sub_43f660"). geom.py stops short of one thing, deliberately: it reads the
12 floats per 48-byte element but does NOT apply the quarter-turn about Y that
the loader performs at load time. This oracle adds that step, in float32 and in
the loader's own order:

    rot = BuildYRotationMatrix(1.5707963f)      # [c,0,s, 0,1,0, -s,0,c]
    tmp = MatrixMultiply(rot, m)                # out[r][c] = sum_k a[r][k]*b[k][c]
    m   = CopyMatrix(tmp)

so that what the C produces can be compared element by element.

LAYOUT (LEGOLAND/loaders.c PosTable / PosItem, and geom.py's parse_3d):

    u32 per      # items per frame  (geom.py's A, "frames per part")
    u32 count    # frames           (geom.py's B, "part count")
    count * per * { int a, int b, int c, float m[9] }        # 48 bytes each

The first three words are read raw into `int` fields by the loader and as
floats by geom.py. Both are right: the loader never looks at them, it only
copies the bytes, so the interpretation is free. Nothing in the serialised
record is a pointer, which is what makes the *record* layout size-independent
-- but the loader reaches the bytes through RES_OpenFile, and that path is
ILP32-only (see portable/tests/test_res_archive.c), so the test is still
wasm32-only.

Output is counts, a digest and a small sample of derived matrix values.

    python3 tools/oracle_geom.py --header <out.h> [--volume Legoland]
"""
import argparse
import json
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import geom  # noqa: E402

FNV_BASIS = 0xcbf29ce484222325
FNV_PRIME = 0x100000001b3
MASK64 = (1 << 64) - 1

ELEM = 48
ANGLE = 1.5707963          # the literal in LEGOLAND/loaders.c, as a float
N_MEMBERS = 4              # how many .pos members the test loads
SAMPLE_ELEMS = 6           # per member: how many elements are listed in full


def f32(x):
    """Round a Python float to float32, as every C float operation does."""
    return struct.unpack("<f", struct.pack("<f", x))[0]


def fnv_str(s, h=FNV_BASIS):
    for b in s.encode("latin1"):
        h = ((h ^ b) * FNV_PRIME) & MASK64
    return h


def y_rotation(angle):
    a = f32(angle)
    s = f32(math.sin(a))
    c = f32(math.cos(a))
    return [c, 0.0, s, 0.0, 1.0, 0.0, f32(-s), 0.0, c]


def matmul(a, b):
    """MatrixMultiply, term by term and rounded like the C does."""
    out = [0.0] * 9
    for r in range(3):
        for col in range(3):
            acc = f32(a[r * 3 + 0] * b[0 * 3 + col])
            acc = f32(acc + f32(a[r * 3 + 1] * b[1 * 3 + col]))
            acc = f32(acc + f32(a[r * 3 + 2] * b[2 * 3 + col]))
            out[r * 3 + col] = acc
    return out


def parse_pos(body):
    """The loader's own reading of the member: per, count, items, rotated."""
    per, count = struct.unpack_from("<II", body, 0)
    need = 8 + per * count * ELEM
    if need > len(body):
        return None
    rot = y_rotation(ANGLE)
    frames = []
    o = 8
    for _i in range(count):
        items = []
        for _j in range(per):
            a, b, c = struct.unpack_from("<iii", body, o)
            m = [f32(v) for v in struct.unpack_from("<9f", body, o + 12)]
            items.append({"a": a, "b": b, "c": c, "m": matmul(rot, m)})
            o += ELEM
        frames.append(items)
    return {"per": per, "count": count, "frames": frames,
            "bytes_used": need, "member_size": len(body),
            "trailing": len(body) - need}


def cf(v):
    """A C float literal that is always a valid one ("0" alone is not)."""
    t = "%.9g" % v
    if "." not in t and "e" not in t and "E" not in t and "inf" not in t:
        t += ".0"
    return t + "f"


def quant(v):
    """Integer rendering for the digest: the C test quantises identically."""
    return int(math.floor(v * 4096.0 + 0.5))


def digest(p):
    h = FNV_BASIS
    h = fnv_str("%d;%d;" % (p["per"], p["count"]), h)
    for items in p["frames"]:
        for it in items:
            h = fnv_str("%d;%d;%d;" % (it["a"], it["b"], it["c"]), h)
            for v in it["m"]:
                h = fnv_str("%d;" % quant(v), h)
    return h


def res_members(path):
    data = open(path, "rb").read()
    return data, geom.parse_res_dir(data)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header")
    ap.add_argument("--json")
    ap.add_argument("--volume", default="Legoland")
    args = ap.parse_args()

    path = os.path.join(ROOT, "gamedata", "disc", args.volume + ".res")
    data, recs = res_members(path)

    # .pos members that decode cleanly, smallest first so the test stays quick.
    cand = []
    for r in recs:
        off, size, name = r[0], r[1], r[2]
        if not name.lower().endswith(".pos") or size < 8:
            continue
        p = parse_pos(data[off:off + size])
        if p is None or p["count"] == 0 or p["per"] == 0:
            continue
        cand.append((name, off, size, p))
    cand.sort(key=lambda t: (t[2], t[0].upper()))
    picked = cand[:N_MEMBERS // 2] + cand[-(N_MEMBERS - N_MEMBERS // 2):]
    seen, members = set(), []
    for t in picked:
        if t[0].upper() in seen:
            continue
        seen.add(t[0].upper())
        members.append(t)

    rows = []
    for name, off, size, p in members:
        sample = []
        flat = [it for items in p["frames"] for it in items]
        step = max(1, len(flat) // SAMPLE_ELEMS)
        for i in range(0, len(flat), step):
            if len(sample) == SAMPLE_ELEMS:
                break
            sample.append((i, flat[i]))
        rows.append({"name": name, "size": size, "per": p["per"],
                     "count": p["count"], "elements": p["per"] * p["count"],
                     "bytes_used": p["bytes_used"], "trailing": p["trailing"],
                     "digest": digest(p), "sample": sample})

    rot = y_rotation(ANGLE)
    out = {"volume": args.volume, "pos_members_found": len(cand),
           "members": [{k: (("0x%016x" % v) if k == "digest" else v)
                        for k, v in r.items() if k != "sample"} for r in rows],
           "y_rotation": rot}

    if args.header:
        with open(args.header, "w") as fh:
            w = fh.write
            w("/* GENERATED by tools/oracle_geom.py -- do not edit. */\n")
            w("#ifndef ORACLE_GEOM_H\n#define ORACLE_GEOM_H\n\n")
            w('#define ORACLE_GEOM_VOLUME "%s"\n' % args.volume)
            w("#define ORACLE_GEOM_ANGLE  %s\n" % cf(ANGLE))
            w("#define ORACLE_GEOM_QUANT  4096.0f\n")
            w("/* tolerance on a single matrix term, in quantiser units */\n")
            w("#define ORACLE_GEOM_TOL    1\n\n")
            w("/* BuildYRotationMatrix(ORACLE_GEOM_ANGLE) */\n")
            w("static const float oracle_geom_rot[9] = { %s };\n\n"
              % ", ".join(cf(v) for v in rot))
            w("static const struct {\n"
              "    const char*        name;\n"
              "    int                size;\n"
              "    int                per;\n"
              "    int                count;\n"
              "    int                bytes_used;\n"
              "    int                trailing;\n"
              "    unsigned long long digest;\n"
              "} oracle_geom_members[] = {\n")
            for r in rows:
                w('    { "%s", %d, %d, %d, %d, %d, 0x%016xull },\n'
                  % (r["name"], r["size"], r["per"], r["count"],
                     r["bytes_used"], r["trailing"], r["digest"]))
            w("};\n#define ORACLE_GEOM_MEMBER_N %d\n\n" % len(rows))
            w("/* A few elements spelled out, for a readable failure: "
              "{member, flat element index, a, b, c, rotated m[9]} */\n")
            w("static const struct {\n"
              "    int   member;\n"
              "    int   elem;\n"
              "    int   a, b, c;\n"
              "    float m[9];\n"
              "} oracle_geom_samples[] = {\n")
            nsamp = 0
            for mi, r in enumerate(rows):
                for idx, it in r["sample"]:
                    w("    { %d, %d, %d, %d, %d, { %s } },\n"
                      % (mi, idx, it["a"], it["b"], it["c"],
                         ", ".join(cf(v) for v in it["m"])))
                    nsamp += 1
            w("};\n#define ORACLE_GEOM_SAMPLE_N %d\n\n" % nsamp)
            w("#endif\n")

    if args.json:
        with open(args.json, "w") as fh:
            json.dump(out, fh, indent=1)
    if not args.header and not args.json:
        json.dump(out, sys.stdout, indent=1)
        print()


if __name__ == "__main__":
    main()
