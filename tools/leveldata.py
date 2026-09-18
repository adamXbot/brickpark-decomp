#!/usr/bin/env python3
r"""LEGOLAND (PC 2000, Krisalis) level / map parser.

Parses the isometric park level format `.MAP` used by the engine's
`LoadBaseMap` @ VA 0x00461a50 (export ordinal 344).  The 10 shipped game
levels are GLONE.MAP .. GLTEN.MAP, stored inside `gamedata/disc/Legoland.res`
(the `.\LevelMaps\%s` lookup goes through the RES archive).  Smaller
ONE.MAP..FIVE.MAP (driving-school / tutorial) and `freeplay big map.MAP`
also live there.

Everything below is confirmed by reading LoadBaseMap's disassembly and by
parsing real bytes: every one of the 17 shipped maps decodes to EOF with zero
bytes left over (see verify/levels/).

.MAP layout (all multibyte little-endian):

    str   name              # str = u32 len + `len` bytes (no NUL)
    str   tsm_mapping        # -> a .TSM tile-mapping (icm logical name)
    str   terrain            # -> a terrain .ILF (icm logical name)
    u16   width
    u16   height
    u32   n_classes ; n_classes * str        # object-class table (icm names)
    u32   n_objects ; n_objects * {u32 class_idx, u32 x, u32 y}
    u32   n_pathsets; n_pathsets * str        # path tile-sets (.TSF, icm names)
    u32   s1 ; s1 bytes   # RLE layer 1: tile graphics   (SetMapTile)
    u32   s2 ; s2 bytes   # RLE layer 2: map flags        (SetMapFlags)
    u32   s3 ; s3 bytes   # RLE layer 3: RF / path flags   (Set_RFFlags, AddPathTileGFX, AddPathSquare)
    u32   s4 ; s4 bytes   # RLE layer 4: user flags        (Set_UserFlags)
    u32   n_extra ; n_extra * 20 bytes        # per-object extra records (func 0x462c00)
    [ "BRIDGES!" (8) + str bridge_terrain ]   # optional tag; absent => not consumed
    <terrain-tile stream>                     # variable state machine, fills w*h, runs to EOF

The four size-prefixed layers (s1..s4) share a 2-bit-tagged RLE that walks the
grid row-major (control byte: n = b&0x3f, mode = b&0xc0); they are skipped by
their declared size here (self-terminating at grid-fill inside the engine).

The final terrain-tile stream has NO size prefix; LoadBaseMap reads it 1/2
bytes at a time straight from the file until the grid is full.  State machine
(ebp): 0 = read a control byte (0 -> solid-run state 1; k>0 -> skip k cells
then state 1); 1 = read u16 per cell, 0xffff ends the run (state 0), else the
cell's terrain tile = terrain_table[hi].base + lo (we record (hi, lo)); 2 =
skip counter.
"""
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
# LL_GAMEDATA: a game data tree outside this checkout (brickpark-browser sets it).
GAMEDATA = os.environ.get("LL_GAMEDATA") or os.path.join(ROOT, "gamedata")
RES_PATH = os.path.join(GAMEDATA, "disc", "Legoland.res")


# ---------------------------------------------------------------------------
# .res archive: recover FILE leaves (see docs/FORMATS.md .res section)
# ---------------------------------------------------------------------------
def res_index(res_path=RES_PATH):
    """Return {filename: (size, offset)} for every FILE leaf in a .res."""
    import re

    data = open(res_path, "rb").read()
    diroff = struct.unpack_from("<I", data, 0)[0]
    out = {}
    for m in re.finditer(rb"\xff\xff\xff\xff", data[diroff:]):
        p = diroff + m.start()
        if p + 20 > len(data):
            continue
        _x, zero, size, off = struct.unpack_from("<IIII", data, p + 4)
        if zero != 0 or not (0 <= off < len(data)) or not (0 < size <= len(data)):
            continue
        end = data.find(b"\x00", p + 20)
        name = data[p + 20:end]
        if not name or any(c < 0x20 or c > 0x7E for c in name):
            continue
        out[name.decode()] = (size, off)
    return out, data


def load_map_bytes(name_or_path, res_path=RES_PATH):
    """Load raw .MAP bytes from a file path or a level name inside the .res."""
    if os.path.isfile(name_or_path):
        return open(name_or_path, "rb").read()
    idx, data = res_index(res_path)
    want = name_or_path
    if not want.upper().endswith(".MAP"):
        want += ".MAP"
    for k, (size, off) in idx.items():
        if k.upper() == want.upper():
            return data[off:off + size]
    raise FileNotFoundError(f"{name_or_path!r} not found as a file or in {res_path}")


# ---------------------------------------------------------------------------
# .MAP parser
# ---------------------------------------------------------------------------
class Reader:
    def __init__(self, b):
        self.b = b
        self.o = 0

    def u8(self):
        v = self.b[self.o]
        self.o += 1
        return v

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

    def eof(self):
        return self.o >= len(self.b)


def parse_map(b):
    r = Reader(b)
    m = {}
    m["name"] = r.s()
    m["tsm_mapping"] = r.s()
    m["terrain"] = r.s()
    m["width"] = r.u16()
    m["height"] = r.u16()
    w, h = m["width"], m["height"]

    nc = r.u32()
    m["object_classes"] = [r.s() for _ in range(nc)]
    no = r.u32()
    m["objects"] = [
        {"class": r.u32(), "x": r.u32(), "y": r.u32()} for _ in range(no)
    ]
    nps = r.u32()
    m["path_tilesets"] = [r.s() for _ in range(nps)]

    # four size-prefixed RLE layers (tile gfx, map flags, rf/path flags, user flags)
    layer_names = ["tile_gfx", "map_flags", "rf_flags", "user_flags"]
    m["layers"] = {}
    for ln in layer_names:
        sz = r.u32()
        m["layers"][ln] = {"rle_size": sz, "off": r.o}
        r.o += sz

    m["n_extra_records"] = r.u32()  # 20-byte per-object records
    r.o += 20 * m["n_extra_records"]

    m["has_bridges"] = False
    if b[r.o:r.o + 8] == b"BRIDGES!":
        r.o += 8
        m["has_bridges"] = True
        m["bridge_terrain"] = r.s()

    # final terrain-tile stream: state machine, fills the grid
    grid = [[None] * w for _ in range(h)]
    state = 0
    counter = 0
    solid = 0
    row = 0
    done = False
    while row < h and not done:
        col = 0
        while col < w:
            if state == 0:
                if r.eof():
                    done = True
                    break
                c = r.u8()
                state = 1 if c == 0 else 2
                counter = c
                continue
            if state == 2:
                if counter != 0:
                    counter -= 1
                    col += 1  # skipped (transparent) cell
                else:
                    state = 1
                continue
            # state == 1: solid run of per-cell u16 codes
            if r.o + 2 > len(b):
                done = True
                break
            code = r.u16()
            if code == 0xFFFF:
                state = 0
                col += 1
            else:
                grid[row][col] = (code >> 8 & 0xFF, code & 0xFF)  # (terrain_idx, delta)
                solid += 1
                col += 1
        row += 1

    m["terrain_grid"] = grid
    m["terrain_cells"] = solid
    m["bytes_consumed"] = r.o
    m["file_size"] = len(b)
    m["leftover"] = len(b) - r.o
    return m


def summarize(m):
    lines = []
    lines.append(f"level        : {m['name']}")
    lines.append(f"grid         : {m['width']} x {m['height']}  ({m['width'] * m['height']} tiles)")
    lines.append(f"tile mapping : {m['tsm_mapping']}  (.TSM)")
    lines.append(f"terrain      : {m['terrain']}  (.ILF)")
    lines.append(f"path tilesets: {', '.join(m['path_tilesets']) or '(none)'}")
    lines.append(f"object classes ({len(m['object_classes'])}): {', '.join(m['object_classes']) or '(none)'}")
    lines.append(f"placed objects: {len(m['objects'])}")
    # per-class counts
    counts = {}
    for o in m["objects"]:
        cn = m["object_classes"][o["class"]] if o["class"] < len(m["object_classes"]) else f"?{o['class']}"
        counts[cn] = counts.get(cn, 0) + 1
    for cn, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        lines.append(f"    {n:5d}  {cn}")
    lines.append(f"tile layers  : tile_gfx={m['layers']['tile_gfx']['rle_size']}B "
                 f"map_flags={m['layers']['map_flags']['rle_size']}B "
                 f"rf_flags={m['layers']['rf_flags']['rle_size']}B "
                 f"user_flags={m['layers']['user_flags']['rle_size']}B")
    lines.append(f"terrain tiles: {m['terrain_cells']} explicit cells "
                 f"(rest transparent/base); bridges={m['has_bridges']}")
    lines.append(f"parse        : consumed {m['bytes_consumed']}/{m['file_size']} bytes "
                 f"(leftover {m['leftover']})")
    return "\n".join(lines)


def to_json(m):
    d = dict(m)
    # compact terrain grid: list of {x,y,idx,delta} for explicit cells
    cells = []
    for y, rowv in enumerate(m["terrain_grid"]):
        for x, v in enumerate(rowv):
            if v is not None:
                cells.append({"x": x, "y": y, "terrain_idx": v[0], "delta": v[1]})
    d["terrain_grid"] = cells
    return d


def main():
    if len(sys.argv) < 2:
        print("usage: leveldata.py <LEVELNAME | path/to.MAP> [--json out.json]")
        print("       (level names live in gamedata/disc/Legoland.res, e.g. GLONE)")
        # list available maps
        try:
            idx, _ = res_index()
            maps = sorted(k for k in idx if k.upper().endswith(".MAP"))
            print("\navailable maps in Legoland.res:")
            for k in maps:
                print("   ", k)
        except Exception as e:
            print("(could not list res:", e, ")")
        return 1
    b = load_map_bytes(sys.argv[1])
    m = parse_map(b)
    print(summarize(m))
    if "--json" in sys.argv:
        out = sys.argv[sys.argv.index("--json") + 1]
        json.dump(to_json(m), open(out, "w"), indent=1)
        print(f"\nwrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
