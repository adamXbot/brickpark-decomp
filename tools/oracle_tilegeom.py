#!/usr/bin/env python3
r"""Oracle for the isometric tile<->screen geometry and the walker tile tests.

Entry points under test:

    GetTileDimensions  0x00460540  (LEGOLAND/map.c)      w = 2h from the sprite
    GetTileCentre      0x0045ad60  (LEGOLAND/tilehelp.c) tile -> pixel centre
    GetTileBounds      0x0045acc0  (LEGOLAND/pathbuild.c) tile -> pixel rect
    OverNewTile        0x00483650  (LEGOLAND/tilehelp.c) left the 256-unit tile
    CrossTileCentre    0x004837d0  (LEGOLAND/tilehelp.c) crossed sub-tile 0x80

The formulas are the ones LEGOLAND/tilehelp.c's header states and that
tools/tilemap.py implements independently for its per-cell placement
(`left = TH*(x-y-1)`, `top = ((TH+1)>>1)*(x+y)` at scroll/origin zero, with
TH = the default ground sprite's height, 16 for the shipped 32x16 tiles):

    centre_x = ((w+1)>>1) * (tx - ty)     - (g_scroll_x>>8) + map->origin_x
    centre_y = ((h+1)>>1) * (tx + ty + 1) - (g_scroll_y>>8) + map->origin_y
    left     = ((w+1)>>1) * (tx - ty - 1) - (g_scroll_x>>8) + map->origin_x
    top      = ((h+1)>>1) * (tx + ty)     - (g_scroll_y>>8) + map->origin_y
    right    = left + w - 1 ;  bottom = top + h - 1

tilemap.py pins only the scroll=origin=0 case; this oracle extends it to a
non-zero viewport origin and a non-zero 8.8 scroll, which is the form the two
engine functions actually compute. The extension is the *only* thing here that
tilemap.py does not already assert, and it is flagged as such below.

The sampled cells come from a real shipped level inside
gamedata/disc/Legoland.res, so the grid the geometry is exercised over (and
the map dimensions checked against) are the game's own; nothing but
coordinates and derived pixel numbers leaves this script.

    python3 tools/oracle_tilegeom.py --header <out.h> [--level GLONE]
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import leveldata  # noqa: E402

# The shipped ground tiles are 32x16 (tools/tilemap.py's TH); the engine reads
# the height out of g_tile_sprites[g_default_tile] at +0x16, so the C test
# installs a Sprite with this height and the two must agree.
SPRITE_H = 16

# Viewport origin (Map +0x20/+0x22, unsigned short) and the 8.8 scroll pair.
ORIGIN_X, ORIGIN_Y = 320, 48
SCROLL_X, SCROLL_Y = 0x1234, -0x0500      # 8.8 fixed point, >>8 when applied


def centre(tx, ty, h, origin_x, origin_y, scroll_x, scroll_y):
    w = h + h
    return (((w + 1) >> 1) * (tx - ty) - (scroll_x >> 8) + origin_x,
            ((h + 1) >> 1) * (tx + ty + 1) - (scroll_y >> 8) + origin_y)


def bounds(tx, ty, h, origin_x, origin_y, scroll_x, scroll_y):
    w = h + h
    left = ((w + 1) >> 1) * (tx - ty - 1) - (scroll_x >> 8) + origin_x
    top = ((h + 1) >> 1) * (tx + ty) - (scroll_y >> 8) + origin_y
    return left, top, left + w - 1, top + h - 1


def sample_cells(width, height):
    """A spread of real grid coordinates: corners, edges and a diagonal."""
    pts = [(0, 0), (width - 1, 0), (0, height - 1), (width - 1, height - 1),
           (width // 2, height // 2), (1, height - 2), (width - 2, 1)]
    step = max(1, min(width, height) // 7)
    d = 0
    while d < min(width, height):
        pts.append((d, d))
        pts.append((d, min(height - 1, (d * 3) % height)))
        d += step
    seen, out = set(), []
    for p in pts:
        if p not in seen and 0 <= p[0] < width and 0 <= p[1] < height:
            seen.add(p)
            out.append(p)
    return out


# Walker probes for OverNewTile / CrossTileCentre: world coords are 24.8, one
# map tile is 256 units and 0x80 is the tile centre (LEGOLAND/tilehelp.c).
#   dir 0,1,4,5 -> the y axis is the one tested; 2,3 and anything else -> x.
WALKERS = [
    # (wx, wy, dir, step_x, step_y)
    (0x0480, 0x0380, 0, 0x0480, 0x0381),   # same tile, y below centre: no cross
    (0x0480, 0x037f, 0, 0x0480, 0x0380),   # y crosses 0x80: cross
    (0x0480, 0x0380, 0, 0x0480, 0x0480),   # y leaves the tile: new tile
    (0x0480, 0x037f, 2, 0x0480, 0x0380),   # dir 2 tests x, so no cross
    (0x047f, 0x0380, 2, 0x0480, 0x0380),   # x crosses 0x80: cross
    (0x047f, 0x0380, 7, 0x0480, 0x0380),   # out-of-range dir falls to x
    (0x0480, 0x0380, 3, 0x0580, 0x0380),   # x leaves the tile
    (0x0000, 0x0000, 0, 0x00ff, 0x00ff),   # tile 0,0 interior
    (0x0000, 0x0000, 0, 0x0100, 0x0000),   # step to tile 1,0
    (-0x0080, 0x0180, 1, -0x0081, 0x0180), # negative world x stays in tile -1
]


def over_new_tile(wx, wy, x, y):
    """((wx^x) & ~0xff)==0 and ((wy^y) & ~0xff)==0 -> same tile."""
    same = ((wx ^ x) & ~0xff) == 0 and ((wy ^ y) & ~0xff) == 0
    return 0 if same else 1


def cross_tile_centre(wx, wy, d, x, y):
    delta = (wy ^ y) if d in (0, 1, 4, 5) else (wx ^ x)
    if delta & 0x80 and over_new_tile(wx, wy, x, y) == 0:
        return 1
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--header")
    ap.add_argument("--json")
    ap.add_argument("--level", default="GLONE")
    ap.add_argument("--res", default=None)
    args = ap.parse_args()

    res = args.res or leveldata.RES_PATH
    m = leveldata.parse_map(leveldata.load_map_bytes(args.level, res))
    w, h = m["width"], m["height"]
    cells = sample_cells(w, h)

    rows = []
    for tx, ty in cells:
        cx, cy = centre(tx, ty, SPRITE_H, ORIGIN_X, ORIGIN_Y, SCROLL_X, SCROLL_Y)
        l, t, r, b = bounds(tx, ty, SPRITE_H, ORIGIN_X, ORIGIN_Y,
                            SCROLL_X, SCROLL_Y)
        rows.append((tx, ty, cx, cy, l, t, r, b))

    walkers = []
    for wx, wy, d, x, y in WALKERS:
        walkers.append((wx, wy, d, x, y,
                        over_new_tile(wx, wy, x, y),
                        cross_tile_centre(wx, wy, d, x, y)))

    if args.header:
        with open(args.header, "w") as fh:
            fw = fh.write
            fw("/* GENERATED by tools/oracle_tilegeom.py -- do not edit. */\n")
            fw("#ifndef ORACLE_TILEGEOM_H\n#define ORACLE_TILEGEOM_H\n\n")
            fw('#define ORACLE_TG_LEVEL    "%s"\n' % args.level)
            fw("#define ORACLE_TG_WIDTH    %d\n" % w)
            fw("#define ORACLE_TG_HEIGHT   %d\n" % h)
            fw("#define ORACLE_TG_SPRITE_H %d\n" % SPRITE_H)
            fw("#define ORACLE_TG_TILE_W   %d\n" % (SPRITE_H * 2))
            fw("#define ORACLE_TG_ORIGIN_X %d\n" % ORIGIN_X)
            fw("#define ORACLE_TG_ORIGIN_Y %d\n" % ORIGIN_Y)
            fw("#define ORACLE_TG_SCROLL_X %d\n" % SCROLL_X)
            fw("#define ORACLE_TG_SCROLL_Y %d\n\n" % SCROLL_Y)
            fw("/* {tx, ty, centre_x, centre_y, left, top, right, bottom} */\n")
            fw("static const int oracle_tg_cells[][8] = {\n")
            for r in rows:
                fw("    { %d, %d, %d, %d, %d, %d, %d, %d },\n" % r)
            fw("};\n#define ORACLE_TG_CELL_N %d\n\n" % len(rows))
            fw("/* {wx, wy, dir, step_x, step_y, OverNewTile, "
               "CrossTileCentre} */\n")
            fw("static const int oracle_tg_walkers[][7] = {\n")
            for r in walkers:
                fw("    { %d, %d, %d, %d, %d, %d, %d },\n" % r)
            fw("};\n#define ORACLE_TG_WALKER_N %d\n\n" % len(walkers))
            fw("#endif\n")

    out = {"level": args.level, "width": w, "height": h,
           "sprite_h": SPRITE_H, "cells": len(rows), "walkers": len(walkers),
           "first_cell": rows[0] if rows else None}
    if args.json:
        with open(args.json, "w") as fh:
            json.dump(out, fh, indent=1)
    if not args.header and not args.json:
        json.dump(out, sys.stdout, indent=1)
        print()


if __name__ == "__main__":
    main()
