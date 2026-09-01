# Shared RE context for LEGOLAND format work

Everything reverse-engineered so far. Read this + `docs/FORMATS.md`,
`docs/BINARIES.md`, `docs/INSTALLSHIELD_Z.md` before starting.

## Paths (local, absolute)

- Project root: `/Users/systemadmin/Downloads/legoland/legoland`
- Game binary: `original/legoland.exe` (VC6 PE, image base 0x400000)
- Extracted `main.z` members (levels, tiles, music, anims, the BUILD MENU
  config file confusingly named `legoland.exe`): `gamedata/main/`
- Loose disc archives: `gamedata/disc/Graphics1.res` (20 MB),
  `gamedata/disc/Legoland.res` (16 MB)
- Full 120 MB `Graphics2.res` and `Speech/*.wav` live on the mounted disc at
  `/Volumes/LEGOLAND/` (may or may not be mounted; prefer `gamedata/`).

## Tools already written

- `tools/iscab.py` + `tools/blast.py` — InstallShield-Z extractor (done).
- `tools/dump_exports.py` — PE export dumper. `symbols/legoland.exports.txt`
  has all 716 exports as `name<TAB>ordinal<TAB>0xRVA`.
- `tools/disasm.py` — capstone function disassembler. Usage:
  `python3 tools/disasm.py original/legoland.exe <ExportName|0xRVA> [insn_count]`.
  RVAs in the export file are file RVAs; disasm prints VAs (RVA+0x400000) and
  annotates call targets with export names.

## The binary

Single VC6 `legoland.exe`, DirectDraw 2-D isometric engine (no Direct3D).
716 named C exports (675 functions and 41 data symbols) expose most of the
internal API. Grep `symbols/legoland.exports.txt` to find the loader/renderer
for any format, then disassemble it. Pixels are
**RGB555** (export `LLS555To565` converts to 565 for the display).

## `.res` archives

`u32` at +0 = directory offset. Member data packed from +4. Directory is a
hierarchical tree at the tail. FILE (leaf) records look like:

```
ff ff ff ff | u32 X | u32 zero=0 | u32 size | u32 data_offset | name\0
```

Folder records differ (a count where files have 0xffffffff). A robust scan for
`ff ff ff ff` + `zero==0` + in-range size/offset recovers all 574 sprite
entries in Graphics1.res.

## `COMP` sprite block (the linchpin)

Each Graphics member is a `COMP` block:

```
+0  char[4] "COMP"
+4  u32 width      (e.g. 640)
+8  u32 height     (e.g. 480)
+12 u32 bpp        (8 = paletted, 16 = RGB555)
+16 u32 count/flags (1)
+20 u32 reserved (0)
+24 u32 s0 = block_size - 24  (payload length: the four size dwords + data)
+28 u32 s1                     \
+32 u32 s2                      >  three sub-stream sizes
+36 u32 s3                     /
+40 ...  compressed data
```

The decoder is `__BMPLoader` @ VA 0x0044e010 (called via `LoadSourceImage`).
It branches on extension `.lls` / `.llz` and on the global display-mode dword
`[0x668088]`. The 8-bit path (@ ~0x0044e580) reads a 768-byte RGB palette and
converts 888->555. The 16-bit path decompresses the sub-streams. The remainder
stream after s1+s2+s3 is a low-entropy control/RLE stream; s1..s3 look like
pixel/run streams. **You must trace the actual loop in `__BMPLoader` to get the
algorithm exactly right — do not guess.** Follow calls from 0x0044e158
(compressed `.lls` path) and read the copy/run loop.

## Verification bar

A decoder is only "done" when it round-trips real data: decoded dimensions
match the header, and the output is a coherent image/structure (not noise).
Render sample sprites to PNG and report color histograms / non-degeneracy.
