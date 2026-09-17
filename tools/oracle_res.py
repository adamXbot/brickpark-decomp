#!/usr/bin/env python3
r"""Oracle for the RES archive layer (scope PORT-C).

Entry points under test:

    RES_OpenVolume        0x00489750  LEGOLAND/data2.c
    RES_LoadDirectory     0x004895a0  LEGOLAND/resaudio2.c
    AddMasterDir          0x00489440  LEGOLAND/pathobj2.c
    RES_FindVolumeDir     0x00489550  LEGOLAND/pathobj2.c
    RES_OpenFileFromVolume 0x00489a00 LEGOLAND/data2.c
    RES_ReadFile          0x00489cf0  LEGOLAND/res.c
    RES_SetFilePointer    0x00489d70  LEGOLAND/memdb.c
    RES_GetFilePointer    0x00489db0  LEGOLAND/sweep4.c
    RES_GetFileSize       0x00489ce0  LEGOLAND/sweep4.c

Oracle: tools/resfile.py (parse_leaves) and tools/leveldata.py (res_index).
Both recover the FILE leaves of a .res, and they do it DIFFERENTLY:

  * resfile.parse_leaves scans the WHOLE file for the 0xffffffff leaf marker,
    so a member's own payload can produce a false leaf;
  * leveldata.res_index scans only from the u32 directory offset at +0, which
    is where the directory image actually lives.

The engine does neither: RES_OpenVolume reads the tail image whole and
RES_LoadDirectory walks it as a TREE of {sib, sub, isdir, size, base, name}
nodes linked by image offsets. So there are three independent recoveries of
the same set here and this oracle reports all of them. The TREE WALK is the
expectation the C is held to, because it is the engine's own algorithm; the
two byte scans are reported alongside it, and they are both short:

  * every byte scan rejects `size == 0`, so a zero-length member is invisible
    to it (`3DData\New\zoom2.pos` in Legoland.res);
  * resfile.parse_leaves additionally skips a leaf whose data offset it has
    already seen (`if off in seen: continue`), which discards the archives'
    genuine ALIAS entries -- two directory records pointing at one blob, which
    the engine files twice (4 in Graphics1.res, 7 in Graphics2.res, 1 in
    Legoland.res).

Both are decoder filters, not facts about the format; see
docs/lanes/scope-port-c.md for the verdicts.

What the C test compares against is a DIGEST over the member set, so nothing
but counts and one 64-bit number per archive leaves this script:

    FNV-1a-64 over, for each member sorted by (name upper-cased, base):
        "<NAME>;<size>;<base>;"

plus a handful of named probe members whose contents are checked by hash
through RES_ReadFile (so the read path, the seek path and the size clamp are
all exercised on real bytes).

    python3 tools/oracle_res.py --header <out.h> [--volume Graphics1]
"""
import argparse
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
# LL_GAMEDATA: a game data tree outside this checkout (legoland-browser sets it).
GAMEDATA = os.environ.get("LL_GAMEDATA") or os.path.join(ROOT, "gamedata")
sys.path.insert(0, HERE)
import resfile    # noqa: E402
import leveldata  # noqa: E402

FNV_BASIS = 0xcbf29ce484222325
FNV_PRIME = 0x100000001b3
MASK64 = (1 << 64) - 1

# The volume the test mounts. Graphics1.res is the biggest and the one the
# sprite loaders read; Legoland.res carries the level maps and the models.
DEFAULT_VOLUME = "Graphics1"


def fnv(data, h=FNV_BASIS):
    for b in data:
        h = ((h ^ b) * FNV_PRIME) & MASK64
    return h


def fnv_str(s, h=FNV_BASIS):
    return fnv(s.encode("latin1"), h)


def member_digest(members):
    """FNV-1a-64 over the canonical rendering the C test also builds.

    A MULTISET: the archives really do carry two directory entries for the
    same member (see the divergence notes), and the engine files both, so the
    digest must not silently collapse them.
    """
    h = FNV_BASIS
    for name, size, base in sorted(members, key=lambda r: (r[0].upper(), r[2], r[1])):
        h = fnv_str("%s;%d;%d;" % (name.upper(), size, base), h)
    return h


def directory_scan(data):
    """leveldata.res_index's recovery: leaves from the directory image only."""
    diroff = struct.unpack_from("<I", data, 0)[0]
    out = []
    i = diroff
    n = len(data)
    while i + 20 <= n:
        if data[i:i + 4] == b"\xff\xff\xff\xff":
            _x, zero, size, off = struct.unpack_from("<IIII", data, i + 4)
            if zero == 0 and 0 < size <= n and 4 <= off and off + size <= n:
                e = data.find(b"\x00", i + 20)
                name = data[i + 20:e]
                if name and all(0x20 <= c < 0x7f for c in name):
                    out.append((name.decode("latin1"), size, off))
        i += 1
    # dedupe on (name, base): the scan steps one byte at a time
    seen, uniq = set(), []
    for r in out:
        if (r[0].upper(), r[2]) not in seen:
            seen.add((r[0].upper(), r[2]))
            uniq.append(r)
    return uniq


def walk_tree(data):
    """RES_LoadDirectory's own recovery: the tail image as a node tree.

    Node (LEGOLAND/resaudio2.c RImgNode, 0x14 + inline name):
        +0 sib  +4 sub  +8 isdir  +0xc size  +0x10 base  +0x14 name\0
    `sib` and `sub` are offsets from the START OF THE IMAGE (-1 = none), which
    is the file's tail starting at the u32 directory offset at +0.
    """
    diroff = struct.unpack_from("<I", data, 0)[0]
    img = data[diroff:]
    out = []
    seen = set()

    def node(off, path):
        while off != -1 and off != 0xffffffff:
            if off in seen or off + 0x14 > len(img):
                return
            seen.add(off)
            sib, sub, isdir, size, base = struct.unpack_from("<iiiii", img, off)
            e = img.find(b"\x00", off + 0x14)
            name = img[off + 0x14:e].decode("latin1")
            if isdir == 0:
                out.append((name, size, base, path))
            if sub != -1:
                node(sub, path + name + "\\" if isdir else path)
            off = sib
            path = path + name + "\\" if isdir else path
        return

    # RES_OpenVolume hands RES_LoadDirectory the image root at offset 0 and the
    # master-directory root name; the root node's own siblings are the members
    # of the root directory.
    node(0, "")
    return out


# Probe members read back through RES_ReadFile: a small one, a large one and
# one from a sub-directory, chosen by size rank so the choice is data-driven
# and stable rather than hard-coded asset names.
N_PROBES = 6
PROBE_READ = 4096          # bytes hashed from the front of each probe
PROBE_SEEK = 333           # RES_SetFilePointer target inside each probe


def pick_probes(members):
    ranked = sorted(members, key=lambda r: (r[1], r[0].upper()))
    if not ranked:
        return []
    idx = [0, len(ranked) // 5, len(ranked) // 3, len(ranked) // 2,
           (4 * len(ranked)) // 5, len(ranked) - 1]
    out, seen = [], set()
    for i in idx:
        r = ranked[i]
        if r[0].upper() not in seen:
            seen.add(r[0].upper())
            out.append(r)
        if len(out) == N_PROBES:
            break
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header")
    ap.add_argument("--json")
    ap.add_argument("--volume", default=DEFAULT_VOLUME)
    args = ap.parse_args()

    path = os.path.join(GAMEDATA, "disc", args.volume + ".res")
    data = open(path, "rb").read()
    diroff = struct.unpack_from("<I", data, 0)[0]

    flat = [(r["name"], r["size"], r["offset"])
            for r in resfile.parse_leaves(data)]
    dirs = directory_scan(data)
    tree = [(n, s, b) for n, s, b, _p in walk_tree(data)]

    probes = pick_probes([r for r in tree if r[1] > 0])
    probe_rows = []
    for name, size, base in probes:
        body = data[base:base + size]
        probe_rows.append({
            "name": name,
            "size": size,
            "base": base,
            "head_n": min(PROBE_READ, size),
            "head_fnv": fnv(body[:PROBE_READ]),
            "whole_fnv": fnv(body),
            # RES_SetFilePointer(f, PROBE_SEEK) then one read of 64 bytes
            "seek": PROBE_SEEK if size > PROBE_SEEK + 64 else 0,
            "seek_fnv": fnv(body[PROBE_SEEK:PROBE_SEEK + 64]
                            if size > PROBE_SEEK + 64 else body[:64]),
            "seek_n": 64 if size > PROBE_SEEK + 64 else min(64, size),
        })

    out = {
        "volume": args.volume,
        "file_size": len(data),
        "directory_offset": diroff,
        "flat_scan_members": len(flat),
        "directory_scan_members": len(dirs),
        "tree_walk_members": len(tree),
        "digest_directory_scan": "0x%016x" % member_digest(dirs),
        "digest_tree_walk": "0x%016x" % member_digest(tree),
        "digest_flat_scan": "0x%016x" % member_digest(flat),
        "agree_tree_vs_directory":
            member_digest(tree) == member_digest(dirs),
        "agree_flat_vs_directory":
            member_digest(flat) == member_digest(dirs),
        "in_tree_not_in_directory_scan": sorted(
            set((n.upper(), s_, b) for n, s_, b in tree)
            - set((n.upper(), s_, b) for n, s_, b in dirs)),
        "in_directory_scan_not_in_flat_scan": sorted(
            set((n.upper(), s_, b) for n, s_, b in dirs)
            - set((n.upper(), s_, b) for n, s_, b in flat)),
        "probes": [{k: (("0x%016x" % v) if k.endswith("fnv") else v)
                    for k, v in p.items()} for p in probe_rows],
    }

    if args.header:
        with open(args.header, "w") as fh:
            w = fh.write
            w("/* GENERATED by tools/oracle_res.py -- do not edit. */\n")
            w("#ifndef ORACLE_RES_H\n#define ORACLE_RES_H\n\n")
            w('#define ORACLE_RES_VOLUME    "%s"\n' % args.volume)
            w("#define ORACLE_RES_FILE_SIZE %d\n" % len(data))
            w("#define ORACLE_RES_DIROFF    %d\n" % diroff)
            w("/* the engine's own recovery (the node tree RES_LoadDirectory\n"
              "   walks), which is what the C must reproduce */\n")
            w("#define ORACLE_RES_MEMBERS   %d\n" % len(tree))
            w("#define ORACLE_RES_DIGEST    0x%016xull\n\n"
              % member_digest(tree))
            w("/* the two byte-scan recoveries, for the divergence notes in\n"
              "   docs/lanes/scope-port-c.md: both reject size==0 members and\n"
              "   tools/resfile.py additionally drops members that share a data\n"
              "   offset with another member. */\n")
            w("#define ORACLE_RES_DIRSCAN_MEMBERS %d\n" % len(dirs))
            w("#define ORACLE_RES_DIRSCAN_DIGEST  0x%016xull\n"
              % member_digest(dirs))
            w("#define ORACLE_RES_FLAT_MEMBERS %d\n" % len(flat))
            w("#define ORACLE_RES_FLAT_DIGEST  0x%016xull\n\n"
              % member_digest(flat))
            w("#define ORACLE_RES_PROBE_READ %d\n" % PROBE_READ)
            w("static const struct {\n"
              "    const char*        name;\n"
              "    int                size;\n"
              "    int                base;\n"
              "    int                head_n;\n"
              "    unsigned long long head_fnv;\n"
              "    unsigned long long whole_fnv;\n"
              "    int                seek;\n"
              "    int                seek_n;\n"
              "    unsigned long long seek_fnv;\n"
              "} oracle_res_probes[] = {\n")
            for p in probe_rows:
                w('    { "%s", %d, %d, %d, 0x%016xull, 0x%016xull, '
                  '%d, %d, 0x%016xull },\n'
                  % (p["name"].replace("\\", "\\\\"), p["size"], p["base"],
                     p["head_n"], p["head_fnv"], p["whole_fnv"],
                     p["seek"], p["seek_n"], p["seek_fnv"]))
            w("};\n#define ORACLE_RES_PROBE_N %d\n\n" % len(probe_rows))
            w("#endif\n")

    if args.json:
        with open(args.json, "w") as fh:
            json.dump(out, fh, indent=1)
    if not args.header and not args.json:
        json.dump(out, sys.stdout, indent=1)
        print()


if __name__ == "__main__":
    main()
