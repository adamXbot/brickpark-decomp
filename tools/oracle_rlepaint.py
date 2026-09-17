#!/usr/bin/env python3
r"""Oracle for the type-3 RLE sprite painters, over a REAL shipped sprite.

Entry points under test (LEGOLAND/rlepaint.c, LEGOLAND/rlepaint2.c):

    RLEPaintFast   0x00467f00   no clip, no hit test
    RLEPaintClipR  0x00467d10   the same frame cut at a right edge

portable/tests/test_rle_paint.c already checks all ten painters against a
synthetic sprite built from its own operation list. That is complete but it is
the test's own idea of what a frame looks like. This oracle closes the other
half: one frame of one sprite from the shipped archives, decoded by the
clean-room Python reader in tools/comp.py (`_Bits2`, `parse_header`, and the
grammar walk in `_decode16`), with the expected image reported as a digest so
that no game asset is ever written out.

The sprite is chosen deterministically: the first 16-bpp COMP member of the
archive, in archive order, whose frame 0 is at least MIN_W x MIN_H and uses
the literal single, the escape, the literal run AND the repeat run -- so the
check cannot pass on a frame that is one flat fill -- preferring, among those,
the first that also has a transparent opcode. The preference matters because
most of Graphics1.res's big frames are fully opaque, and a frame with no
transparency never exercises the "advance without writing" half of the
grammar. Only the member's identity, its byte range and two digests are
emitted; no asset content.

THE DIGEST. The painters leave transparent pixels untouched, so the reference
image is "the surface as it stands after the paint": every pixel starts at a
SENTINEL and an opaque operation overwrites it. The sentinel is chosen PER
FRAME as a 16-bit value that frame never uses (a fixed one does not work --
460 of Graphics2.res's 530 16-bpp members contain any given constant), and is
emitted in the header so the C pre-fills its scratch surface with the same
value. FNV-1a 64 over the u16 words in row order, little-endian, is what the
C recomputes.

    python3 tools/oracle_rlepaint.py --header <out.h> [--res <path>]
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import comp        # noqa: E402  the clean-room COMP reader
import resfile     # noqa: E402  the archive directory scanner

ROOT = os.path.dirname(HERE)
# LL_GAMEDATA: a game data tree outside this checkout (legoland-browser sets it).
GAMEDATA = os.environ.get("LL_GAMEDATA") or os.path.join(ROOT, "gamedata")
DEFAULT_RES = os.path.join(GAMEDATA, "disc", "Graphics1.res")

MIN_W = 16
MIN_H = 16

FNV_BASIS = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
MASK64 = (1 << 64) - 1


def fnv_bytes(h, b):
    for byte in b:
        h = ((h ^ byte) * FNV_PRIME) & MASK64
    return h


def walk_frame0(data, off):
    """Decode frame 0 of a 16-bpp COMP member to (words, opcode histogram).

    The control/pixel/length addressing and the grammar are comp.py's
    `_decode16`; only the store differs (the raw u16 word instead of an RGB
    triple) and the histogram is new.
    """
    hd = comp.parse_header(data, off)
    if hd["bpp"] != 16:
        raise ValueError("not a 16-bpp COMP member")
    w, h = hd["width"], hd["height"]
    s1, s2 = hd["s1"], hd["s2"]
    base = off + 0x28                 # COMP header 0x18 + frame header 0x10
    pix = base
    lenp = base + 2 * s1
    ctrl = base + 2 * s1 + s2

    bits = comp._Bits2(data, ctrl)
    img = [None] * (w * h)            # None = never written = transparent
    hist = {"p0": 0, "p1": 0, "p2": 0, "p3": 0, "s0": 0, "s1": 0, "s2": 0}
    x = y = 0
    guard = 0
    limit = w * h * 4 + 4096

    def put(word):
        if 0 <= x < w and y < h:
            img[y * w + x] = word

    while y < h:
        guard += 1
        if guard > limit:
            raise ValueError("runaway stream")
        hi, lo = bits.next()
        if hi == 0:
            hist["p1" if lo else "p0"] += 1
            put(struct.unpack_from("<H", data, pix)[0])
            pix += 2
            x += 1
        elif lo == 0:
            hist["p2"] += 1
            x += 1
        else:
            hist["p3"] += 1
            L = data[lenp]
            lenp += 1
            if L == 0:
                y += 1
                x = 0
                continue
            shi, slo = bits.next()
            if shi:
                hist["s2"] += 1
                x += L
            elif slo == 0:
                hist["s0"] += 1
                for _ in range(L):
                    put(struct.unpack_from("<H", data, pix)[0])
                    pix += 2
                    x += 1
            else:
                hist["s1"] += 1
                word = struct.unpack_from("<H", data, pix)[0]
                pix += 2
                for _ in range(L):
                    put(word)
                    x += 1

    frame_size = struct.unpack_from("<I", data, off + 24)[0]
    return {"w": w, "h": h, "img": img, "hist": hist,
            "frame_size": frame_size, "member_size": hd["s0"] + 24}


def digest(img, w, h, lo, hi, sentinel):
    """FNV-1a 64 over the image, keeping only source columns [lo, hi).
    Everything else -- clipped away, or never written -- is `sentinel`, which
    is what the C pre-fills its scratch surface with."""
    d = FNV_BASIS
    for y in range(h):
        row = bytearray()
        for x in range(w):
            v = img[y * w + x] if lo <= x < hi else None
            row += struct.pack("<H", sentinel if v is None else v)
        d = fnv_bytes(d, row)
    return d


def free_word(img):
    """A 16-bit value the frame never uses, so "untouched" is unambiguous.
    None if the frame somehow uses all 65536 (possible only for a frame with
    more than 65536 opaque pixels)."""
    used = set(v for v in img if v is not None)
    for v in range(0x10000):
        if v not in used:
            return v
    return None


def pick(path):
    data = resfile.load(path)
    plain = None
    for leaf in resfile.parse_leaves(data):
        if not leaf["is_comp"]:
            continue
        try:
            hd = comp.parse_header(data, leaf["offset"])
        except ValueError:
            continue
        if hd["bpp"] != 16 or hd["width"] < MIN_W or hd["height"] < MIN_H:
            continue
        try:
            dec = walk_frame0(data, leaf["offset"])
        except (ValueError, struct.error, IndexError):
            continue
        hs = dec["hist"]
        # the four opcodes every frame uses, so the check cannot pass on a
        # single flat fill
        if not (hs["p0"] and hs["p3"] and hs["s0"] and hs["s1"]):
            continue
        sentinel = free_word(dec["img"])
        if sentinel is None:
            continue
        dec["sentinel"] = sentinel
        if hs["p2"] or hs["s2"]:
            return leaf, dec          # ... and it exercises transparency
        if plain is None:
            plain = (leaf, dec)
    if plain is not None:
        return plain                  # no transparent frame qualified
    raise SystemExit("oracle_rlepaint: no suitable 16-bpp COMP member in %s"
                     % path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header", required=True)
    ap.add_argument("--res", default=DEFAULT_RES)
    args = ap.parse_args()

    leaf, dec = pick(args.res)
    w, h = dec["w"], dec["h"]
    clip = w // 2                      # the right-clip case: half the width

    out = []
    add = out.append
    add("/* GENERATED by tools/oracle_rlepaint.py -- do not edit, do not commit")
    add(" * expectations for portable/tests/test_rle_paint.c's real-sprite check.")
    add(" * Only an identity, a byte range and two digests: no asset content. */")
    add("#ifndef LL_ORACLE_RLEPAINT_H")
    add("#define LL_ORACLE_RLEPAINT_H")
    # absolute: the ctest working directory is the build tree, not the repo
    add('#define LL_RLE_RES_PATH   "%s"'
        % os.path.abspath(args.res).replace("\\", "/"))
    add('#define LL_RLE_MEMBER     "%s"' % leaf["name"])
    add("#define LL_RLE_OFFSET     %du" % leaf["offset"])
    add("#define LL_RLE_SIZE       %du" % leaf["size"])
    add("#define LL_RLE_W          %d" % w)
    add("#define LL_RLE_H          %d" % h)
    add("#define LL_RLE_FRAME_SIZE %du" % dec["frame_size"])
    add("#define LL_RLE_SENTINEL   0x%04xu" % dec["sentinel"])
    add("#define LL_RLE_CLIP_W     %d" % clip)
    add("#define LL_RLE_DIGEST     0x%016xull"
        % digest(dec["img"], w, h, 0, w, dec["sentinel"]))
    add("#define LL_RLE_DIGEST_CLIP 0x%016xull"
        % digest(dec["img"], w, h, 0, clip, dec["sentinel"]))
    hs = dec["hist"]
    add("/* opcodes in frame 0: primary 0=%d 1=%d 2=%d 3=%d,"
        " secondary 0=%d 1=%d 2=%d */"
        % (hs["p0"], hs["p1"], hs["p2"], hs["p3"], hs["s0"], hs["s1"],
           hs["s2"]))
    add("#endif")
    add("")

    with open(args.header, "w") as f:
        f.write("\n".join(out))
    print("oracle_rlepaint: %s %s %dx%d frame0 %d bytes"
          % (os.path.basename(args.res), leaf["name"], w, h, dec["frame_size"]))


if __name__ == "__main__":
    main()
