#!/usr/bin/env python3
r"""LEGOLAND (PC 2000, Krisalis) tilemap resolver — cell -> concrete .lls sprite.

Clean-room reference implementation of the engine's cell->sprite pipeline, i.e.
`LoadMapTiles` (0x45aad0) + `LoadBaseMap` (0x461a50) + `RenderFullMap`
(0x4567a0) + the path autotiler (`DrawPathCell` 0x460e90). Every arithmetic step
is grounded in the disassembly; see scratchpad/slope_re/*.md for the per-function
specs and the synthesis writeup.

WHAT IT COMPUTES
================
Given the ICM element database (gamedata/main/Legoland.icm) and a parsed .MAP,
it resolves for every grid cell:
  * the drawn ground/terrain tile sprite (a `.lls` name in Graphics1/2.res),
  * path cells' autotiled OUTLnn sprite(s) (+ concave-corner overlays),
  * placed objects (class name; primary CSP/ILF sprite where nameable),
  * the terrain-stream base/ground plane (cell+0xa),
  * the screen placement (iso tile bounds) for each cell.

THE KEY MECHANISM (base + delta)
================================
Ground tiles are globally-registered TSF tiles. When a `.TSF` loads, its N
sprites are assigned a contiguous run of global slots in `TileSpriteArray`
starting at an AllocTileSpace `base` (LLIDB_LoadTSFData stores base at struct+0;
LLIDB_LoadTSMData both binds a mapping to its TSF(s) and triggers their load,
fixing base order). `TileSpriteArray[base+delta]` is exactly the delta-th sprite
loaded from that TSF, i.e. `TSF.images[delta]`.

The tile_gfx RLE (LoadBaseMap 0x461e66..0x462206) carries a 1-byte accumulator
(`acc`) selecting a tile GROUP + mode, plus per-cell data bytes = a delta:
    acc & 0x20 == 0  -> GROUND: idx = acc-1; group = level_tsm.records[idx];
                        finalword = group.TSF.base + databyte;
                        image = group.TSF.images[databyte]           (cell+8, +0xa)
    acc & 0x20 != 0  -> OBJECT/PATH: place pathsets[acc & 0x1f] object;
                        cell+0xa = default ground word                (cell+8 untouched)
The terrain-tile stream (LoadBaseMap 0x4627d6..0x4628c6) writes cell+0xa via the
SAME level TSM: idx = (code>>8)-1, delta = code&0xff, 0xffff ends a run.

Path cells are marked by the rf_flags layer: where the RF byte has bits 3|4 set
(0x18), the engine calls AddPathTileGFX (cell+0xc |= 0x10 PATH). At render time
the path sprite is chosen live from the 4 orthogonal neighbours' path state
(4-bit autotile) into NORMPATH's OUTL00..OUTL15 (+OUTL16..18 concave overlays).

Usage:
    python3 tools/tilemap.py <LEVEL>            # summary + coverage
    python3 tools/tilemap.py <LEVEL> --json out.json   # per-cell dump
"""
import json
import os
import struct
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import leveldata  # noqa: E402  (res_index / parse_map / load_map_bytes)
import resfile    # noqa: E402  (parse_leaves)

RES_LEGO = os.path.join(ROOT, "gamedata", "disc", "Legoland.res")
RES_GFX1 = os.path.join(ROOT, "gamedata", "disc", "Graphics1.res")
RES_GFX2 = os.path.join(ROOT, "gamedata", "disc", "Graphics2.res")
ICM_PATH = os.path.join(ROOT, "gamedata", "main", "Legoland.icm")

# LoadMapTiles preloads these before every level (0x45ab48/63/ba); MAPPING 1.TSM
# subs load first, giving the deterministic global base order.
DEFAULT_MAPPING = "MAPPING 1"          # .TSM  (subs: BASIC TILES 1, NORMAL PATH TILES)
DEFAULT_BASIC = "BASIC TILES 1"        # .TSF  (UI/cursor tiles, base 0)
DEFAULT_PATH = "NORMAL PATH TILES"     # .TSF  (NORMPATH; OUTL autotile set)
OUTL_LOCAL_BASE = 3                    # NORMPATH image index of OUTL00 (idx 0..2 = PATH15/BLU/RED)


# --------------------------------------------------------------------------- #
# little-endian reader
# --------------------------------------------------------------------------- #
class R:
    def __init__(self, b):
        self.b = b
        self.o = 0

    def u16(self):
        v = struct.unpack_from("<H", self.b, self.o)[0]
        self.o += 2
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.b, self.o)[0]
        self.o += 4
        return v

    def s(self):
        n = self.u32()
        v = self.b[self.o:self.o + n]
        self.o += n
        return v.decode("latin1")


# --------------------------------------------------------------------------- #
# .res member access (flat name -> bytes), case-insensitive
# --------------------------------------------------------------------------- #
class ResArchive:
    def __init__(self, path):
        self.data = resfile.load(path)
        self.by_name = {}
        for r in resfile.parse_leaves(self.data):
            self.by_name.setdefault(r["name"].upper(), r)

    def has(self, name):
        return name.upper() in self.by_name

    def get(self, name):
        r = self.by_name[name.upper()]
        return self.data[r["offset"]:r["offset"] + r["size"]]


# --------------------------------------------------------------------------- #
# ICM element database (LLIDB_LoadICM 0x47aff0)
#   file: u32 count; count*20-byte records (raw); tail = per-record {str,str}
#   After load: record = {label, value(filename), flags(type)}.  Index = slot.
#   Lookups are BY NAME, case-insensitive (LLIDB_FindElement 0x47b330).
# --------------------------------------------------------------------------- #
def load_icm(path=ICM_PATH):
    b = open(path, "rb").read()
    count = struct.unpack_from("<I", b, 4)[0]  # NOTE: first u32 at +0 is a header
    # LLIDB_LoadICM reads count as the FIRST u32 of the file (0x47b06b). Verify:
    c0 = struct.unpack_from("<I", b, 0)[0]
    # The spec (icm_loadorder.md) confirms count=294 read at file+0.
    count = c0
    rounded = (count + 0xff) & ~0xff
    recs_end = 4 + count * 20            # records start at +4 (after the count u32)
    o = recs_end
    elems = []
    for i in range(count):
        # raw record fields we keep: type at +8
        rec_off = 4 + i * 20
        flags = struct.unpack_from("<I", b, rec_off + 8)[0]
        # tail: two length-prefixed strings (label, value)
        ln = struct.unpack_from("<I", b, o)[0]; o += 4
        label = b[o:o + ln].decode("latin1"); o += ln
        lv = struct.unpack_from("<I", b, o)[0]; o += 4
        value = b[o:o + lv].decode("latin1"); o += lv
        # flags fixup as in LLIDB_LoadICM: clear bits 0,1,3 (mask ~0x0b); keep type nibble
        flags &= ~0x0b
        elems.append({"index": i, "label": label, "value": value,
                      "type": flags & 0xfff0})
    by_name = {}
    for e in elems:
        by_name.setdefault(e["label"].upper(), e)
    return {"elems": elems, "by_name": by_name, "leftover": len(b) - o}


# --------------------------------------------------------------------------- #
# Asset parsers (LLIDB_LoadTSFData/TSMData/ILFData/CSPData)
# --------------------------------------------------------------------------- #
def parse_tsf2(b):
    """u32 n; str name; n*{u32 code,u32 f2}; n* str image; [tail link]."""
    r = R(b)
    n = r.u32()
    name = r.s()
    codes = []
    for _ in range(n):
        c = r.u32(); f2 = r.u32()
        codes.append(c)
    imgs = [r.s() for _ in range(n)]
    return {"kind": "TSF", "n": n, "name": name, "codes": codes, "images": imgs}


def parse_tsm(b):
    """u32 count; str name; count* str tilesetName."""
    r = R(b)
    count = r.u32()
    name = r.s()
    subs = [r.s() for _ in range(count)]
    return {"kind": "TSM", "count": count, "name": name, "subs": subs}


def parse_ilf(b):
    """u16 n; u16 type; str name; n*{u32 dx,u32 dy}; n* str image.  dx/dy doubled at load."""
    r = R(b)
    n = r.u16()
    typ = r.u16()
    name = r.s()
    offs = []
    for _ in range(n):
        dx = struct.unpack_from("<i", b, r.o)[0]; r.o += 4
        dy = struct.unpack_from("<i", b, r.o)[0]; r.o += 4
        offs.append((dx * 2, dy * 2))         # engine shl 1 (0x47d0f6)
    imgs = [r.s() for _ in range(n)]
    return {"kind": "ILF", "n": n, "type": typ, "name": name,
            "offsets": offs, "images": imgs}


def parse_csp(b):
    """same layout as ILF."""
    d = parse_ilf(b)
    d["kind"] = "CSP"
    return d


# --------------------------------------------------------------------------- #
# The loader / global-tile-space simulator
#   Replays LoadMapTiles + LoadBaseMap's asset load order from an EMPTY tile
#   space so AllocTileSpace bases are contiguous & deterministic.
#   (LoadData is idempotent/ref-counted: each TSF allocates a base exactly once.)
# --------------------------------------------------------------------------- #
class Loader:
    def __init__(self, icm, res):
        self.icm = icm
        self.res = res
        self.tsf = {}          # name.upper() -> {parsed, base}
        self.next_base = 0     # AllocTileSpace high-water (contiguous from empty)
        self.global_slots = {} # global index -> (tsf_name, delta, image)

    def _find(self, name):
        e = self.icm["by_name"].get(name.upper())
        if e is None:
            raise KeyError(f"ICM element not found: {name!r}")
        return e

    def load_tsf(self, name):
        """Idempotent: allocate a base once; return the loaded TSF record."""
        key = name.upper()
        if key in self.tsf:
            return self.tsf[key]
        e = self._find(name)
        fn = e["value"]
        if not self.res.has(fn):
            raise KeyError(f"TSF file not in res: {fn!r}")
        t = parse_tsf2(self.res.get(fn))
        base = self.next_base
        self.next_base += t["n"]
        rec = {"parsed": t, "base": base, "elem": e}
        self.tsf[key] = rec
        for d, img in enumerate(t["images"]):
            self.global_slots[base + d] = (t["name"], d, img)
        return rec

    def load_tsm(self, name):
        """Load a .TSM and (in order) each sub-TSF; return list of TSF recs."""
        e = self._find(name)
        tsm = parse_tsm(self.res.get(e["value"]))
        recs = [self.load_tsf(sub) for sub in tsm["subs"]]
        return tsm, recs

    def load_ilf(self, name):
        e = self._find(name)
        return parse_ilf(self.res.get(e["value"]))

    def load_csp(self, name):
        e = self._find(name)
        if not self.res.has(e["value"]):
            return None
        return parse_csp(self.res.get(e["value"]))


# --------------------------------------------------------------------------- #
# tile_gfx RLE decode (LoadBaseMap 0x461e66..0x462206)
# --------------------------------------------------------------------------- #
def decode_tilegfx(buf, w, h):
    """Return per-cell dict: {'kind':'ground','delta':d} | {'kind':'object','pathset':p}
    | {'kind':'empty'}.  Index by y*w + x."""
    total = w * h
    out = [None] * total
    edi = 2          # first 2 bytes reserved (0x461ea8)
    acc = 0
    cur = 0
    while cur < total:
        b = buf[edi]; edi += 1
        n = b & 0x3f
        mode = b & 0xc0
        if mode != 0 and n == 0:
            n = 64
        if mode == 0x00:            # SET accumulator
            acc = n
        elif mode in (0x40, 0x80):  # LITERAL / FILL run of n cells
            for i in range(n):
                if cur >= total:
                    break
                db = buf[edi]       # data byte (per-cell for 0x40, shared for 0x80)
                if acc & 0x20:
                    out[cur] = {"kind": "object", "pathset": acc & 0x1f}
                else:
                    out[cur] = {"kind": "ground", "group": (acc - 1) & 0xff,
                                "delta": db}
                cur += 1
                if mode == 0x40:
                    edi += 1
            if mode == 0x80:
                edi += 1
        elif mode == 0xc0:          # EMPTY run
            for i in range(n):
                if cur >= total:
                    break
                out[cur] = {"kind": "empty"}
                cur += 1
    return out, edi


# --------------------------------------------------------------------------- #
# flag-layer RLE decode (map_flags / rf_flags / user_flags)
#   No accumulator; edi=0; only modes 0x40 (literal) & 0x80 (fill) emit cells;
#   0x00/0xc0 are 0-cell no-ops; n==0 -> 64 unconditionally. (LoadBaseMap
#   0x462216.. / 0x462394.. / 0x4625c4..)
# --------------------------------------------------------------------------- #
def decode_flags(buf, w, h):
    total = w * h
    out = [0] * total
    edi = 0
    cur = 0
    while cur < total and edi < len(buf):
        b = buf[edi]; edi += 1
        n = b & 0x3f
        mode = b & 0xc0
        if n == 0:
            n = 64
        if mode == 0x40:            # literal
            for i in range(n):
                if cur >= total:
                    break
                out[cur] = buf[edi]; edi += 1; cur += 1
        elif mode == 0x80:          # fill
            db = buf[edi]
            for i in range(n):
                if cur >= total:
                    break
                out[cur] = db; cur += 1
            edi += 1
        # mode 0x00 / 0xc0: no cells
    return out, edi, cur


# --------------------------------------------------------------------------- #
# main resolver
# --------------------------------------------------------------------------- #
def resolve_level(level, icm, res, gfx_names=None):
    raw = leveldata.load_map_bytes(level)
    m = leveldata.parse_map(raw)
    w, h = m["width"], m["height"]
    total = w * h

    ld = Loader(icm, res)
    # --- LoadMapTiles preload order (fixes base 0..) ---
    ld.load_tsm(DEFAULT_MAPPING)     # BASIC TILES 1 (base 0), NORMAL PATH TILES (base 9)
    ld.load_tsf(DEFAULT_BASIC)       # cached
    path_rec = ld.load_tsf(DEFAULT_PATH)  # cached; NORMPATH
    # --- LoadBaseMap per-level order ---
    _, level_tsf_recs = ld.load_tsm(m["tsm_mapping"])   # ground TSF(s)
    terrain_ilf = None
    try:
        terrain_ilf = ld.load_ilf(m["terrain"])
    except KeyError:
        pass
    level_pathset_recs = []
    for pn in m["path_tilesets"]:
        try:
            level_pathset_recs.append(ld.load_tsf(pn))
        except KeyError:
            level_pathset_recs.append(None)

    ground_base = level_tsf_recs[0]["base"] if level_tsf_recs else 0  # [0x667ca4]
    outl_global_base = path_rec["base"] + OUTL_LOCAL_BASE             # [[0x832bf0]] = B

    # level TSM records used by tile_gfx (idx=acc-1) and terrain (idx=hi-1)
    tsm_records = level_tsf_recs

    # --- decode layers ---
    tg_buf = raw[m["layers"]["tile_gfx"]["off"]:
                 m["layers"]["tile_gfx"]["off"] + m["layers"]["tile_gfx"]["rle_size"]]
    tg, tg_edi = decode_tilegfx(tg_buf, w, h)

    mf_buf = raw[m["layers"]["map_flags"]["off"]:
                 m["layers"]["map_flags"]["off"] + m["layers"]["map_flags"]["rle_size"]]
    mapflags, mf_edi, mf_cells = decode_flags(mf_buf, w, h)
    rf_buf = raw[m["layers"]["rf_flags"]["off"]:
                 m["layers"]["rf_flags"]["off"] + m["layers"]["rf_flags"]["rle_size"]]
    rfflags, rf_edi, rf_cells = decode_flags(rf_buf, w, h)

    # --- path cells (is-path predicate, 0x45ce10) ---
    #   rf bit0 (force-path) -> PATH; else PATH iff (map_flags & 0x10) and not
    #   rf bit1 (not-path override).  Grounded: in shipped maps every tile_gfx
    #   object-branch cell has rf&1 and map_flags 0x58 (0x40|0x10|0x08).
    is_path = [False] * total
    for i in range(total):
        rfb = rfflags[i]
        if rfb & 1:
            is_path[i] = True
        elif (mapflags[i] & 0x10) and not (rfb & 2):
            is_path[i] = True

    def P(x, y):
        if 0 <= x < w and 0 <= y < h:
            return is_path[y * w + x]
        return False

    def path_sprites(x, y):
        N = P(x, y - 1); E = P(x + 1, y); S = P(x, y + 1); Wn = P(x - 1, y)
        mm = (1 if N else 0) | (2 if E else 0) | (4 if S else 0) | (8 if Wn else 0)
        imgs = [path_rec["parsed"]["images"][OUTL_LOCAL_BASE + mm]]  # OUTL{mm}
        c = 0
        if (mm & 0x0C) == 0x0C and not P(x - 1, y + 1):
            c |= 1
        if (mm & 0x03) == 0x03 and not P(x + 1, y - 1):
            c |= 2
        if c:
            imgs.append(path_rec["parsed"]["images"][OUTL_LOCAL_BASE + 15 + c])  # OUTL16/17/18
        return mm, c, imgs

    # --- terrain stream (cell+0xa) ---
    terrain_grid = m["terrain_grid"]  # [(hi,lo) or None]

    # --- object placements (from the .MAP object list, class order) ---
    obj_at = {}
    for o in m["objects"]:
        cls = o["class"]
        cname = m["object_classes"][cls] if cls < len(m["object_classes"]) else f"?{cls}"
        obj_at[(o["x"], o["y"])] = cname

    # --- per-cell resolution ---
    cells = []
    cov = Counter()
    for y in range(h):
        for x in range(w):
            i = y * w + x
            rec = {"x": x, "y": y,
                   "map_flags": mapflags[i], "rf_flags": rfflags[i],
                   "renderable": bool(mapflags[i] & 0x08)}  # Phase B gate (0x456cc8)
            tgc = tg[i]
            # ground tile drawn = cell+8 (from tile_gfx ground branch)
            resolved_img = None
            if tgc is None:
                rec["tile_gfx"] = "none"
            elif tgc["kind"] == "ground":
                gidx = tgc["group"]
                delta = tgc["delta"]
                if 0 <= gidx < len(tsm_records):
                    tsf = tsm_records[gidx]["parsed"]
                    if delta < len(tsf["images"]):
                        resolved_img = tsf["images"][delta]
                        gslot = tsm_records[gidx]["base"] + delta
                        rec["tile_gfx"] = "ground"
                        rec["ground_tsf"] = tsf["name"]
                        rec["delta"] = delta
                        rec["global_tile"] = gslot
                        rec["image"] = resolved_img
                        cov["ground_resolved"] += 1
                    else:
                        rec["tile_gfx"] = "ground"; rec["delta"] = delta
                        rec["image"] = None
                        cov["ground_bad_delta"] += 1
                else:
                    rec["tile_gfx"] = "ground"; rec["group"] = gidx
                    cov["ground_bad_group"] += 1
            elif tgc["kind"] == "object":
                rec["tile_gfx"] = "object"
                rec["pathset"] = tgc["pathset"]
                cov["tilegfx_object"] += 1
            elif tgc["kind"] == "empty":
                rec["tile_gfx"] = "empty"
                cov["empty"] += 1

            # path overlay (render-time autotile) takes over the drawn sprite
            if is_path[i]:
                mm, c, pimgs = path_sprites(x, y)
                rec["path"] = True
                rec["path_mask"] = mm
                rec["path_images"] = pimgs
                rec["image"] = pimgs[0]
                cov["path_resolved"] += 1

            # placed object (footprint anchor)
            if (x, y) in obj_at:
                rec["object_class"] = obj_at[(x, y)]
                cov["placed_object"] += 1

            # terrain-stream base plane (cell+0xa)
            tv = terrain_grid[y][x]
            if tv is not None:
                hi, lo = tv
                tidx = hi - 1
                if 0 <= tidx < len(tsm_records) and lo < len(tsm_records[tidx]["parsed"]["images"]):
                    rec["terrain_base_image"] = tsm_records[tidx]["parsed"]["images"][lo]
                    rec["terrain_base_global"] = tsm_records[tidx]["base"] + lo
                    cov["terrain_resolved"] += 1

            # screen placement (iso tile bounds, origin/scroll = 0 baseline)
            TH = 16
            HH = (TH + 1) >> 1
            rec["left"] = TH * (x - y - 1)
            rec["top"] = HH * (x + y)
            rec["w"] = 2 * TH
            rec["hpx"] = TH

            if resolved_img is not None or rec.get("image"):
                cov["cells_with_sprite"] += 1
            cells.append(rec)

    report = {
        "level": m["name"], "file_level": level,
        "width": w, "height": h, "total_cells": total,
        "tsm_mapping": m["tsm_mapping"], "terrain": m["terrain"],
        "ground_tsf": [r["parsed"]["name"] for r in level_tsf_recs],
        "ground_base": ground_base,
        "path_tsf": path_rec["parsed"]["name"], "outl_global_base": outl_global_base,
        "path_tilesets": m["path_tilesets"],
        "global_slot_count": ld.next_base,
        "layer_consumption": {
            "tile_gfx": [tg_edi, m["layers"]["tile_gfx"]["rle_size"]],
            "map_flags": [mf_edi, m["layers"]["map_flags"]["rle_size"]],
            "rf_flags": [rf_edi, m["layers"]["rf_flags"]["rle_size"]],
        },
        "terrain_ilf_images": terrain_ilf["images"] if terrain_ilf else None,
        "coverage": dict(cov),
    }
    return report, cells


def check_gfx_names(cells, path_rec_imgs, gfx1, gfx2):
    """Verify resolved .lls names exist in Graphics1/2.res."""
    names = set()
    for c in cells:
        if c.get("image"):
            names.add(c["image"])
        for im in c.get("path_images", []):
            names.add(im)
    present = 0
    missing = []
    for nm in names:
        base = nm.upper()
        # .lls stored in graphics res as COMP members named e.g. "sandy1.lls"
        if gfx1.has(nm) or gfx2.has(nm) or gfx1.has(base) or gfx2.has(base):
            present += 1
        else:
            missing.append(nm)
    return len(names), present, sorted(missing)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    level = sys.argv[1]
    icm = load_icm()
    res = ResArchive(RES_LEGO)
    report, cells = resolve_level(level, icm, res)

    # coverage vs "set tile_gfx cells" (ground + object + path, i.e. non-none)
    cov = report["coverage"]
    total = report["total_cells"]
    set_tilegfx = cov.get("ground_resolved", 0) + cov.get("ground_bad_delta", 0) + \
        cov.get("ground_bad_group", 0) + cov.get("tilegfx_object", 0)
    resolved = cov.get("cells_with_sprite", 0)
    ground_cells = cov.get("ground_resolved", 0) + cov.get("ground_bad_delta", 0) + \
        cov.get("ground_bad_group", 0)

    print(f"level          : {report['level']}  ({report['width']}x{report['height']} = {total})")
    print(f"tsm/ground     : {report['tsm_mapping']} -> {report['ground_tsf']} (base {report['ground_base']})")
    print(f"terrain ILF    : {report['terrain']}")
    print(f"path tileset   : {report['path_tsf']}  OUTL00 global base = {report['outl_global_base']}")
    print(f"global slots    : {report['global_slot_count']}")
    print(f"layer consume  : {report['layer_consumption']}")
    print("coverage counters:")
    for k, v in sorted(cov.items()):
        print(f"    {k:22s} {v}")
    print(f"\nground cells resolved to .lls : {cov.get('ground_resolved',0)}/{ground_cells}"
          f" = {100.0*cov.get('ground_resolved',0)/max(1,ground_cells):.1f}%")
    print(f"set tile_gfx cells with a sprite: {resolved}/{set_tilegfx}"
          f" = {100.0*resolved/max(1,set_tilegfx):.1f}%")
    print(f"all grid cells with a sprite    : {resolved}/{total}"
          f" = {100.0*resolved/total:.1f}%")

    # verify .lls names exist in the graphics archives
    try:
        gfx1 = ResArchive(RES_GFX1)
        gfx2 = ResArchive(RES_GFX2)
        nnames, present, missing = check_gfx_names(cells, None, gfx1, gfx2)
        print(f"\ndistinct .lls names used: {nnames}; present in Graphics1/2.res: {present}")
        if missing:
            print("  missing:", missing[:20])
    except Exception as e:
        print("(graphics res check skipped:", e, ")")

    if "--json" in sys.argv:
        out = sys.argv[sys.argv.index("--json") + 1]
        json.dump({"report": report, "cells": cells}, open(out, "w"), indent=0)
        print(f"\nwrote {out}  ({len(cells)} cells)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
