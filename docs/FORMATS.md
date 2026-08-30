# LEGOLAND data formats

Findings from the 2000 USA disc. Confirmed-by-parsing unless marked *(guess)*.

## `.res` — resource archives (`Graphics1.res`, `Graphics2.res`, `Legoland.res`)

A `.res` is a single archive with a **hierarchical name directory at the tail**
and the file data packed from the front.

```
+0x00  u32  offset of the directory
+0x04  ...  member data (each Graphics member is a `COMP` block)
@dir:  tree of folders/files: flags, size, data-offset, NUL-name
```

- `Graphics1.res` (20 MB) / `Graphics2.res` (120 MB): trees of `.lls`
  ("LEGOLAND sprite") images under folders like `Graphics/Icons/…`, each a
  `COMP` block.
- `Legoland.res` (16 MB): **same `.res` container** (dir-offset@+0, tail name
  tree), but its members are LEGO `.3d` model/animation files, not `COMP`
  sprites. The "floats at +0" are just the first member's data packed at +4
  (its `08 00 00 00 d3 00 00 00` header + float payload). See **GEOMETRY**.

## GEOMETRY — `Legoland.res` `.3d` models  *(confirmed by parsing + disasm)*

`Legoland.res` is the ordinary `.res` archive format: `u32` directory-offset at
`+0` (= `0x00fa3d2c`), member data packed from `+4`, hierarchical name tree at
the tail. The robust `ff ff ff ff | X | zero | size | offset | name\0` FILE-record
scan (`tools/geom.py`) recovers **594 members**: **32 `.3d` models** plus
companion `.txt` / `.obj` / `.loc` and `.lls` texture members, all under a
`3DData` folder. Names match the `.3d` strings in `legoland.exe` (`ManWalk…`,
`WomanStand…`, `yipee…`, `sitman…`).

### `.3d` record layout

Recovered by disassembling the engine's own `.3d` loader **`sub_43f660`** (VA
`0x0043f660`; opens via `RES_OpenFile`, reads via `RES_ReadFile`; called from
the 3D-bloke code):

```
+0  u32  A = frames_per_part   (inner count; 1 for a static model)
+4  u32  B = part_count        (outer count; one billboard/part per entry)
+8  ...  B parts, part-major; each part = A elements; each element = 48 bytes:
           float pos[3]        // anchor position (left as-is at load)
           float mat[3][3]     // 3x3 basis/orientation matrix
```

The loader allocates `B` pointers, then for each part a block of `A*48` bytes;
per element it reads `pos` (3 floats) then the `3x3` (9 floats) and applies
`BuildYRotationMatrix(π/2)` → `MatrixMultiply` → `CopyMatrix` to the `3x3`
only (a load-time 90° Y coordinate fixup; `pos` is untouched). So an element is
a per-part, per-frame **rigid transform** (position + oriented basis) for the
DirectDraw engine's 3D-positioned sprites/"blokes". Read as four consecutive
`xyz` points (`pos` + the three matrix rows), ~51% of elements form a planar
quad (part 0 of `yipee` is a clean rectangle) and ~7% are collapsed to a point
(unused part/frame slots) — consistent with billboard quads, but the
code-accurate description is `vec3 + mat3`, not four equal vertices.

The animation section is `8 + B*A*48` bytes; `.3d` members are larger, with a
trailing block of `[-1,1]` floats after it (candidate: per-part normals / UVs)
that `sub_43f660` does not read — **not yet decoded**.

### Verified counts (via `tools/geom.py`)

| model | A (frames) | B (parts) | anim bytes | member size |
| --- | --- | --- | --- | --- |
| `yipee.yippee2.3d` | 8 | 211 | 81 032 | 104 620 |
| `manwalkeat.man_burger.3d` | 16 | 423 | 324 872 | 722 252 |
| `WomanStand.womanstand.3d` | 1 (static) | 66 | 3 176 | 7 160 |
| `WomanWalk.womanwalk.3d` | 8 | 66 | 25 352 | 30 652 |

`A=1` static poses vs `A=8` walk/wave cycles at the **same** `B=66` part count
confirm `A`=frames, `B`=parts. The `8 + B*A*48` section fits inside every real
model (31/32); only the dev placeholder `test.3d` (header claims `B=211` but the
member is 39 640 B, too small) is inconsistent. `tools/geom.py` dumps
`verify/geometry/{summary.json, sample_records.json, model0_frame0.obj}`.

## `COMP` — compressed sprite  *(decoded; verified against all 429 `Graphics1.res` sprites)*

Leaf directory records in the `.res` are
`ff ff ff ff | u32 X | u32 zero=0 | u32 size | u32 data_offset | name\0`
(parser: [`tools/resfile.py`](../tools/resfile.py) `list`/`extract`). A robust
sentinel scan recovers **570 leaves in `Graphics1.res`, 429 of them `COMP`
sprites** (`.lls`); every shipped `COMP` is `bpp==16` (RGB555).

Decoder: [`tools/comp.py`](../tools/comp.py) (writes PNG, stdlib only) and the
byte-identical browser port [`web/comp.js`](../web/comp.js)
(`decodeCOMP(arrayBuffer, offset) -> {width, height, rgba}`).

Header (little-endian, 0x28 bytes):

| offset | type | field | notes |
| --- | --- | --- | --- |
| +0x00 | char[4] | `"COMP"` | (loader overwrites it in RAM after load) |
| +0x04 | u32 | width | e.g. 640 |
| +0x08 | u32 | height | e.g. 480 |
| +0x0c | u32 | bpp | 16 = RGB555 (only kind shipped); 8 = paletted |
| +0x10 | u32 | count/flags | 1 |
| +0x14 | u32 | reserved | 0 (bit0 only picks a blitter group; same pixels) |
| +0x18 | u32 | `s0` | payload length = `block_size - 24` (measured from +0x18) |
| +0x1c | u32 | `s1` | **pixel-word count** → pixel stream = `2*s1` bytes |
| +0x20 | u32 | `s2` | **length-stream byte count** |
| +0x24 | u32 | `s3` | **control-opcode count** (ctrl bytes = `ceil(s3/16)*4`) |
| +0x28 | … | payload | three sub-streams below |

Payload, three sub-streams concatenated from +0x28:

```
pixels : 2*s1 bytes   RGB555 literal pixels (16-bit LE words)
length : s2   bytes   run-length bytes
control: remainder    2-bit opcodes, 16 per u32, LSB-first (s3 opcodes)
```

`payload = 0x10 + 2*s1 + s2 + control_bytes`; the control length is implicit
(`s3` is the opcode *count*, not a byte size). Verified exact on every sample
(e.g. `printinfo.lls`: `s1=615,s2=240,s3=464` → `0x10+1230+240+116 = 1602 = s0`).

### Decode loop (read from the engine's own software blitter)

`__BMPLoader` @ `0x0044e010` loads the raw block and tags the image type
(`bpp 16 → type 3`; `LLS555To565` @ `0x0047d6a0` is only a 555→565 colour
repack for the display surface, **not** decompression). At draw time
`RenderSprite` → `0x00464ee0` dispatches type 3 to `0x00466770`, which splits
the sub-streams and calls the per-row decoders `0x004673f0` / `0x00467d10`
(identical opcode logic). The opcode machine reads a 2-bit code from the control
stream (`lo` = even bit, `hi` = odd bit of the current u32, LSB-first via a
rotating mask):

| code | action |
| --- | --- |
| `00` / `01` (hi=0) | emit **1 literal** pixel (next pixel word) |
| `10` (hi=1,lo=0) | emit **1 transparent** pixel |
| `11` (hi=1,lo=1) | **escape**: read length byte `L` from the length stream |

For an escape with `L>0`, read one more 2-bit sub-opcode:

| sub | action |
| --- | --- |
| hi=1 (`10`/`11`) | `L` transparent pixels |
| `00` | `L` literal pixels (copy run) |
| `01` | `L` copies of one pixel word (fill run) |

`L == 0` is the **end-of-scanline** marker → advance to next row (`x=0,y++`).
Transparency is entirely opcode-driven: untouched pixels stay transparent
(alpha 0). The COMP path has **no per-pixel colour key**, so a pixel *value* of
0 is a genuine black pixel (the `0x7ff`/`0x3ff`/`0xfe` keys from
`GetTransparentColour` belong to the DirectDraw hardware-blit path for opaque
backgrounds, not to this decoder).

RGB555 → RGB888: `R=bits14..10, G=9..5, B=4..0`, each 5-bit channel expanded as
`(v<<3)|(v>>2)`.

### Verification

All **429** sprites decode with the pixel stream consumed **exactly**
(`bytes == 2*s1`); decoded `W×H` matches the header and the RGBA buffer is
exactly `W*H*4` bytes. The fourteen 640×480 backgrounds are coherent images, not
noise (`TitleScreen1.lls` — the LEGOLAND title art — 5 812 distinct colours;
`CertificateScreen.lls` 13 563; `AdvertScreen.lls` 10 465). `web/comp.js`
reproduces the Python RGBA byte-for-byte (MD5-identical). Sample PNGs +
histograms + an ASCII luminance sanity grid live in
`scratchpad/verify/comp/`.

#### Independent adversarial re-verification (2nd agent, different sample)

Re-ran `tools/comp.py` over **all 429** COMP leaves and inspected a
**disjoint** sample from the original report. Confirmed:
`Graphics1.res` = 429 COMP of 570 leaves, every block `bpp==16`; **429/429**
decode with pixel stream consumed exactly (`bytes==2*s1`), decoded `W×H`
equal to header, `len(rgba)==W*H*4`, payload arithmetic
`0x10 + 2*s1 + s2 + ceil(s3/16)*4 == s0` closing exactly, and no
control/length over-read or block-past-EOF on any sprite. Four 640×480
backgrounds **not** in the first report were rendered and visually verified as
real UI art (`FreeplayScreenBK` menu, `optionscreenBK` minifig-DJ scene,
`InterfaceBG` HUD frame with a transparent play-area centre, `Watch` icon with
transparent background); neighbour mean-abs-difference 3.5–10.5 per channel
(random noise ≈ 85), thousands of distinct colours each. The row decoder
`0x004673f0` disassembly matches `comp.py` (mask init 3, `rol 2` + advance-on-
wrap, `0xaaaaaaaa`/`0x55555555` hi/lo tests, length-byte escape, `L==0` ends
row). Codec + `.res` container: **confirmed**.

## Audio / music / video  *(catalogued by `tools/audioinfo.py`, header-verified)*

All counts below were produced by parsing real RIFF headers across
`gamedata/main/` + the mounted disc (`/Volumes/LEGOLAND/`). See
`scratchpad/verify/audio_video/catalog.txt`.

### Speech WAVs — `Speech/*.wav` (1266 files, loose on disc)

**Not plain PCM** (the earlier note was wrong). Every one of the 1266 files is:

| field | value |
| --- | --- |
| wFormatTag | `0x0002` = **Microsoft ADPCM** (4-bit) |
| channels | 1 (mono) |
| sample rate | 22050 Hz |
| bits/sample | 4 |
| nBlockAlign | 512 bytes |
| samplesPerBlock | 1012 |
| chunks | `fmt ` (cbSize 32, with coef table) + `fact` + `data` |

`…z.wav` twins carry the **same** MS-ADPCM format, not a heavier "compressed"
variant — the `z` names mirror in-game object names (`Path`/`Pathz`,
`balloon`/`balloonz`), i.e. a second speech set, not a codec difference.

**Browser playability:** MS-ADPCM is *not* natively decodable by WebAudio
`decodeAudioData`. Either transcode to PCM/Opus at build time, or ship a small
JS MS-ADPCM decoder (the format is trivial: per-block predictor + 4-bit
nibbles, coef table in `fmt `). Cheap either way.

### AVI video — Indeo 5 (40 files total)

| set | codec | size | fps | count | audio |
| --- | --- | --- | --- | --- | --- |
| `AD_*.avi` character anims (`gamedata/main/`) | IV50 | 112×96 | 29.97/30 | 26 | **silent** (video-only) |
| disc-root cutscenes | IV50 | 320×200 (2× 320×240) | 15 | 13 | stereo |
| one legacy cutscene (`California.avi`) | **IV32** (Indeo 3.2) | 320×200 | 15 | 1 | PCM |

Video handler fourcc is `IV50` (Indeo 5, `Ir50_32.dll`) for all but one
`IV32`. `strf` biBitCount = 24. The 26 `AD_*` clips have **no audio stream**.
Cutscene audio streams are mixed: IMA-ADPCM (6), MS-ADPCM (3), PCM (5), all
22050/44100 Hz stereo/mono.

Note: the `avih` dwTotalFrames field is reliable for the `AD_*` clips (16–64
frames) but holds junk for several disc cutscenes; use the per-stream `strh`
dwLength for those.

**Browser playability:** Indeo 5/3 is **not browser-native** and is a
proprietary/legacy codec — must be transcoded (e.g. to H.264/VP9/AV1 MP4/WebM)
at build time. `ffmpeg` decodes IV50/IV32.

### DirectMusic — `.sty` / `.sgt` / `.bnd` / `.bnv`

All are **RIFF DirectMusic forms** except `.bnv`:

| ext | RIFF form fourcc | meaning | count |
| --- | --- | --- | --- |
| `.sty` | `RIFF …DMST` (styh) | DirectMusic **Style** | 212 |
| `.sgt` | `RIFF …DMSG` (segh) | DirectMusic **Segment** | 30 |
| `.bnd` | `RIFF …DMBD` | DirectMusic **Band** | 1 |
| `.bnv` | *non-RIFF*, magic `01 01 40 00 …` | engine-native band/visitor blob (ride visitor sets, e.g. `BlokeBox0N` name table) | 23 |

Styles/segments carry a `UNFO/UNAM` UTF-16 name (e.g. "EItran2", "Band19").
Loaded by `LoadMusicStyle` / `LoadMusicSegment` / `LoadMusicBand` (see
BINARIES.md).

**Browser playability:** DirectMusic dynamic score has **no browser runtime**.
It is effectively out of scope for a faithful port — options are (a) pre-render
the score to audio stems, or (b) reimplement a MIDI-ish sequencer over the
style/segment data. The `.bnv` files are game-logic data, not audio.

### Summary — what plays natively vs needs work

- **Native (with a tiny JS shim):** speech WAVs — MS-ADPCM decode in ~50 lines,
  then WebAudio.
- **Transcode at build time:** all AVI (Indeo 5/3 → MP4/WebM).
- **Out of scope / hard:** DirectMusic `.sty`/`.sgt`/`.bnd` dynamic score.

## Level & object data (from `main.z`)  *(RE in progress)*

10 levels, each with a `.ltx` / `.lms` / `.lfm` triple; tile sets `.tsf` /
`.tsm`; image lists `.ilf`; object definitions `.odf`; `.bnv` (23); plus the
`BUILD MENU` config listing tile sets, mappings and object classes per level.

## LEVELS — park maps, tiles, objects  *(confirmed by parsing all 17 maps + reading `LoadBaseMap`)*

Parser: `tools/leveldata.py`. Verification dumps: `scratchpad/verify/levels/`.

### Where the real levels live

The `.ltx` files in `gamedata/main/` are **not** the park levels — they are the
10 saved *roller-coaster designs* `ROLLERCOASTER0000..0009.ltx` (see below).
The actual playable park **levels are `.MAP` files inside
`gamedata/disc/Legoland.res`**, loaded by name through `.\LevelMaps\%s`. The 10
game levels are `GLONE.MAP … GLTEN.MAP`; `ONE.MAP…FIVE.MAP` are smaller
tutorial/driving-school maps, plus `freeplay big map.MAP` and `icmedit.map`.
All 17 parse to **exactly EOF (0 bytes leftover)**.

### `Legoland.icm` — the "BUILD MENU" master index  *(fully parsed)*

The config file the task calls "legoland.exe" is `gamedata/main/Legoland.icm`
(17221 B). Loader `LLIDB_LoadICM` @ 0x0047aff0. Structure:

```
+0    u32 count (294)
      count × 20-byte records: {u32 ptr0, u32 ptr1, u32 size/flags, u32 data_ptr, u32 0}
                               (the ptr fields are stale save-time heap addresses)
@tail for each record: two length-prefixed strings  {u32 len; len bytes}
      -> record = (label, value): "BASIC TILES 1" -> "BASIC TILES 1.TSF"
```

The value column maps a **logical name → asset filename** and drives every
`.TSM/.TSF/.ILF/.ODF/.CSP/.MAP` lookup. All 24 distinct names referenced by the
10 GL levels resolve through this index to real members of `Legoland.res`.
Note: `str` here and in `.MAP` = `u32 len` + `len` raw bytes (no NUL).

### `.MAP` — isometric park level  *(fully parsed; `LoadBaseMap` @ 0x00461a50 / export 344)*

```
str   name
str   tsm_mapping        # logical name of a .TSM tile-mapping (via .icm)
str   terrain            # logical name of a terrain .ILF   (via .icm)
u16   width              # stored at g_map+0x14
u16   height             # stored at g_map+0x16
u32   n_classes ; n_classes × str          # object-class table (logical names)
u32   n_objects ; n_objects × {u32 class_idx, u32 x, u32 y}   # placed objects
u32   n_pathsets; n_pathsets × str         # path tile-sets (.TSF logical names)
u32 s1 ; s1 bytes        # RLE layer: tile graphics  (-> SetMapTile)
u32 s2 ; s2 bytes        # RLE layer: map flags      (-> SetMapFlags)
u32 s3 ; s3 bytes        # RLE layer: RF/path flags  (-> Set_RFFlags/AddPathTileGFX/AddPathSquare)
u32 s4 ; s4 bytes        # RLE layer: user flags     (-> Set_UserFlags)
u32 n_extra ; n_extra × 20 bytes           # per-object extra records (func 0x462c00)
["BRIDGES!"(8) + str bridge_terrain]       # optional tag; if absent, not consumed
<terrain-tile stream>                      # variable, no size prefix, fills the grid to EOF
```

- **Grid** is `width × height`, row-major, iso tiles (engine tile stride 0x14).
- **Objects** (rides/shops/scenery/hedges) are `{class_idx, x, y}`, `x<width`,
  `y<height` (verified in-bounds on all maps). `class_idx` indexes the map's own
  object-class table; e.g. GLTHREE places 521 `HEDGE` + `CASTLE BBQ`, `FORT`,
  `SPIDER RIDE`, `ENTRANCE 1`, … at grid coords.
- **Layers s1–s4** share one 2-bit-tagged RLE (control byte: `n=b&0x3f`
  (0→64), `mode=b&0xc0`, jump table @ 0x462900) that walks the grid row-major.
  They are size-prefixed and self-terminate at grid-fill; `leveldata.py` skips
  them by size (their per-cell tile/flag values need the loaded `.TSF/.ILF`
  base ids to resolve to absolute graphics).
- **Terrain-tile stream** (last, no size prefix) is a small state machine read
  1–2 bytes at a time until the grid is full: state 0 reads a control byte
  (`0`→ solid-run state 1; `k>0`→ skip `k` cells then state 1); state 1 reads a
  `u16` per cell, `0xFFFF` ends the run (→state 0), else the cell's terrain tile
  = `terrain_table[hi].base + lo` for `hi = code>>8`, `lo = code&0xff`.
  `leveldata.py` records `(hi, lo)` per explicit cell; decoding it is what makes
  every map land exactly on EOF.

Example (`GLONE.MAP`): `72×102`, mapping `EXPLORER MAPPING`, terrain
`NEW WESTERN TERRAIN`, path set `NORMAL PATH TILES`, 3 objects
(`POTTING SHED`, `MECHANICS HUT`, `ENTRANCE 1`).

### Tile / image / object asset headers *(header shapes confirmed; bodies partial)*

- **`.TSM`** tile-mapping: `u32 count` then length-prefixed strings — maps a
  mapping name to its `.TSF` tile set (`"BASIC MAPPING" → "CASTLE GRASS SET"`).
- **`.TSF`** tile set: `u32 n_tiles`, `str name`, then per-tile records.
- **`.ILF`** image list: `u16 n_images`, `u16 type`, `str name`, then records.
- **`.ODF`** object def: `u32 size`, `u32 count`, then binary fields
  (footprint / bounding box, sprite refs). Loaded by `LLIDB_LoadODFData`
  (0x0007bf70) / `LoadObjectClass` (0x00080b40).

### Roller-coaster designs (`gamedata/main/`, the `.ltx` triples)

- **`.ltx`** = `u32 width`, `u32 height`, then `width*height` tile bytes.
  `ROLLERCOASTER00NN.ltx` are all `32×32` (8 + 1024 = 1032 B). Read into a flat
  buffer by the coaster loader @ 0x00420750 (`"%s.ltx"`).
- **`ROLLERCOASTER.obj`** / **`.txt`** = CRLF text lists of the `.lms/.lfm`
  model names and sprite part names the coaster uses.
- **`Rollercoaster.lpt`** = palette: `u32 count` (145) + `count` × RGBA (BGR0),
  584 B.
- **`.lms/.lfm`** = per-object 3-D model geometry / frame data (`"%s.lms"`,
  `"%s.lfm"`), keyed by the `.obj` name list.

## Tile pipeline — painting a park (`.TSM` / `.TSF` / `.ILF` / `.ODF` / `.CSP`)

How a level's grid becomes real sprites. All these members live in
`Legoland.res` and resolve by name (`logical + ext`); the `.lls` tile/object
sprites they name are decoded from `Graphics1.res` / `Graphics2.res`. Ground and
path tiles are **32×16** — a 2:1 isometric diamond (`GetTileDimensions`
@0x00460540: width = 2 × height). Loaders: `LLIDB_LoadTSMData` (0x0047ce40),
`LLIDB_LoadTSFData` (0x0047cba0), `LLIDB_LoadILFData` (0x0047cfc0),
`LoadBaseMap` (0x00461a50), `SetMapTile` (0x00461780).

- **`.TSF` tile set:** `u32 n`, `str name`, `n × {u32 tile_code, u32}`,
  `n × str image.lls`. Maps a tile code to a ground/path sprite
  (`EXPLORER SAND SET` → 97=`sandy1.lls`, 65=`sandy2.lls`, …).
- **`.TSM` mapping:** `u32 count`, `count × {str mappingName, str tileSetName}`.
  A level's `tsm_mapping` names the `.TSM`; it points at the `.TSF`(s).
- **`.ILF` image list:** `u16 n`, `u16 type`, `str name`, then `n`
  length-prefixed image names — the terrain edge/cliff tiles (`wst n cliff 1.lls`)
  keyed by the level's terrain-stream index.
- **`.ODF` object definition:** binary header + ascii strings. The object's main
  sprite is the first `.LLS`/`.CSP` string that is neither an `ICON` nor a `_BU`
  build-up variant (`hedge.odf`→`HEJT5.LLS`, `FORT.ODF`→`FORT SPRITE.CSP`). Also
  names the behaviour DLL, theme, menu and description.
- **`.CSP` composite sprite:** `u16 n`, `u16 type`, `str name`,
  `n × {s32 dx, s32 dy}`, `n × str image.lls`. The parts layer at per-part pixel
  offsets to build a big/animated object (`FORT SPRITE` = fort3/2/1.lls).

### Per-cell resolution — solved (`LoadBaseMap` 0x00461a50)

The `tile_gfx` RLE is **accumulator-based**, not a flat byte array (this was the
missing piece). Reverse-engineered from `LoadBaseMap`'s decode loop and the
LLIDB loaders (see `scratchpad/slope_re/*.md`):

- Control byte `b`: `n = b&0x3f`, `mode = b&0xc0` (for mode≠0, `n==0`→64). The
  `tile_gfx` stream starts at byte offset **2** (a 2-byte reserved marker).
- `mode 0x00` **SET**s a 1-byte accumulator `acc` (advances 0 cells). `0x40` =
  literal run of `n` cells (one data byte each), `0x80` = fill run (one data byte
  reused), `0xc0` = empty run.
- Per cell: `acc & 0x20 == 0` → **ground**: `group = acc-1`, `delta = dataByte`,
  and the drawn sprite = `level_TSM.tileset[group].images[delta]` (the engine
  stores the absolute slot `TSF.base + delta` at cell+8; `TileSpriteArray[slot]`
  is exactly `images[delta]`). `acc & 0x20 != 0` → **path/object**: a path square
  from `pathset[acc&0x1f]`.
- **Path autotiling** (render-time): a cell is a path where the rf_flags byte has
  bit0, or map_flags bit4 set and not rf bit1. The sprite is picked from the 4
  orthogonal neighbours' path state — a 4-bit mask into `NORMPATH`'s
  `OUTL{0..15}`, plus concave-corner overlays `OUTL16..18`.
- **Terrain stream** writes the base plane (cell+0xa) via the *same* level TSM:
  `group = hi-1`, `delta = lo` (the ground plane, not the cliffs).
- **Cliff/edge terrain** is a separate layer: the `.MAP`'s `n_extra` 20-byte
  records (`{i32 x, i32 y, i32 2x, i32 2y, u32 image}`) are turned into render
  objects by `sub_462c00`, each drawing terrain-`.ILF` image `image&0xff`
  (`(image>>8)&0xff` selects the bridge ILF) as a tall bottom-anchored sprite at
  iso screen `(x, y)`. These are the raised perimeter walls (`wst n cliff 1..4`,
  `wst left/right`, `new west ent`, …); `sub_462c60` binds each object's sprite.

This is implemented byte-for-byte in [`tools/tilemap.py`](../tools/tilemap.py)
(Python reference) and [`web/tilemap.js`](../web/tilemap.js) (browser). The two
agree **cell-for-cell** and every one of the 17 shipped maps resolves **100 %**
of its cells to a concrete `.lls` sprite (36 864 cells of the big map in ~4 ms).
The Levels tab now paints each cell's exact tile — ground, crystal, and
neighbour-autotiled paths — under the object sprites.

The browser renderer ([`web/tiles.js`](../web/tiles.js) +
[`web/level.js`](../web/level.js) + the Levels tab in
[`web/app.js`](../web/app.js)) paints every park isometrically from this chain:
real ground tiles, `.LLS` objects, and `.CSP` composite rides/buildings, with
scroll-zoom and drag-pan.
