#!/usr/bin/env python3
"""Parser for Legoland.res -- the world/model geometry archive.

Legoland.res is a .res archive in the SAME container format as Graphics1.res
(u32 directory-offset at +0, member data packed from +4, a hierarchical name
directory as a tail tree). Its members are LEGO ".3d" model/animation files
(plus companion .txt / .obj / .loc / .lls texture members).

The .3d record layout was recovered by disassembling the game's own loader
`sub_43f660` in legoland.exe (called for .3d models via RES_OpenFile /
RES_ReadFile). That routine:

    A = read_u32()                 # frames per part (inner count)
    B = read_u32()                 # part count      (outer count)
    parts = malloc(B * ptr)        # one pointer per part
    for b in range(B):             # outer loop == B
        block = malloc(A * 48)     # A elements of 48 bytes
        for a in range(A):         # inner loop == A
            v0 = read 3 floats     # vertex 0 (x,y,z)
            v1,v2,v3 = read 9 floats
            # BuildYRotationMatrix(pi/2); MatrixMultiply; CopyMatrix
            #   -> rotates the 4 corner points 90 deg about Y at load time

So each 48-byte element is 12 floats == a vec3 POSITION followed by a 3x3
MATRIX (three vectors). Only the 3x3 is rotated 90 deg about Y at load; the
position is left as-is. This is a per-part, per-frame rigid transform record
(anchor point + oriented basis) for the engine's 3D-positioned sprites/blokes.
Read as four consecutive xyz points, position + the 3 matrix rows form a
planar quad for a large fraction of parts (part 0 of yipee is a clean
rectangle), but many are non-planar or collapsed -- so the code-accurate view
is vec3 + mat3, not four equal vertices.

Storage is part-major: for each of B parts, an array of A per-frame records.
A static model has A==1; an animated one has A>1 (e.g. an 8-frame walk cycle)
with the same B part count.

The animation/quad section is  8 + B*A*48  bytes. .3d members are larger than
that; the remainder is a trailing float block (values in [-1,1], candidate:
per-quad normals / UVs) that this loader does not read -- left unparsed here.
"""
import json
import os
import struct
import sys

ELEM = 48          # bytes per element = 4 vertices * 3 floats
VERTS = 4


def parse_res_dir(data):
    """Return list of (offset, size, name) file members via the tail directory.

    Robust scan for FILE (leaf) records:
        ff ff ff ff | u32 X | u32 zero==0 | u32 size | u32 data_offset | name\\0
    (folder records use a child-count where files carry the 0xffffffff marker).
    """
    diroff = struct.unpack_from("<I", data, 0)[0]
    dd = data[diroff:]
    recs = []
    i = 0
    n = len(dd)
    while i < n - 20:
        if dd[i:i + 4] == b"\xff\xff\xff\xff":
            X, z, size, off = struct.unpack_from("<IIII", dd, i + 4)
            if z == 0 and 0 < size < len(data) and 4 <= off <= len(data) \
                    and off + size <= len(data):
                j = i + 20
                k = dd.find(b"\x00", j)
                if k > j:
                    nm = dd[j:k]
                    if all(32 <= c < 127 for c in nm):
                        recs.append((off, size, nm.decode("latin1")))
                        i = k
                        continue
        i += 1
    return recs


def parse_3d(data, off, size):
    """Parse one .3d member. Returns dict with counts + decoded quads."""
    A, B = struct.unpack_from("<II", data, off)          # frames, parts
    anim_bytes = 8 + B * A * ELEM
    ok = anim_bytes <= size
    parts = []
    if ok:
        p = off + 8
        for b in range(B):
            frames = []
            for a in range(A):
                base = p + (b * A + a) * ELEM
                f = struct.unpack_from("<12f", data, base)
                frames.append([list(f[0:3]), list(f[3:6]),
                               list(f[6:9]), list(f[9:12])])
            parts.append(frames)
    return {
        "frames_per_part": A,
        "part_count": B,
        "elements_total": A * B,
        "bytes_per_element": ELEM,
        "floats_per_element": 12,
        "anim_section_bytes": anim_bytes,
        "member_size": size,
        "trailing_bytes": size - anim_bytes,
        "fits_in_member": ok,
        "parts": parts,
    }


def planar_dev(quad):
    v0, v1, v2, v3 = quad
    a = [v1[i] - v0[i] for i in range(3)]
    b = [v2[i] - v0[i] for i in range(3)]
    c = [v3[i] - v0[i] for i in range(3)]
    nrm = [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
           a[0] * b[1] - a[1] * b[0]]
    nl = sum(x * x for x in nrm) ** 0.5 or 1.0
    nrm = [x / nl for x in nrm]
    return abs(sum(nrm[i] * c[i] for i in range(3)))


def main():
    res = sys.argv[1] if len(sys.argv) > 1 else \
        "gamedata/disc/Legoland.res"
    outdir = sys.argv[2] if len(sys.argv) > 2 else None
    data = open(res, "rb").read()
    recs = parse_res_dir(data)
    threed = [r for r in recs if r[2].lower().endswith(".3d")]

    summary = {
        "file": os.path.abspath(res),
        "file_size": len(data),
        "dir_offset": struct.unpack_from("<I", data, 0)[0],
        "container": "res-archive (dir-offset@+0, tail name tree; same as Graphics*.res)",
        "member_count": len(recs),
        "model_3d_count": len(threed),
        "element_layout": "vec3 position + 3x3 matrix (12 floats, 48 bytes); "
                          "loader rotates the 3x3 90deg about Y at load",
        "record_layout": "u32 frames_per_part, u32 part_count, then "
                         "part_count*frames_per_part elements (part-major)",
        "models": [],
    }
    tot = coll = planar = 0
    for off, size, name in threed:
        m = parse_3d(data, off, size)
        if m["fits_in_member"]:
            for b in range(m["part_count"]):
                for a in range(m["frames_per_part"]):
                    q = m["parts"][b][a]
                    tot += 1
                    scale = max((sum((q[i][k] - q[j][k]) ** 2
                                     for k in range(3))) ** 0.5
                                for i in range(4) for j in range(4))
                    if scale < 1e-4:
                        coll += 1
                        continue
                    if planar_dev(q) / scale < 0.01:
                        planar += 1
        summary["models"].append({
            "name": name, "offset": off, "size": size,
            "frames_per_part": m["frames_per_part"],
            "part_count": m["part_count"],
            "elements_total": m["elements_total"],
            "anim_section_bytes": m["anim_section_bytes"],
            "trailing_bytes": m["trailing_bytes"],
            "fits_in_member": m["fits_in_member"],
        })
    summary["elements_total_all_models"] = tot
    summary["elements_collapsed_pct"] = round(100 * coll / tot, 1) if tot else 0
    summary["elements_planar_quad_pct"] = round(100 * planar / tot, 1) if tot else 0

    if outdir:
        os.makedirs(outdir, exist_ok=True)
        with open(os.path.join(outdir, "summary.json"), "w") as fh:
            json.dump(summary, fh, indent=2)
        # dump first model's first-frame quads as OBJ for eyeballing
        off, size, name = threed[0]
        m = parse_3d(data, off, size)
        with open(os.path.join(outdir, "model0_frame0.obj"), "w") as fh:
            fh.write("# %s  part_count=%d frame0 quads\n" % (name, m["part_count"]))
            vi = 1
            for b in range(m["part_count"]):
                q = m["parts"][b][0]
                for v in q:
                    fh.write("v %f %f %f\n" % tuple(v))
                fh.write("f %d %d %d %d\n" % (vi, vi + 1, vi + 3, vi + 2))
                vi += 4
        with open(os.path.join(outdir, "sample_records.json"), "w") as fh:
            off, size, name = threed[0]
            m = parse_3d(data, off, size)
            json.dump({
                "model": name,
                "frames_per_part": m["frames_per_part"],
                "part_count": m["part_count"],
                "part0_frame0_quad": m["parts"][0][0],
                "part0_frame1_quad": m["parts"][0][1],
                "part1_frame0_quad": m["parts"][1][0],
            }, fh, indent=2)

    print(json.dumps(summary, indent=2)[:1200])
    print("...\nmodels:", len(threed), "max_planar_dev(sampled):", max_planar)


if __name__ == "__main__":
    main()
