# Matching decompilation — workflow

The goal of the decomp proper: rewrite `legoland.exe` in C that, compiled with
the **Visual C++ 6.0 SP3** toolchain the game shipped with, reproduces the
original machine code **function-by-function**. `legoland.exe` exports 716
named symbols: 675 function entries in `.text` and 41 data exports
([`symbols/legoland.exports.txt`](../symbols/legoland.exports.txt)), so we
already have most of the names — a head start LEGO Island never had.

## Toolchain

We reuse the VC6 SP3 toolchain (wibo + `msvc6.3` + `sp3-libs`) — the same
compiler era as LEGO Alpha Team, and the one whose Rich header `legoland.exe`
carries (cl 12.00.8168 / codegen 8447). It runs natively on macOS/Linux via
[wibo](https://github.com/decompals/wibo), no Wine/Docker. `toolchain/` is a
machine-local symlink; point it at any assembled VC6 SP3 tree
(`ALPHATEAM_VC6_ROOT` overrides).

## The loop

1. Disassemble the target: `tools/disasm.py original/legoland.exe <ExportName|0xRVA>`.
2. Write matching C in `LEGOLAND/*.c`, annotated isle-style:

   ```c
   // FUNCTION: LEGOLAND 0x00441ec0
   void* GetSpriteForLayer(RenderObj* obj, int layer)
   {
       return obj->layers->sprites[layer];
   }
   ```

3. Check the match: `tools/match.py LEGOLAND/foo.c FuncName 0x00441ec0`. It
   compiles the file with VC6, extracts the function's code from the COFF object
   (applying relocations), and diffs it instruction-by-instruction against the
   original, printing a side-by-side and a match %.
4. Iterate the C until 100% (branch direction, local ordering, and struct field
   offsets are the usual levers — VC6 codegen is stable and reproducible).
5. `tools/verify.py` runs every annotated function and prints the tally.

Only struct **offsets** are load-bearing; field/type names are ours. Types live
in [`LEGOLAND/legoland.h`](../LEGOLAND/legoland.h).

### Full-body verification (important)

Until 2026-09-03 `tools/match.py` (and therefore `tools/verify.py`)
disassembled only up to the **first `ret`**, so a function with an early-return
guard was compared only on its prologue and a void tail-`jmp` wrapper could not
be bounded at all. **Both now use the same extent rules as `tools/audit.py`:**
the original's true extent is found by control flow (the first `ret` or
unconditional `jmp` that no earlier branch jumps past, following `switch` jump
tables and stopping at the 16-aligned padding after a `noreturn` call), the
compiled COMDAT is trimmed to that extent, and a match requires the same
instruction count, the same byte length, zero normalised mismatches and no
branch escaping the extent. `match.py` prints `[orig=NNi/NNNB; extent ok]` on
its `MATCH:` line only when all four hold, and `verify.py` counts nothing
without that token. `audit.py` imports the walker from `match.py`, so the two
gates cannot disagree. `tools/matchfull.py` (full body, difflib-aligned) is the
iteration tool. Prologue-only or fabricated-tail reconstructions are never
committed as `// FUNCTION:`.

    python3 tools/audit.py LEGOLAND/foo.c      # [OK]/[WIP]/[REJECT], ends PASS/FAIL

Beware the two ways `matchfull` misleads: it truncates the original to the
compiled length (a short reconstruction can score a false 100%), and it keeps
decoding the `.rdata` jump table that `/Gy` places after a `switch` function's
final `ret` (a correct function can score 77%). `audit.py` handles both.

## Status

**As of 2026-09-07: 2751 functions at 100%** — 665 of the 675 code exports
(98.5%) plus 2086 recovered unexported functions, together **66.1% of the
game's ~628 KB of code** (`python3 tools/coverage.py`; 77.3% including
partials).
`SaveGame` and `LoadGame` are both exact so the whole `.sav` format is
documented and reproduced; `tri3d.c` reproduces the software 3D renderer;
`docs/RIDE_CALLBACKS.md` names 265 ride callbacks and which object slot each
fills. See `docs/HANDOFF.md` for the session checkpoint and what to do next.

10 exports remain, every one a genuine partial carrying its measured residual
and first diverging instruction index in a note above its marker. (Until
2026-09-03 another 14 exact exports — and 48 exact internal functions — were
held at `// WIP-FUNCTION:` only because `tools/match.py` stopped at the first
`ret` and could not bound a void tail-jump wrapper; it scored `KillHelp`
37.5%. The extent rules were ported into `match.py` that day and all 62 were
promoted — see "Tail-jump functions" below.) Run `python3 tools/remaining.py`
for the live list.

### Three progress numbers, and which one to quote

| measure | tool | value |
| --- | --- | --- |
| exported functions matched | `tools/remaining.py` | 665 of 675 (98.5%) |
| unmatched callees | `tools/callees.py` | moves both ways — the frontier, not progress |
| **bytes of game code matched** | **`tools/coverage.py`** | **75.1% (92.1% with partials)** — 2026-09-09, after the LL wave |

The first two are both true and both misleading on their own.

**Exports are a fraction of the game.** They are only the symbols the linker
exposed; 2751 functions are matched but just 665 of them are exports. Quoting
98.5% as "the project is nearly done" is wrong by a wide margin.

**The unmatched-callee number moves in both directions.** Every newly matched
file declares `extern`s for its own callees, so a productive round can RAISE
it: batch 24 matched 185 functions and the figure went from 631 callees /
31,000 instructions to 796 / 41,400, because the new files revealed more
frontier than they consumed. It measures the frontier, not progress.

**Bytes of matched code against bytes of game code is the honest headline.**
It only moves by doing work. `.text` is 679 KB, of which about 51 KB is the
statically-linked CRT (from 0x0049e000 up) and not a decompilation target,
leaving ~628 KB of game code. 244 KB of that is matched exactly.

So: nearly every EXPORTED function is done, and that was the right first
target because exports are the subsystem entry points — but roughly seventy
per cent of the game's code is unexported and most of it is still ahead.

Counts come from committed markers
(`git ls-files 'LEGOLAND/*.c' | xargs grep -h '^// FUNCTION: LEGOLAND' | wc -l`);
`tools/progress.py` reads the working tree, so run it on a clean checkout.
`docs/HANDOFF.md` is the continuation guide and `docs/LANE_BRIEF.md` the
verbatim brief every matching agent receives.

Files by subsystem: map pipeline (`loadmap.c`, `mapinit.c`, `mapbuild.c`,
`maprestore.c`, `pathgfx.c`, `pathsq.c`, `pathbuild.c`, `pathtile2.c`,
`tilehelp.c`, `objmap.c`, `objmap2.c`), simulation (`bnvpath.c`, `bnvmove.c`,
`blokeai.c`, `blokemisc.c`, `blokeanim.c`, `blokelist.c`, `workers.c`,
`workers2.c`, `rides.c`, `ridesave.c`, `power.c`, `money.c`, `buildtick.c`,
`workorder.c`, `lifecycle.c`, `math3d.c`, `bigsim.c`), rendering
(`renderinit.c`, `renderlist.c`, `render2.c`, `printlist.c`, `renderview.c`,
`bigrender.c`, `gpu.c`, `rin.c`, `layervis.c`, `surface.c`,
`sprite_override.c`, `spritemisc.c`, `sprite2.c`, `scroll.c`, `scrolltick.c`,
`text.c`), UI (`panelui.c`, `iconui.c`, `fpui.c`, `fpui2.c`, `mapscreen.c`,
`screens2.c`, `bigscreens.c`, `bighelp.c`, `popup.c`, `input.c`, `input2.c`,
`wndenv.c`), audio (`audiomisc.c`, `audio2.c`, `audio3.c`, `music.c`), data
(`llidb_odf.c`, `memdb.c`, `res.c`, `saveprof.c`, `profiles.c`, `savegame.c`,
`loaders.c`, `data2.c`, `listdel.c`), host/display (`screen.c`), plus
`sweep1–5.c` (small accessors) and `util.c`.

The map/render accessors, the `SetMapTile` family, and `GetRectArea` —
**14/14 at 100%**:

| addr | function | |
| --- | --- | --- |
| 0x004015c0 | `GetSpriteSize` | render accessors |
| 0x00460540 | `GetTileDimensions` | |
| 0x00441e80 | `GetLLSForSprite` | |
| 0x00441ea0 | `GetLLSForLayer` | |
| 0x00441ec0 | `GetSpriteForLayer` | |
| 0x00441ee0 | `GetRenderOffsetForLayer` | |
| 0x00461610 | `Get_RFFlags` | cell flag get/set |
| 0x00461760 | `Get_MapFlags` | |
| 0x004617d0 | `GetMapFlags` | |
| 0x004616e0 | `Set_RFFlags` | |
| 0x00461810 | `SetMapFlags` | |
| 0x00461780 | `SetMapTile` | |
| 0x00461630 | `GetCurrentRFFlags` | RF dispatch (calls a tile handler) |
| 0x00480960 | `GetRectArea` | footprint area over a Rect list |

`GetCurrentRFFlags` is the first non-trivial one — a bounds-checked lookup that
dispatches to a per-tile handler via a function pointer; it matched once the
handler was hoisted into a local (VC6 computes the callback once, then calls).

Reported as **normalized instruction match** (the checker normalises relocated
addresses/immediates and relative call/jump targets); not literal byte-identity.

### Bloke counter lifecycle (matched, 100% full-body)

The per-object-class visitor counters are now complete in `LEGOLAND/sweep3.c`:

| addr | function | |
| --- | --- | --- |
| 0x00480e10 | `AllocBlokeCounters` | allocate one byte per bloke for class kinds other than 0 and 2 |
| 0x00480e60 | `FreeBlokeCounters` | free and clear each class's counter array |
| 0x00480e90 | `ClearBlokeCounters` | clear one bloke index across every allocated class array |

These share the object-class list rooted at `g_objcls_head` with
`ClearObjectCounters`; the class kind is at +0x20 and the byte-array pointer at
+0xc8. Together with the already-matched `IncrementBlokeCounter` and
`GetBlokeCounter`, the allocation, reset, increment, read, and teardown path is
fully reconstructed.

### Object placement (matched, 100% full-body)

- **`PutObjOnMap` (0x00459ad0)** — 128/128. Places an object descriptor onto the
  map: runs the class placement callback, accumulates the object-type build
  stats (per-category area + `GetRectArea` + `AddObjectsPowerStats`), and for the
  `ENTRANCE 1` class writes the entrance render coords (`g_entrance_x/y`). The
  `ENTRANCE 1` coordinate tail needed two VC6 codegen levers to match its
  register allocation: (1) fetch the element data through a named `Elem*`
  intermediate (`Elem* e = ElemID(...); data = e->data;`) so `data` lands in edx
  (not eax); (2) form the `cell = 0 / cell = &g_map_rows[py][px]` join with
  explicit `goto`s so the address `lea` targets the index register and frees eax
  for the coordinate accumulator. A plain if/else strands the last 12
  instructions in an eax/ecx swap. Resolved by a parallel phrasing search.

## LoadBaseMap interface (for host / WASM integration)

`LoadBaseMap` @ 0x00461a50 is **matched at 100%** — all 1214 instructions of its
body, index for index, 3758 bytes in both. It is the largest function in the
decomp and lives in [`LEGOLAND/loadmap.c`](../LEGOLAND/loadmap.c). Its contract
below is stable for host/WASM integration:

**Signature:** `int LoadBaseMap(const char* mapName)` → 1 on success, negative on
failure (map element not found).

**Inputs / prerequisites (must be set up before the call):**

- The RES layer is mounted and the LLIDB is loaded (`LLIDB_LoadICM`): the map,
  its `tsm_mapping` and `terrain` are resolved via `LLIDB_FindElement` +
  `LLIDB_LoadData`, and the `.MAP` bytes are pulled with `RES_OpenFile` /
  `RES_ReadFile`. So `Legoland.res` + the ICM must be available.
- `g_map` (`0x004bcbf4`) points at an allocated map header; `LoadBaseMap` writes
  `width` (+0x14) and `height` (+0x16) from the `.MAP`.
- `g_map_rows` (`0x00801400`) points at an allocated `Cell*[height]`, each row an
  allocated `Cell[width]` (20-byte cells). `LoadBaseMap` zeroes `cell+0xc/+0x12`
  then fills the cells; it does **not** allocate the grid. **Map ownership stays
  with the host allocator.** (An earlier note here attributed the allocation to
  `InitGameMap` @ 0x459850 — that is **wrong**. Matching that function showed its
  8-instruction body resolves the `CASTLE OBJ` element into `0x0080ff64` and
  loads the 23-entry FX table at `0x004b9228`; it never touches `g_map` or
  `g_map_rows`. The grid allocator is `LoadMapTiles` 0x0045aad0 (`pathtile2.c`): one `calloc(0x14041f, 1)` kept at 0x667c9c, `g_map_rows` = align32(block+0x1f), rows at align32(block+0x41f) stepping 0x1400 — 256 rows × 256 cells × 20 B.)
- The tile-sprite tables `TileSpriteArray` (0x805f60) / `TileSpriteInfo`
  (0x801f40) are populated on demand by the TSF loads (`AllocTileSpace`); the
  `.lls` tile sprites resolve from `Graphics1/2.res`.

**Outputs / side effects (what the host reads back):**

- Per cell (`g_map_rows[y][x]`, stride 0x14): `+0x08` displayed tile,
  `+0x0a` ground/terrain tile, `+0x0c` map flags, `+0x0e` user flags,
  `+0x10` RF/path flags, `+0x00` object descriptor (object cells).
- Placed objects go through `PutObjOnMap`, which also advances the build-stat
  accumulators (`0x00667ce0…cf8`) and, for `ENTRANCE 1`, the entrance render
  coords (`g_entrance_x/y` @ 0x4b8320/0x4b8324).
- The perimeter cliff/bridge render objects are built from the `n_extra` records
  by `sub_462c00` into the terrain-object list head `0x00667ca8` (drawn later by
  `RenderFullMap`); `sub_462c60` binds each to its terrain-`.ILF` sprite.
- `LoadBaseMap` ends by calling `RenderInit` (0x462c60) and setting `0x667d50=1`.

**Asset-buffer expectations:** everything is pulled through the RES/LLIDB layer
during the call — there is no caller-supplied asset buffer. The host must have
the RES archives readable and the ICM loaded; the grid memory is the only buffer
the host owns and must keep alive for the map's lifetime.

**Map-loading pipeline (matched, 100% full-body):** every function `LoadBaseMap`
calls directly is now exact, so the whole map-load path is reconstructed.

| addr | function | file | |
| --- | --- | --- | --- |
| 0x00461a50 | `LoadBaseMap` | `loadmap.c` | 1214 insns — the whole loader |
| 0x00459850 | `InitGameMap` | `mapinit.c` | resolves `CASTLE OBJ`, loads the FX table |
| 0x0047b3f0 | `ElemID` | `mapinit.c` | name → `Elem*` (no stack frame: reuses the arg slot) |
| 0x00481c50 | `AddPathSquare` | `mapinit.c` | pushes a 0x24-byte path rect |
| 0x0045d350 | `AddPathTileGFX` | `pathgfx.c` | path overlay on a cell |
| 0x00462c00 | `BuildPerimeterObject` | `pathgfx.c` | perimeter cliff/bridge node → terrain list |
| 0x00462c60 | `BindTerrainObjectSprites` | `renderinit.c` | binds each terrain node to its sprite |
| 0x004618d0 | `SetBridgeDrawOffsets` | `renderinit.c` | per-theme bridge draw offsets |
| 0x00489b60 | `RES_OpenFile` | `res.c` | archive open |
| 0x00489cf0 | `RES_ReadFile` | `res.c` | archive read |

`BindTerrainObjectSprites` carries one deliberate `*(volatile int*)` read as a
**codegen lever** (VC6 otherwise CSEs that load with the two `& 0xff` reads below
it, where the original reloads the field in each arm). It is confined to that one
read and commented in place; replace it if a cleaner spelling is found.

**Map-build / placement helpers (matched, 100% full-body — `LEGOLAND/mapbuild.c`):**

| addr | function | |
| --- | --- | --- |
| 0x00459880 | `ResetBuildStats` | zero the 8 build-stat accumulators (0x667ce0..cfc) before the `.MAP` walk |
| 0x004598d0 | `TallyFootprintCell` | per-cell footprint tally: `blocked--` off-map/marked/env, `special++` for type 2/3 |
| 0x00459960 | `ResetBuildTimer` | stamp `g_build_timer` (0x667d10) with `GetGameTimer()` |

`ResetBuildStats` clears the same footprint/power tallies `PutObjOnMap` advances
(`g_area_total`/`g_area_type1..5`/`g_count_env` + 0x667cfc); LoadBaseMap calls it
before placing. `TallyFootprintCell(pos, &blocked, &special)` is the bounds-
checked cell classifier used to score a footprint (it reuses the same
`g_map`/`g_map_rows` cell-fetch as the `PutObjOnMap` ENTRANCE tail): it
decrements `*blocked` for out-of-bounds / `flags&0x10` / environment-class cells
and increments `*special` for cells holding a type 2 or 3 object. It matches with
the bounds/null failures sharing one `goto fail` block while the two flag-based
`(*blocked)--` paths stay inline (VC6 does not tail-merge them). The remaining
LoadBaseMap helpers (`sub_49e573`/`sub_49e4ff` env-state + a heap wrapper,
`sub_4663f0` render) reach into the environment/render subsystems and are later
batches.

## LLIDB image database (asset resolution)

The LLIDB is the name→asset registry the whole asset pipeline goes through:
`FindElement(name) → LoadData(elem) → per-type parser → parsed table`. Element
struct (20 bytes, insertion-indexed via a paged table): `name`@0, `image`@4,
`type_flags`@8 (`(type&0xfff0)|loaded-bit0`), `data`@0xc, `refcount`@0x10.
Globals: capacity @0x6691a0, count @0x6691a4, page table @0x6691a8.

**Matched (100% normalized, full-body — the entire LLIDB core + registration +
every per-type parser):**

| addr | function | |
| --- | --- | --- |
| 0x0047b2d0 | `LLIDB_GetCount` | count |
| 0x0047b2e0 | `LLIDB_GetElement` | paged index → element |
| 0x0047b330 | `LLIDB_FindElement` | name scan (stricmp) |
| 0x0047b410 | `LLIDB_FindElementFromDataPtr` | data-ptr scan |
| 0x0047b5a0 | `LLIDB_GrowAndGetIndex` | paged-table allocator (256-elem pages) |
| 0x0047b610 | `LLIDB_RegisterNewElement` | dedup-or-insert name/image slot |
| 0x0047d3a0 | `LLIDB_LoadData` | lazy loader/dispatcher (bit0=loaded; switch on type&0xfff0) |
| 0x0047ce40 | `LLIDB_LoadTSMData` | `.TSM` tileset-list parser |
| 0x0047cba0 | `LLIDB_LoadTSFData` | `.TSF` tile-sprite parser |
| 0x0047cfc0 | `LLIDB_LoadILFData` | `.ILF` image-list parser |
| 0x0047d1a0 | `LLIDB_LoadCSPData` | `.CSP` composite-sprite parser |
| 0x0047bf70 | `LLIDB_LoadODFData` | `.ODF` object-definition parser |

`LoadData` dispatch: 0x10/0x1010→ODF, 0x20→TSM, 0x40→TSF, 0x400→ILF,
0x2000→CSP, 0x200/0x800→no backing file (returns 0).

The registration path (`GrowAndGetIndex` grows the paged table in 256-element
pages — realloc the page array + malloc a 0x1400 page; `RegisterNewElement`
dedups by name then strdups name/image into a fresh slot writing only
+0/+4/+8/+0x10, count++) and all five parsers now match at 100% full-body.
Parser code lives in `LEGOLAND/llidb_load.c` (TSM/TSF/ILF/CSP) and
`LEGOLAND/llidb_odf.c` (ODF, the 625-instruction object-def loader in its own
translation unit).

**On-disk formats & outputs (for the runtime):**

- `LLIDB_LoadTSMData` (0x47ce40): opens `TileData\<image>`, reads `u32 count`,
  `mallocs (count+1)` **8-byte records `{LLElem* entry, void* loaded}`**, reads
  a self-name (discarded), then per tileset reads `u32 len`+name → FindElement
  → LoadData, filling `{entry, loaded}`; terminates `{-1,-1}`. Sets
  `elem->data`=recs, `type_flags|=1`.
- `LLIDB_LoadTSFData` (0x47cba0): opens `TileData\<image>`, builds a 36-byte
  descriptor `{+4 n_tiles, +0 base_slot = AllocTileSpace id & 0xffff, +0xc
  code[], +0x10 second[], +8 sprites[]}`, loads each `.lls` tile sprite (auto-
  playing anim types 2/3 with >1 frame), then optionally links a **parent** tile
  element and writes itself into `parent->data+0x74`. **base_slot** is what tile
  codes are added to (see FORMATS.md tile pipeline).
- `LLIDB_LoadILFData` (0x47cfc0, `ImageData\<image>`) / `LLIDB_LoadCSPData`
  (0x47d1a0, `CompSprite\<image>`): image list `{u16 n, u16 type, name,
  n×(dx,dy doubled at load), n× .lls}` into a 36-byte descriptor `{+4 n, +0xc
  dx[], +0x10 dy[], +8 sprites[]}`; each sprite `LoadSprite`d. CSP additionally
  null-checks every loaded sprite before committing. On any alloc/parse failure
  both call `LLIDB_FreeILFTable` and return 0.
- `LLIDB_LoadODFData` (0x47bf70): opens `Objdesc\<image>`, builds a 0xd0-byte
  `ObjDef`, links it onto `g_odf_head` (0x669240), reads the fixed header then
  resolves the class's sprite/icon/build-anim/child LLIDB elements (missing
  names → `ODFError`, missing icon → `InstituteIcon.lls`), walks `count`
  localized string records (keeps `english`/record 0, skips the rest into
  `f78/f7c/f80`), then `SetStandardCallbacks` → optional `LoadObjectLibrary`
  (OC_USEDLL 0x10000) → `SetCustomCallbacks` → `ObjDefFinalize`.

Full disassembly-grounded drafts for all of the above are in
`scratchpad/slope_re/*.md` and the loader-draft workflow output.

## Global names from the export table

41 of the 716 exports are DATA, not functions (`python3 tools/remaining.py
--data`). They give the game's OWN names for globals the reconstruction had
been naming by address, so prefer these when naming things in new files. Where
a file already uses a different name, the recovered meaning was right; only the
label differs.

| VA | exported name | what the reconstruction calls it |
| --- | --- | --- |
| 0x004b9220 | `BGFullUpdate` | background full-update flag |
| 0x004bcbf4 | `lpConfig` | `g_map` in legoland.h — really ONE config record: screen pixel size at +0x00/+0x02, map extent at +0x14/+0x16 (panelui.c and fpui2.c read it as the screen dims, which is why both readings are correct) |
| 0x004bdea0 | `SPRITE_ClipRect` | `g_clip_rect` — the clip RECT `{0,0,640,480}` (data, not code) |
| 0x00667c54 / 0x00667c58 | `QueryObj` / `QueryClass` | the object under the query cursor |
| 0x00667ca8 / 0x00667cac | `OverlayList` / `OverlayILF` | the overlay chain (maprestore.c `ClearOverlays`) |
| 0x00667cb4 / 0x00667cb8 | `ScrollX` / `ScrollY` | the scroll position (24.8) |
| 0x00667cbc / 0x00667cc0 | `ScrollSpeedX` / `ScrollSpeedY` | the autoscroll velocities (pathtile2.c `MouseScrollMap`) |
| 0x00667d70 | `DDRAWENV` | the whole host GPU block documented in gpu.c |
| 0x006681fc | `LastFrameMS` | the frame-time tick scale |
| 0x006687d0 | `FocussedIconPtr` | the focussed icon (fpui.c `CheckFocussedIcon`) |
| 0x00669240 | `ObjectClassList` | `g_odf_head`, the 0xd0-byte ObjDef chain |
| 0x0066b574 | `FirstBloke` | the people chain head |
| 0x0079a694 | `DMusicInitialised` | the music-ready flag (audio2.c) |
| 0x0079a8a8 / 0x0079a8ac | `GardenerList` / `MechanicList` | the hired-worker lists (workers.c guessed both correctly) |
| 0x007cacd4 | `FrameNumber` | frame counter |
| 0x007fd620 | `NewObjectPtr` | the object being placed |
| 0x007fea48 | `FramesPerSecond` | fps |
| 0x007febc0 | `EditCursor` | the edit cursor (objmap.c / objmap2.c) |
| 0x00800400 / 0x00801b24 | `ObjectPartArray` / `ObjectPartCount` | the object-part table |
| 0x00801400 | `GameMap` | `g_map_rows` — the 256x256 Cell grid (allocated by `LoadMapTiles`) |
| 0x00801f40 | `TileSpriteInfo` | `g_tile_info` |
| 0x00805f60 | `TileSpriteArray` | `g_tile_sprites` |
| 0x0080ff74, 0x0080ff78, 0x0081014c, 0x008119a0, 0x008119a8, 0x008119ac | `NEWFLC_AutoPlay`, `NEWFLC_PauseType`, `NEWFLC_ID`, `NEWFLC_CheckDuplicate`, `NEWFLC_BuffSize`, `NEWFLC_Repeat` | the FLC/FLI video player settings |
| 0x00810160 | `QueryCursor` | the query-mode cursor |
| 0x008119b0 | `EditMode` | edit mode |
| 0x00813a40 | `GamePad` | the game-button block (bighelp.c calls it `GameInput`) |
| 0x00813b00 | `CONTROLLERBUFFER` | the Controller record (input.c / input2.c) |
| 0x00830fc0 | `PathCursor` | the path-laying cursor |
| 0x00832800 | `MapStats` | the block objmap.c calls the "MapAI state block" — it is the park STATISTICS accumulator that `DoMapAI` fills (categories, counts, income), which is the better reading of that function |
| 0x00832bf0 | `PathSprite` | the u16 path tile code (pathbuild.c) |

## Tooling note (for Codex)

**Applied 2026-09-03 with the user's go-ahead:** `tools/match.py` now bounds
both bodies by the original's true extent (the walker that lived in
`tools/audit.py`, which now imports it), normalises every direct branch/call
target including bare-decimal and `loop` forms, writes its object to a per-pid
path (`--obj` to override) and honours `LEGOLAND_CL` for the compiler wrapper.
`tools/verify.py` counts a function only when `match.py` prints `extent ok`.
The change was validated on hand-assembled sequences for each shape (tail-jmp,
`noreturn` tail, recursive self-call, early-return guard, escaping branch),
then against the binary: the ported walker agrees with the pre-port `audit.py`
on all 1580 marker addresses, `audit.py` PASSes every file, and `verify.py`
reports **1473/1473** after the 62 promotions (run alone, 2026-09-03 ~14:30).

### Tail-jump functions (`match.py` change — applied 2026-09-03)

A void wrapper whose last statement is a call compiles to a tail `jmp` with
**no `ret`** of its own, e.g. `UnLoad_PopUpInfo` (0x00471450: `push 0x2c3 /
call RemoveIconGroup / add esp,4 / jmp 0x471170`) and `UnLoad_Interface_Icons`
(0x00474800). `match.py`'s first-`ret` walk runs through the nop padding into
the next routine and scores them 64% / 67% although they are exact. `audit.py`
already handles this: the original ends at the first `ret` or unconditional
direct `jmp` that nothing jumps past (forward targets include the case blocks
of a `switch` jump table, read from `.rdata`; a target before the entry, at or
after the next export, or a 16-aligned address reached across nop padding is
external), and the compiled body is trimmed to that extent and checked for
branches that escape it (relocated targets carry match.py's 0x00990099
sentinel and are external by construction). Both functions sat at `// WIP-FUNCTION:` with a note until the rule was
ported; they and the other 60 audit-exact WIPs are now `// FUNCTION:` and
`verify.py` confirms all of them.

### SEH scope-table functions (`match.py` change — applied 2026-09-08)

A VC6 `__try/__except` body whose try block is straight-line ends in a `jmp`
over its filter and handler blocks, and nothing in the code branches to
those blocks: they are reached only through the scope table in `.rdata`
that the SEH prologue pushes (`push -1 / push <scopetable> /
push __except_handler3 / mov eax, fs:[0]`). `true_extent` took that `jmp`
as the function's end, so `WinMain` (0x00453d10) walked 31i/93B of its
48i/143B, the compiled epilogue jump was flagged ESCAPES, and a
byte-identical body could never print `[OK]`. `WriteExceptionReport` and
`ReportModuleDetails` only passed because forward branches inside their try
blocks had already carried `furthest` past the `jmp`. No C spelling moves
the original's terminator, so the walker learned the table: on an SEH
prologue it records the scope-table VA, and at every `mov dword ptr
[ebp-4], K` that enters trylevel K it adds entry K's filter and handler
(`{EnclosingLevel, Filter, Handler}`, 12 bytes each) to `furthest`. Reading
only the levels the body enters bounds the table — the next SEH function's
entries follow contiguously (0x00453da0's are followed by 0x00454380's).
A full-tree `audit.py` before/after over 2,972 annotated bodies changed one
line, WinMain REJECT -> OK; the other six SEH frames in the binary (three
of them CRT) re-walk to the same extent. `inventory.py`'s own
`seh_extent` fallback is now redundant for game code.

### VC6 SP3 codegen levers (learned the hard way on `LoadBaseMap`)

- **`ZBuffer_FillPoly` `0x00423350` (closed 2026-09-08, 101 of 101 exact,
  `schoolcar6.c`).** The hand-written `__asm` span filler; the same levers
  closed its shaded twin `ZBuffer_FillShadedPoly` `0x0041fa10` on the
  still-unmerged `scope/LL9`:
  - **A multi-member aggregate local outranks every spilled scalar for a
    frame home.** VC6 places such an aggregate deepest-first in declaration
    order AHEAD of the scalars it still enregisters, so whatever scalars
    remain fall back to the dead argument slots. Grouping the dead-store
    copy, the row pointer and `pitch` as
    `struct { int dead; short* row; int pitch; } r` — with `r.row` named
    inside the `__asm` block — pins those three at `-0x10`/`-0xc`/`-8` and
    drops `ylast` and `y` into the dead `n` and `key` slots the original
    uses. As five plain scalars VC6 hands `ylast` the `-0xc` home and the
    row pointer the argument slot, which is the residual this body sat on
    for 46 strict mismatches. The aggregate must exceed four bytes to
    rank: a four-byte `dead` in ANY aggregate shape (`short[2]`,
    `char[4]`, a two-short struct, a one-member struct or array) is
    scalarised and sorts with the scalars. This is the local-frame
    counterpart to the by-value parameter rule under SCOPES Y AND Z.
  - **A plain store into a memory-resident aggregate member survives
    dead-store elimination — and must NOT be made `volatile`.**
    `r.dead = g_zb_4b5b20;` reproduces the original's dead store on its
    own. A volatile cast on the member, a volatile cast on `&r`, or a
    `volatile` member makes the whole aggregate address-exposed: the frame
    grows to `0x64`, `pitch*2` is hoisted into the `n` slot, and the body
    loses 40 instructions. Only a STANDALONE scalar dead store needs the
    volatile cast (a plain scalar store is deleted outright even in a
    function containing `__asm`), which is why the earlier WIP body
    carried one.
  - **Stores then a read-modify-write hoist the FIRST field.** The original
    lifts `e->x` above the `e->side` branch ahead of `e->step`; written
    `ed[k].x = e->x - e->step;` VC6 forms the `e->step` CSE temporary first
    and hoists that instead. Written
    `ed[k].x = e->x; ed[k+1].x = e->step; ed[k].x -= ed[k+1].x;` through
    the address-taken array the pair comes out in the original's order, and
    store-to-load forwarding folds the RMW back to one register subtract,
    so the instruction count does not move.
  - With the row pointer and `pitch` inside the aggregate the body no
    longer wants the free `volatile` read of `y` that the old WIP note
    prescribed — it is one instruction long with it. Set-up order stays
    load-bearing at one to three instructions apiece: `pitch` before `row`,
    or the dead store moved to either side of `ylast`, each shifts a load
    in the head.

- **SCOPE Codex-F (closed 2026-09-09, 26 of 26 exact; evidence in
  `docs/lanes/codex-f.md`).** Coaster draw passes, rider updates, route
  callees (`coaster10.c`, `ridemachine2.c`, `uistubs2.c`).
  `Copters_UpdateCarRider` took nine documented passes; the two levers that
  closed it are general and also closed scope AC's `PutOne3DBlokeOnRide`:
  - **An alias pointer defeats VC6's commutative-operand canonicalisation.**
    VC6 canonicalises a commutative `fmul` (and an integer `imul`) whose two
    operands are constant offsets off ONE pointer, which silently reverses
    the emitted `fld`/`fmul` order; it cannot order operands reached through
    two different pointers, so source order survives. `kg = kf;` and reading
    the affected operands through `kg` is free and fixes it. **Source operand
    order is inert** — all 64 permutations measured, twice, on two bases — so
    when an fld/fmul or imul pair comes out reversed, change the operand's
    SPELLING, not its position. A `volatile` alias, a `char*` cast on the one
    offending offset, and an alias for a whole table all work equally.
  - **A scope-V cancelled-pair anchor receives an allocator priority bump, so
    it must be a value whose own ranking does not matter.** The anchor is
    live wherever the cancel is. Anchoring on the value you are trying to
    place ranks it above its neighbour and costs a register swap; anchoring
    on a nearby variable rotates the callee-saved trio instead. Use a
    link-time **address constant** (`t.oy = (int)g_copter_ord_a;`) or an
    already-materialised induction value (`t.oy = j * 4;` when ecx holds
    j*4) — both bump nothing. The constant must be assigned to the struct
    MEMBER first; used directly in the cancel it folds in the front end.
    The two levers are independent: a 2x3 grid of anchors x operand fixes all
    reach 0.
  - **`/FAcs /Fa<path>` is the tool for a residual that is pure register
    ranking.** It prints VC6's own frame symbol table, so a carrier that
    cost a frame slot shows up as a `_name$ = -N` equate, and it attributes
    every instruction to its source line. It also shows that **VC6 overlaps
    locals onto dead PARAMETER slots** (`_value$ = 8` sharing `_person$ = 8`
    in `bnvpath.c`), which is why the original's inline-asm operands live in
    dead parameter homes — pointer puns and plain `float` locals are
    equivalent there.
  - **A struct member that must live ACROSS a loop gets a real frame slot and
    is reloaded; one confined to straight-line code is free.** Keep a
    cancelled-pair carrier inside one basic block.
  - **Falsified, do not chase:** "the original was compiled as if the
    fld/fmul/fistp asm block wrote EAX". `ApplyObjectOrientationToPerson`
    (0x00484950, bnvpath.c, 0 mismatches) keeps `person` in EAX across NINE
    consecutive such blocks. Adding an EAX clobber only perturbs a body that
    is already on a knife edge. Compiler build (all four VC6 service packs
    emit byte-identical code), the C++ front end, and every codegen flag are
    likewise ruled out for this class.

- **SCOPE V (closed 2026-09-08, 62 of 62 exact; evidence in
  `docs/lanes/scope-v.md`).** Script-event tick handlers + goal checks
  (`eventtick.c`, `eventgoal.c`). `EventTick_Clear` took nine documented
  passes and two levers:
  - **Sibling-copy forwarding kill:** a `rep movsd` into one member of a
    local aggregate stops store-to-load forwarding for every other member
    (a plain narrowing read reloads: `mov dl, [esp+N]`). Hole members
    (`int pad0, top, pad1, bottom`) reproduce a Rect's two never-stored
    slots without memory-homing the two that are; write member sums as one
    statement each (`L.top = f.top + by`), never `=` then `+=`.
  - **Cancelled-pointer copy web:** `t.x = bx; t.x += (int)d; t.x -= (int)d;`
    through a struct member keeps `t.x` a separate web from `bx` (isel drops
    the add/sub), so a byte store from `t.x` costs `mov ecx, ebp` instead of
    recolouring `bx` into edx. Every identity expression written on `bx`
    itself folds and moves x.
  - Allocator rule measured on the way: webs are coloured by weighted use
    count (a register byte use adds ~2), ties to the first-defined; a web
    with a byte need displaces `next` from ebx before it accepts a fix-up
    copy. A `volatile` load is pinned at its statement; the store it feeds
    sinks into the address-sorted free-store group.

- **SCOPE AG (closed 2026-09-08, 3 of 3 exact; evidence in
  `docs/lanes/scope-ag.md`).** Certificate print path + WinMain SEH shell
  (`certificate.c`, `winmain.c`):
  - **Gate: a straight-line `__try` body's only path to its filter is the
    SEH scope table.** `true_extent` now reads it (section above); a
    future SEH body needs nothing special. The filter call belongs in the
    `__except (...)` expression, not the handler: `__except
    (WriteExceptionReport(_exception_info(), "main thread")) {}` puts the
    call before the filter's `ret` and leaves the handler as
    `mov esp,[ebp-0x18]`; `__except (1) { report(); }` swaps them.
    `int r = -1` is the `or esi,-1 / mov [ebp-0x1c],esi` pair.
  - **SaveScreenshotBmp (620 insns):** `StretchDIBits`'s destination
    width must stay an unnamed argument written as
    `pageW - (pageW/8) - (pageW/8)` — a named `destW` reused at `TextOut`
    back-propagates into the call and swaps the sub destination (97.7%);
    `2*(pageW/8)` finishes destW before `mov edx,[pBmi]` (94.6%).
    `TextOut` X is `pageW / 2`, Y is `pageH * 678 / pBmi->biHeight`
    (`idiv [pBmi+8]`, keeps pBmi live past `font1`).
  - Field-by-field `BITMAPINFOHEADER` stores (not `*dst = src`, which is
    `rep movsd`); the palette shift count from the stack header
    (`[esp+0x46]`), not the dest; DOCINFO zeroes `lpszOutput` /
    `lpszDatatype` / `fwType` before `lpszDocName` so the name store sinks
    past the `StartDocA` pushes.
  - **A direct `call` to an import thunk (`call 0x49e442` -> `jmp [IAT]`,
    5 bytes) is a plain `extern` declaration; `__declspec(dllimport)`
    emits `call dword ptr [IAT]` (6 bytes).** `EnumPrintersA` is the
    thunk form; every other import in the body is `call [IAT]`.
  - Frame: `{returned, pBits, needed, memdc}` as one 16-byte addressed
    object packs the 0x28..0x34 run and needs `char printers[0xA80]` for
    the 0xb88 frame (cbBuf is 0x540; the tail is unused high space).
  - `returned <= 0` on an `unsigned long` is `jbe`; `== 0` is `je`.

- **SCOPE AI (closed 2026-09-07, 18 of 18 exact; evidence in
  `docs/lanes/scope-ai.md`).** MIDI + path-square / class companions
  (`music2.c`, `pathobj2.c`):
  - **LoadObjectClassSibling:** `tok = buf` before `strcpy` so `tok` lives
    across `rep movs` in ebp; with four callee-saved occupied, literal `4`
    stays immediate (not hoisted to ebx). Unsigned index over the group
    table for `jb` cursor.
  - **FindPathSquareRoute:** top-tested `while (count != 0)` with
    `count = 1` — VC6 folds/bottom-tests and shares the `return 0`
    epilogue (`do`/`while` exiles it).

- **SCOPE AK (closed 2026-09-07, 20 of 20 exact; evidence in
  `docs/lanes/scope-ak.md`).** Narration / sample / LoadStrings
  (`narration2.c`):
  - **LoadStrings / NarrRingA_Available:** equal-weight register webs —
    declare the winning locals (`buf`/`size`/`pos`, or `cur` before
    `avail`) *before* the arrays so edi/slot ranking matches. Frame-slot
    declaration order stays irrelevant; this is a web tie-break.
  - **LoadStrings quotes:** `if (c == '"') c = '"'; else quotes++;` —
    dead re-assignment folds away but keeps the `c = 0` arm inline.

- **SCOPE AJ (closed 2026-09-07, 27 of 27 exact; evidence in
  `docs/lanes/scope-aj.md`).** RES volume + frontend save/UI
  (`resaudio2.c`, `frontend2.c`):
  - **ConvertWAVToPCM:** `#pragma pack(2)` so 18-byte `WAVEFORMATEX` copies
    as 4 dwords + word (not 5-dword `rep movsd`); never `acmStreamClose`
    (original bug).
  - **PrintTextGetEnd:** `(rc.left + rc.right)` load order.
  - **RES_LoadDirectory:** `e->dir = d; e->vol = v;` adjacent-store order;
    rewrite image offsets to pointers in place.

- **SCOPE AH (closed 2026-09-07, 12 of 12 exact; evidence in
  `docs/lanes/scope-ah.md`).** Pop-up / help / icon-bar helpers
  (`popupmisc.c`):
  - **RenderIconsSkipGroup:** `memset(&ctx.owner, 0, 8)` under
    `#pragma intrinsic(memset)` (not two `= 0` stores) for zero-reg +
    downstream allocation.
  - **SelectNextBuildObject:** biased `const char**` cursor at `.next`
    (`e[-1]`/`e[0]`, `e += 2`); fail-arm store order
    `g_edit_changed` / `g_release_swallow` / `g_input.flags &= ~0x1400`.
  - **ShowCursorErrorMessage:** `int` wrapper returning `ShowMessage(...)`
    (`void` emits `pop ecx` not `add esp,4`).

- **SCOPES AD / AE / AA (merged 2026-09-07; AD later closed 9/9).** Evidence
  in `docs/lanes/scope-ad.md`, `scope-ae.md`, `scope-aa.md`.
  - **AD Relock:** `dwSize = 0x6c` before `IntersectRect`; write
    `rect.bottom = s->h - 1` before the status push so edx/eax/ecx rank.
  - **AD RLE recolour/highlight:** naked asm twins of AB (mask & / >>1);
    no `rep movsw` on literal runs.
  - **AD ShowCapacityOverlay:** C cannot jointly emit scale-in-eax and
    dest-lea-early; close `__declspec(naked)` like `BltAdvisor`.
  - **AE (closed 7/7):** `act = b->action; switch (b->action)` (goldrush);
    entrance tile as two ints; Cell stride 0x14; for-latch GetNext +
    `reserved = f & 1`. CafeBrolly SuggestNextMove arms: no named
    `world`/`dest`/`out` pointers — mirror `Garderner_Repair`
    (`SuggestNextMove(&b->world, &b->dest, &leg); target = leg;
    CalcMoveLine(b->world, leg, path)`).
  - **AA (closed 14/14):** AppraisalDueTick nested guards; FormatBlokeMessage
    indexed for with signed `jl`; JoinSeatList intrinsic memset +
    `unsigned short flags`. LeavePark: `unsigned char lim`, plain
    `(f64&1)?6:0xa`, case 11/12 CML arms end in `break` not `return`.
    PickRide: plain f64 value ternaries; duplicated case-10 mood arms with
    `break`; `default: if (b->action >= 0) goto done;` late-folds so `ja`
    shares the epilogue.

- **SCOPE AF (closed 2026-09-07, 8 of 8 exact; evidence in
  `docs/lanes/scope-af.md`).** Bubble-help / text-cache helpers
  (`bubblecache.c`):
  - **Volatile cache count** in lookup latches — same free volatile as
    `FindCachedText` / `ExpireCachedText`.
  - **RasterizeText:** `volatile` count so the increment is a second load;
    `int f = *(volatile int*)&format` so format wins eax before strlen's
    `xor eax`; keep `HeapAlloc_w(strlen(text)+1)` as one expression.
  - **DrawCachedTextSprite:** `p = s` first; `rc.left = 0` then
    `memset(&rc.top, 0, 12)` splits the four-zero edi web so `push edi`
    stays below the find.
  - **FootprintClearanceTest:** pass/return `Pos` by value (not
    address-taken `elem`) so the missing `push ecx` / frame slot lands —
    beats the FR02 dead-arg floor.

- **SCOPE AC (merged 2026-09-07, 14 of 15 exact; evidence in
  `docs/lanes/scope-ac.md`).** Advisor movie helpers and InitMan texture
  callees (`advisor.c`, `mantex.c`):
  - **OpenMovie without audio: omit `audio = 0`.** `LoadAdvisorMovie`
    matches only when `video = 0` alone shares ebx as the zero/video
    carrier; writing `audio = 0` and/or clip+0x18 costs the −2B / residual
    seen before the close. Leave frames/fps/w/h uninitialized.
  - **Float cam subtract via a small float frame.** `float frame[3];
    frame[0]=pos->x; frame[1]=pos->y; frame[0]-=320.f; frame[1]-=270.f;`
    yields `sub esp,0xc` and the fld/mov-ybits/fsub schedule in
    `RiderTrackToScreen`.
  - **ReadAltLine CR/LF:** the stop path must still test `c != '\\r'`
    before consuming LF so a maxlen hit on CR eats the following LF.
  - **FindAltNameIndex:** `while (NameCompare != 0)` with `strlen==0`
    (not `list[0]==0`) for the `repne scasb` empty test.
  - **`PutOne3DBlokeOnRide` closed 2026-09-09** with the two Codex-F levers
    (see the Codex-F entry): an address-constant cancelled-pair anchor
    (`t.y = (int)g_ride_mtx_chan;`) plus an `sb` alias for the sign table.
  - **Floor left WIP:** `LoadAltTextures` fail-path ebx / scalar-home
    permutation (−4B, ~78 mism). Do not reopen without new register
    evidence.

- **SCOPE AB (merged 2026-09-07, 8 of 8 exact; evidence in
  `docs/lanes/scope-ab.md`).** The eight SoftBlitRLEPlain specialised
  painters (`rlepaint.c`):
  - **Hand-written assembly, not C.** The rotating 2-bit mask idiom
    (`ebx=3`, `rol ebx,2`, wrap via `and ebx,1` + `lea edx,[edx+ebx*4]`),
    mid-stream callee-saved pushes, and `rep movsw`/`rep stosw` clip splits
    do not lower from any C spelling. Same `__declspec(naked)` + `__asm`
    pattern as `tri3d.c` / `coastermath.c`. Keep `NAKED` off the signature
    line so the `// FUNCTION:` marker stays immediately above the name.
  - **Family transfer.** `RLEPaintFast` is the unclipped core; ClipR adds a
    width budget and right-edge run splits; ClipL adds left skip then paints;
    ClipLR combines both; Hit twins OR `g_blit_hit` before each opaque emit.
    Diff adjacent leaves — do not re-derive each clip/hit delta.
  - **Type-3 leaf A/B/C roles.** A = u16 pixels, B = u8 lengths, C = packed
    2-bit controls (softblit2.c's header names for these leaves are wrong;
    do not "fix" the caller's comments from this scope).
  - **Original defect retained.** HitL/HitR/Hit mishandle primary code 1 in
    the top-skip loop (fall through after the `0xAAAAAAAA` test); HitLR and
    all four no-hit leaves jump correctly. Shipped 16-bpp assets have no
    primary code 1 (dormant).

- **SCOPES Y AND Z (merged 2026-09-07, 76 of 76 exact; evidence in
  `docs/lanes/scope-y.md` and `docs/lanes/scope-z.md`).**
  - **A by-value aggregate protects dead parameter homes.** Z's
    `TurnIfBlocked(Bloke*, Pos)` keeps the random direction byte in the dead
    bloke parameter slot; two scalar coordinates put it in the third
    parameter slot (six offset mismatches, 97 instructions). This transfers
    to Y's `DrawAppraisalBar(AppraisalBox, int, int, int)`: four scalar box
    coordinates let the shared top+2 temporary reuse the dead right slot,
    eliminating a four-byte local frame and two instructions. The aggregate
    restores the local frame and all 98 instructions / 246 bytes. Its sign
    flag must also be assigned in both branches, with the green comparison
    arm first, to retain the original zero placement and red-arm `jl`.
  - **World-coordinate copies must be one aggregate assignment.** Z's
    `b->world = next` places the animation argument push between the two
    stores; separate x/y assignments leave two strict differences. The
    same change closes all ten movement handlers.
  - **Inline helper argument order sets the shift schedule.** Z's
    `CellAt(int y, int x)` retains both coordinate shifts before the bounds
    test, closing `BeginTileWait` (54i) and `TryTileWait` (79i). The opposite
    parameter order emits a `js` and leaves each body one instruction short.
    Reading old y before old x closes the final six register differences
    in `NotifyTileTransition` (107i).
  - **Adjacent switch cases preserve the original signed range test.**
    Cases 1 and 2 sharing the resume block in `LowAI_WaitForTile` produce
    a dword mask and signed comparisons, with the byte local spilled in a
    dead parameter slot. Equivalent `if` tests narrow to AL and lose the
    spill. The switch matches 123 instructions / 321 bytes.
  - **Comparison operand order remains observable.** Z's
    `radius * radius >= delta.x * delta.x + delta.y * delta.y` gives the
    original `cmp ecx,edx / setge`; reversing the equivalent inequality
    emits `cmp edx,ecx / setle` (two differences in `BlokeNearTarget`).
  - **Report flags precede slot values in source, despite their emitted
    store order.** Y's `flags |= BIT` before the value assignment keeps the
    full-width OR and interleaved flags load/store. Written last, it narrows
    the OR and scores 11/14 on the first zone setter. The correct shape
    transfers to all 25 report setters. One 3x5 sprite array similarly
    preserves the loader's middle-row anchoring at offsets -0x14/+0x14.
  - **Put the nonempty result before the empty-case return.** In Y's
    `PercentObjectsLinked`, `if (total != 0) return linked * 100 / total;`
    followed by `return 100;` matches 74/74. The early empty-case return
    relocates the epilogue and scored 67/74 in the interrupted session.
  - Declaration reconciliation: Y uses main's `UnreferenceSprite` for
    `0x00497bd0`; the former `KillSprite` name describes a different
    operation. Z retains `(Pos, unsigned char)` for `GetTileInDir` and a
    short speed for `NavigMoveLine`; these caller-side types are measured
    codegen choices, not a reason to change other files' declarations.

- **FROM THE PARALLEL SESSION `scope-r` (48 of 48 exact — the level-database
  reader `ParseKeywordSections`, the parse primitives and the first keyword
  handlers, `levelkw.c`; evidence in `docs/lanes/scope-r.md`).** Folded:
  - **A set-and-break found flag survives only with a DEFINITION of it at
    the merge point** (`ParseKeywordSections`, 194/194 after three passes
    63% -> 99% -> 100%). VC6 SP3 folds a flag that is set right before a
    `break` and tested after the loop: it threads every edge into the test
    with the flag's constant value, exiles the match block, and re-allocates
    everything after with the freed registers. It threads only when the
    test is the FIRST statement of the merge block; any definition there
    whose value the propagator cannot reduce to a constant on either edge
    stops it. The exact spelling is two flags, both zeroed before the loop,
    the loop setting one, the merge or-ing them: `found = 0; handled = 0;
    for (i = 0; i < count; i++) { if (strcmp(words[0], table[i].keyword) ==
    0) { found = 1; rc = table[i].handler(words, nwords - 1, extra); if (rc
    == 0) skipped++; break; } } handled = found | handled; if (!handled &&
    fallback) rc = fallback(…);` — both live in `bl`, the or of two
    register variables is not folded, the allocator coalesces them, and the
    original's `test bl,bl / jne` follows, with `table` cached in `ebp` and
    the strength-reduced cursor coalesced onto it. A self-modifying
    `handled ^= 1` (99%) and a cursor reset `e = table` at the merge (96%)
    show the same mechanism but leave their own instruction. The only kept
    flag of its kind in the executable.
  - **A five-element store loop is an index loop, not a pointer loop**:
    `for (i = 0; i < 5; i++) g_rate_t0[i] = atoi(args[i + 1])` gives the
    strength-reduced pointer with a SIGNED `jl` against the end address;
    the pointer form compiles to `jb` with a `lea/sub` base (23/34 ->
    34/34).
  - **Single-return `if/else` beats two returns whenever the arms differ
    only in the callee** — it hoists the shared push above the compare and
    sinks the saved-register push (`PROMPT` 41/49 -> 49/49 with argc in
    esi; `WORKERS` `if (level == 1) {…} else AddEvent_Workers(a, b); return
    1;` sinks `push esi` past the early returns, while `return 1` inside
    the arm pushed esi in the prologue, 44/55). `BREIFINGFILE`/`HINTSFILE`
    need `if (!g_level_db_active) return 1;` with `name = kEmpty` as the
    declaration's initialiser (35/42 -> 42/42; a wrapper `if`, a ternary or
    an `if/else` assignment all land elsewhere).
  - **`LOOKAT`: copy `pos.x/pos.y` into locals AFTER the shifts are stored,
    with `y` declared before `x`** — the shifted values stay in edx/ecx for
    the store and are copied to edi/esi only inside the arm (`push edi`
    sinks there), and `w`/`h` land in the dead `args`/`argc` parameter
    slots as in the original. Temporaries first 62/77; `x` declared first
    swaps edi/esi (72/77).
  - **`ReadLine` is `while (1)` with separate `if (c == '\r') break; if (c
    == '\n') break;` and `if (++n >= max) break;`** — not rotated (one
    `RES_ReadFile` call, `jge exit; jmp top`), the two breaks threaded into
    the post-loop `if (c == '\r')` (46/66 -> 58/58); `for (;;)` with `||`
    and `n++; if (n >= max)` rotates into two calls.
  - `SplitWords`' empty-word case is `continue`, not `break`, and the loop
    is `while (*s)` with the initial test peeled. Direct byte reads of
    `g_level_flags` at the call give `mov dl,[0x669050]` right before the
    pushes; a plain `elem = 0` reuses `_stricmp`'s zero register by itself.
    `GARDENER`/`MECHANIC`'s count loop is `while (i != 0) { …; i--; }` on a
    copy of n, with the default `if (argc < 3 || (n = atoi(args[3])) < 1)
    n = 1;` — one shared `mov eax,1` for both fall-throughs.
  - Extern divergences: `NewScriptEvent(void*, void*, void*)` here (PROMPT
    passes `(char*, char* or 0, (void*)1)`) against eventmake.c's
    definition; `GenerateGardener`/`GenerateMechanic` return `void*` here
    (workers.c: `void`); `ParseKeywordSections(void* f, KeywordEntry*
    table, int count, int extra)` (movie3.c declares the table `const
    void*`); `strspn`/`strcspn` return `unsigned int`; `strchr` is a real
    call — `#pragma intrinsic` only for `strlen`, `strcpy`, `strcmp`. At
    merge the three primitive DEFINITIONS were renamed to the tree's
    `KwLineApplies` / `KwSectionMatches` / `KwHasArgs` (R's notes:
    `LineApplies`, `LevelMaskMatches`, `HasArgs`); code unchanged.

- **FROM THE PARALLEL SESSION `scope-x` (54 of 54 exact — event ticks part
  2 and the goal primitives, `eventtick2.c` and `eventgoalprim.c`; evidence
  in `docs/lanes/scope-x.md`).** Folded:
  - **One local definition pointer resolves scratch-register allocation**:
    naming the `ObjDef*` before the callback test (LOOPCOMPOSITE 26/28 ->
    28/28) or inside the condition arm (FIXRIDES 57/61 -> 61/61); naming
    the element or the callback alone, or a volatile name load, did
    nothing.
  - **The scope of a position aggregate controls store scheduling**:
    RIDEVISITORS' local `Pos` inside the inner arm delayed both stores
    (64/69); the same `Pos` declared once at function scope interleaves
    x-store / y-load exactly (69/69). Volatile x/y reads cost four bytes
    and seven mismatches.
  - **A free volatile row-pointer read controls the loop-invariant choice**:
    CLEARAREA hoisted the row table and reloaded the map config (66
    mismatches, 202B vs 197B); a single `*(Cell** volatile*)&g_map_rows`
    at the valid-cell lookup keeps the config in edx and reloads the row
    table in the original arm (81/81, 197B). Confined to that load; the
    global itself stays ordinary.
  - **Keep the timer result as the accumulation destination**: `deadline =
    GetGameTimer(); deadline += minutes * 60000;` gives the original eax
    accumulation where the one-expression form rotates the time into ecx;
    `deadline = 0` in the else arm, not before the guard, gives the
    separate zero-return block (17/17, 54B).
  - Caller-side types: `SetThemeIcon` and `AddLevelFlag` take a SIGNED BYTE
    second parameter here (the keyword callers declare `int`);
    `GetLevelFlag` returns a signed byte (CHECKFLAG's `movsx ecx,al`);
    `GetRideVisitCountAt` returns `unsigned short` (the caller masks eax to
    0xffff, the body writes ax); the +0xc0 loop-size callback is cdecl
    `(Elem*, int) -> int`. New names: `g_goal_kind_count` 0x0066872c,
    `g_appraisal_minutes` 0x00832978, `SetButtonFlash` 0x00476030 (a
    bounds-checked write to the nine flash states at 0x007fdd00),
    `SetThemeIconEnabled` 0x00476140.

- **FROM THE PARALLEL SESSION `scope-w` (72 of 72 exact — the script-event
  constructors `AddEvent_*` 0x0046b590..0x0046c510 with `NewScriptEvent`,
  `LinkStepEvent`/`LinkGoalEvent`, `SetScriptStepText` and `ShowStepHint`;
  69 first-try; evidence in `docs/lanes/scope-w.md`).** Folded:
  - **A 16-byte field copy is ONE aggregate assignment, `e->area = *r`**:
    VC6 emits `lea ecx,[eax+0x28]` then four `mov esi,[edx+k] / mov
    [ecx+k],esi` pairs with the fourth's load hoisted above the flags
    store, exactly the original; the 8-byte `e->pos = *pos` is two
    register moves. Four scalar stores give four different temporaries.
  - **Emitted store order is the source order EXCEPT across an aggregate
    copy** (`AddEvent_Needin`): written `f1c = count; elem = elem; area =
    *r;` VC6 emitted elem first (4 mismatches); written `elem = elem; f1c
    = count; area = *r;` it emits f1c first, as the original. The other 70
    constructors kept their source order — the flags store included, which
    is why `AddEvent_Take`/`Addbricks`/`Currency`/`Entrancefee` write
    `flags = 0` BEFORE the argument store and `Give` between its two.
  - **Nest the body under the guard for an exiled `return 0`**
    (`ShowStepHint`, BL07's PlayMovie shape): `if (!g_script_cur) return
    0;` keeps `xor eax,eax / ret` inline behind the test (19 mismatches);
    `if (g_script_cur) { … return 1; … return 1; } return 0;` puts it
    last. Same family as U's `GetFreePlayItemInfo` and S's `SELECT*`.
  - **Read the parameter order off the frame** (`SetScriptStepText`): the
    step is `[esp+0xc]` after one push and the text `[esp+0x10]` after
    three, so the text is the FIRST argument; the brief's caller-side
    guess had them swapped.
  - `if (step->events) { e->next = …; step->events = e; } else
    step->events = e;` with the store in both arms is exact for
    `LinkStepEvent` (two stores through different registers);
    `LinkGoalEvent` has the single shared store after the `if` — spell
    what the original's block count says.
  - `for (step = head; step; step = step->next) { if (…) break; prev =
    step; }` gives the rotated walk with the null head threaded straight
    to the "no predecessor" arm and `push esi` sunk past it.
  - Extern divergences: the goal constructors take `unsigned char flags`
    first (`mov cl, byte ptr [esp+0xc]`), as levelkw2.c declares; the
    rect/pos constructors take `Rect*` / `Pos*` here where levelkw3.c
    passes `int*` (either is fine on the caller's side; the callee's copy
    needs the aggregate); `g_script_root` is `ScriptEvent*` (movie3.c:
    `void*`). Nothing else: no volatile, no register spelling.

- **FROM THE PARALLEL SESSION `scope-u` (9 of 9 exact — the exception-report
  writer, the first `__try/__except` bodies in the tree, and the free-play
  object-description loader; evidence in `docs/lanes/scope-u.md`).** Folded:
  - **An EBP/SEH frame lays its locals out by VC6's symbol-hash bucket, so
    the local NAMES decide the frame — in `/O2` bodies with `__try`, not
    only under `#pragma optimize("", off)` (LEVERS FR10's regime
    generalises).** Every local, stored or not, gets a home; bucket 0 is
    assigned first from ebp down, the same bucket most-recently-declared
    first, block scopes after the enclosing scope. Single letters hash to
    `c mod 16`, two and three letters to `(4*c[n-2] + c[n-1] + 6) mod 16`,
    and from four letters the first character re-enters (`aaaa` 15, `baaa`
    0) — probe with `/FAs` rather than compute. `WriteExceptionReport`
    needed 23 names in non-decreasing buckets with ties declared in reverse
    slot order (58 -> 4 -> 0, frame 0xb40 -> 0xb44); `ReportModuleDetails`
    15 -> 3 -> 0 by `nt` -> `pe` (bucket 2 -> 12, below `h`) and adding the
    original's block-scoped `IMAGE_DOS_HEADER* dos = base`. Declaration
    order still carries the initialiser order; both constraints were
    satisfiable at once.
  - **A named `pagesize` local decides which candidate loses its register**
    (`ReportModuleLine` 41 -> 0): with `si.dwPageSize` read at each use VC6
    gave ebp to the parameter and reloaded the page size; the original keeps
    the page size in edi for its three loop uses and re-reads the parameter
    at its one call.
  - **Nest the body under `if (f)` and the tail under `if (name[0])`** so
    both failures fall into ONE trailing `return 0` (`GetFreePlayItemInfo`
    280 -> 137 -> 0); leading `return 0` guards gave three inline epilogues.
    The `d == 0` guard keeps its own inline `xor eax,eax` before the pops.
  - `code == tab[i].code` gives `cmp edx,[ecx]`; the reverse gives
    `cmp [ecx],edx`. `rw = "Read from"; if (info[0]) rw = "Write to";` is
    `mov eax,A / test / je / mov eax,B`; the ternary tests first and loads
    after. `char date[100] = ""` inside the `if` block runs its fill after
    the guard, where the original has it.
  - `__asm mov eax, dword ptr fs:[4]` / `__asm mov stackend, eax` is the
    only way to the original's `mov eax, fs:[4]`; `NtCurrentTeb()` reads
    `fs:[0x18]`.
  - **Gate: an SEH frame's `fs:[0]` is a relocation against
    `__except_list`, an UNDEFINED external (section 0) that the CRT library
    defines as the absolute 0.** `tools/match.py` now resolves it to 0
    (`KNOWN_ABSOLUTE`) instead of applying the sentinel, which had scored
    every SEH prologue/epilogue as `fs:[<abs>]` against the original's
    `fs:[0]`; the two `__try` bodies audit `[OK]`. Any future absolute CRT
    symbol goes into that table.
  - Extern divergences: `LLIDB_FindElement(const char*, FPElem**, unsigned
    int*)` with a local `FPElem` whose +0x08 is a BYTE (`mov dl,[eax+8] /
    test dl,0x10`; legoland.h's LLElem has a dword there);
    `FormatFileTime(char*, FILETIME)` by value (both halves pushed).

- **FROM THE PARALLEL SESSION `scope-s` (37 of 37 exact — the second tier of
  level-database keyword handlers, REMOVE .. ENDSCREENS; evidence in
  `docs/lanes/scope-s.md`).** Folded:
  - **A shared `-1` test over a ternary is how VC6 spells "else jump
    straight into the call" (SELECTTHEME, SELECTTAB).** Exact: `if
    (KwLineApplies(...)) { i = argc ? Lookup(...) : 0; if (i != -1) { call;
    return 1; } } return 0;` — one textual `return 0` collects both failures
    into the last block and VC6 jump-threads the constant arm past the `!=
    -1` test into the call, which IS the exiled `xor eax,eax / jmp`. `if
    (argc) { i = Lookup(); if (i == -1) return 0; } else i = 0; call; return
    1;` keeps two return-0 sites (30/42); `goto fail` 25/39; a second
    `return 0` inside the arm 37/43; two separate call sites with a constant
    argument 42/51 — VC6 does not merge calls whose argument differs in
    constant-ness.
  - **A parameter re-read from its stack home while its register copy is
    live needs the volatile slot read** (SELECTMODE's else arm is `mov eax,
    [esp+0x10]`, the `argc` slot): `: *(volatile int*)&argc` reproduces it;
    `: argc`, a cast, a pre-call `int n = argc` copy, or routing the early
    uses through the copy all let VC6 thread the known zero into `xor
    eax,eax / jmp` (42/44). First use of the "read the stack PARAMETER
    itself" lever on an `int`.
  - `if (!KwLineApplies(...)) return 0;` emits a bare `ret` (eax is the
    call's zero) while it is the only `return 0`; add a second and both
    become `xor` blocks.
  - **Push sinking decides whether the early `return 1` shares the tail,
    and no source change is needed either way**: the plain `if
    (!g_level_db_active) return 1;` gave REMOVERANGE/COMPOSITE (ebp/edi
    pushed after the test) and the SELECT*/GIVE shape their inline `pop /
    mov eax,1 / pop / ret`, and REMOVE, RESEARCH, PARKVISITORS' family and
    FOREVER (tail is a join or has no pushes) the merged form.
  - `int popup = 1;` at declaration (GIVE) puts `mov ebp,1` above the guard
    and lets the early `return 1` come out as `mov eax,ebp`. Locals for call
    results keep `add esp` merged: `n = atoi(argv[1]); AddEvent_X(flags,
    n);` -> one `add esp,0xc` (the 27-instruction family, HAPPINESS 0x14,
    STUDAREA 0x1c).
  - `char buf[0x200] = "";` is the exact spelling of the 1-byte copy + `rep
    stosd / stosw / stosb` fill; `strcat(buf, ";")` then `strcat(buf,
    argv[3])` are the two `repne scasb … rep movsd/movsb` sequences.
  - Extern divergences: the goal constructors take `unsigned char flags`
    first (the callers `mov al,[0x669050] / push eax`; an `int` parameter
    would zero-extend); `SetLevelGoalState(int, const char*)` here vs
    movie3.c's `(int, int)`; `atoi` 0x004a04b9 declared `extern int
    atoi(const char*)` (savegame.c reaches it via `<stdlib.h>`). At merge
    the file's three primitive names were aligned to T's `KwLineApplies` /
    `KwHasArgs` / `NameCompare` — extern renames, code unchanged.

- **FROM THE PARALLEL SESSION `scope-t` (29 of 29 exact — the last
  twenty-two level-database keyword handlers and the process start-up;
  evidence in `docs/lanes/scope-t.md`).** Folded:
  - **A `|=` of a small constant on an `unsigned short` field is narrowed
    to `or byte ptr [base+index+disp], imm8` and leaves a dead `lea` of
    the field's address behind.** `LevelKw_GLUE`'s `g_map_rows[y][x].flags
    |= 0x40` with `unsigned short flags`: 63/63. With `unsigned char` the
    same statement splits into load / `or bl,0x40` / store through a
    `lea`'d pointer (58/67); through a `Cell*` it is `mov bl,0x40 / or
    [mem],bl` plus a `push ebx` (58/66); bit-fields behave like the byte.
    Template: pathgfx.c's exact `AddPathTileGFX` carries the identical
    `or byte ptr [ecx+edx*4+0xc],0x10 / lea eax,[ecx+edx*4+0xc]` pair.
    **Read the field width from the neighbours, not from the `or`.**
  - **An index loop `for (i = 1; i <= argc; i++)` over `argv[i]` puts the
    pointer step in the loop PREHEADER, after the guard**: VC6
    strength-reduces it to a countdown (`cmp edi,1 / jl`, `dec edi / jne`)
    plus a pointer IV initialised after the guard. A separate pointer
    (`p = argv + 1; for (n = argc; n >= 1; n--, p++)`) hoists the `add`
    above the guard (63/64); `if (argc >= 1) { p = argv + 1; n = argc; do
    … while (--n); }` places it right but lets a preceding `if (argc == 0)
    bits = -1;` be jump-threaded past the guard (`jmp end` instead of the
    fall-through, 67/68); stepping the parameters themselves re-rolls the
    register roles (55/64). `FLASHBUTTON` 64/64, `FLASHBUTTOFF` 67/67.
  - **`if (!flag) return 1;` as an early return keeps its epilogue inline
    and lets the later pushes sink** (`LevelKw_PLACE` 58 -> 71/71,
    `DEGRADE` 41 -> 54/54); the enclosing `if (flag) { … } return 1;` merges
    the inactive path into the final return and pins the pushes at entry.
    The other twenty handlers, which have no sunk push, want the enclosing
    form — same source family, two spellings, decided by the push.
  - **The fall-through arm is the one written first** (`REPORT`: `if
    (NameCompare(argv[2], "off")) { … } else { off = 1; }` 119 -> 126), and
    **a ternary argument becomes two calls sharing their trailing pushes**
    (`if (NameCompare(a, "HAPPY_VIS") == 0) idx = Lookup("Happpy_Vis", …);
    else idx = Lookup(a, …);` 126 -> 129/129; the ternary materialises the
    pointer in a register first). Third sighting of P's `SetPointer(2)/(1)`
    rule.
  - **Two independent stores, reverse source order, twice more**:
    `MAXBLOKES` `g_visitor_cap = g_map->max_blokes; g_visitor_cap_extra =
    0;` (30/30); `GameMain` `g_hinstance = hinst; g_ncmdshow = ncmdshow;`
    (84/84).
  - **`int i` with an unsigned bound** (`i < sizeof(tbl) / sizeof(tbl[0])`)
    gives a `jb` open loop and a signed `jle` failure loop from one
    variable; the close-on-failure loop is `if (i > 0) { p = tbl; do {
    Close(*p); p++; } while (--i); }` — the increment as its own statement
    after the call (`*p++` lifts the `add` above the push). `InitSession`
    262/262 after those two.
  - `if (!p) return p;` after `malloc` and `r = Check(); if (!r) return
    r;` give the `test / jne / ret` shapes with no `xor` (Q's `return
    (int)c` rule, twice more); the arm that also frees (`if (!q) { free(p);
    return 0; }`) is the one that carries `xor eax,eax`.
  - Uninitialised locals whose only definition is in one arm (`REPORT`'s
    `a`, `b` when the word is `off`) are read from the argc slot at the
    join (`mov ebx,[esp+0x18] / mov ebp,[esp+0x18]`) — FR09 seen from the
    source side: declare them, assign them in the one arm, and let VC6
    home the phantom reads.

- **FROM THE PARALLEL SESSION `scope-p` (10 of 10 exact — the per-frame
  dispatcher, the in-game frame, the 751-instruction map click handler;
  evidence in `docs/lanes/scope-p.md`).** Folded:
  - **A sub-object `memset` keeps a small struct's zeros out of the constant
    web (RC08)**: `InGameFrame`'s blit context {1,0,0} as three stores or as
    `= {1,0,0}` webbed the two zeros with the guard compare in esi and pinned
    `push esi/edi` to the prologue (224/282); `ctx.a = 1; memset(&ctx.b, 0,
    8)` gives `xor eax,eax` + two stores and lets the pushes sink past the AI
    phase. (Scope Q hit the identical thing in `MapScreenFrame` the same
    evening.) Six type variants were inert; only the memset split the web.
  - **Push sinking needs the sunk values in a `static __inline` helper (or an
    inner block) opened after the guard — and only once the zero web is
    gone**; with the web present the helper was inert. `InGameFrame`'s body
    from "ProcessStuff" to the switch lives in `InGameFrameBody(&ctx)`.
  - **Register phase of a guarded block: test the global directly and name
    the copy inside**: `if (g_edit_mode == 0 || g_edit_mode == 2) { int edit
    = g_edit_mode; ... }` (283 -> 293 -> 299/299); a copy before the test,
    x/y locals or a `Pos` copy were 1–2 registers out of phase.
  - **A cached global is not a local**: `HandleMapClick`'s edi is
    `g_hit_type`'s forwarded value (`mov edi,K / mov [g_hit_type],edi`, then
    reloads after calls); a local `hit` constant-propagates (`mov [mem],K`)
    and lands in ecx. Read the global at every use (also `g_sel_def`,
    `g_input.map.x/y`, `g_edit_object` in the loops, `g_icon_value` in the
    work-order arms).
  - **Identical `if/else if` arms are cross-jumped at IR level unless they
    differ; name the pointer differently in each** (`d`, `d2`, `d3`): the
    original keeps three copies of the slot-+0x94 call with the scratch
    rotation advancing across them; one shared arm was 3 instructions short.
  - **A second name for the same address defeats a CSE the original does
    not have.** The leading test reads `g_edit.mode` (the aggregate) while
    the off-map compare reads `g_edit_mode`; with ONE name the top load
    survives `g_sel_def = 0` and is re-materialised, misaligning 600
    positions; the loops' bounds read `g_level_map` while the classification
    reads `g_map` (0x004bcbf4), or the post-call reloads come out in
    temporary-creation order (4 strict, 0 rb). The tree already carried both
    names for both addresses. **So two names for one address can be a
    LEVER, not only hygiene** — record such pairs, do not "fix" them blindly.
  - **`g_hit_info` as one 12-byte aggregate {type, obj, cell}** for the
    by-value pop-up call: the copy takes its first dword from the cached
    register and loads the rest in field order; three globals plus a local
    rotate the scratch registers one step. A `Pos` copy of the cursor point
    sets the rotation for that record copy (12 -> 4).
  - **VC6 moves an 8-byte struct high dword FIRST**: `g_query_cursor.origin
    = g_input.map` is the only way to get the y-before-x order the original
    has there — invisible to the normalised gate (`relocs.py` flagged the
    swapped identities at i=706..710 after `audit.py` said `[OK]`); the
    other arm assigns x then y as scalars and is exact that way.
  - `if (CursorIsValid(&c)) SetPointer(2); else SetPointer(1);` — a
    ternary argument compiles to `neg/sbb/neg/inc`; the original branches on
    the pushes and merges the call. **Byte fields stored twice need `int`
    temporaries** (`unsigned char` grows the frame by 8). **Load the
    definition before the two stores** (`g_sel_def = obj->def; g_sel_bpos.w
    = sq`) to get the stores in the original order. The one-web appeared by
    itself once the `int` temporaries and direct global reads were in.
  - `InitScreens(char)` here (a byte load) where the definition says `int`;
    `PopUpInfoSetUp(HitInfo, int, int)` takes the 12-byte record by value;
    the version-resource imports WITHOUT `dllimport` (thunks at
    0x0049e3a0..0x0049e3ac), `SystemParametersInfoA` WITH it.

- **FROM THE PARALLEL SESSION `scope-q` (12 of 17 exact — the session and
  main loop, the map-screen frame, level state, the cursor segment fills;
  evidence in `docs/lanes/scope-q.md`).** Folded:
  - **A one-case `switch` compiles to `sub eax,K / jne`; `if (x == K)` to
    `cmp`.** `KillFrontEndScreenIfActive` 2/4 -> 5/5.
  - **A called copy `(dst, src, n)` followed by `dst[n-1] = 0` is
    `strncpy`.** Written as `memcpy` under `#pragma function(memcpy)` it is
    byte-identical AND passes `relocs.py`, because the address comment was
    the author's own. Scope O declared the same CRT address as `strncpy`
    and the CRT body at 0x004a0110 tests each byte for NUL. **A wrong callee
    NAME with the right address annotation is invisible to the relocation
    gate; cross-file name agreement at the merge is what catches it.**
  - **A `volatile` flag polled by a wait loop keeps its first load below the
    merged argument cleanup**: `RunGame` 120/121 with `int g_music_disabled`
    (the load hoists above `add esp,0x20`), 121/121 with the `volatile`
    musicthread.c already declares. Evidence the original header had it.
  - **Count the pending pushes before reading a frame offset.**
    `MapScreenFrame`'s `lea ecx,[esp+8]` looked like `&ctx.sub` until the
    `push 5` of the preceding `SetPointer(5)` (cleaned up only at the merged
    `add esp,0x18`) was counted: it is `&ctx`, the whole {1, 0, 0} record. A
    probe with distinct constants settles such questions in one compile.
  - **Two zero stores plus three zero pushes across calls form a
    callee-saved zero web (an extra `push esi`); `memset(&ctx.sub, 0, 8)`
    does not.** 63/98 -> 101/102; the memset's zeros go through scratch eax
    and the pushes and a later `clip.left = 0` become immediates.
  - **A three-call tail duplicated into an early arm is kept as a copy**
    (`SetPointer(6); UpdateFocussedIconPtr(); PopRenderingStatus();
    RenderingComplete(); return;` then the same three calls after the if);
    written once, the arm jumps to the shared copy. BL05's two-call
    threshold read from above.
  - **A block-scoped aggregate initialiser stores at the use site with
    immediates**: `ClipRect clip = { 0, 0x20, 0x280, 0x174 };` declared
    inside the branch gives the four immediate stores interleaved with the
    `GetClipping(&saved)` setup (FR06 from the branch side).
  - **`i = n; while (i--)` is the trip-count idiom that keeps `i` and
    derives the counter as `i + 1`**: `mov eax,[h] / inc / mov ecx,eax /
    dec / test ecx / je / inc / mov [h],eax` (`i = h + 1`) and `mov ecx,eax /
    dec eax / test ecx / lea edi,[eax+1]` (`while (h--)` on the parameter).
    `for (i = h; i != 0; i--)` folds the dance to `test eax,eax`; `i >= 0`
    gives a `jl` guard.
  - **VC6 SP3 idiom-recognises a constant-trip fill loop into `rep stosd`**
    (12 `unsigned short` stores of one value: `mov si,ax / shl esi,0x10 /
    mov si,ax / mov ecx,6 / rep stosd`) though it never unrolls; the
    original's 16 and 32 explicit `mov word ptr` stores need explicit
    assignments (or an initialiser).
  - **Nesting shows in the failure edges**: `FindObjDoorTile`'s "no
    entrance" branch jumps to the final `return 1`, so the exit check is
    INSIDE the entrance block; sequential ifs hoist the shared `pos` load
    above the first test.
  - **`if (!c) return (int)c;` gives `test eax,eax / jne / ret` with no
    `xor`** — the tested register is the return value (`DoorTileStep`).
  - **A `Pos` struct copy loads both fields before either use**; separate
    `int` temporaries interleave the loads (79 vs 82/89).
  - **Two `LLElem*` out-pointer locals: the first declared takes the lowest
    slot** (`ResetLevelObjects`, first compile); the nested map clear
    `for (y..) for (x..) g_map_rows[y][x].f0c = 0` with `unsigned short`
    dimensions gives the `jbe` guard, `movzx`-then-`jl` latch, per-pass
    global reloads and the `[edx+ecx-8]` pre-incremented addressing.
  - **Measured negative — a switch default arm the layout will not give.**
    The cursor fills' kind chain (`sub eax,0 / je / dec / je / dec / je`)
    falls through, in the original, into a CLONED exit epilogue for the
    default, then case 1 (`jmp`) and case 2 adjacent to the shared loop;
    ours inverts the last test to `jne end` and places case 2 first.
    Seventeen spellings identical (default first/last/absent, `return`/
    `break`/`goto`, empty defaults, dead stores, case order, loops inside or
    outside the switch, `h++; while (h--)`); the shape `dec / je / four pops
    / add esp / ret` occurs in no other function of the binary. Both fills
    (`DrawCursorSegmentB` 94%, `A` 77%) wait on this one block decision.
  - **Measured negative — a latch store sunk below the compare.**
    `TallyBuildFootprints`: the original reloads the counter, loads the
    bound, cleans up, increments, compares, then stores; ours stores before
    the bound load in eight spellings (`++`, `+= 1`, `= x + 1`, while form,
    `int[2]`, reversed compare); a separate register counter costs 50.
- **A COMMUTATIVE SUM OF TWO CALLS IS EVALUATED LATER-DECLARED-CALLEE FIRST,
  so the externs' DECLARATION ORDER decides the call order (scope O,
  2026-09-05).** `PrimeMovieAudio` (0x00476bf0): `end = AVIStreamLength(a) +
  AVIStreamStart(a)` AND `AVIStreamStart(a) + AVIStreamLength(a)` both call
  Start first while Length is declared before Start; declaring Start first
  makes both call Length first, which is the original. The instruction gate
  is blind to it (47/47 either way) — `relocs.py` reported the swapped callee
  targets. Splitting the sum into two statements fixes the order but swaps
  the edi/esi roles of the two locals (42 of 47); a named temporary is worse
  (34). The one known case where the ORDER of extern declarations (not their
  types) is a lever. Thunk identities were confirmed from the import table
  with pefile, not inferred from use.
- **The relocation gate catches reconstruction errors the instruction gate
  passes — run it on every exact body (scope O).** `StartMovieAudio`
  (0x00476910) was 197/197 storing `si.dwLength`; the original stores
  `si.dwSampleSize` (+0x30). Same-sized field, same instruction, wrong
  identity.
- **`switch` on a global keeps a stored constant register-backed through the
  join that an `if` gets jump-threaded across (scope O).** `MovieTicks`
  (0x00476680): after `if (!QPF) g_mode = 1; else { g_mode = 2; ... }`, an
  `if (g_mode != 2)` — on the global or on a local copy — is threaded (the
  stores become immediates, 20 of 30); `switch (g_mode) { case 2: ...;
  default: ... }` keeps `mov eax,1 / mov [g],eax` and `sub eax,2 / je`,
  30/30.
- **A value that must take a callee-saved register without crossing a call
  is assigned BEFORE the call, and VC6 rematerialises the constant after it
  (scope O).** `UpdateMovieAudio` (0x00476d20): `count = 11` after the stop
  call gives count eax and the frame argument esi, with edi's push sunk to
  the loop (324 of 344); `count = 11` anywhere before the `frame % blocks`
  division (which needs eax) gives count esi, frame edi, both pushes at the
  guards (342–343); the exact statement order `pos = ...; restart = 1;
  count = 11; block = frame % blocks; restart_pos = ...` puts the
  rematerialised `mov esi,0xb` where the original has it, 344/344. Using
  the count as the loop's own counter homes it in memory from the start
  (315).
- **A byte scanned in a `do ; while (buf[i++] != '\r' && i < size);` is a
  compiler temporary and stays out of the scratch pool; a named `char c`
  takes ecx and shifts every later register (scope O).** `LoadTextFileLines`
  (0x00490680): named temp 62 of 86 (the file handle moves to edi, `max`
  into a pushed ebx); the empty do-while with the test in the latch lands
  the temp in ebx (the handle's register, restored each iteration as the
  original does), `lines` in ecx, `max` in edx, 86/86; a `while` with the
  same test peels the first compare (60 of 88).
- **A global read at every use plus a local copy taken BEFORE the guard is
  the CSE'd load-then-copy shape `mov eax,[g] / test eax,eax / mov ebp,eax /
  je / lea esi,[eax*4]` (scope O).** `MarkPathSquareReachable`
  (0x004829c0): `n = g; if (n) { malloc(n*4); memcpy(.., n*4) }` fuses the
  webs, `mov ebp,[g] / test ebp,ebp / lea esi,[ebp*4]`, and as a side effect
  loses edi's prologue push (the flag RMW takes esi; the memcpy gets a local
  `push edi / pop edi` bracket): 40 of 51 at the original's byte length. `n
  = g` inside `if (g)` with `size = g * 4`: 51 of 52. `n = g` BEFORE `if (g)`
  with `size = g * 4`: 52/52. Guarding on `n` instead of `g`: back to 40.
- **All-subscript lockstep cursors on one index reproduce a MIXED anchoring
  — one row cursor anchored at +2, the other at +0 with its +1 store taken
  off the first cursor plus a loop-invariant difference (scope O).**
  `BlitDIBToScreen` (0x00465850): `src[x]`, `d0[2*x]`, `d1[2*x]`,
  `d0[2*x+1]`, `d1[2*x+1]` in a `for (x = 0; x < w; x++)` gives the
  original's `[edx]`, `[ecx-2]`, `[ecx+ebp]`, `[ecx]` with the source in
  edi and the column count (a compiler temp) in memory: 66 -> 100 of 113.
  Walking pointers in any of eight store orders spill the source pointer and
  anchor the rows the other way (55–66). Three more steps closed it: the
  DIB's `h`/`w` read BEFORE the surface toggle's global store (a load through
  the pointer cannot rise above it; 100 -> 105), an explicit countdown `cnt
  = y + 1` from the `y = h - 1` that also positions the last DIB row (the
  original's `lea edx,[ebp-1]` kept across the top fill and `inc edx` at the
  loop head; -> 113/113), and a `short` pixel temp (`unsigned short` narrows
  the 565 mask to 0xffe0 against the original's 0xffffffe0; `int` promotes
  the load, 46 of 115).
- **Two smaller ones from scope O.** A struct wrapper `struct { long
  fmtsize; AviStreamInfo si; } f;` pins the address-taken scalar BELOW the
  0x8c aggregate (`StartMovieAudio`, the one displacement of 196 of 197);
  as two locals VC6 put the aggregate at the bottom whatever the declaration
  order. A local copy of a global that is later multiplied keeps it in a
  callee-saved register across the call between (`blocks =
  g_movie_audio_scale; ... Create(fmt, bpb * blocks)` -> `mov ebx,[scale] /
  imul ecx,ebx`; reading the global at the multiply reloads it, 187 of 197).
- **RELOCATION IDENTITY IS NOT CHECKED BY THE GATE; `tools/relocs.py` checks
  it (scope L, merged 2026-09-05).** `verify.py`/`audit.py` normalise every
  absolute operand, so a body that names the wrong same-sized global, the
  wrong callback, or a mis-annotated address passes at ZERO mismatches. Scope
  L's tool reads our object's COFF relocations, resolves each to the address
  its `/* 0x... */` annotation claims, and compares with the original's operand
  at the same instruction index. First sweep of 2,355 exact bodies: 21,097
  relocation positions, 77 strict differences in 20 functions (0.85% of
  bodies), 1,255 unresolved (string/float literals, jump tables, unannotated
  symbols — not mismatches; the full ledger is `docs/lanes/scope-l.md`). Seven
  were real errors and thirteen were operand/statement order; K's `RunMovie`
  (merged after the sweep) added one more. Seventeen were fixed at integration
  in one pass, four wait on files owned by running scopes (HANDOFF §1).
  **Run `$PY tools/relocs.py LEGOLAND/<file>.c` before committing an exact
  body: zero `MISMATCH` lines is the bar (UNRESOLVED is fine).** The gate
  script runs it per file now. What the seventeen fixes measured:
  - **`L op R` with two plain memory operands loads the RIGHT operand and
    folds the LEFT**: `mov reg,[R] / imul reg,[L]`, and `fld [R] / fadd [L]`
    on x87. `DoMapAI`'s four products, `SoftBlitSprite`'s pitch×y and
    `SetupTrackDrawView`'s `fld/fadd` were each one operand swap from the
    original's addresses. Subtraction is not commutable, so `fld [L] / fsub [R]`
    stays in source order. A body that passes the gate with its multiplicands
    the wrong way round is what this looks like.
  - **Two independent global-to-global copies are emitted in REVERSE source
    order.** `click_x = map_x; click_y = map_y;` gives the original's y-first
    loads (`ReadGameButtons`); `g_sp_width = ddsd_width; g_sp_height =
    ddsd_height;` gives height-first (`SoftPrint_Clear`). Three copies
    (`StoreNewSaveGameToDisk`) came out as loads 1,3,2 / stores 1,2,3 from
    source order 3,1,2. The scheduler's permutation is fixed for a given DAG,
    so MEASURE with relocs.py rather than reason: `MapIconInput`'s six stores
    took four orders (winner `ui_flags &=; g_6687b0 = 4; g_edit_changed = 0;
    saved = mode; mode = 1; g_8119bc = 1;`), and three of the losing orders
    changed the normalised SHAPE (3 mismatches), not just the addresses.
  - **A jump-table switch lays its case bodies out in SOURCE order, and the
    addresses prove it.** `GetGFXFName`'s cases 4/5/6 select
    `g_gfx_dirs[5]/[3]/[4]`; the original's blocks are physically 5,6,4,
    which only `case 5: … case 6: … case 4:` produces. The normalised gate
    could not see it because the three blocks have the same shape.
  - **A 16-byte struct assignment expands to four dword moves in field order
    a,b,c,d.** `SetupTrackDrawView`'s original copies b,c,d, then the scalar
    angle, then a — reachable only as five field-wise statements in that
    order; `g_view_cur = g_view_wide;` is the same instruction shape with the
    wrong addresses at nine positions.
  - **The inline `memcmp` puts its FIRST argument in `esi`.**
    `memcmp(":IMPROVISE", &g_type_buf[10], 10)` is the original
    (`UpdateControllerFromKeyboardData`, two sites); buffer-first passes the
    gate with literal and buffer exchanged.
  - **Which subtrahend of `y - a - b + 1` lands in which register follows the
    source order, but the direction depends on the body.** `RenderMouseBounds`
    needed `y - view.y - y_off + 1`; `MapScreenSetScrollPos` needed the same
    swap at both of its sites — same expression text, and the pre-fix bodies
    had emitted OPPOSITE orders. Measure, don't transfer.
  - **A `union { unsigned char b; unsigned int dw; }` inside a
    `#pragma pack(1)` record is still four bytes wide.** bigscreens.c's
    `CurProfile` had its save-type byte at +0x48 instead of +0x45 and two
    exact bodies read the wrong byte (`PrintSavedGameDetails`,
    `InitGameInterface`). A byte field plus
    `*(unsigned int*)&g_cur_profile.save_slot` at the one wide read gives
    identical code with the right addresses.
  - **cdecl pushes right-to-left, so the first `push` is the LAST argument.**
    `InitPopUpInfo` called `InitPopUpTools(PU_ToolA, PU_ToolB)` where the
    original pushes `PU_ToolA` (0x473310) first — the ok and close callbacks
    were swapped inside a 374-instruction body that passed at zero.
  - **An extern's address comment is the tool's only evidence of identity.**
    schoolcar.c's `g_coaster_regions` was annotated `0x008299a0` (that is
    `g_eye`); the object is the clip-rect ring sentinel at `0x00829a3c` that
    coaster3d.c, coastertiny.c and coaster9.c call `g_clip_ring`. The C was
    right, the annotation was wrong, two bodies reported hits until the
    comment was fixed. A hit can be a stale comment; the same address under
    two names is a hygiene item, not a bug.
  - **The same right-operand rule holds for `|` of two byte loads**: `RunMovie`
    had `(g_key_state[0x9d] | g_key_state[0x1d]) & 0x80` and emitted the 0x1d
    load first; the original loads 0x9d first, so the source is
    `[0x1d] | [0x9d]` (K's file, caught by the per-file gate at the merge).
- **FROM THE PARALLEL SESSION `scope-k` (28 of 28 exact — the AVI movie
  player, the path masks, the texture records; evidence in
  `docs/lanes/scope-k-movie.md`, `scope-k-pathmask.md`,
  `scope-k-texture.md`).** Folded:
  - **Two globals copied as a unit must be ONE struct, and a 12-byte struct
    assignment is what STOPS dead-store elimination.** `RestoreFrontEndState`
    writes `g_cur_screen` twice; as six scalar assignments VC6 deletes the
    first store (17 for the original's 20; nine other orders floored at 20).
    As `g_edit = *game; g_front = *screen; g_front.screen = mode;` the copy
    is lowered too late for DSE, all three stores survive, the bug store
    schedules into the middle of the copy — 20/20 first try.
  - **`cond ? 0 : K` is `setcc / dec / and K`; the polarity is the whole
    residual.** `LoadLevelDatabase`: `(rc >= 0) ? 0 : 2` gives
    `setl / and al,0xfe / add eax,2` (3 wrong, 59B for 58B); `(rc < 0) ? 2 : 0`,
    `((rc >= 0) - 1) & 2` and the `if/return` form are all byte-identical to
    the original `setge / dec / and eax,2`; `(rc >> 31) & 2` is `sar/and`.
  - **Two leading guards that return the same constant merge into ONE
    trailing block only when they are ONE `if`.** `RewindNarrationBuffer`:
    two separate `if (...) return 0;` give two inline epilogues (8 wrong);
    `goto` on both merges only one; `if (a == 2 || a == 0) return 0;` gives
    the single trailing `xor eax,eax / add esp,0x1008 / ret`.
  - **A shared failure tail is cross-jumped only when there are THREE textual
    copies.** `OpenMovie`: one copy plus two `goto`s exiles the early arm with
    no compare of its own (125); two copies keep a 10-instruction inline
    epilogue (115); three copies merge the suffixes as the original does —
    the early arm keeps only its own `cmp [g_avi_open_count],ebx` and jumps
    INTO the later copy's `jne` (74).
  - **A store to a global kills the CSE of a load through a pointer**
    (`OpenMovie`, `RunMovie`: read `mv->width/height` once into locals, or
    the stores to `g_movie_bmi` force two extra loads).
  - **The assignment ORDER inside a chained assignment decides everything.**
    `frame = (int)(next = AVIStreamGetFrame(...))` matches; the other nesting
    rotates the loop, reloads `frame` in a new preheader and spills it — 123
    against 6 (`RunMovie`).
  - **An overloaded sentinel variable is what wins the fourth callee-saved
    register.** `RunMovie`: one `void* frame` puts `shown` in ebx and `frame`
    in the frame (123, all allocation); `int frame` (a -1 sentinel that also
    carries the prefetched index) plus `void* next` (homed in the dead `mv`
    slot) is the original's allocation. `volatile shown` also moves `frame`
    into ebx (108) but costs a load per read; the split is free.
  - **DECLARATION ORDER decides the callee-saved RANKING when several locals
    are initialised at entry** — an exception to "declaration order is
    irrelevant", which was measured on spill SLOTS. All 24 orders of
    `{frame, cur, start, shown}` in `RunMovie`: six exact (every order with
    `frame` first and `start` not second, plus `shown, frame, cur, start`),
    six cost 2, five cost 5, seven cost 6 — always which zero initialiser
    becomes the shared zero register the two entry guards compare against.
  - **The statement order of four independent assignments decides which one
    is spilled.** `OpenMovie`'s `vids` arm: `frames` first gives 2, `fps` first
    74; all four uninitialised locals share ONE frame home.
  - **`mv->file = pfile;` BEFORE `mv->getframe = 0;`** — store order is the
    same either way (VC6 reorders adjacent stores) but the LOAD of `pfile`
    moves between them when written after (2; eight fill orders measured).
  - **A `for` over a GLOBAL counter keeps its zero-trip guard when a COM call
    may write the global** (`RewindNarrationBuffer`: `cmp [mem],0xa / jge`
    before the loop, three reloads per pass; the two callee-saved pushes for
    the inlined memcpy/memset sink past that guard on their own).
  - **`PrintCursor` is `NewPrintCent` (text.c) with DrawText flags 0x25 -> 0x24**,
    84/84 first compile. AVIFile/ACM entry points are declared WITHOUT
    `__declspec(dllimport)` (`call <thunk>`) while the GDI calls in the same
    file need it (`call [__imp__]`).
  - **A `volatile` DECLARATION on a counter global shared by two TUs is worth
    5 instructions and two defects.** `FreeCachedTextEntry`: plain
    `extern int g_text_cache_count` let VC6 schedule the `.text` reload above
    the count's RMW and hoist the compaction loop's bound into a `count - i`
    down-count; `volatile int` costs nothing (the original emits exactly
    load/dec/store and the two reloads): 31/45 -> 41/41. render5.c's
    `ExpireCachedText` needed the same barrier at its one read (a cast).
    **When two functions in different TUs share a counter and both need its
    reloads, suspect the original's header declared it volatile.**
  - **A `rep movsd` struct copy out of an ARRAY is not the copy out of a
    POINTER.** `g_text_cache[j] = g_text_cache[j+1]` gives the original's
    single cursor; `p = &g_text_cache[i]; … *p = p[1]` with `p++` manufactures
    a second IV, a fourth push, 45 for 41.
  - **Whether the off-map stand-in cell's DEAD store survives tells you
    whether the copy's address is taken.** Same probe macro: `PathEdgeMask`/
    `PathCornerMask` pass `&cell` to `IsPathCell` and keep all three stores;
    `IsPathRectClear` reads two flag bytes from the local and VC6 drops the
    unread `tile`. (simcore.c records the same macro dropping only its LAST
    probe's store.) A probe that keeps every store is one whose copy escapes.
  - **A probe macro's coordinate arguments must be pre-computed LOCALS**, or
    VC6 folds the ±1 into the addressing mode (`lea esi,[eax+ecx*4-0x14]`,
    87 of 99 for `PathCornerMask`); `px = at->x - 1; py = at->y + 1;` first
    gives `mov/mov/dec/inc` and closes it. **But VC6 schedules the MODIFIED
    coordinate's load first whatever the source order** (`PathEdgeMask`, all
    four probes) — the load order there is not a lever; don't chase it.
  - **Four repeats of one probe with a byte accumulator give three bit-set
    forms automatically**: `mov byte ptr [esp+0x13],1` (mask known zero),
    `or byte ptr [esp+0x13],2/4` in memory, and the last folded into the
    epilogue as `mov al,[..] / pops / je / or al,8`. One `char mask` and
    four `mask |= bit;`.
  - **A zero register appears in a probe chain when a callee-saved register
    is pushed anyway and has no other use** (`PathEdgeMask` `xor ebx,ebx`,
    every `>= 0` test `cmp reg,ebx`; `PathCornerMask`, whose EBX holds
    `edges`, emits the identical probe with immediates).
  - **The lazy bounds-check inline (`CellForPos`) reads `at->y` only AFTER
    `at->x` passes its width test; the eager one (`MapCellAt`) reads both up
    front** — distinguishable in the listing (`DrawCursorTileAt`). And
    `cell != 0` after that inline is a real third `&&` term: three tests,
    three terms, dropping it loses an instruction.
  - **The two loop counters of a nested scan are ONE `Pos` aggregate, not two
    `int`s.** VC6 SP3 does not strength-reduce an array index that is an
    aggregate member, so `g_map_rows[p.y][p.x]` stays base+index, the
    row-table load hoists to the OUTER preheader (`mov ebx,[g_map_rows]`
    once) and x lands in ebp — `IsPathRectClear` 70/70 after twenty
    spellings at 72/73. Isolation: `Pos` for the inner (row) counter alone
    suffices; an anonymous struct works; the lever is aggregate membership of
    the ROW index. Templates: `ScanPathArea5x5`, fpui2.c's `BuildObjInfoList`.
  - **`if (p == 0) return 0;` on a just-loaded pointer emits the bare inline
    `ret`; `if (p) { ... } return 0;` EXILES the failure arm past it**
    (`ObjNextRider` 13 vs 14/14).
  - **`for (i = 0; i < cap; i++, p++)` generates the counter's update BEFORE
    the pointer's** (`inc ecx / add eax,4`); `p++` as the last body statement
    gives the reverse (`FindDetailImageSlot`, third confirmation of the
    `RecolourModelParts` entry).
  - **A global read by BOTH arms of an if/else must be spelled directly in
    the first arm and as a STATEMENT `list = g_detail_images;` right before
    the second arm's loop** — not a named local at the top (the load hoists
    above the compare, 3) and not twice with a subscript search (39, ESCAPES).
    `DetailImage_AllocSlot` 50/50. **A pointer-walk search and a subscript
    search are different functions**: the subscript form keeps the base in
    esi, pays a `lea` at the return and pushes edi in the prologue, breaking
    the `rep stosd`'s local push/pop bracket — a stray callee-saved push
    around an intrinsic is a register-pressure symptom from the OTHER end.
    And `cap += 0x80; return &list[cap - 0x80];` — the original's
    `lea eax,[edx+eax*4-0x200]` uses the already-incremented register.
  - **A nine-case power-of-two switch (1..256) lowers to a three-way split
    and ascending case order is the block order**: `cmp 0x10 / jg / je /
    dec / cmp 7 / ja / jmp [8-entry table]` low, `add eax,-0x20 / cmp 0xe0 /
    ja / movzx from a 0xe1-byte index table / jmp [5-entry]` high.
    `BuildTextureRecord` 136 exact first compile; the byte index table is
    VC6's choice, not evidence of a source table.
  - **A byte local homed in a dead argument slot is read back as a DWORD plus
    `and 0xff`, not `movsx/movzx`** — plain `unsigned char c;`, no cast
    (`BuildTextureRecord`, both loop variables in the two dead arg slots).
    `img->w * y + x` spelled at BOTH uses recomputes the row base per pixel
    (`imul` inside the inner loop) — the naive spelling matches.
  - **`p = row;` as its own cursor, or VC6 addresses the source as
    `[row+x]`** (`mov dl,[eax+ebp]`, the reverse allocation); with `p` the
    row home is written once per ROW. And `*dst++ = *p++` differs from
    `*dst = *p++; dst++;` (load / inc p / store / inc dst; nine instructions
    from one split) — `ConvertSourceImage`.
  - **Write a two-call tail TWICE when both edges of an `if` need it**:
    written once after the `if` the two `push raw` sites merge into one
    reload (190 for 192); duplicated, VC6 keeps one copy of the calls and one
    push per predecessor. The duplicated SOURCE is what produces the
    per-predecessor argument push.
  - `BITMAPFILEHEADER` needs `#pragma pack(2)` (`bfOffBits` at +0x0a);
    a nested `RES_OpenFile(GetGFXFName(...))` MERGES the cdecl cleanup when
    the result is the only argument (the other way round from the recorded
    split rule); an address-taken struct defeats constant folding of a
    repeated test (`bmi.biBitCount == 8` twice with calls between).

- **FROM THE PARALLEL SESSION `codex-e` (44 of 44 exact — the seven ride
  state machines and the coaster's last callees; evidence in
  `docs/lanes/codex-e.md`).** Folded:
  - **The two-byte `memcmp` intrinsic is the SEARCH shape, not the LOOP
    shape.** In the four rotary-ride `StepMachine`s it introduced a `lea` of
    `rider+0xc` and shifted everything after (33/33/39/32); a scalar
    `rec->tile.key == rider->tile.key` closes all four, with the loop
    re-reading each key at the original sites and no volatile. So: `memcmp`
    for a record SEARCH (`*_FindRecord`), scalar compare inside a tick loop.
  - **A two-value ternary for a byte revolution reset**: `rand()%2 ? 4 : 3`
    gives `setne al` then a 32-bit `add eax,3`; `(rand()%2 != 0) + K` does the
    add in `al` — one mismatch and a byte each.
  - **The order of two groups of clears decides which register is the live
    zero**: the tower's four height clears must precede the four car flag
    clears so EBX is the zero register before any mask operation (20 at
    unchanged 56i/175B). Same family as "where a shared register is freed
    decides which carries the zero".
  - **A common tail written AFTER the join, not inside the guard, keeps the
    original SPLIT cleanup** (`add esp,4` then `add esp,8`) and VC6 duplicates
    the final call itself; inside the guard the cleanups merge into
    `add esp,0xc` (18 -> 0).
  - **Read an aliased field into a byte local BEFORE either record store**
    (`lls->frames` -> one `AL` load, store, `dec al`, store across five cars;
    a second read after the first store reloads through the alias — 97).
  - **A named float local keeps a pending argument's dead-slot spill/reload**
    where `f(route, g(route))` writes the x87 return straight over it. **Name
    the image length before initialising the counter, then compute
    `end = p + length`** — the pointer addition then follows the counter's
    zero. **A bit test needs an `int` local AND a boolean branch**
    (`int flags = *(signed char*)&n->flags; if (flags & 1) return 1;
    return 0;` -> `movsx / and eax,1`; every other spelling narrows to
    `mov al`) — confirms the recorded `Route_IsClosed` rule.
  - Second independent warning this round: **normalised matches do not prove
    global identities.** A whole-struct assignment passed 19i/105B with zero
    mismatches while loading `a/b/c` instead of `b/c/d` at three sites; twelve
    relocations were mismapped. Check relocations wherever several same-sized
    globals are read or stored in one body.

- **Two arrays indexed by ONE variable versus one array indexed by an OFFSET
  expression is a codegen decision worth 300 instructions.**
  `g_seg_first[a]` / `g_seg_second[a]` (two link-time bases, one index
  register -> `[edi+K1]` / `[edi+K2]`, the index merged with the hoisted zero
  register) against `g_segments[a]` / `g_segments[a+5]` (VC6 folds the pair
  into one walking pointer and the zero register loses its rank):
  319 -> 18 on `MusicThread` from that change alone. Corollary: **when a
  loop's induction variable can start at the value already in the hoisted
  zero register, VC6 MERGES them and the zero register gets a better physical
  register**, permuting the whole callee-saved assignment.
- **A cross-thread flag global must be declared `volatile` even where no read
  needs it.** `extern volatile int g_music_disabled;` was worth 9 at two
  unrelated sites: plain, VC6 hoists two global loads above the `= 1` store
  and sinks the shutdown store pair into the epilogue's pops. Other files
  declare the same global plain `int` — externs legitimately disagreeing per
  translation unit, again.
- **A counted loop written over the natural 1-based quantity beats a 0-based
  one plus `+1`**: `for (a = 1; a < 5; a++)` gives the memory-homed `a`, the
  `5*a` induction variable and the rebased `cmp edi,0x10`; `for (a = 0;
  a < 4; a++)` with `a+1` recomputes inside the body. **`bad = (x != y);
  if (bad) f(..., bad);` is the source of `xor edx,edx / cmp / setne dl /
  mov eax,edx / cmp eax,<zeroreg> / je`** — confirmed thirty times over. And
  **`if (n < K) p = f(K); else p = f(n);` cross-jumps onto one call and leaves
  `push K / jmp / push eax`** where the ternary yields `mov eax,K / push eax`
  (second measurement; `ReadNarrationWaveHeader` was the first).
- **`MusicThread` (0x00492db0) is real, is C, and is 81% two macros written
  out thirty times each** (`LOAD_SEGMENT` 30x39, `BUILD_SEGMENT` 30x46,
  byte-periodic at a 0x80 stride). The 3161 count is exact: the body ends on
  the message loop's back edge, and the three "trailing functions" a
  disassembler sees are an alignment `nop`, the five-entry jump table, and
  the next function (the thread STARTER, which `CreateThread`s this body and
  stores the handle `KillMusicSystem` terminates). Its 7-mismatch residual:
  the original caches `__imp__WaitForSingleObject`/`__imp__ResetEvent` in
  esi/edi and reloads both TWICE — preheader and latch — because only the
  notification path's `repe cmpsd` clobbers them; ours loads once at the loop
  top, two instructions shorter and correctly placed by every spelling tried
  (`while(1)`/`for(;;)`, trailing `continue`, dllimport order, volatile
  reads). Recorded so nobody re-derives it; a 99.78% body.

- **FROM THE PARALLEL SESSION `codex-d` (49 of 49 exact — the ride-record
  verbs and the path/order helpers; evidence in `docs/lanes/codex-d.md`).**
  Folded: **`int i = 0` BEFORE computing a u16 key moves the zero into its
  original position** (fifth instance of the early-zero rule); **explicit
  `int x = pos.x; int y = pos.y;` before a pair of calls restores the
  callee-saved assignment** where inline argument fields cost 8;
  **`NewGardenerOrder` needs its tail assignment written INSIDE both arms** — a
  shared tail store emitted 72 B against 85 (the tail-duplication rule from
  the record side). And a caution for every integrator: **a normalised audit
  is NOT sufficient for global IDENTITIES** — `NewMechanicOrder` passed
  19i/67B with zero mismatches while its head and tail stores were SWAPPED
  (the two globals normalise to the same token); an independent COFF
  relocation review caught it. Where two globals of the same size are stored
  in one body, check the relocations, not just the mnemonics.

- **FROM THE PARALLEL SESSIONS `scope-e` (147 of 147 exact — the frontier's
  1-17-instruction tail) and `scope-i` (the UI/system/render partials: one
  close, two improvements, twelve honest floors); evidence in
  `docs/lanes/scope-e.md`, `scope-i.md`.** Folded:
  - **Five record searches closed by the two-byte `memcmp` intrinsic.**
    `memcmp(&r->tile, tile, 2)` expands after loop-invariant hoisting so the
    loop reloads the record key into `dx` and compares the query through
    memory at exactly the two original sites (16i/40B each) — the established
    form in `JailCell_FindRecord`/`Carousel_FindRec`. Scalar equality and
    volatile reads were the wrong abstraction; all five close with no volatile.
  - **A null guard before a returned Win32 result splits the return**:
    `if (!path) return 0; return SetCurrentDirectoryA(path);` gives the
    original's `jne/ret` split (7i/17B) where a `void` wrapper shares one
    final return (6i/16B) — and recovers the real return type.
  - **`int i = 0;` BEFORE the pointer initialiser, with no initialiser in the
    `for` clause, produces the original scratch-register order** (fourth
    instance) — and returning the loop ordinal from `Copters_StepRider`
    coalesces the counter with eax and removes a duplicate success store.
  - **Equal `return` cases merge LATE: keep an explicit event-bit case that
    returns the same value as the default.** It prevents VC6 replacing the
    preceding conditional with `setne/inc`; the extra test disappears from
    the code, leaving the original `test al,4 / mov al,2 / jne / mov al,1 /
    ret` — and the eliminated test's mask is NOT uniquely recoverable
    (`DefaultIconInput`).
  - **x87 calling convention in the original: the reciprocal-sqrt target
    consumes and returns ST(0)** — an inline-assembly call followed by
    rounding through the argument's float home is the body; `fldcw control`
    is the float-mode restore, EBP frame and all. Both `[OK]`.
  - **`UpdatePersonPos` closed by representing the two unscaled isometric
    coordinates as ONE `Pos` before storing either scaled output**
    (`projected.x = bx - by; projected.y = by + bx;` recovers
    `lea ecx,[ebx+ebp]`), after which the compensating volatile read had to
    be REMOVED to close the last six. A volatile that once helped can become
    the residual once the real cause is fixed — re-test every shim after a
    structural change.
  - **`PaintTileLayer`: grouping `halfw` with the existing ESCAPED `tile`
    `Pos` keeps an ordinary memory home and refuses the unwanted second-cell
    induction variable at the original 0x34 frame** — the non-volatile refusal
    the previous note asked for (389 -> 378, register roles recovered); grouping
    all seven intervening homes regresses to 395 with ESCAPES.
    **`RenderFullMap`: an explicit `ILFTable` carrying the `sprite+8` ADDRESS
    to the loop condition** gives the original's `add eax,8 / mov eax,[eax]`
    at the latch (844 -> 827); every other of the six recorded register facts
    costs 873-1066 when forced, and the paired `RenderView` corrections tested
    TOGETHER regress (381 -> 549) — both now stated as floors with the tests.
  - Two brief corrections: `InitExitCheckBox` was NOT unexplored — five prior
    passes exist in its note (the "constant-web floor": three dword zero
    stores, no fourth use); and a scope table listed fifteen functions where
    its prose said fourteen. Read the note before the brief.

- **A `memset` intrinsic placed AFTER some plain zero stores gives the body
  TWO zero registers.** With the memset first, VC6 forwards the fill's own
  zero out of eax and every store shrinks to the 5-byte `mov moffs32,eax`
  (63 B against 73); with three plain stores BEFORE it, the zero already
  exists, VC6 still hoists `rep stosd` above them but materialises
  `xor edx,edx` in the fill's scheduling slot and all nine stores use edx —
  exactly the missing 11 bytes. Nine spellings, one exact
  (`ResetNarrationStreamState`).
- **A CALL between two subscripts of the same index defeats VC6's
  induction-variable elimination — and that is how you get TWO lockstep
  cursors.** `table[i].elem` before the call and `table[i].kind` in the latch
  as subscripts of one `i` produce the original's two cursors plus the
  `mov eax,esi` copy; a `p++` walk collapses to one and is two instructions
  short (`LLIDB_UnLoadTSMData`, ten spellings). Refines "spell two lockstep
  cursors the SAME way to eliminate one": the call is the barrier.
- **An eager root copy of an OUT parameter (`void** dst = out;`) moves its
  load between `push esi` and `push edi` and re-ranks two strength-reduced
  cursors** — 7 -> 0 at identical counts (`CollectUsedTSFTables`); the same
  lever closed `SkipStrings` (stepping the parameter itself leaves the
  zero-trip arm returning `[esp+8]` where the original returns edx).
- **Read a table's entry COUNT into a named local BEFORE deriving the
  cursor** — left in the loop condition, VC6 hoists the count after the
  `lea`, puts `i = 0` above the pushes and the table pointer lands in edx,
  losing the 5-byte `moffs32` load (`FindCoasterColour`, 6 -> 0). **`int i =
  0;` as an initialiser AFTER the first call, not inside the guarded `for`,**
  puts `xor esi,esi` ahead of `test eax,eax`, where the zero register then
  doubles as a compare operand (`GetObjRiderN`, 3 -> 0).
- Data-model corrections from `savemisc2.c`, each read off the disassembly:
  `SaveIconStateChunk` writes four DWORDs, not `u8[0x10]`, and stores the
  INVERSE of flag 0x400; `SaveScriptString`'s framing is u32 length (-1 =
  NULL) then exactly `len` bytes with NO terminator (the loader adds the NUL,
  which is why it mallocs `len+1`) — savechunks.c's "len+1 bytes" note is
  wrong; `Raster_SaveState` saves nothing (it points the 16-bit rasteriser at
  the locked surface, and its "restore" partner is a bare `ret`); rin.c's
  `UnInit3DPrintList` is really the shading-ramp cache teardown; the goal list
  and the script-event list share a record type; a FREE track end maps to
  direction 1 (NORTH), not a sentinel.

- **FROM THE PARALLEL SESSION `fable-d` (34 of 34 exact — the narration
  wave-header reader, the movie player, the coaster's entrance track and RK4
  callbacks, the cursor-tile painters; evidence in `docs/lanes/fable-d.md`).
  The strongest, folded here:**
  - **A trailing `return 0` block at the END of a function is what makes VC6
    retarget every earlier guard's failure branch to it — and that is not
    always what the original did.** `ReadNarrationWaveHeader` has nine
    `if (_read(...) != 4) return 0;` guards; written as
    `while (...) { ... } return 0;` VC6 collects all nine into the trailing
    block (151 of 187). Written as `for (;;) { if (...) return 0; ... }`, with
    no trailing block, each guard keeps its own inline `xor eax,eax / pop /
    add esp / ret` — the original's ELEVEN identical copies, 187/187. VC6
    rotates the `for (;;)` into exactly the `while` shape, so the spellings
    differ ONLY in whether the merge target exists. Sharpens "nested ifs,
    failures to ONE trailing `return 0`" from the other side.
  - **WHICH identical `return 0` block VC6 merges into is a `goto` decision,
    worth four bytes and invisible to the strict/rb/ob triage.** Two separate
    `return 0;` statements merge the later copy BACKWARDS into a block 323
    bytes earlier (six-byte `jne`); routing both through one `goto chunkfail;`
    at the loop's end pins a two-byte forward `jne`. 187/187 either way — only
    the byte length shows it, which is why the gate checks bytes.
  - **To EXILE a `return K` block past the function, NEST the rest of the body
    inside the guard — a `goto` to a trailing label does not do it when the
    label has more than one predecessor.** `if (mv) { ...; return played; }
    return 1;` exiles the block and gives `test esi,esi / je <far>`;
    `if (!mv) return 1;` leaves it inline; two `goto suppressed;` left the
    block inline at the SECOND goto. So the recorded "write it `goto fail;` to
    pin the merged block at the END" prescription holds only for a
    single-predecessor label; the general handle is the nesting (`PlayMovie`).
  - **`if (a < K) g = f(K); else g = f(a);` — two textual calls — gives
    `push K / jmp / push eax / call`; the ternary `f(a < K ? K : a)` gives
    `mov eax,K / push eax / call`** and also merged a malloc's `add esp,4`
    into the next cleanup. **An if/else whose two arms both begin with a call
    sharing a constant argument gets that push HEAD-MERGED into the test
    block** — a lone `push` between a load and its `cmp` is the tell.
  - **An explicit `i = 0;` statement at the TOP of a function is what gives
    VC6 a SECOND zero register** — the still-zero long-lived index supplies
    the float-zero stores while ebx supplies the `push 0` arguments
    (`Castle_InitEntranceTrack`, 170 of 230 -> 187 of 231 on that alone).
    **Where a shared register is FREED decides which register carries a
    function-wide zero**: six stores written BEFORE `sq = corner;` keep ebx
    busy so VC6 takes edx for the zero; written after, ebx frees early, VC6
    takes ebx, and EVERY register in the last two loops shifts — 201 -> 231/231
    with no other change. Also there: **`prev` before `next` in each store
    pair is load-bearing** (227 the other way).
  - **A `short` local keeps its sign extension at the USE; an `int` one moves
    it to the definition and loses an instruction** — when a body is exactly
    one short and the original sign-extends late, narrow the local's type.
    **An `int` local with a `(short)` cast at its use** is how a dword store
    and a `movsx word ptr` load coexist on one slot; and
    `mov ax,[m] / lea ecx,[eax+eax] / movsx ecx,cx / movsx eax,ax` reads as
    `short w = (short)(h + h);` — the doubling done 32-bit then narrowed only
    happens when the RESULT is a `short`.
  - **Three adjacent constant stores to one array are emitted in SOURCE
    order** (`m[11] = m[7] = m[3] = 0.0f` exact only descending, all six
    permutations swept) — the "adjacent stores come out REVERSED" rule is for a
    PAIR; with three, sweep. **The `- 1` of an end bound belongs in the loop
    CONDITION**, not the end pointer's initialiser (`while (p < end - 1)` gives
    `add` + `lea [eax-1]`; folded, one `lea [eax+esi-1]`).
  - **A float value that must survive a call needs its OWN IR temporary, and a
    block-scoped SECOND local supplies it** — `{ float brake = -(vv / (d+d)); }`
    forces `vv`'s memory home and restores `call / fmul [vv] / add esp /
    fmul [v]` (49 -> 57/57). **`x * 2` on a float is `fadd st(0),st(0)` only
    when the operand is ALREADY on the stack.** **A `double`-typed threshold
    (`< 0.05`, no `f`) survives as `fcomp qword`**; `fcomp / test ah,1 / je`
    is `<`, not `>=`. **`1.175494351e-38f` is FLT_MIN** and reproduces.
  - **A struct copy placed between a count and a divide keeps the `fidiv`
    spill out of the copied-to local's frame slot** — the store must be live
    across the divide for the slot to be denied.
  - **The by-value `WinRect` field-assignment-order rule holds for LIVE values,
    not just constants** (`PrintReportLine`, 13 of 43 at identical bytes, rb 0
    -> exact on the reorder alone). **A 15-byte packed struct copy must be a
    STRUCT ASSIGNMENT** — it materialises the source address in a register
    (`lea ecx,[edx+0x38]` then dword/dword/dword/word/byte); a named pointer
    to the sub-struct does NOT reach it (VC6 folds it back).
  - **A list search whose success body lives INSIDE the loop threads away the
    post-loop null test**, and VC6 DUPLICATES the two-byte `ret` for the
    exhausted edge because the pushes live inside the found path — read a
    lone mid-function `ret` with no pops as exactly this shape.
  - **VC6's inline `strcpy` copies a string LITERAL as one dword plus one
    byte; a named `extern const char[]` gets the full scan-and-copy
    expansion** — both sit in one body (`PlayMovie`). **`#pragma
    function(memcpy)` around one body is how a file mixes a CALLED and an
    INLINED `memcpy`**; under `/O2` alone the intrinsic wins everywhere.
  - **Naming the derived limit hoists a `lea` above a two-arm clamp**
    (`int lim = base + 0x28;` gives `lea ecx,[eax+0x28]` scheduled into the
    first clamp's gap and a memory-operand compare; 7 -> 0). **Nine cdecl
    pushes across a straight-line function merge into ONE `add esp,0x58`**, so
    one local is addressed as `[esp]` at one call and `[esp+8]` at the next —
    resolve displacements by push depth before deciding two `lea`s name
    different locals. **Seven literal zeros hoist the zero register with no
    loop** — a run of `cmp reg,<callee-saved>` guards in a teardown function
    means that many literal zeros.
  - **`i++` as a STATEMENT before a cursor advance, versus in the `for`
    increment, swaps two ALU ops and sinks the store below the compare** — the
    increment-order rule from the increment side, with a free second
    consequence. **A `char` local homed in a dead parameter slot is passed on
    as a RAW DWORD** (stale high bytes visible in the argument, the `& 0xff`
    a SEPARATE `and` after the call). **A three-case `dec/je` chain lays its
    blocks in REVERSE source order and the LAST case shares the function's
    epilogue.** **A twin found by callee overlap, not size** — two adjacent
    functions calling the same three helpers in order were one source two
    arguments apart. And **`&arr[i]` was enough for both strength-reduced
    cursors in `render5.c`** — the biased-pointer form is for READ-ONLY tables
    walked by more than one field.

- **"Store a global, then walk a pointer chain": the chain head must be a
  FUNCTION-LEVEL local assigned AFTER the store.** Inline
  (`p->owner->obj->elem->flags |= 8`) the chain takes the eax->ecx->edx
  rotation where the original's first dereference is in place
  (`mov eax,[eax+0x18]`) — 3 wrong at identical length; as a BLOCK-scope local
  the rotation is right but the global store sinks below three of the four
  argument pushes — 3 wrong the other way. Only the function-level declaration
  with the store first gets both (7 spellings, `ChildrenBarInput`, transferred
  to its twin). Scope decides the SCHEDULE, not just the frame slot.
- **A call result added to a global must be a NAMED LOCAL to make the
  global's load the `add`'s destination.** `g_x + f(a)` inline gives
  `add <result>,<load>`; `int c = f(a); ... g_x + c` gives the original's
  `mov ecx,[g_x] / add ecx,eax`. Commutation is inert. Three instructions and a
  byte on `FreePlayItemAvailable`.
- **A preheader `mov ecx,1` paired with a trailing `lea eax,[ecx-1]` proves
  the source counted from ONE** — the 0-based `for (i = 0; i < 4; i++) ...
  return i;` is two instructions shorter and emits no `lea` (`GetBlokeAgeGroup`;
  the `i <= N-1` -> `jle` rule confirmed at the same site). **A merged
  `add esp` spans every call in a guarded block when no call consumes
  another's result** — `0x14`, `0x1c` and `0x114` (a 256-byte buffer included)
  measured in one file.
- Five UI families confirmed as ONE source each (`Advert*Input` x3,
  `EnqueueStep*Event`, `*ChildrenBarInput`, `PlayReport*`, `*HelpExpired`),
  and an original quirk worth the runtime knowing: `ObjectHelpExpired`'s
  timer arm is `GetGameTimer() - g_advisor_last < 0` against a stamp set to
  NOW every frame, so object help is dropped on the first frame after the
  bubble closes.

- **FROM THE PARALLEL SESSION `codex-c` (88 of 88 exact — the frontier's
  1-28-instruction tail; evidence in `docs/lanes/codex-c.md`).** Small bodies
  close almost entirely on the recorded rules; the few new measurements: **a
  separate `cursor` local closes an ordinal list lookup** (13i/29B with 9
  mismatches -> 11i/22B exact, signed vs unsigned index inert); **a free
  volatile read of `boat->piece` closes a four-register residual without
  changing the body** where merely splitting the two pointer reads into
  statements was inert; `RouteStepAxis` needed a free volatile read of `x1`
  to keep an independent subtraction/test (17i -> the original 18i); and
  `screen.c` declares its callbacks with EMPTY parameter lists while the
  bodies forward real arguments — the cdecl ABI hides it, and the caller
  declarations were correctly left alone. `sub_411650` is `LFBoat_IsOnDrop`
  (it compares against the LOG FLUME DROP class). Eighty-plus of the
  eighty-eight closed on the first compile.

- **A memory-operand FOLD inside a loop is a register-PRESSURE symptom — look
  at what is held across the loop, not at the expression.** `Anim3D_OffsetAt`:
  accumulating straight into the 8-byte struct return (`r.x += ...`) keeps the
  part cursor in EBX across the inner loop, leaves it one scratch register and
  folds both fractional loads into `add reg,[mem]` — 45 for the original's 51.
  Accumulating in plain `int` locals and assigning into the returned `Pos` at
  the END spills the cursor to the dead argument slot, gives the inner loop
  EBX+EBP and materialises all four loads — exact first try. **This is the
  REVERSE of "an 8-byte struct RETURN must be the ACCUMULATOR"**: that rule
  holds for a straight-line sum, not for a loop accumulation (~50 spellings).
- **A whole-struct copy from a const table, then `-=` on one field, folds the
  subtraction into a memory operand and pins the pair's store order**
  (`o = tbl[car]; o.oy -= rec->car[car].height;` -> `mov/mov/store ox/
  sub eax,[mem]/store oy`); field-by-field loads the subtrahend into a register
  and sinks the store past the argument push (45 -> 0, 39 -> 0 on the two tower
  draw passes).
- **Two sums used as call arguments become `lea`s only through an aggregate
  local.** Inline in the argument list VC6 emits `add` into one operand's
  register; `p.ox = a.ox + b.ox; p.oy = a.oy + b.oy; f(s, p.ox, p.oy, ...)`
  gives two `lea`s into fresh registers (11 -> 0), and the `lea` operand order
  then follows source order per component.
- **Wrapping two shifted coordinates in ONE aggregate blocks VC6's
  shift-distribution algebra.** `(x<<8) + (y<<8)` as two `int` locals is
  rewritten to `(x+y)<<8` and the two `shl`s sink below the `imul`; as one
  struct the distribution does not run and both `shl`s stay ahead of the call.
  Confirmed from the other side: keeping the (dead) x chain alive preserves
  the shifts with plain ints too — the transform is the algebra pass running
  AFTER dead-code elimination has left each shift a single use. Whole residual
  of `SpaceTower_PlaceCar` (41 -> 0, the pair assigned y first).
- **A free volatile read on a loop invariant the original reloads kills a
  DERIVED induction variable** (`px + *(volatile int*)&halfw` stops VC6
  manufacturing a second column IV with its own frame slot; frame 0x38 -> the
  original's 0x34). Its price: the volatile object's home moves up the frame,
  sliding two neighbours — recorded at `PaintTileLayer`'s marker.
- **For two derived values from one field, define the DOUBLED one first to get
  the `lea`.** `tw = (short)(h*2); th = h;` gives `lea ecx,[eax+eax]`; th-then-tw
  gives an in-place `add eax,eax` and the whole head comes out one form off.
  Isolated kernels emit `lea` for every spelling — a pressure/order effect in
  situ, not a spelling one.
- **Confirmed: `BsBoat_Animate` reproduces `JcBoat_Animate`'s three-instruction
  `imul`-operand-rank residual at the same indices** — the retired analysis in
  roads.c predicted from the disassembly alone that the unported twin had the
  identical form. Do not re-open either; a fix on one closes both. And
  `mechrides.c`'s `g_spacetower_state`/`g_spacetower_phase` are elements 0 and
  3 of a four-pointer seat-matte table indexed by car (names left, noted).

- **FROM THE PARALLEL SESSIONS `fable-c` (26 of 26 exact) and `codex-b` (60 of
  60 exact); full evidence in `docs/lanes/fable-c.md`, `codex-b.md`.** The
  strongest, folded here:
  - **Do NOT write the second `return`: let VC6 duplicate the epilogue.**
    `LoadTextFile`'s failure epilogue reads the never-written home of the
    result pointer (`mov eax,[esp+0xc]`). One `return text;` after the `if`
    keeps all three pushes in the prologue and makes VC6 tail-duplicate the
    epilogue itself, reading the uninitialised home — 47/47. A `return` in the
    failure arm sinks two pushes and reads `[esp+4]` (31/45). **Two
    uninitialised locals, one per guard, are how a function gets two different
    frame homes** — VC6 homes one in the dead `list` slot and the other in the
    still-live `index` slot, which is why the shipped `LookupTextureName`
    calls `SkipStrings((char*)index, index)` on a null list. Reachable, not
    an accident.
  - **A lone `dec` before a `je` on a parameter is a one-case `switch`, not an
    `if`.** `switch (kind) { case 1: ... }` emits `mov eax,<arg> / dec eax /
    je`; `if (x == 1)` emits `cmp dword ptr [mem],1 / je`; `kind--; if (kind)`
    gives `mov/dec/test/je`, one too many. Extends the two-case `dec/je/dec/jne`
    entry.
  - **The for-increment order of two lockstep cursors decides the latch's
    emission order; the declaration order is inert.** `for (...; p++, i++)`
    exact, `..., i++, p++)` wrong (`RecolourModelParts`). And **a three-term
    `for` increment (`i++, line++, y += 0x18`) puts the TRIP COUNTER first in
    the latch and fixes the whole callee-saved ranking with it** — data steps
    as body statements instead: 50 of 59; in the increment clause after `i++`:
    59/59 (`PrintScreenMode7`). **`bit <<= 1` belongs in the increment clause
    AFTER `i++`** (`RES_FindVolumeOnAnyDrive`): the counter's update must be
    generated BEFORE the other induction variable's, whichever clause holds
    them. Three reachable cases of the latch-order question, all by source
    order of the updates.
  - **A counted `for` with a literal bound keeps its test in the LATCH and
    gives the SECOND condition the loop head, with no peel** — VC6 proves the
    first `i < 14` and deletes it, so there is no peeled copy to double as an
    `if`. When the latch is a literal-bounded counter and there is no peel,
    the loop was a counted `for` and the other test was a `break`.
  - **A coordinate pair feeding a SUM OF SQUARES must be ONE `Pos` aggregate
    LOCAL** — the member becomes the destination symbol of its own product
    (`y*y` in edx, `x*x` in eax); two `int` locals emit the mirror and it is
    unreachable by every spelling (80 of 84, six volatile reads included);
    `Pos p` throughout is 84/84 and both sum orders then work. The by-value
    `Pos` mechanism, now on a local at an arithmetic site.
  - **A call result handed straight to a COM method must be a named local**
    (`SetVolume(obj, VolumeFromDistSq(d2))` evaluates the object and vtable
    BEFORE the inner call, spills the vtable pointer and takes a fourth push —
    90 for an 84-instruction function). The callee-object side of "two
    struct-return handles must be separate statements".
  - **Three `return 0`s do NOT produce the zero-register join; a FLAG set in
    the innermost block does.** `mov eax,edi` with `xor edi,edi` eleven
    instructions earlier: `int found = 0;` with the tests as NESTED ifs whose
    innermost body is `found = 1;` and `return found;` at the end — 92/92.
    Three textual `return 0;` tail-duplicate the epilogue three times (115);
    `goto fail; fail: return 0;` merges them but emits `xor eax,eax` in the
    merged block (88). Sharpens the `xor <callee-saved>,<same>` join rule: the
    zero must be a VARIABLE live from before the first call.
  - **`found = 1;` BEFORE the `strcpy`, not after — 18 of 138.** Written after
    (the natural order), VC6 exiles the flag to a stack home and the parameter
    takes ebp instead. The statement order decides WHICH of two webs is
    enregistered, invisible except as the frame size. **And the mask guard
    must be `if (mask) { loop }`, not `if (!mask) return found;`** — the early
    return coalesces `found = 0` with `i = 0` into one hoisted zero and the
    flag never gets a register; the three levers reach 138/138 only TOGETHER
    (117, 120, 135 for the partial combinations). A `strict 39 /
    register-blind 24` residual that read as "structural" was two statement
    positions.
  - **`x &= ~1` on an `int` already in a register narrows to `and al,0xfe`**
    — the recorded "`&= ~` does NOT narrow" was measured on a read-modify-write
    straight to memory; once loaded, both `|= 0x1000` (`or ch,0x10`) and
    `&= ~1` use the byte form. **`-(unsigned short field) * 2` is
    `xor/mov16/neg/shl`** — on a freshly zero-extended value being pushed VC6
    shifts in place; the `*2 -> lea/add` rule is for a value already in a
    register needing a fresh destination. **`field >> 1` on a u16 is `shr`;
    `-field >> 1` is `neg` + `sar`** — the read-side tell for a u16 used in
    signed arithmetic.
  - **A 16-byte rect copied out of a class record is FOUR field assignments,
    read off the non-sequential load order** (`+0x3c, +0x44, +0x40, +0x48` =
    each following subtraction's operands adjacent). **A 12-byte struct
    assignment is `mov ecx,<src>` plus three register-move pairs** — the
    source-address copy is part of the lowering, not a second pointer.
    **`char root[4] = "c:\\";` is a `.rdata` dword copy** (`memcpy`, a 4-byte
    struct assign and a `long*` copy are all byte-identical; intrinsic `strcpy`
    is sixteen instructions dearer).
  - **A subscript walk over a GLOBAL array gives a SIGNED `cmp cursor,end /
    jl` against a link-time constant** — reads like a pointer walk but is not
    (a pointer walk compares `jb`). **An address-taken counter is incremented
    THROUGH MEMORY** (`mov edx,[esp] / inc edx / mov [esp],edx`) when the loop
    body holds its only other use; address-taken forces the home store,
    whether the loop also reloads depends on what else is live. **Repeated
    `if (!Write(...)) return 0;` guards cross-jump BACKWARDS into the first
    inline `return 0`** — no construct needed.
  - **A byte-typed constant hoisted into a callee-saved register (`mov bl,4`)
    needs no construct** — four `& 4` tests on different words, ebx pushed
    anyway; the zero-web rule with a non-zero constant. **`do { } while
    (strlen(s) == 0)` is the intrinsic `repne scasb` with the back-edge INTO
    the counter update** (`not ecx / dec ecx / je top`, no separate compare).
  - **Naming `next = prev->next` inside a delete loop changes the allocation
    web** where a free volatile head read was inert (`DeleteIcon`, 23i/57B
    exact). **Evaluate the current key state before the previous-state bit**
    even though VC6 schedules the previous byte first — the order adds the
    original's EBX web (`GetTypedChar`). **A named `which = rand() % 5` before
    the call** moves argument setup before `rand` (28i/87B exact) — and a
    caller-local `unsigned rand` reproduces the original's unsigned `div`
    where some units declare `rand` signed.

- **Two block-scoped out-param locals, one per arm, stop VC6 head-merging the
  arms' identical argument setup.** As one function-level local the two
  `lea reg,[esp+4] / push / push` blocks are identical IR and VC6 hoists them
  above the branch (33 against 36). Two block-scoped locals are distinct
  symbols at merge time; the allocator still colours both into the ONE dead
  argument slot, so the function keeps no frame. The original's `lea ecx` in
  one arm and `lea edx` in the other IS the proof they were never merged
  (`TrackFitEndGeom`).
- **`e = g;` immediately after the call that filled `&g` puts an out-value in
  one register for both a compare and the return** — the load schedules into
  the call's return slot, before the pending `add esp,8`. Read only at its
  uses, VC6 emits a separate `[esp+4]` load per block: an address-taken local
  is not CSE'd across a branch.
- **VC6's fold of `base + i*STRIDE` into a two-register `movsx` addressing
  mode is NOT reachable from C** — ~30 spellings inert (named `&list[i]`,
  `list+i`, pointer locals at either level, `const short*` to the field,
  `char*` byte arithmetic, integer-typed addresses, casts at the use,
  array-of-array rows, 2-byte elements indexed `[i*8]`, a whole-record copy,
  an inline helper, a `short` index, five declaration orders). Only a
  volatile access blocks it, and the volatile is a scheduling barrier, so the
  AGI-filling instructions land after it. The 12-mismatch residual of
  `CoasterCar_BuildRider` (byte-exact, `strict == rb == ob`).
- **A preheader `xor <iv>,<iv>` proves the byte offset is VC6's own strength
  reduction, not a source variable** — an explicit `off` local moves that
  `xor` ABOVE the zero-trip guard (measured both ways). A cheap reading rule
  for any counted array walk.
- **"Size is not evidence of twinning" applies WITHIN one body too:**
  `CoasterCar_BuildRider`'s three substitution loops are 0x42/0x40/0x42
  bytes — loops 1 and 3 materialise the record address, loop 2 folds one add
  away. VC6 is deterministic, so the middle loop was WRITTEN differently.

- **Test the GLOBAL, not the local copy, to stop VC6 propagating a list
  head's null-ness onto the cursor** — two sites, opposite symptoms, one rule.
  `run = g_head; if (g_head == 0) return 0; while (run)` splits the web so the
  load lands in eax BEFORE `push esi/edi` and VC6 emits the original's
  `mov esi,eax`; `if (run == 0)` propagates and is one instruction short
  (`LFTrack_FindPiece`). Reading `g_open_head` directly at all three uses keeps
  the CSE in one register but leaves the `while (p)` peel standing as the
  original's redundant `mov eax,edi / test eax,eax`; a `head` local drops both
  (`AddOpenNode`, eleven shapes). Negative: a free volatile read of a global
  list head pins the load BELOW both callee-saved pushes, not above.
- **A strength-reduced cursor over an inline struct array is anchored on the
  SECOND store statement's offset** — `pos` written second gives
  `lea esi,[run+0x4c]`, the original; written first the anchor slides to the
  third statement's field (twelve orders, `LFRun_Start`). **Walk that array by
  SUBSCRIPT, not through a named pointer** — it fixes the anchor AND lets the
  `piece` store schedule ahead of the call-result store; through the pointer
  all 48 statement orders floored at 3.
- **A coordinate pair copied from a struct to an out-`Pos` must be ONE `Pos`
  field assigned whole.** `*out = d->entrance;` breaks the CSE with the bounds
  test, which is what lets `push esi` sink out of the prologue into the other
  arm; two `int` fields cost 23 at the identical instruction count, ~20
  spellings floored there (`GetObjectDoorOffset`). Corollary: **reversing the
  store order in the COLD arm also sinks the push** — the cold arm's store
  order decides the hot arm's allocation.
- **The alias-kill rule extends to GLOBALS: a store to one global kills the
  CSE of an unrelated global load.** `g_elem->data->rect...` spelled twice
  around a `g_tile.x = ...` store reloads the element AND re-derives the class
  (15); naming `ObjDef* d` closes it (`UpdateEntranceTile`).
- **`if (s == 0) A(); if (s != 0) B();` as two separate `if`s gives ONE call
  site for B** with the first test's non-zero edge threaded into it — the
  shape of both `Control*` worker ticks. An `if/else` emits two.
- **TOOLING DEFECT, second instance — FIXED with the first (see above):**
  `MatMul` (0x00426120) is 40 instructions / 103 bytes; instruction 9 jumps
  forward over the loop reload and the walker used to stop there, reporting
  `orig=10i/28B` and ESCAPES against a complete, zero-mismatch body. With the
  fix it audits `[OK]` at its full extent. Also a data-quality
  catch from the same session: `coaster.c`'s extern comment
  `/* 0x004299a30 -> ... */` carries a malformed nine-digit address, which is
  why `callees.py` listed 0x004299a3 (instruction 8 of 0x00429990) as a
  function; the real entry is 0x00429a30.

- **VC6 SP3's tail-duplication THRESHOLD, measured both ways on one source.**
  A shared tail of ONE call + `add esp,4` is COPIED into the early-return arm
  (the eight record unlinks); a tail of TWO calls + a 0x10-byte argument block
  is JUMPED to (`PlaneRide_RemoveRecord`). When a twin's early arm ends in a
  `jmp` where its sibling has a copy, look at the tail's SIZE, not the source.
- **Two zero registers in one small function is a sub-object `memset`.**
  `memset(&rec->cars, 0, 6)` -> `lea eax,[rec+0xd] / xor ecx,ecx /
  mov [eax],ecx / mov [eax+4],cx` — a base register for the member AND a
  second zero register beside the web's `xor ebx,ebx`. Six byte stores give +2
  instructions; a struct assign from a `static const` zero loads from `.rdata`
  (+1); from a zeroed local it spills. (`Balloonz_NewRecord`.)
- **`x += -t * 8`, `x -= t * 8` and `x += (-t) << 3` are byte-identical** —
  VC6 canonicalises all three to `neg/shl/add`; do not sweep them. What
  matters at that site is the CSE: a store through `r->bloke` kills it, so two
  separate `r->bloke->world.` expressions reproduce the original's second
  load where a named `Bloke*` local keeps one.
- **`StandardRemoveObject` needs a FIFTH prototype** (`Pump_Remove`): the
  `BPosW`-by-value spelling pushes the packed square's whole home dword
  unmasked; `unsigned int` inserts `and edx,0xffff`. Five caller-side
  spellings of one callee in the tree now; each is a lever. **Six calls, one
  `add esp,0x30`** (`Entrance1_Create`) — when nothing consumes a result as an
  argument, every cleanup defers.
- **The dword-plus-mask idiom at a by-value parameter whose address is ALSO
  taken:** `FindRec((MapSquare*)&sq)` keeps `sq` in its argument slot, giving
  both the aligned `mov edx,[esp+0x30] / and edx,0xff` and the misaligned
  `mov eax,[esp+0x31] / and eax,0xff`. Neither a pointer parameter nor an
  `unsigned int` produces both halves.
- Negative, 46 spellings (`Pump_SnapToRoad`): **VC6 always jump-threads a
  provably-NULL pointer into a following `if (p)`.** The original's
  un-threaded form (`xor eax,eax`, fall into a join starting `xor ecx,ecx`,
  re-test a known zero, return via `mov eax,ecx`) is reachable only through a
  `volatile` local at the cost of a stack home the original does not have.
  Twin of the "global constant propagation deletes a straight-line
  `v = 0; return v;`" entry — the same pass, seen from a pointer.
- **The record-unlink family is one source compiled EIGHT times**, not six:
  GOLD RUSH and EARTH SLIDE share it with the six mechanical rides. Identical
  index for index after substituting the head global and link offset
  (+0x04/+0x10/+0x2c/+0x0c), joust.c's volatile link-walk and the null-head
  bug included. And **a `+0xbc` save callback is one source across four
  classes** (only the head global and record size differ) — it is
  `EarthSlide_Save` with the queue pass deleted. The save format: a chain of
  `int 1` + one RAW record image, closed by `int 0`; the record's own `next`
  goes into the file and the loader overwrites it.

- **`ZBuffer_FillPoly` (0x00423350) is partly hand-written assembly — a third
  asm site after the four `tri3d.c` rasterisers.** Three independent proofs:
  an EBP frame in an `/O2` file; `xchg ebx,eax` (0x93) and `add ebx,1` where
  VC6 always emits `inc`; and a `jns/jmp` pair where `js` alone would do. The
  boundary is visible in VC6's post-`__asm` reloads. Closed 2026-09-08 at 101 of
  101 with the proofs (levers at the top of this section); the same triage
  applies to any body showing those signatures.
- **An address-taken out-param local declared in the BLOCK where it is used
  takes a dead argument slot; at function level it takes a frame slot and
  pushes a float temporary into the argument slot instead.** All 24
  declaration orders inert; all six with block scope exact — the last 8 of
  `TrackJoinPieces`. Third instance of scope-decides-the-slot.
- **Make a cursor the callee's OUT-PARAMETER (`&cur`) to stop VC6 enregistering
  it** in a fourth callee-saved register: 66 of 79 -> 8, and it freed ebx for
  a float's dword copy. **A named pointer INTO a sub-record (`&p->cls->part`)
  produces `add eax,0x3c` while folding the first read into `[eax+0x3c]`**; a
  pointer to the outer struct does not.
- **`i <= N-1` versus `i < N` on a strength-reduced table cursor is `jle`
  against `&tbl[N-1]` versus `jl` against `&tbl[N]`** — the one instruction
  between exact and not in three loops.
- **`fsub st(3)` (against a value already on the x87 stack) needs a DISTINCT
  variable AND a free volatile read at the copy;** `acc = saved;` is
  copy-propagated and folds to `fsub dword ptr [mem]`, six bytes longer (all
  eight subsets of three such reads measured). **A free volatile read of a
  memory-homed loop variable at a multiply frees a callee-saved register for a
  hoisted global** — 86 -> 46 and the whole instruction count.
- Negative, measured at two sites (`Route_StepFree`, `Route_StepToPieceEnd`):
  **the shared `lea` base register for a three-scalar snapshot group is
  unreachable from C** — pointer, array, walking-cursor, struct-copy and
  volatile spellings all fold back to `esi + disp`. **`row` and `ylast`
  holding each other's frame homes** (`ZBuffer_FillPoly`) was recorded here
  as a floor after 135 statement orders, 11 declaration orders and six
  volatile reads — WRONGLY: it fell on 2026-09-08 to the aggregate
  frame-home rule at the top of this section. A tie-break that resists every
  spelling of the SCALARS can still move when one of them joins an
  aggregate; retire a frame-home floor before trusting it.
- Mechanics worth knowing across the coaster: the route's physics object is an
  **RK4 solver descriptor** (nodes 0, 1/2, 1/2, 1; weights 1/6, 1/3, 1/3, 1/6)
  over a car-shaped state vector; a piece boundary is landed by BISECTION
  (~16 probes, each restoring the whole train); `TrackJoint`'s +0x00 is a
  four-way DIRECTION bit, not a height (coaster.c's name corrected in
  `coaster5.c`, offsets untouched); and `if (i == 0x1d) i = 0x1d;` in
  `FastRSqrt_InitTables` is a breakpoint hook the developers left in.

- **A missing callee-saved push is a LIVE-VALUE deficit, not an allocation
  problem.** `LFPath_Point` was 64 instructions with three pushes; naming
  `j = i + 1` BEFORE the branch (used one way per arm) made five values live
  across the join and `ebp` appeared — 69/69, byte-exact. When the original
  has one more push than you do, look for a value that should be computed
  before the split rather than inside each arm.
- **`n - i - 1` and `n - 1 - i` are different objects.** Variable-first
  subtracts into a copy of `n` and folds the `-1` into a `-8` displacement, so
  ONE `path->n` read serves both arms; constant-first must materialise `n-1`.
  Related: **a loop guard comparing `i + 1` against a count, with no decrement
  anywhere, proves the source counted from `i + 1`** — the `j < n - 1` spelling
  materialises `n-1` and rebuilds the whole surrounding allocation.
- **A `switch` whose cases 1 and 2 share a body and whose case 3 is separate
  lowers to a RANGE chain** (`test/jle`, `cmp 2/jle`, `cmp 3/jne`), not a jump
  table — VC6 clusters the sorted values into [1..2] and [3..3] and lays the
  last cluster inline. An `if (k == 1 || k == 2)` chain gives `cmp eax,1 / je`
  instead, so the range test is the tell. And **a jump-table bound can reuse
  the outer case constant** (`cmp ecx,eax / ja` against the register still
  holding 3), which is why the bounds check is unsigned.
- **Two globals whose parallel arrays are walked in lockstep must be ONE
  struct** — confirmed at a new site from BOTH symptoms: as separate externs
  VC6 builds two induction variables, keeps the count in a register, reloads
  the PARAMETER from its slot each iteration and pulls in a fourth push for a
  hoisted zero; as one object it runs a single cursor and reloads the count
  (the alias kill). 59i/172B -> 53i/155B (`RemoveNewObjectMarker`).
- **VC6 anchors a strength-reduced cursor on the element indexed by the PLAIN
  induction variable, and the anchor drags the counter's placement with it.**
  `spr[j-1] = spr[j]` and `spr[j] = spr[j+1]` both anchor on `spr[j]`. Renaming
  to fix the anchor makes the other index a DERIVED IV, which moves the copy
  below the guard, sinks the `inc` to the end of the body and turns a `lea`
  into a `mov`. Named pointers, lockstep spellings and free volatiles do not
  separate the two decisions — the anchor and the schedule are one. (The
  5-mismatch residual of `RemoveNewObjectMarker`, ~40 builds.)
- **`while (strlen(p))` under `#pragma intrinsic(strlen)` needs no compare**:
  the inlined `not ecx / dec ecx` leaves the length in the flags, so guard and
  latch are a bare `je`/`jne`. **`test byte ptr [reg-4], al` with `al == 1` is
  a fused return constant** — the `mov eax,1` in a preheader is the `return 1`
  value materialised early and reused as the flag mask.
- **`if (param == arr[i])` versus `if (arr[i] == param)`** decides
  `cmp reg,mem` versus `cmp mem,reg` — same length, one mismatch, readable off
  the listing (third instance of the compare-operand-order rule).

- **An 8-byte struct RETURN must be the ACCUMULATOR of any sum added to it.**
  `p.x += (...) << 8;` keeps the return's own eax/edx as the `add` destination
  and leaves the y half untouched in edx across the whole x block.
  `b->target.x = p.x + (...)` — and its reversed operand order, byte-identical
  — makes the shifted sum the destination, forces a `mov ebp,edx` to save p.y,
  and is one instruction long. The whole residual of `SpaceTower_StepAnim`;
  19 spellings measured.
- **"Read a class/def global directly at every use" has a STORE-ORDERING
  caveat.** As a named `RideDef* def` local the pointer takes a SCRATCH
  register and pushes the tile byte onto edx; as VC6's own CSE temporary it
  takes EBP — the original's choice. But that CSE only survives if both sums
  are computed BEFORE either store to the object being written; with the
  stores interleaved the global is reloaded (16 mismatches). Compute, then
  store.
- **A three-way choice: nested `if` versus an `else if` chain decides WHICH
  arm is inline.** `if (dy == 0) A; else if (dx >= 0) B; else C;` puts A inline
  and exiles B and C. When the original's inline arm is C, only
  `if (dy != 0) { if (dx < 0) C; else B; } else { A; }` produces it — and it
  lays the two cold blocks in the original's order too (`BsBoat_StepLeg`).
- **`for (i = 0, n = 0; ...)` — the comma operator orders the two `xor`s**
  (counter first, then accumulator); `n = 0;` as its own preceding statement
  reverses the pair. **A 16-bit compare's operand order is readable off the
  listing**: `cell->owner.w != st->key.w` gives `cmp cx,[ebx]`; the other way
  round gives `cmp [ebx],cx`.
- **The aggregate/dead-parameter-slot rule, confirmed forward and worth the
  whole frame.** Two `turn` deltas as plain `int`s let VC6 home the spilled one
  in the freed `b` argument slot and the frame vanishes; as one `Pos` it takes
  a fresh slot and the frame is the original's `sub esp,8` — every instruction
  after index 0 was shifted by one until that changed (`BsBoat_StepLeg`, 294).
  And a caution on the zero-web threshold: eighteen literal zeros in
  `Restaurant2_NewRecord` hoist `xor ebx,ebx` as expected, far past it.
- **Tooling note:** `matchfull.py` over-reports a function whose `ret` is
  followed by a `.rdata` jump table (295/301 for `BsBoat_StepLeg`); `audit.py`
  bounds it correctly (294/294). Trust the gate.

- **An explicit `& 0xff` and a `(unsigned char)` cast are different objects —
  the REVERSE of the recorded char-local narrowing rule.** `PaintPathRect`'s
  parity step is `neg dl / sbb edx,edx / add edx,2 / and edx,0xff`: the ternary
  is evaluated 32-bit and narrowed AFTER. Either cast spelling (a byte local or
  a cast at the use) propagates the byte width BACKWARDS into `sbb dl,dl` +
  `movzx` — 2 wrong at identical length. `step & 0xff`, `(unsigned)step & 0xff`
  and an `unsigned short` intermediate all reproduce the original. Related:
  **`x + y` must stay an `int` to become its own induction variable** — as an
  int VC6 strength-reduces it into `lea esi,[eax+edi] / inc esi` and pushes a
  fourth callee-saved register; narrowed to a byte it recomputes
  `bl = dl / add bl,al` every iteration, three instructions short.
- **A value that must survive a call in a callee-saved register cannot be a
  FIELD of an address-taken aggregate.** Reading two shifted scrolls back out
  of a by-address `Pos` costs two reloads and merges the `add esp,8` into the
  epilogue (36 of 42); as two plain `int` locals assigned INTO the `Pos` they
  take esi/edi across the call and it is exact (`RenderGroundLayer`).
  Corollary: two reads of the same u16 global field do NOT CSE across a store
  to the address-taken aggregate — read the field back
  (`view.right = g->view_w + view.left`) for the original's single load.
- **Inlining a two-argument helper on two byte fields of one pointer:**
  `CellAt(at->x, at->y)` makes the POINTER the first IR temporary and it takes
  eax; hoisting the fields into `int x, y` locals first pushes the pointer down
  the rotation to ecx, where the original has it (7 of 41 at identical length).
  The rotation is set by which value is the first temporary.
- **Independent statements come out reversed** (second measurement): the two
  `sub`s of `SetPathSquareDistance`, and the two `imul`s that follow them, are
  emitted opposite to source order, while the `a*a + b*b` sum's operand order
  is inert. **`or ah,1` / `or dh,2` on a DWORD flags global is `|= 0x100` /
  `|= 0x200`** — the u16 byte-narrowing extended to a 32-bit destination.
  **A counted loop bounded by a LOCAL gets a down-counter with no reload;
  bounded by a STRUCT FIELD it reloads every iteration and counts up** — both
  in one file (`FreeAnim3D` / `LLIDB_UnLoadODFData`).
- **Caller-side extern types, two more instances, do not align:**
  `g_path_tile_base` needs `unsigned short*` in `sysmisc3.c` (the original's
  `add dx,[ebx]` is a 16-bit add) where pathsq.c declares `int*`; and two
  functions are MISNAMED by their callers' externs and kept so for resolution —
  0x0047b7b0 is not a loader but an ICM error reporter (a six-arm switch
  raising `MessageBoxA`), and 0x0047cdd0 unloads .TSF, not ODF, so LLIDB
  element type 0x40 is .TSF. Flagged in `sysmisc3.c`'s header.

- **`rep stosd` is NOT proof of `memset`.** VC6 SP3 turns a constant-count
  array fill into `rep stosd` even when the fill value is a non-zero ADDRESS
  constant: `for (i = 0; i < 0x400; i++) tab[i] = &fallback;` is
  `mov ecx,400h / mov eax,OFFSET / mov edi,OFFSET / rep stosd`
  (`CoasterShades_Init`, exact first try).
- **A strength-reduced record cursor is anchored at the field with the MOST
  references; ties break to the LAST reference; and offset 0 never wins from
  a `->` or subscript access.** All six read orders of a three-field row
  measured on `InitTrackDrawModes`: three orders anchor at +8, three at +4;
  only a DUPLICATED `e->in` reference reaches +0, and it permutes the loads.
  Confirmed forward on `ZBuffer_RunCommand`: writing `edge[i].side` in BOTH
  arms anchors that cursor at +4, naming `c->v[i].y` three times anchors the
  other at +2 — 16 of 53 together. Refines "VC6 biases the IV to the MIDDLE
  of the accessed offsets": it is the reference count, not the midpoint.
- **A struct copy from a global whose fields were just assigned constants lets
  VC6 constant-fold the FIRST field read back through the `rep movsd`.**
  `g_base = g_tmpl; g_base.m[0] *= 0.5f;` becomes one
  `mov [m0], 0x3f4d3a3f`. ONE free volatile read on that field defeats it
  (the original loads there anyway); `memcpy` and an inline `Half(float*)`
  helper still fold. The whole residual of `Coaster3D_ResetScene`. Same
  mechanism as "struct-copy forward-propagation picks the FIRST field read".
- **Declaring a list-walk pointer in EACH ARM'S OWN SCOPE splits its web and
  flips the callee-saved ranking.** As one function-level local it out-ranks
  the `out` parameter and swaps esi/edi through 24 instructions (36 of 60,
  register-blind 0); two block-scope declarations give `out` esi, and VC6
  still hoists the shared load above the branch. 60/60 (`WriteCoasterNodes`).
  Scope as a ranking lever, from the other direction.
- **Explicit parentheses stop VC6 reassociating FP constants.**
  `(int)x * 0.2 * 5.0f` folds `0.2 * 5.0` to exactly 1.0 and emits NEITHER
  multiply; `((int)x * 0.2) * 5.0f` keeps both (94 -> 104 on
  `DrawSupportShadow` — and the cast placement there is an original bug: it
  truncates to whole units instead of snapping to the 5-unit grid). **A
  double-typed intermediate pools the second constant as a double**
  (`fmul qword`) where the float-typed one gives `fmul dword ptr [5.0f]` and
  also swaps which operand of the following adds is `fld`ed (106 vs 111/111).
- **"Float source order is the REVERSE of `fld` order" does not reach an
  accumulator site.** `acc += step`, `acc = acc + step` and `acc = step + acc`
  are byte-identical (all `fld step / fadd acc`); only a free volatile read of
  `acc` puts the `fld` on `acc`. **An embedded assignment `c = pa - (m = e) *
  a;` is what produces `fst` (store-and-keep)** rather than `fstp` + `fld`.
- **A snapshot handed to a float callee must be a RAW DWORD with an `int`
  prototype.** As `float` locals the snapshots get stack homes, the loop
  counter takes edi and 17 instructions move; as ints they take edi/ebx across
  the call and the counters spill, matching the original (`Route_StepFree`).
  Related: **a context field zeroed as an `int` merges with `i = 0` into one
  hoisted zero register**; typed `float` and zeroed with `0.0f` it keeps both
  stores immediate and the guard `test eax,eax`.
- **Two tables pushed as `push OFFSET` are ARRAYS, not pointer variables** —
  declared as `void*` scalars the call loads their contents and costs four
  instructions.

- **A `Pos` by-value parameter where the caller declares two ints — third
  confirmation, new mechanism.** Same `__cdecl` ABI, but as an aggregate
  member `tile.y` becomes the DESTINATION SYMBOL of its own sum (`add edx,ecx`
  not `add ecx,edx`) and the `mov ecx,edx` argument copy comes back: 33 of 56
  -> 0 of 57 on `Restaurant1_WalkToSeatSpot` after ~60 other variants. The
  caller's four-int extern is left alone. So the by-value `Pos` is three
  levers in one — left-to-right schedule, an extra definition for the
  rotation, and the destination-symbol rule for its fields.
- **A coordinate pair computed into ONE `Pos` aggregate LOCAL, not straight
  into the struct fields.** Written into `b->target.x`/`.y` directly, VC6
  completes and stores x before it even loads `node.y` (20 of 60); as two
  plain ints it hoists both parameter loads and grabs EBX (63 instructions);
  as one aggregate it evaluates x, y, both shifts, both stores — exact
  (`WalkPath_Advance`). The aggregate placement rule from the value side.
- **Name the intermediate POINTER into a const table**
  (`PanSlot* s = &g_pan_slots[b->pan];`) to get the original's `lea` plus
  scaled-subscript pair while VC6 still re-derives the other field as a
  subscript: 6 -> 0 on `MoveToPanEdge`, and it transferred to
  `StandUpFromPan` first try. **A flat `int` table versus a struct array is a
  scaling lever**: `[eax*4 + disp32]` with the index scaled by 6 is
  `const int tbl[]`; a 24-byte struct array scales by 3 and addresses
  `[eax*8 + disp32]`, one instruction short.
- **A float read into a local EARLY is what schedules its `fld` early.** Read
  at the use site the `fld` sinks 20 indices and VC6 pre-scales the index into
  a register (`shl eax,3` + `[eax+base]`); as a local it stays at index 7 with
  `[ecx*8+base]`. 22 -> 6. Same family as "values that must survive a call
  have to be NAMED LOCALS read before it" — read at their use sites they load
  after the call and VC6 pushes one callee-saved register fewer (confirmed
  again on `Restaurant1_WalkToSeatSpot`, indices 1..7).
- Negative, measured on `GoldRush_KneelAtPan` (95 variants): **a store cannot
  migrate across a call, so a field store AFTER a call proves the source
  statement is there — but making the stored value SURVIVE the call in a
  callee-saved register (one web from load, across `__ftol`, to store and
  argument copy) is not reachable from C.** The closest point (58/58, right
  prologue and store positions, 26 mismatches) needs a local plus a volatile
  read and was not adopted.

- **The zero-web THRESHOLD is between three and five literal zeros — and
  below it, the placement of a `rep movsd` decides.** `BoatingSchool_Update`
  has three literal zeros: with the struct copy interleaved among them VC6
  hoists `xor esi,esi` (esi is pushed for the copy anyway) into a chain
  store's slot (32 of 46); finishing the store chain BEFORE the copy leaves all
  three as immediates (46/46). Its sibling with FIVE zeros carries the zero
  register in the original. Sharpens "VC6 hoists a constant zero into a
  callee-saved register only when that register is pushed anyway": the push is
  necessary, the count decides, and near the threshold a copy's position tips
  it.
- **The push-sinking rule needs ONE `return K` in the guarded block, not
  two.** `return 2;` in both inner arms puts `push esi` back in the real
  prologue and forces `mov al,[esp+8] / test al,2`; an if/else with a single
  trailing `return 2;` lets VC6 tail-duplicate the epilogue itself and both
  pushes sink (55/55, `BuildObjectIconInput`). Sharpens the wave-fourteen entry.
- **Walk a list through its head GLOBAL to get the 5-byte accumulator-form
  store.** A local cursor with `g_head = p;` stores from a register (6 bytes);
  ~20 loop spellings floored one byte long. `while (g_head) { next =
  g_head->next; free(g_head); g_head = next; }` makes VC6 forward the
  just-stored head into eax and `mov [imm32],eax` falls out — exact.
- **A named sum BLOCKS VC6's algebra.** `step += limit - (base + step)`
  written inline folds to `step = limit - base`, merges two clamp blocks and
  loses four instructions; `t = base + step; if (t < limit) step += limit - t;`
  keeps the original's unsimplified `sub/add` pair. The temporary prevents an
  optimisation here rather than enabling one — the mirror of "a named local is
  not a duplicated expression".

- **Size is NOT evidence of twinning — diff first.** `Sub_411680` and
  `Sub_411810` are both 121 instructions and share nothing but a boat pointer
  (`LFBoat_Advance` vs `LFBoat_Fall`). And **a twin's block layout is a
  hypothesis, not an inheritance**: `LFPiece_HasCursor` needs
  `if (s == 0) { one } else { walk }`, the INVERSE of its twin
  `LFPiece_HasRider` (41 -> 49 of 49). Same-size and same-family both earn a
  diff, not a copy.
- **Block-scoped `int tw, th;` — one pair per block — is what homes BOTH in the
  dead argument slots;** one function-scope pair homes only `tw` and shifts
  the whole frame. Worth 98 of 299 on `LFBoat_Draw`. The scope decides the
  colouring, so declare the out-params where they are used.
- **A by-value `Pos` argument takes one more register DEFINITION than
  `(int, int)`, and that definition advances the scratch rotation for the rest
  of the function.** Closed the last 17 of 299 on `LFBoat_Draw`; all four
  `(int, int)` operand orders are byte-identical, so it is unreachable by
  permuting sums. Extends the `LFRun_Tick` entry (there the payoff was the
  left-to-right SCHEDULE; here it is the ROTATION).
- **Naming a pointer that a call kills pins a callee-saved register AND the
  frame.** `Person3D* person = b->rider->person;` took `LFBoat_Draw` 132 -> 48.
  The named pointer is reloaded once and kept; inline it is re-derived after
  every call.
- **A free volatile read can be used to REMOVE a CSE**, not only to advance a
  rotation: `*(LFPiece* volatile*)&b->piece->fwd` at a guard keeps `fwd` out
  of a register and drops a fourth callee-saved push (66 -> 63 instructions).
- **An uninitialised float local reads as an `fld` of a live local's slot in a
  loop preheader.** `gap = z;` before the loop schedules the `fld` one early
  and the exit `fstp` two late; leaving `gap` uninitialised is exact (an
  original bug reproduced — the `||` guard makes it unreachable).
- **`if ((nz = a + b) > K)` — an assignment fused into a float condition —
  emits `fst` + `fcomp`; two statements emit `fstp` + `fld` + `fcomp`.**
- **A two-case `switch` is not `if / else if`**: `dec/je/dec/jne` with case 2
  as the fall-through, against `cmp eax,1 / jne` with the opposite block order
  and a different register for the three `= 1` stores.
- **VC6 DOES reassociate a float chain of two literals, and a two-step float
  local blocks it:** `(t - 2.0f) * 0.5f * 120.0f` folds to `* 60.0f`; a local
  holding the first step keeps both multiplies (worth 2 at two sites). The
  same local also picks `fiadd` over `fild` + `faddp`.
- **The packed `{u8,u8}` dword-plus-mask idiom applies to a `BPosW` LOCAL**,
  not just a by-value parameter slot. **`bp = b->piece;` placed BEFORE a flags
  test splits `test byte ptr [mem],K` into `mov al,[mem] / ... / test al,K`.**

- **FROM THE PARALLEL SESSION `fable-b` (13 of 18 exact; full evidence in
  `docs/lanes/fable-b*.md`). The strongest, folded here:**
  - **A shared tail that RELOADS locals from the frame proves every arm left
    them in memory — force it with a volatile STORE through a cast in every
    arm.** Written naturally, VC6 keeps the values in registers, DUPLICATES
    the ten-instruction tail into every switch arm and emits 85 instructions
    for a 66-instruction function; `*(int volatile*)&p.x = e;` in each arm
    makes the arms end identically, VC6 cross-jumps them into ONE tail, and it
    is 66/66 with the arm layout and the `ja` default exact. Passing the pair
    by value, an inline reader helper and a `goto` join all stay at 85.
  - **Two written-out copies of a tail merge only if INSTRUCTION-identical,
    and store-forwarding of a just-stored global defeats that.** One arm
    forwards (`push edi`) where the other reloads (`mov ecx,[head]`), so no
    merge. A volatile STORE does NOT stop the forwarding, nor does an array
    view at the same offset. What works: a PAIR of volatile reads hoisted into
    locals in the original's load order — with only one of the two volatile,
    the loads cannot both hoist above the first push, and a single inline
    volatile read is always emitted FIRST in its block regardless of statement
    position (that last fact is the whole residual of `UpdateSampleSource`).
  - **`xor <callee-saved>,<same> / mov eax,<it>` proves a two-predecessor
    JOIN, never a straight-line `v = 0; return v;`.** VC6's constant
    propagation is global: `L: head = 0; return head;` as the last statement
    becomes `xor eax,eax` cross-jumped into a neighbouring `return 0` and the
    block VANISHES (124 -> 117), surviving every disguise (`return head = 0`,
    a cast, `memset`, `head = prev` where `prev` is known zero, a label or
    `goto` split). Also: VC6 lays cold blocks in source-generation order, and a
    single-predecessor `goto` target is generated INLINE with the branch that
    reaches it — moving the label to the end of the function does not move
    the code.
  - **A `while` loop's condition is whichever test VC6 leaves in the LATCH;
    the peeled copy doubles as the enclosing `if`.** `if (key < cur->key) {
    while (key < cur->key) { if (!cur->prev) break; cur = cur->prev; } }` —
    the redundant re-test is the point: VC6 peels it, the peeled copy IS the
    `if`'s compare, and the `else if` reuses its flags. The other way round
    (`while (cur->prev) { advance; if (key >= cur->key) break; }`) peels the
    NULL test and costs three per arm (83 vs 77/77); `for (;;)`, `do/while`
    and backward `goto`s are all normalised to the wrong-way form. Read the
    latch off the original to find which test the loop was written on.
  - **A search's failure arm as the `else` of `if (p)` keeps ONE copy;
    `if (!p) { ...; return; }` emits TWO** — the loop-exhausted edge proves
    `p == 0`, VC6 folds the test on that path and duplicates the block rather
    than jumping (10 instructions, `UnlinkGardenerOrder`).
  - **An aggregate local blocks reuse of a DEAD PARAMETER's home slot.** Two
    plain `int`s let VC6 home the spilled one in the freed argument slot and
    collapse the frame to `push ecx`; as one `Pos` it takes a fresh slot and
    the frame is the original's `sub esp,8` (11 of 64, the only residual).
    Counterpart of "two argument slots reused as locals keep an 8-byte frame":
    when the original's frame is bigger by exactly one spilled scalar, that
    scalar was inside an aggregate.
  - **An ASCENDING array walk anchors its induction variable on the LAST field
    touched; a DESCENDING one on the FIRST — and the spelling must follow the
    anchor.** Up: the pointer starts at `&wob[i].y` and stores through
    `[eax-4]/[eax]`; only two per-field assignments produce that (a
    whole-struct copy, a `*p++` walk and a y-then-x pair all anchor on `.x`).
    Down: `[eax]/[eax+4]`, which is the whole-struct assignment. Measure per
    loop, by direction. From the read side: a const-table cursor must be a
    POINTER, and sometimes a BIASED one — `const int* spot = &tbl[0].y;` read
    as `spot[-1]/spot[0]` and stepped `spot += 2` is exact where every
    `const Pos*` spelling is 2 or worse.
  - **A duplicated cursor-advance block beats the `&&` that would merge it.**
    Two guards each ending in `continue` give the original's two inline
    copies AND the extra references that rank the locals into edi/ebx; one
    `&&` guard emits a single shared miss block reached by `jne`, seven
    instructions short (19 of 70). The exile rule's other side.
  - **Naming the intermediate POINTER (`d = e->data;`), not the value, advances
    the scratch rotation** — a `short`/`unsigned short` value local does not,
    an `int` one is worse, and a volatile read of the pointer is
    byte-identical to the named pointer (on that body the free-volatile trick
    and the named pointer are the same lever). The twin with ONE compare needs
    no local: **the first counter-example to "twins share their residual index
    for index"** — they share source and diverge exactly where register
    pressure differs.
  - **"Restore, then test" is a named call result.** `ok = SaveGameWrite(...);
    ev->time += g_now; if (!ok) return 0;` schedules the restore between the
    call and `test eax,eax`; `if (!SaveGameWrite(...))` puts the `add` after
    the branch. **Two arms calling the same function are two textual calls,
    not a ternary argument** — each arm's own `add esp,4` is the tell.
    **`memset` the whole struct then store the one non-zero field** gives
    `rep stosd` followed by the lone store after the pushes.
  - **`test byte ptr [mem],1` on byte +3 of a word is `& 0x100` on the
    `unsigned short`**, not `& 1` on a byte field (the byte field costs a load
    and slides ahead of a pending `add esp`); the read side of the recorded
    `|= 0x100` narrowing. **Reading two fields into locals BEFORE an
    intervening call is what puts them in callee-saved registers** (inline in
    the later expressions: loaded after the call, one push fewer, 58 of 74).
    **VC6 homes an address-taken out-pointer pair in the caller's incoming
    argument slots** once both parameters are root-copied — read `[esp+N]` by
    push depth or it looks like a wild write into the caller's frame.
  - **`sete al / test al,1` is `((x == 0) & 1) == 0`, and the `== 0` half is a
    LAYOUT lever** (success inline, failure to the ONE trailing `return 0`;
    `if ((...) & 1) goto fail;` inverts and parks the failure block mid-body).
    **`sub esp,N` with `mov [esp+k],imm` stores and no matching `add esp` is
    a local aggregate initialiser** (`WinRect dst = { 0, 0, 640, 480 };`), not
    an argument list. **`mov esi,[__imp__X] / call esi` in a loop needs no
    construct** — VC6 hoists the IAT load itself.
  - **Two near-identical if/else arms were written out IN FULL, error handler
    and tail included — the layout is the proof.** A shared block sandwiched
    between two arms was written twice (VC6 cross-jumps the copies and parks
    the survivor after the first loop); written once, it lands between the
    arms and each arm loses its own inline guard (90 vs 92/92).

- **Two lockstep cursors: spell them the SAME way to let VC6 eliminate one
  induction variable; spell one as a subscript and the other as a walk to keep
  both.** This is the REACHABLE half of the recorded "`sub esi,ecx` +
  `[esi+ecx]` is not reachable by loop spelling" entry — that entry swept loop
  forms, not the cursor spellings. Found forward on `TransformVerts` (8 -> 0,
  and it also un-commutes the multiply) and applied backwards on
  `Coaster3D_DrawMesh` (17 -> 8).
- **`while (n-- > 0)` is a counted loop with its own trip-count
  materialisation.** VC6 evaluates the guard on the pre-decrement value and
  rebuilds the trip with `inc`: `mov ecx,eax / dec eax / test ecx,ecx / jle /
  inc eax`. `for (i = 0; i < n; i++)` loses the dec/inc; `i <= n-1` gets them
  but tests `n-1` with `jl`. Nine spellings measured. Sibling of the unsigned
  `while (n-- != 0)` idiom already recorded.
- **A degenerate `cmp/jne +0` comes ONLY from a self-assignment arm.**
  `if (n == 5) n = 5;` survives as the test with an empty jump; `n = n;`,
  `{}`, `;` and a dead local store are all deleted whole, and an if/else with
  two identical calls merges with NO branch. (An original bug in
  `Coaster3D_BuildPieceGeometry`, reproduced.)
- **Three distinct in-place x87 idioms, one per site, none of them a C cast:**
  `fild x / fstp x` (int -> float in place), `fild x / fmul k / fistp x`
  (scale-and-round in place), `fld f / fistp i` (float local -> separate int).
  Worth 24 bytes on `Raster_SubmitPoly`; a float temporary at the wrong site
  costs two instructions each time.
- **A `volatile float` LOCAL is how a loop-invariant FP constant gets a stack
  home.** Plain `float k = 65536.0f;` is constant-propagated into the `.rdata`
  pool and loses the original's `mov [ebp-0x40],0x47800000`; the volatile is
  free because the original reloads it every iteration. That one store made the
  body byte-exact.
- **Two float locals force `fild/fild/fsubp` where `(float)a - (float)b` folds
  into `fisub`** — the whole instruction count on `Coaster3D_SetupView`.
- **ONE free volatile read on the FIRST field read back after a struct copy
  pins the whole group after the `rep movsd`.** Without it VC6
  forward-propagates and hoists all four loads above the copy (30 -> 10);
  making all four volatile is WORSE (21) because each load then stays glued to
  its own store instead of being batched. The barrier's value is in batching,
  so use exactly one.
- **`&arr[n++]` in a call argument advances the cursor BEFORE the call**,
  forcing the `mov reg,cursor / push reg` copy the original has — but it is
  per call site (right in one loop, worse by 4 in the next).
- **Compute both components of a coordinate pair before storing either** (x,
  y, store, store — not x, store, y, store): that is what lets VC6 fill the
  first conversion's x87 latency gap (16 -> 10).
- **Hoist a shared multiple into ONE local and derive the other from it**
  (`six = 6*s; ... 3*six`): 75 -> 53 on `InitTrackTopology`, where hoisting
  both into their own locals is worse (80).
- **TOOLING DEFECT — FIXED 2026-09-05 (`_loop_entry` in `tools/match.py`):
  the extent walker stopped at an unconditional `jmp` that no EARLIER branch
  crossed — but a rotated loop's entry `jmp` in the middle of a function is
  exactly that.** The walker now peeks past a forward `jmp`'s target for a
  branch back into the skipped region and, if one exists, treats the `jmp` as
  a loop entry; a tail-jmp wrapper never satisfies that test. `MatMul` went
  from a truncated 10i/28B ESCAPES to 40i/103B exact and was promoted;
  `Coaster3D_BuildPieceGeometry` is now bounded at 143i/528B. Original text
  of the defect follows.** The walker ignores
  the jmp's OWN forward target, under-bounds the function, and then reports
  ESCAPES against the truncated extent. `Coaster3D_BuildPieceGeometry`
  (0x004284d0) can never print `[OK]` until this is fixed, although ours
  matches the truncated extent exactly. Fix sketch: when a forward
  unconditional jmp is the candidate end, peek past its target for a backward
  branch into `(jmp_end, target]` — a loop body — and extend if found; a
  tail-jump wrapper has no such back-edge.

- **An ADDRESS-TAKEN counter turns VC6's strength reduction OFF.** Reusing one
  `int n` (whose `&n` is passed to a reader) for both the counts and the loop
  counter forces it into memory: the counter is reloaded each iteration and
  `i*3 / lea [base + i*4]` recomputed, instead of VC6 giving it EDI and
  manufacturing an EBX induction variable for `i*12` — and EDI's push splits
  into a guarded path. Worth **90 of 93** on `LFAnim_LoadRefs`. If a loop's
  counter is reloaded from the frame every iteration, look for `&counter`
  somewhere else in the function.
- **A 16-element shift of 8-byte structs is sixteen SEPARATE assignments.** A
  `for` loop compiles to 19 instructions (VC6 SP3 does not unroll), an
  intrinsic `memcpy` of the same 128 bytes to `rep movsd`; only the written-out
  statements give the original's 71, each `mov edx,[base+src] /
  mov [base+dst],edx`. (`SchoolCarIdleStep` — and the sixteenth copy reads one
  past the array, an original bug reproduced.)
- **Whether a zero float is an INITIALISER or a STATEMENT decides which of two
  floats takes the dead argument slot.** `float acc = 0.0f;` stores at the top
  of the IR and gives `acc` a real frame slot; the same as a statement AFTER
  the owner load lands at the original's index and moves `acc` into the dead
  parameter slot, freeing `[ebp-4]` for the other float. One statement earlier
  does neither (16 vs 0, `Route_AdvanceTrain`).
- **Float source order is the REVERSE of `fld` order.** VC6 `fld`s the operand
  written SECOND in a two-term float sum: `(rear + front) * 0.5f` loads
  `front` first. All six mismatches on `RouteCar_SetPosition`. Same family as
  "adjacent address stores come out reversed".
- **`((float)now - rt->started) * K` emits `fild/fisub`** — the subtraction is
  done in float with an integer memory operand; `(float)(now - started)` emits
  an integer `sub` plus one `fild`. Read which from the presence of `fisub`.
- **A value used in both arms of an `if` must be READ IN BOTH ARMS to get one
  head-merged load in the right place.** `s = out->z;` before the `if`
  schedules its `fld` four instructions early (32); as the first statement of
  each arm, VC6 head-merges the two loads into one `fld` scheduled into the
  `fcomp/fnstsw` gap — the original.
- **A two-value float swap must be written with NO temporary.** `out->z =
  out->x; out->x = -s;` is exact; a `float` temp gets a stack home (spill plus
  copy), and an `int`-bits temp CSEs with an integer sign test on the same
  address and degrades `test dword ptr [mem],K` into `mov` + `test reg`.
- **A switch-produced index masked at the USE (`and eax,0xff`) is an `int`
  subscripted through an `(unsigned char)` cast**, not an `unsigned char`
  local — the byte local makes the switch arms byte moves (45 of 65).
- **A bubble sort's inner guard reads its form off the condition kind:** `jle`
  (signed) pins `for (k = 0; k < i; k++)`; `je` says `for (k = i; k != 0;
  k--)`. The swap's store order (`out[k] = out[k+1]` before `out[k+1] = t`)
  only comes out right in SUBSCRIPT form — through a walking pointer VC6
  reverses the pair.
- **Initialiser order, extended to four locals**: all 24 orders of
  `LFRun_LoadPieces`'s initialisers measured; exactly one is exact (`tag, p,
  head, prev`), the natural order costs 3. Sweep it — it is 24 compiles.
- Negatives recorded so they are not re-derived: **a mixed two-axis schedule**
  (`SchoolCarBlockedAhead`: ux chain first, y difference first) has exactly two
  regimes decided by which axis the source writes first, all 40 legal
  interleavings hit one of them, and the barrier that would mix them re-ranks
  the register it sits on; **a DEAD reload of a global at a loop exit** (VC6
  restoring a hoisted global with no consumer) is not reachable from twelve
  sort spellings, an inline helper with or without the count, or a free
  volatile read.

- **THE MECHANISM behind the free-volatile lever: a volatile access cannot
  cross the callee-saved pushes (or any store).** That makes a loop-guard
  global load a placement lever: `for (i = 0; i < *(volatile int*)&g_count;
  i++)` — without it VC6 hoists the guard load to index 0, ABOVE
  `push ebx/ebp/esi/edi`; the volatile pins it to index 5, where the original
  has it. 9 of 67 at identical byte length on `FindCachedText`, eight loop
  spellings all floored at 9; free because the original reloads the count in
  the latch anyway.
- **VC6 biases a strength-reduced array induction variable to the MIDDLE of
  the accessed offsets, not to the first access.** A walk over fields at
  +8/+0xc/+0x10 runs its IV at +0xc; one over +8..+0x18 runs at +0x10. Read
  the record framing off the midpoint — an IV that looks like "the record
  starts 0xc bytes in" is just the bias.
- **A push sinks past a leading guard only when the guarded block ends in its
  OWN `return K`.** `if (ev & 2) { ...; return 1; } return 1;` gives the
  original's `test byte ptr [esp+8],2 / je end / push edi` plus a bare
  `mov al,1 / ret` at the end; `if (ev & 2) { ... } return 1;` pushes at entry,
  merges the early exit into an existing epilogue, and comes out ONE
  instruction short — and it also turns `test byte ptr [esp+8],2` into the
  two-instruction `mov al,[esp+8] / test al,2`, because with the push at entry
  the argument must be read before esp moves. `if (!(ev & 2)) return 1;` and a
  `goto` are both wrong (94/100). Sharpens the recorded split-prologue rule.
- **A state-copy local with a guard: copy from the global, then test the
  GLOBAL again.** `state = g->f00; if (state == 2 || state == 3)` makes VC6 emit
  `mov al,cl` and split the web; `state = g->f00; if (g->f00 == 2 ||
  g->f00 == 3)` keeps ONE register for both compares, the local store and the
  loop's peeled first read. And assign the loop head (`p = g_list;`) BEFORE
  the guard or its load will not sit in the push block. Together the whole
  head of `UpdateSidePanelScroll` (51 -> 14).
- **`p->x += step;` on a short field is not the same object as naming the
  sum.** A `short sx` for `p->x + step` swaps which of {the widened field, the
  sum} gets EAX, the field then dies early and four more instructions move —
  14 of 121 at IDENTICAL byte length, with every sum spelling, width, sharing
  and narrowing variant inert. Only deleting the named temp (`p->x += step;`
  with the following test re-reading `p->x`) reaches 0. Same family as "a
  duplicated expression is not a named local": the temporary is the object.
- **A three-way colour choice around a by-value rect is THREE textual calls,
  not one call with a colour variable.** The original emits TWO copies of the
  rect argument block (one built through edx, one through ecx) with only
  `push font / push text / call` merged; a jump-threaded constant into one call
  would give ONE shared block. Two argument blocks in different scratch
  registers = two source calls whose registers diverged before the merge.
- **A parameter's TYPE can be the whole function: `char` where the caller
  declares `int`.** `UpdateSidePanelScroll(char step)` — every use is `movsx`
  from `bl`; an `int` parameter costs 105 of 121 and five instructions.
  ABI-identical under `__cdecl`; the caller's `int` extern is left alone and
  the divergence noted at the marker.

- **"A free volatile read moves nothing => global web rank => floor" is a
  STRONG signal, NOT a proof — corrected 2026-09-05.** In
  `BoatingSchool_Destroy` and `JungleCruise_Destroy` free volatile reads on the
  list head, count and sprite globals were all inert, and twelve loop
  respellings too — yet naming the array element in a local
  (`spr = ilf->sprites[(unsigned char)i]; LLSStop(GetLLSForSprite(spr));`
  instead of the nested form) added one IR temporary that advanced the
  eax->ecx->edx rotation and fixed EVERY register in the rest of both
  functions (12 and 6, plus a byte each). So a rotation can be advanced by an
  extra IR TEMPORARY where a volatile barrier does nothing. Before retiring a
  function on the volatile test, try the one-temporary spellings: name an array
  element, name a call result, split a nested call. The triage tables carry
  this caveat now.
- **Halve-toward-zero must be a POINTER helper.** `neg/sar/neg` on the negative
  arm is hand-written (a plain `/2` lowers to `cdq/sub/sar`), and it is
  `static __inline void Half(int* v)` — NOT the by-value form — that gives the
  pair a memory home so the two sums accumulate into the freshly loaded
  tile-bounds register. By-value spellings, both operand orders, four
  sum-into-a-local shapes and four free-volatile placements all floored at 3-5;
  the pointer form is 0 (`DrivingSchool_Draw`, 26 -> 0).
- **The draw offset must be ONE `Pos` aggregate.** Two plain `int` locals
  interleave each load with its own halving AND swap the ebx/edi ranking of two
  other locals back in the prologue — 16 mismatches from one declaration. The
  aggregate placement/allocation rule at a fourth site.
- **Two struct-return handles must be separate statements — confirmed in the
  argument-evaluation direction.** `LLSSetFrame(GetLLSForSprite(g), lls->frame)`
  evaluates the field first and splits `add esp,4` / `add esp,8`; a temporary
  for the call result puts the call first and merges them into `add esp,0xc`.
  Worth 25 on `JcMonkeyFish_GetDrawDesc`.
- **Same-slot callbacks across classes are one shape; two of the same size are
  usually one source compiled twice.** `screencb2.c` closed 17 of 17 by
  grouping the seventeen by ObjDef slot (+0xb8 load, +0xac destroy, +0xa4
  create, +0x90 update), building ONE body per group carefully, then diffing
  each sibling's disassembly against it — nine matched first try, and
  `Carousel_Load`/`Balloonz_Load` differ only in a record size and two globals.
  Disassemble the whole family before writing any of it.

- **Struct-copy forward-propagation picks the FIRST field read.** After
  `dst = kConst;`, VC6 reads the first field back from the copy's `.rdata`
  source and the rest from the destination, so the order of the following
  field updates is load-bearing: the four `+=` on each jetty rect must be
  written top, left, right, bottom; any other order costs 16-35. Closed both
  `*_Create` tails in `screencb.c`.
- **A callee may need a SECOND (and third) extern prototype inside one
  function.** `StandardRemoveObject` needs `unsigned int tile` at one call site
  and `unsigned short tile` at another in `DrivingSchool_Remove` — the u16
  spelling emits `mov ax,[mem] / push eax` with no zero-extension, and that one
  instruction was the whole residual; `BsMermaid_Remove` needs a third, `BPosW`
  by value. `screencb.c` declares 0x0045f220 three times
  (`StandardRemoveObject`, `_W`, `_B`), following `ridecb6.c`'s precedent for
  0x0041b0d0. **Do not "align" them** — each is a caller-side lever.
- **A union's ARRAY view breaks a store-to-load unification the struct view
  does not — but only at NON-ZERO offsets.** `sq.c[1] = ...` stops VC6
  unifying the later `sq.b.y` read with the cell load (75 of 107 on the
  water-selection twins); `sq.c[0]` cannot, because element 0 is at `&sq`, the
  address `.b.x` already resolves to. So the trick rematerialises the y half
  and can never rematerialise the x half.
- **Two struct-return handles must be separate statements.**
  `f(g(a), h(b)->field)` reads the field before the other call and emits the
  cleanups separately; splitting them keeps the pointer in ebx across the
  second call and lets VC6 defer every cleanup into one `add esp,44h`.
- **Emitted store order is NOT evidence of source order.** In `CastleBbq_Tick`
  the y store is emitted first, yet the source is x-first: VC6 reorders the
  adjacent pair, and the later re-read follows the EMITTED order. Measure both
  orders rather than reading one off the listing (same family as "adjacent
  address stores come out reversed").
- **Twins written from one source are identical index for index in their
  residual too.** `BsWater_DrawSelection` and `JcWater_DrawSelection` share a
  32-mismatch residual that matches position for position — which is itself
  the proof that they are one piece of source compiled twice, and means a fix
  on either closes both. Same for `FOODCART FOOD` and `SHARK CAFE` (one
  statement apart). When two functions have the same size, diff their
  disassemblies before writing the second.

- **FROM THE PARALLEL SESSION `fable-a` (12 of 14 exact; full evidence in
  `docs/lanes/fable-a*.md`). The strongest, folded here:**
  - **VC6's callee-saved ranking INVERTS when a loop exists at IR time.** A
    four-line probe: with no loop, four `int` parameters take the callee-saved
    registers and three dereferenced POINTERS are homed in memory and reloaded
    at every use, even with more references. Add a self-tail-call — VC6 turns it
    into a loop at IR level — and the ranking flips: the pointers take the
    registers, the ints spill, and a secondary induction variable appears. The
    original `JungleCruise_TraceRoute` has the loop-free allocation WITH the
    loop, so its tail-call conversion ran AFTER allocation and ours runs before
    — a phase-ordering difference, not a source shape, and the whole residual.
    **Run the probe before spending a wave on an allocation residual in a
    self-recursive function.** Corollary: ANY dead trailing statement (`w = w;`,
    `if (route) ;`) blocks tail-call detection — folded for layout, but it
    survives long enough to flip the regime. A free way to switch a function
    between the two allocations.
  - **Tie-break among equally-referenced parameters: the LATER parameter
    wins** (two probes, both directions). No statement order changes it; only a
    strictly higher count or a regime change does. And **splitting a
    parameter's references across a local copy does NOT split its web** — VC6
    coalesces the copy and the combined count ranks. Retires "demote a
    parameter by renaming half its uses".
  - **A free volatile read CLOSED a strict 14 / register-blind 0 residual**
    (`SubtractObjRect`: same instructions, same order, two registers swapped;
    nine spellings byte-identical). So rb = 0 is NOT automatically a floor —
    the triage row for `strict >> rb` is corrected below to "try the free
    volatile first". Placement is the whole lever there too: on the other of
    two candidate loads it closes 11 of 14 and moves a spill store two early.
  - **Free volatile reads can buy the exact BYTE LENGTH without the match** —
    on `TraceRoute` the byte-exact row (105 mismatches, 339/339 B) was chosen
    over the lowest-mismatch row (101, 340 B), because byte-exact says every
    encoding is right and the residual is purely allocation.
  - **An append into a global array has THREE spellings that differ in two
    independent ways.** Through a `Rect4* q = &arr[n]` pointer: the address is
    materialised in a register (the original's form) but the stores KILL the
    CSEs of an address-taken local, reloading it in every block (57/119).
    Direct `arr[n].f = v` subscripts: the CSEs survive but the base folds into
    each store's disp32, one instruction short per block. **Fill a scalarised
    local and assign `arr[n] = t;`**: both right — the struct-assignment
    lowering materialises the address AND reads every value before the first
    store (117/118 first try). An inline `SetRect4(q, ...)` helper is NOT a
    substitute: `q` is still an opaque pointer and the kill only moves one
    block later.
  - **A loop-condition read of a global the body also writes: mirror it in a
    local and put the re-read in the MISS arm as an `else`.** `i < g_count`
    puts the load in the shared latch; `i < n` with `else { n = g_count; }`
    makes the arm exactly one load ending in a jump, which the exile rule parks
    past the epilogue — the original. `if (!hit) { n = g_count; continue; }`
    does NOT exile it (VC6 inverts and inlines). **The arm has to be an
    `else`, not a guard with a `continue`.**
  - **Run a rect walk on plain ints and write the `Pos` only at the call
    sites.** `for (p.y = ...)` on an address-taken pair pins it in memory and
    loses the `x * 20` strength reduction (136 vs 122 instructions); `int x, y`
    with `p.x = x; p.y = y;` immediately before the calls is 122/122 first try
    — even though the sibling `RemObjFromMap` matches with the OTHER shape.
    The sibling is not evidence either way.
  - **Two identical arms merge INTO the copy on the fall-through path; the
    copy in a JUMP arm is the one merged away — regardless of textual
    position.** (Reconciled 2026-09-05: this entry first said "merge at the
    FIRST site, the later arm jumps", and `UpdateHelpTick` measured the exact
    opposite — its survivor is the LATER site and the EARLIER arm jumps. Both
    are the same rule: whichever copy is laid out inline survives.) Evidence
    here: three `axis = 5; cost = -1;` blocks — writing the second as the
    fall-through (`if (!cond) { ... } else`) makes VC6 emit a second copy;
    writing it as the jump arm (`if (cond) { ... } else { 5; -1; }`) merges it
    into the earlier inline copy. In `UpdateHelpTick`, `flags |= 0x80` in two
    branches: `if (flags & 7) { |= } else { unlink }` puts the OR inline and
    exiles the unlink; `if ((flags & 7) == 0) { unlink } else { |= }` makes the
    OR the jump arm and it merges into the later shared block — that one
    inversion took the function from a 126-instruction misalignment to exact.
    **Ask which copy the original reaches by fall-through, and write the OTHER
    one as the jump arm.** The `return K` first-site rule is the special case
    where the earlier return is the inline one.
  - **Inline-argument arithmetic is evaluated RIGHT to LEFT, and that decides
    whether an inlined guard re-tests.** `CellAt(dx + x, dy + y)` computes the
    Y sum last, its flags carry the `x >= 0` guard, and VC6 emits a bare `js`.
    The original re-tests with `test/jl` — which a Y sum scheduled BETWEEN the
    X sum and the guard forces — so the sums were STATEMENTS (`t.x = ...;
    t.y = ...; CellAt(t.x, t.y)`), not argument expressions (52/76 -> 78/78).
    Narrows "arithmetic in a call argument runs before the guard": it does,
    in argument order; when the original re-tests, it was not in the argument.
  - **Constant stores written AFTER the statement whose call they interleave
    with.** Stores cannot migrate across a call, so two constant stores written
    BEFORE a `rand()`-bearing statement land adjacent before it (6); written
    after, VC6's adjacent-store reordering hoists them into the arithmetic's
    gaps and the interleave is exact. The store/call barrier used as a
    PLACEMENT instrument.
  - **VC6 propagates a compared constant into the arm and stores the
    REGISTER.** `mov word [esi+40h], di` in the `r == 2` arm comes from a plain
    `b->f40 = 2;` — VC6 knows `edi == 2` on that edge. Do not reconstruct it as
    `= (short)r`.
  - **A `unsigned short` parameter kept in `cx` can be the whole function.**
    `JungleCruise_StepRoute` loads its station id once as `mov cx,[esp+0x34]`
    and compares it four times; as `int` compared through a cast it is 107/108
    with that as the only mismatch. And an extern's `unsigned short` parameter
    is what leaves a CALLER's upper half dirty (`mov cx,[esi+4]` into a
    register still holding a counter, whole `ecx` pushed) — an `int` parameter
    zero-extends and costs the match.
  - **Four `__cdecl` calls whose results land in locals share ONE
    `add esp,0x20`** — the converse of "a result passed straight into another
    call splits the add esp" — and the merge shifts every `[esp+N]` argument
    read in between. **Four identical neighbour blocks are written out in
    full**, each with its own callee-saved register; a loop or pointer array
    reproduces neither. **Two argument slots reused as locals keep an 8-byte
    frame** with nothing in the source asking for it — a reason NOT to copy
    arguments into named locals.
  - **`memset(p, 0xf1, n)` is `mov eax,0F1F1F1F1h / rep stosd`** under the
    intrinsic — a non-zero byte constant broadcasts to a dword. First non-zero
    memset constant in the tree.
  - **A step assigned through a temporary is not a basic induction
    variable.** `x - 5` spelled twice makes `x` an IV and VC6 keeps `x + 5` in
    a stack slot with a second latch `sub`; `int nx = x - 5;` used for both the
    probe and the recursive call removes the IV — 9 instructions of noise. The
    opposite direction of "let VC6 do the strength reduction": here the
    original does none, and the temporary is how you stop it.
  - **The order of `p->next = head;` against a neighbouring field store decides
    where the head LOAD lands, not the store** — the stores stay in the
    compiler's order either way. Measured on two allocation sites.
  - Negative, recorded so it is not re-derived — **but body-specific, not
    general (corrected 2026-09-05)**: **induction-variable emission order in a
    loop latch was not reachable by source ordering on `BsWater_SetTile`**
    (`inc ebp` second vs fourth; twenty variants, the first-use hypothesis
    disproved; `strict == rb == ob`). On `ZBuffer_RunCommand` it IS reachable:
    `key[i].idx = i;` written LAST among the three stores gives the original's
    `inc esi / add eax,0x30 / add ecx,8 / add edx,8` where written first it
    gives `inc / add ecx / add edx / add eax` — 24 orders measured, that one
    closed the function. Sweep the store order before accepting a latch-order
    residual as a floor.

- **A pending cdecl `add esp` cannot cross a branch join — so a shared tail
  containing a CALL proves the tail was written TWICE in the source.**
  `SchoolCarManoeuvreD`'s single `add esp,0x24` cleans three calls' arguments
  after a call that sits in the shared tail. Written as an if/else with a
  shared tail, each arm flushes its own `add esp,0x18` mid-store (88 of 116).
  Duplicating the whole tail into both arms and letting VC6 cross-jump the
  copies: **113/113, first try.** Read the argument clean-up: if one `add esp`
  covers calls on both sides of a join, the source had the calls in both arms.
- **A `volatile` STORE through a cast — `*(void* volatile*)&x = e;` — forces
  the home store without making the reads volatile.** The free-volatile lever
  extended to writes: `void* volatile x` costs a reload at every use, whereas
  the cast form stores once and keeps the register. Worth 124 -> 14 on
  `Coaster3D_BuildTrackMesh`.
- **SCOPE, not declaration order, decides frame position for address-taken
  aggregates.** Declaration order is inert (5 permutations byte-identical);
  moving a local into the loop's INNER scope moves it DOWN the frame. Three
  separate "which of this pair is higher" questions on one function were each
  answered by scoping alone.
- **Walk an array by SUBSCRIPT to get VC6's own strength-reduced temporary
  into a dead argument slot.** A `void** p = list;` local coalesces with the
  `list` PARAMETER and keeps that parameter's slot; VC6's own temporary cannot
  coalesce with a parameter and lands in the dead argument slot instead — the
  original's choice, which is what frees the `list` slot for the trip counter.
  Related, and cheap to read: a counted `for (j = 0; j < N; j++)` gives the
  strength-reduced bound a SIGNED `cmp/jl`; a pointer walk gives `jb`.
- **Adjacent address stores come out REVERSED.** `job.v[0..2] = &v[0..2]`
  written 0,1,2 emits 1,0,2 (the original); written 1,0,2 emits 0,1,2 and costs
  4. Read the source order off the inverted output. Same family: two
  independent constant stores set the scratch rotation by their statement
  order — writing `nverts` before `ntris`, the opposite of their emission
  order, was 32 -> 8.
- **Subscripted access versus a named pointer decides whether a subtraction
  folds into a memory operand.** `m->verts[tri[1]].x - m->verts[tri[0]].x` with
  the pointer locals declared AFTER makes VC6 load the first vertex's x and y
  into registers first (the original); with the pointers declared first every
  subtraction folds and the body comes out two instructions short. Worth
  141 -> 23.
- **`x + 0x80000000` and `x | 0x80000000` are different objects.** VC6 folds
  the ADDITION of the sign bit into a single `lea` displacement; the OR does
  not fold. A `lea` carrying a near-2^31 constant says the source added, not
  or'd.
- **`sub esi,ecx` + `[esi+ecx]` is VC6 eliminating one of two induction
  variables, and it is NOT reachable by loop spelling** — hoisted source
  pointer, joined store, inlined helper, both `if` polarities, down-counting,
  `while`, unsigned index and flat `int*` typing are all byte-identical (8
  ways, `Coaster3D_DrawMesh`). When the original keeps `dst - src` in a
  register and addresses `[src + diff]`, that is the allocator's choice, not a
  source shape.

- **A PARTIAL aggregate initialiser is a placement lever.** `BlitCtx ctx = { 1 };`
  puts the implicit zero-fill of the remaining fields at the top of the IR
  (where an initialiser must go) while the explicit `1` schedules with the
  call's argument block. Three assignment statements sink all three stores
  below the argument pushes: 11 of 137 on `DrawPopUpFrame`, **0** with the
  initialiser. All 720 orders of the six leading statements, a pointer local, an
  inline filler, an `int[3]`, a flattened struct, `(void*)0`, `p = n = 0`, a
  volatile store, and the FULL `{1,{0,0}}` initialiser (12) are all inert.
  Transferred straight to `DrawPopUpExtra`.
- **A `Pos` by-value parameter is NOT byte-identical to `(int, int)` when the
  argument is a pair of plain sums — the recorded "byte-identical at a call site
  whose argument is an address-taken struct" rule is narrower than it reads.**
  `Step(q, t)` with `t.x`/`t.y` assigned in field order evaluates the sums
  LEFT-to-right, the original's schedule; `(int, int)` evaluates right-to-left
  and costs 13 of 123 on `LFRun_Tick`. It only works with the class global read
  directly at both uses — the same by-value `Pos` through a `RideDef* def`
  local costs 76.
- **`tw <<= 1` and `tw = tw * 2` are different objects.** The in-place shift is
  `shl edi,1`; the multiply lowers to a two-operand `lea esi,[edx+edx]` into a
  FRESH register, which also swaps the pair the derived values land in — 6 of
  161 directly and 26 more from the wrong pairing. `+=`, `= tw + tw` and `*=`
  all behave like the multiply. Only the shift-assign is in place.
- **Two derived quarter-steps are emitted in REVERSE source order.** Writing
  `dy = th >> 2;` BEFORE `dx = tw >> 2;` is worth 26 of 161 on
  `LFTrack_BuildGeometry`; the natural x-then-y order gives the registers to the
  wrong operands through all six geometry blocks. Same family as the
  "emission order" tie-break: for a pair of independent derived values, try the
  reversed order first.
- **Naming ONE operand of a duplicated compare forces the hoist; naming BOTH
  forces frame homes; and WHICH one you name matters.** Where
  `fwd->sq.b.x != p->sq.b.x` appears in both arms of an `if`: both as fields
  duplicates the whole three-load block into each arm and drops the second
  `test` (71 of 144, two instructions long); one `unsigned char px = p->sq.b.x;`
  makes VC6 hoist the loads and re-emit the test (**2**); two byte locals grow
  the frame from `push ecx` to `sub esp,8` and cost 94 — and naming the OTHER
  operand alone is also the 94 case.
- **A `||` hit-test whose second operand must be hoisted needs the AGGREGATE
  member, not a plain int.** `box.left = icon->x;` before the test keeps the
  value out of the short-circuit arm, where a plain `int left` sinks into it
  (9 of 120 -> 0 on `DrawPopUpExtra`). This is popup.c's recorded `box.right`
  lever from the other side; it transferred within the family, as did the
  four-corner `struct { int left, top, right, bottom; } box;` idiom with
  `box.right - box.left` as the call argument — four separate ints let VC6 fold
  both differences to constants where the original subtracts live values.
- **Two identical `if/else` arms of a 2x2 nest cross-jump-merge; a third that
  differs in ONE argument does not.** Confirmed on `LFTrack_DrawAlt`: cases B
  and C merge, A and D stay separate solely because A passes `mode` and D
  passes 0. So an original that passes a parameter in only one of four
  otherwise-identical arms (a real quirk, reproduced there) is visible in the
  layout, not just the arguments.

- **A by-value struct's field ASSIGNMENT order is its register rotation.** For
  a `WinRect` passed by value, VC6 always emits the stores in field-offset
  order through `mov edi,esp`, but the four constants take the
  eax->ecx->edx->esi ring in the order the SOURCE assigns them. Writing the
  natural left/top/right/bottom put every constant one ring position off at all
  four call sites of `PrintScreenMode8`; top/bottom/left/right took it
  118/149 -> 149/149 with no other change. When a by-value aggregate's
  constants are in the wrong registers, permute the field assignments, not the
  expressions.
- **Zero a struct payload with `memset`, not field-by-field `= 0`, or a
  function-wide zero web steals a callee-saved register.** `ctx.sub.p = 0;
  ctx.sub.n = 0;` merged with every other literal zero in the function (three
  `push 0`s, two zero arguments, a rect field) into one hoisted edi, which
  forced an extra `push ebx` and turned `test eax,eax` into `cmp eax,edi`.
  `memset(&ctx.sub, 0, sizeof ctx.sub)` makes the zero its own
  intrinsic-expansion node: 64% -> 79% and the prologue exact. Sharpens the
  recorded "VC6 hoists a constant zero into a callee-saved register only when
  that register is pushed anyway".
- **The exile rule in the KEEP-OUT direction.** The recorded prescription
  (split a short-circuit `if (a || b) return X;` to bring X back inline) has a
  useful inverse: three separate `if (!x) goto fail;` guards let VC6 pull the
  shared `fail` block INLINE behind the third, whereas the single
  `if (!a || !b || !c) goto fail;` exiles it past the whole function. On
  `LoadCSPSprite` the separate guards cost 13 at the right instruction count;
  the merged guard is exact. When a shared error block is inline and should
  not be, MERGE the guards.
- **A named-local lever transfers to a family, but to a DIFFERENT SITE per
  function — test per site.** bigrender.c's "`f->n16` into a local" lever
  applies in `RenderSpriteX` at the third site (the else arm) and in
  `SoftBlitRLE` at the SECOND (the post-walk call in the base-image arm): the
  site whose block begins after a call, where reloading the scratch globals is
  what the hoisted CSE temporary jumps over. Worth 14. Second-order effect worth
  knowing: with the CSE alive the block's pointer chain landed in the same
  registers as the else arm's, so VC6 cross-jumped FOUR more instructions into
  the shared tail and the body came out 137 against 141. **A short instruction
  count on a two-armed tail is a register-identity symptom, not a missing
  statement.**
- **A call inserted into a setup region is a scheduling barrier that
  invalidates a twin's recorded statement order.** `CalculateMapRenderOrder`
  records its setup as p.x / p.y / node_next / memset / link LAST. Its full-map
  twin wants link FIRST, because an `ElemID` call sits between the setup and
  the loop guard; written last, `mov esi,K / mov [esp+0x10],esi` sinks past the
  guard. That pair was the whole residual (160 -> 162 of 162). A setup order
  inherited from a sibling is a hypothesis whenever a call has been added to
  the region.
- **A duplicated-exit epilogue needs no source construct.** `mov word [esi],0`
  on one path and `mov word [esi],bp` on another, for the SAME `*link = 0;`
  statement, arises automatically from a loop guard that must reload its
  induction variable from memory. Do not build a construct to reproduce it.

- **A switch label sharing a jump-table entry may still be a SEPARATE case body
  in the source.** VC6 merges two identical whole case blocks and points both
  table entries at the survivor — which is indistinguishable in the disassembly
  from `case A: case B:`, but is NOT the same in codegen, because the extra
  block changes the merge candidate set and therefore the layout. On
  `Joust_Update`, giving two labels their own bodies is worth **109 and two
  missing instructions**, and aligned a loop's start index exactly. A third
  label in the same switch IS genuinely shared (its own body costs 23), so
  **this must be tested per label, not decided for the switch.**
- **The busy-guard form decides which copy of a merged tail HOSTS it — and the
  answer is per-body, never inheritable.** `if (b->state == 0) { switch ... }`
  hosts merged call tails in the LAST case of each group;
  `if (b->state != 0) goto endsw;` hosts them in the FIRST. Worth **153** on
  `Joust_Update` — the entire block layout of half its switch.
  `TempleSlide_Update` records the same lever with the OPPOSITE answer. Measure
  both ways on every body; a recorded answer from a sibling is a hypothesis, not
  a result.
- **A string template copied into a local can be a struct assignment, not an
  array initialiser.** `char name[9] = "manBox??";` pins the three-instruction
  copy to the top of the function's IR, where an initialiser must go. Declaring
  `static const struct { char c[9]; } k = {"manBox??"};` and writing `name = k;`
  makes it a STATEMENT, placeable anywhere — which is where the original
  schedules it. Worth 58, and it moved the first divergence from index 3 to 40.
- **A negated condition keeps the test order but swaps the arms' layout.**
  `if (!C) { A } else { B }` emits C's short-circuit tests unchanged with A
  inline and B after; the positive form reverses that. A free way to move a
  block without touching the condition.
- **The original's `setcc` names the FALSE arm of the source ternary.**
  `(s < 3 ? 0x0a : 0x14)` gives `setge / dec / and 0xfffffff6 / add 0x14`;
  `(s >= 3 ? 0x14 : 0x0a)` gives `setl / and 0xa / add 0xa`. So read the
  `setcc`'s condition, negate it, and that is the ternary's test as written.
- **Arithmetic placed in a CALL ARGUMENT is evaluated before the callee's own
  guard.** Calling a bounds-checked inline lookup as `FindInBounds(x, y - 5)`
  puts `add eax,-5` BEFORE the `x >= 0` test; computing the offset into a local
  first does not. Where an inline helper's guard sits after some arithmetic in
  the original, the arithmetic was in the argument.
- **Nest a lookup inside a call argument to keep the scratch rotation running
  across inline expansions.** `Stop(GetLLSForLayer(g_layers, v->layer_a))` keeps
  VC6's eax->ecx->edx rotation advancing across three consecutive expansions
  (`eax,ecx | edx,eax | ecx,edx`); a helper that takes the layer number and
  reads the global itself gets the first expansion right and then RESTARTS at
  eax for the second and third. Where consecutive inline expansions should
  rotate but do not, move the lookup into the argument.
- **Initialiser ORDER decides where a spill store lands.** Three leading
  initialisers: the one whose spill the original places last must be written
  last, so its store falls after the others' `xor`s. Cheap to sweep, and it was
  the difference between exact and not on `BsWater_Probe`.

- **THE AGGREGATE RULE, consolidated 2026-09-05 after three lanes hit it from
  different directions.** VC6 flattens an aggregate whose address is never
  taken, so wrapping locals in a struct changes nothing about how the fields are
  ACCESSED — they stay enregistered, no memory home is forced, and arrays,
  padded structs and unions all scalarise identically. **But aggregate-ness
  still decides PLACEMENT and ALLOCATION**, in two measured ways:
  1. *Frame position.* Two `int` locals take the two LOWEST frame slots; the
     same pair declared as one `Pos` takes the two HIGHEST. Worth 28 of 138 in
     `BuildWalkPath`.
  2. *The callee-saved tie-break between byte-typed locals.* Only EBX is
     byte-addressable among the callee-saved registers, so where two byte-wide
     values must both survive calls, one spills. Written as two scalars, VC6
     gives EBX to whichever has fewer weighted references — the original's
     choice only sometimes. Written as ONE two-byte aggregate, VC6 picks the
     original's winner. On `LFTunnel_Place`: two scalars = 164 of 217 and the
     wrong instruction count; `BPos c` = 5 of 217 at 680/680 bytes.
  **It must be the SHARED aggregate.** `BPosW` and `unsigned char c[2]` are
  byte-identical to `BPos`; TWO one-member structs behave exactly like two
  scalars. And the aggregate fixes a separate-looking symptom for free: with
  scalars VC6 sinks the last coordinate update into a trailing `if`, moving the
  add past a call; as a struct member the store stays put.
  **Scope, tested rather than assumed:** it does NOT transfer to
  `LFDrop_Place`, where `c.x` still wins EBX. The distinguishing feature is that
  in the functions where it works BOTH halves are updated at least once, whereas
  in `LFDrop_Place` `c.x` is written once and never again. Wave eleven had
  recorded that tie-break as forced and unreachable; it is reachable, just not
  everywhere.
- **Read a class/def global DIRECTLY at every use, not through a local
  pointer.** On `LFCsaw_Place` replacing a `RideDef* def` local with two direct
  reads of the global is 18 mismatches -> 0. A local pointer creates a web the
  original does not have; the repeated global read is what VC6 emits when the
  source simply names the global twice. Compare the rule above about a
  duplicated expression versus a named local — same family, opposite of the
  instinct to "cache it once".

- **A free `volatile` read belongs at the DEFINITION site, not the use site —
  VC6 will not hoist a volatile access across a store.** Placing the barrier at
  the later USE reproduces the reload but leaves it scheduled after an
  intervening `push`, one instruction off. Placing it on the value's own
  ORIGINAL load — where the function loads anyway, so it costs nothing — is
  exact. This is a sharpening of the free-volatile lever and it matters: in
  `ClearCellForPath` the difference between the two placements is 166 versus
  167 of 167, and leaving the CSE alive instead costs 36. Roughly 30
  non-volatile spellings floored at 131.
- **An aggregate local takes the TOP of the frame — and that is a lever even
  when it scalarises.** Two `int` locals get the two LOWEST frame slots;
  declaring the same pair as one `Pos` whose address is never taken moves them
  to the two HIGHEST, worth 28 of 138 in `BuildWalkPath`. **This refines "VC6
  flattens an aggregate whose address is never taken"** — the flattening
  governs how the fields are ACCESSED (they stay enregistered, no memory home
  is forced), but the declaration still changes where the pair LANDS in the
  frame. So a struct wrapper is inert for access codegen and NOT inert for frame
  position. Declaration order remains inert either way; it is the
  aggregate-ness that moves them.
- **A per-iteration update of two struct fields can be a whole-struct
  assignment.** `cur.x += dx; cur.y += dy;` loads `cur.x` into a fresh
  callee-saved register and accumulates into it. `t.x = dx + cur.x;
  t.y = dy + cur.y; cur = t;` consumes both loads into dx's and dy's own
  registers and writes the pair back as one copy — the original's shape, worth
  9. All six operand orders and both `+=`/`=` spellings of the first form are
  byte-identical, so this is NOT reachable by permuting the sum; only the
  copy-through-a-temporary shape gets there.
- **An expression spelled twice is not the same as a named local holding it.**
  The CSE web VC6 builds for a duplicated allocation size lands in eax; routed
  through a named `size` local the `lea` comes out on edx. Two bytes, and it was
  the last two in `BuildWalkPath`. The recorded "constant carriers are always
  propagated away" rule covers CONSTANTS; a duplicated computed expression is a
  different case and the web's home differs.
- **A guarded do/while and a counted `for` allocate differently.**
  `if (i > 0) { p = ...; do { ... } while (--i); }` puts the accumulator in ebx
  and the argument pointer in ebp; the equivalent counted `for` over the same
  subscript swaps that pair, worth 12 instructions.

- **Two `return K` sites with the same constant merge at the FIRST site, not the
  last.** So a leading guard written as a plain `return 0;` pulls the merged
  block to the top. Writing it `goto fail;` with `fail: return 0;` as the
  function's final statement pins the merged block at the END, which is the
  original's `je rel32` forward. Worth 190 -> 157 on `SchoolCarNextManoeuvre`.
- **`if (a == 0 || b != c) return X;` EXILES the X block past the whole
  function** — the short-circuit makes the arm its own block ending in a jump,
  and the exile rule then applies. To put X back inline, split the test and jump
  INTO the second `if`'s compound statement:
  `if (a == 0) goto lbl; if (b != c) { lbl: return X; }` — because a single-test
  `if (cond) return X;` always emits X as the test's fall-through. Worth
  157 -> **0**, i.e. it closed the function. Two separate `if`s with two textual
  returns costs 173; the `if (a && b) goto ok;` inversion is inert.
- **An ARRAY of pointers reserves all its frame homes where separate scalars do
  not.** `RoadRec* nb[3]` gives `sub esp,0x14` with 8 dead bytes and pushes
  another local out into a dead argument slot; three plain pointer locals give
  `sub esp,0xc` and put the WRONG local in the argument slot (50 mismatches).
  Only the last element's slot is observable, so which index is which is not
  recoverable from the disassembly — do not try. Note this does not contradict
  "VC6 flattens an aggregate whose address is never taken": the array is
  INDEXED, so its elements are addressed, and the homes are reserved.
- **`if (r & 2) return A; return B;` and `(r & 2) ? A : B` are NOT the same
  object.** Both become VC6's branchless bit trick, but the ternary computes the
  complement into a fresh register (`mov al,bl / not al`) where the branchy
  statement form does it in place (`not bl / movsx eax,bl`) — 18 strict and 3
  bytes apart. About 20 type and cast spellings of the operand could not reach
  it; only the statement form does. When a branchless sequence has one surplus
  register move, try the other statement shape before touching types.
- **A JUMP-TABLE `switch`'s CASE ORDER is its block order.** On
  `SchoolCarNextManoeuvreHorn` the original's layout is cases 1,4,5,3,6,7;
  writing them in the natural 1,3,4,5,6,7 costs 17. Read the block order off
  the original and write the cases in that order. *(Qualified 2026-09-05: when
  VC6 lowers a small switch to a COMPARE CHAIN — `sub eax,0 / dec / dec` — the
  layout follows the case VALUES and source order is inert: all six orders of
  `BuildPTPRoute`'s three cases, an equivalent `if / else if` chain and a
  hoisted call result are byte-identical, block layout included.)*
- **A constant store must be placed so it cannot hoist above a dead argument
  slot's last read.** `flags = 0` assigned BEFORE a call floats up and takes the
  slot; assigned after the call it stays put. Where a local lives in a dead
  argument slot, the last read of that slot is a scheduling boundary for
  anything that would occupy it.

- **BEFORE recording "construct X is unreachable from C", scan `.text` for X.**
  An exhaustive sweep of spellings proves something about the spellings swept,
  not about the instruction. A recorded entry declared `add reg, -K`
  unreachable after sweeping every spelling of a subtraction; the binary
  contains 75 of them, and this tree was already emitting one. The scan is a
  four-byte pattern search and takes seconds. Same rule for "no instance exists
  in the corpus" claims — re-run them loosely.
- **An UP-counting loop materialises its trip count differently from a
  down-counting one.** `for (i = 0; i < N - 2; i++)` and
  `for (n = N + (-2); n > 0; n--)` produce the same `test/jle` guard and
  `dec/jne` latch, so they look interchangeable — but in the up-counting form
  the trip count is computed by the LOOP TRANSFORMATION rather than by the
  source expression, and that node emits `add eax,-2` where the down-counting
  form emits `sub eax,2`. When a loop head differs by exactly that, the
  direction the source was written in is the answer.
- **Only EBX is byte-addressable among the callee-saved registers**, so where
  two byte-typed locals both need to survive calls, exactly one can — and which
  one wins is forced, not accidental. On `LFDrop_Place` names (8 pairs),
  declaration order, statement order, extra definitions, reusing the running
  coordinates and four store orders are ALL byte-identical; only a TYPE change
  flips it, and it flips completely (one type gives the original's whole
  18-instruction head plus a spurious `and ebx,0xff`, the other gives the
  original's exact 341 bytes but a dword head). A residual bracketed from both
  sides like that is "a byte-typed local that ranks like an int" — state it that
  way rather than continuing to sweep spellings.
- **An escaped aggregate versus plain scalars can be the single cause of four
  separate-looking differences.** On `LFEntrance_Add`, replacing the `Pos` used
  by one block with two plain int scalars reproduces the original's whole store
  block instruction for instruction — the hoisted byte load, a store landing
  between a flags load and its `or`, a SPLIT `mov cl,[mem] / sub al,cl` instead
  of the folded form, and the operand order — *and* flips the callee-saved tie
  to the original's. Four things three lanes had chased separately were one
  fact. It is still not committable (the function's head needs the opposite: with
  scalars VC6 folds two definitions into `lea eax,[eax+edx+1]` at IR level), but
  "escaped or not" is the first question to ask of a store block, not the last.

- **CLASSIFY A RESIDUAL BEFORE ATTACKING IT — three blind measures, one
  experiment, and you know which kind you have.** Run strict, register-blind
  (rb) and offset-blind (ob) with frame homes resolved by esp depth (never by
  raw `[esp+N]`, which drifts across branch joins):
  - `strict >> rb` — an ALLOCATION residual. Ask which register the original
    frees and when; source ORDER is nearly powerless here — but **run the free
    volatile experiment before calling it a floor**: `SubtractObjRect` was
    strict 14 / rb 0 and one free volatile read closed all 14 (see the
    `fable-a` entry above).
  - `strict >> ob` — a FRAME residual. Usually unreachable: weights are counted
    on surviving IR, so alias-routing and dead reads change nothing, and
    declaration order is inert.
  - `strict == rb == ob` — a pure SCHEDULING permutation: the same multiset of
    instructions, same registers, same homes, in a different order. `Balloonz_Tick`
    (5) and `ValidateCursor` (5) are both exactly this.
  - rb still HIGH — a real STRUCTURAL difference, and the only kind that is
    reliably reachable from C. **These are the bodies to spend a wave on.**
  Then apply the free-volatile test above to separate a local rotation from a
  global web rank. Measured across seven partials in wave eleven; three that
  looked closest by strict mismatch turned out to be floors under this test.
  **Caveat (wave fourteen): the volatile test can be inert where ONE extra IR
  temporary still advances the rotation** — name an array element or a call
  result, or split a nested call, before concluding "floor".
- **Offset-blind bucketing did NOT mask anything in seven partials checked.**
  The trap is real (it hid an operand order in `Draw3DPersonModel`) but it is
  not common: in `Balloonz_Tick`, `DrawPopUpInfo`, `GetObjectUID` and
  `ValidateCursor`, offset-blind never drops below strict once homes are
  resolved by push depth. Run the check, but do not assume the trap.
- **An aggregate does not force a memory home — confirmed independently.**
  `int a[2]`, `int a[4]`, a struct with a `char pad[8]` or `int pad[3]` tail, and
  a union with a `char c[8]` member ALL scalarise to the same object as the
  plain local (verified in the disassembly: the value stays enregistered and the
  halving is still `sar ecx,1`). This is the same rule as "VC6 flattens an
  aggregate whose address is never taken", now measured from the other
  direction. The recorded "unions on an enregistered scalar are a trap" entry is
  consistent and narrower than it sounds: it is the address-taking that
  de-enregisters, not the union.
- **U-pipe slot choice, not aggregation, decides which of a load/store pair goes
  first.** In `Balloonz_Tick` the two stores were long assumed to be one object
  because they always emit together; sourcing the halves from two INDEPENDENT
  `.data` objects (two string literals, or two `static const int`s) is
  byte-identical, and VC6 places both stores at the same indices regardless of
  what they copy. The original simply takes the load in the first U slot. When
  two instructions always move together, test whether they are actually related
  before building a source construct to pin them.

- **A tail-recursive spelling is byte-identical to the loop** — VC6 SP3
  eliminates the tail call. So when the original looks like it might have been
  written recursively, that is NOT a separate search: write whichever reads
  better. Measured on `WW_AnyBlokeInRect`; retires a hypothesis class that had
  been sitting open.
- **Use a free `volatile` read as a DIAGNOSTIC for which kind of residual you
  have.** A zero-cost `*(volatile int*)&x` (one where the original loads anyway,
  so it adds no instruction) advances VC6's eax->ecx->edx scratch rotation. In
  `BoatingSchool_Tick` that is *the* lever, worth 219 and 115 at two sites. In
  `WW_AnyBlokeInRect` three such reads, singly and together, are byte-identical
  to the committed body. **When a free volatile read moves nothing, the residual
  is a global web RANK, not a local rotation — and no barrier or ordering
  construct will reach it.** That is a one-experiment test for a floor, and it
  cost nothing.
- **`audit.py`'s instruction and byte counts include trailing pad NOPs —
  subtract them before reasoning about a byte deficit.** On `Road_FindDiagonals`
  the reported 64 insns / 153 B is really 62 / 151 once two pad NOPs are
  removed, which turns a vague "four bytes short, look for a 2-vs-3-byte form"
  into an exact account: the deficit is 6 bytes = the original's second
  `mov edi,[esp+10h]` (4) plus its `jmp` (2), and nothing else in the body is
  short. Always resolve a byte deficit to specific missing instructions before
  hunting encoding forms.
- **Do not hand-hoist a loop invariant VC6 already hoists.** Naming rect fields
  as locals in `WW_AnyBlokeInRect` is measurably WORSE — one hoisted field costs
  16, two cost 24 — which confirms the register-resident values are VC6's own
  invariant hoists. If a value is already loop-invariant, giving it a name only
  adds a web for the allocator to place.

- **Alias kill: a store to ANY field of an address-taken struct kills CSE
  availability of an unrelated load.** Not just the field stored — the whole
  object. Proven two ways on `RequestRoute`: a redundant third occurrence
  (`to.x = ...; to.y = ...; to.x = ...`) yields 483 instructions in which the
  first two merge into one register web and the THIRD becomes a fresh
  `mov ecx,[esi+8]`; and interposing a store (`to.y = ...; to.y--; to.x =
  cur->pos.x;`) puts a fresh load 12 indices later. **The consequence is that
  there are only TWO regimes** for such a value — same web as the earlier test
  (no intervening store) or a fresh load (store in between). A
  register-to-register copy is not reachable, so if the original has one, no
  source spelling produces it and the function is at its floor.
- **The callee-saved register PAIR follows web LENGTH, not source order.** VC6
  hands eax to the short web and ecx to the long one, every time. Where the
  original needs the short-web pairing AND a value live across a long range
  simultaneously, those are contradictory and the residual is unreachable. This
  plus the alias-kill rule above account for every outcome on `RequestRoute` —
  which is how a 3-mismatch residual was closed by ARGUMENT rather than by
  enumerating spellings.
- **A one-byte deficit can be a register-vs-immediate COMPARE, not a shift
  form.** `cmp esi,eax` is 2 bytes where `cmp esi,25h` is 3. On
  `ClampPopUpToScreen` the whole missing byte is exactly that: the original
  compares against the immediate, we materialise it into a register first. Check
  this before hunting `shl` vs `lea` — it is the cheaper explanation and it also
  tells you the original did NOT need the constant in a register afterwards.
- **Let VC6 do the strength reduction.** On `JcBoat_Animate` an explicit pointer
  walk with a separate counter keeps the exact byte count but moves the first
  divergence EARLIER; VC6's own strength reduction of `b->wob[j]` is what the
  original has. Where an induction variable is involved, write the array
  subscript and leave it alone.

- **Triage a reconstruction error before "fixing" it: is it OBSERVABLE, and
  does it cost strict?** The two questions are independent and both matter.
  `RenderView` stores `g_sort_count = 0` unconditionally where the original
  stores it inside `if (cell->obj != 0)` — proved from the disassembly (the
  store sits past the `je`, with `edx` untouched since its `xor`, and the
  original materialises TWO zero registers for six zero stores precisely so one
  survives the branch). But a scan of the whole `.text` section finds all 13
  references to that global inside `RenderView` itself, every read in the same
  arm as the store, so the difference is NOT observable and the correct call is
  to record it and leave the body alone — the best of eight placements costs 5
  strict. Contrast the `cr2` bug, which was observable and had to be fixed at
  once. **Scan `.text` for the global's address before deciding**; a
  four-byte little-endian search of the section takes seconds and answers it.

- **The block-exile rule, now stated as an IFF.** An else arm is exiled past the
  fall-through trace **iff the arm itself ends in an unconditional jump** — not
  because of its size, its source position, or which arm is "unlikely". Three
  proofs in-tree: `ScanBlokeSurroundings` (0x450530), where five guard
  conditionals all target ONE two-instruction block `inc dword [esp+0x14] /
  jmp <latch>` parked at index 355 past the epilogue; `KillAllSamplesFromSource`,
  where VC6 tail-DUPLICATES a 4-instruction tail into both arms so both end in
  `jmp`, then exiles the second (53..57) while the first stays inline (28..32);
  and a local confirmation on `RenderFullMap` — duplicating the tail through an
  arm that ends in `continue` exiles the fill and lands the join on the
  original's index exactly, whereas duplicating only the null tests (arm still
  ends in `je`) does not move it at all.
- **A partial cross-jump merge is impossible — this closes routes by argument
  instead of by exhaustion.** A merged suffix's fall-through successor must also
  be shared, so a merge cannot stop part-way; it is the whole tail or nothing.
  Where a "just merge these few instructions" idea would need to stop before the
  common successor, it is not a spelling problem — it cannot exist.
- **A two-term sum's destination register is decided by the destination SYMBOL
  alone.** Association, explicit delta locals in both operand orders, a
  copy-then-accumulate `sy2 = py; sy2 += d;`, and swapping the two operands'
  declaration order are all byte-identical; only `py += ...`, where `py` IS the
  destination symbol, emits the original's `add ebx,ecx`. The resulting web
  cannot then be split — four ways of ending it early are one object. **Retires**
  "make the operand a temporary without lengthening the destination's web": in
  VC6 those are the same thing.
- **x87 stack DEPTH is a codegen lever, not just a spill order.** VC6's x87
  allocator spills by furthest next use, but the ceiling matters too: `fsub
  st(n)` cannot name deeper than `st(7)`, so a value that must stay live across
  a full stack forces its *siblings* to become memory operands. In
  `AnimApplyPart` the original's stack is exactly full at the second
  interpolation — one kept conversion plus five more plus working — which is
  precisely WHY four sibling values are read from memory there. If the original
  reads operands from memory where the reconstruction has them on the stack,
  count the depth before hunting for a source spelling.
- **Extract the frame map by push-depth, and pair homes by register-blind
  text.** `scratchpad/w10joust/fm2.py` prints `orig-home -> our-home` directly,
  tracking `push`/`pop`/`add esp` depth through both bodies so `[esp+N]` is
  resolved against a common origin. This is the cheapest instrument in the tree
  for catching a whole error class, and it immediately falsified a headline note
  claiming `StepSchoolCar`'s frame layout WAS the original's — only 5 of 15
  homes actually agree. Caveat: the older `laneG/fm.py` silently mis-analyses
  `RenderFullMap` (reports 0 frame slots / 66 arg slots); use
  `w9renderview/slots.py` there.

- **A pointer cache is a REGISTER CONSUMER, not a neutral schedule barrier.**
  `p = b->person;` placed above an escaped-struct store does lift the load as
  intended — but it occupies ECX, which pushes the next scratch temp into EDX,
  where it becomes hoistable and drags a whole window with it. The five ride
  `_Activate` functions spent three waves being described as a scheduling
  problem when the residual was this one allocation. **Before adding a cache,
  ask what register the NEXT temp then gets**, and check whether the original
  leaves that register conspicuously idle — in these functions it leaves ECX
  free across ~20 indices and nothing in source raises pressure enough to
  reproduce that.
- **`(void)&x;` and `if (&x) { }` do NOT make a local address-taken.** VC6 folds
  both away completely, so neither is usable as a cheap way to force a spill
  home or defeat enregistration. What does work as a reload forcer is a
  volatile-qualified read through the pointer's own type,
  `(*(T* volatile*)&x)`, which emits a genuine second load at each site.
- **An empty `if` is a reassociation barrier only where it already sits.**
  Inserted at the seam it was measured at, an empty `if` inside a four-
  subtraction accumulator chain is inert; moved to any of the other four seams
  it CAUSES the reassociation it was meant to prevent (a `neg eax` appears) and
  costs 227-242. Barrier constructs are position-specific — re-measure one at
  every seam before treating it as a general instrument.

- **An offset-blind mismatch can hide an operand-order difference — build the
  bijection.** `[ebp+8]` and `[ebp-0x10]` both normalise to `ebp?`, so a pair
  that is really two operands the wrong way round is silently bucketed as
  "frame offset" and excluded from the search. The instrument: take EVERY
  index-aligned pair whose offset-blind text agrees, and read the frame map off
  those — matching lines pin the mapping, after which a mismatching line can
  *contradict* it. Window it, or VC6's slot reuse across disjoint live ranges
  raises false alarms. `scratchpad/w10p3d/bij.py`, `bij2.py`. This reclassified
  2 of `Draw3DPersonModel`'s "367 frame offsets" as a real operand order, and
  is worth running on any function whose residual is mostly `[ebp-N]`.
- **Three-term flat sums are canonicalised too — the "keeps source order" half
  of the four/three-term rule was a mis-summary, now corrected below.** The two
  claims were in the corpus simultaneously and contradicted each other. Three
  independent measurements agree that once a three-term sum is FLAT, all six
  textual orders and every parenthesisation compile identically: the ~200
  variant study, the 24/6 canonicalisation caveat on the add tie-break, and now
  `Draw3DPersonModel` 508/509, where all six orders of `(p->tint << 24) + t +
  yy`, both parenthesisations, a `+=` accumulate form, a tint temporary, local
  renaming and declaration order are **byte-identical** and the emitted order
  does not move under five frame perturbations. What the `StepSchoolCar` error
  actually established is narrower and still holds: do not apply the partial-sum
  aggregate BARRIER to a three-term sum — that is about whether the sum is flat
  at all, not about the order of a flat one. **Consequence: an operand order you
  cannot move by rewriting the sum is not evidence the sum is wrong** — look
  upstream at what is perturbing the canonical order.
- **Canonical operand order for a sum of two struct fields is HIGHER
  DISPLACEMENT FIRST.** Proved on a standalone synthetic: `-(fr->bmax.x +
  fr->bmin.x) >> 1` alone emits `[eax+12]` before `[eax]`, and swapping the
  source is byte-identical. Useful as a *diagnostic*: where the original obeys
  it and the reconstruction does not, the difference is not the sum — something
  upstream is perturbing it. In `Draw3DPersonModel` the perturbation is an
  intervening call, and ~40 spellings, placements and statement splits all cost
  25-600.
- **A frame-weight model can be falsified from the other end, by size.** Rather
  than only counting references, grow one array and watch what it overtakes:
  `mt[9]->mt[12]` (36B->48B) drops `mt` below an 84B array, which forces any
  linear key `size - k*refs` into k in (0.667, 0.89) — and in that whole range a
  96B/27-ref object can never outrank an 84B/64-ref one. That is an independent
  second proof of an unreachable frame order, obtained without another thousand
  source variants.

- **Stashed u16 fields are not always u16 locals.** A `u16` field saved into a
  local and restored later may well be an `int` local: check the width of the
  STORE. `xor ecx,ecx / mov cx,[map+0x20] / mov DWORD [esp+N],ecx` is a
  zero-extending load into a FOUR-byte spill home, where an `unsigned short`
  local emits a 2-byte `mov word ptr`. In `RenderFullMap` that one type change
  was worth 151 of 1064 mismatches.
- **A zero register displaces, it does not just substitute.** A function-wide
  constant zero parked in a callee-saved register costs more than the
  `test reg,reg` forms: it pushes whatever WOULD have used that register into
  another callee-saved one, whose live range then runs to the end of the
  function, so VC6 can no longer sink that register's push. One zero in a tail
  can therefore turn a split prologue into an entry prologue, rename every
  register and shift the whole frame by a slot (`RenderView`). When asking "why
  are the pushes at the entry", look at which register the ORIGINAL frees at
  its pop point, not at first use.
- **Array indexing form decides scaling.** `arr[i].x` with an unscaled `i`
  gives `shl`/`add` plus based stores; `*(int*)((char*)arr + i)` with `i`
  pre-scaled lets VC6 fold the scale into an `lea`.
- **Some of the original is hand-written assembly, and must be reproduced as
  assembly.** The four software triangle rasterisers (`tri3d.c`:
  `DrawFlatTri`, `DrawGouraudTri`, `DrawFlatTexTri`, `DrawGouraudTexTri`, 2743
  instructions) are `__declspec(naked)` in the reconstruction because no C
  produces them. Two independent proofs, both read off the original:
  `xchg dword ptr [ebp+0xc], eax` — no compiler emits `xchg` against memory —
  and callee-saved pushes sitting INSIDE the instruction stream (`push ebx/esi`
  after two movs, `push edi` after a `cmp`: Pentium pairing slots). VC6's
  inline assembler always emits those pushes at the top of an `__asm` region
  and splitting the region does not move them, so they cannot be compiler
  output. Before grinding a function whose codegen looks impossible, check for
  these two signatures — the answer may be that there was never any C.
- **Frame layout (the lever that finished `SaveGame`, 1196 instructions).** VC6
  lays the frame out as **[block-scope pool][function-level locals in
  DECLARATION order, ascending]**, so the width of the block-scope pool decides
  where the function-level run starts: one pooled slot puts the first
  function-level local at 0x14, two would push it to 0x18. Address-taken locals
  of **disjoint** blocks share one pool slot regardless of lexical DEPTH (a
  local four levels down can share with one at the top level) — an earlier note
  claiming they had to be sibling scopes at matching depth was wrong. The trap
  is an enclosing block-scope variable that is live across those blocks: it
  interferes with all of them, forces a second pool slot, and VC6 then gives the
  OUTER variable the LOWER home, which no declaration order, name or nesting
  depth can undo. When two frame homes come out swapped, look for that shape.
- **`goto` block ordering.** VC6 lays a `goto LABEL` target block out BEFORE the
  block the function merely falls through into at the end. When two identical
  epilogues come out in the wrong order, make the one the original puts first
  the labelled goto target (jumping INTO a compound statement is fine).
- **Reading a disassembly**: an `[esp+N]` reference emitted between a `push` and
  its `add esp,N` names a frame home 4 or 8 bytes LOWER than N. Several
  parameters were misread this way before the rule was noticed.
- **Register tie-breaks (`ClampScrollToMap`, 190/190 after sitting at 34%):**
  the source order of *independent multiplies* decides which product stays in
  eax in place — try all permutations early, it is cheap. A clamp written
  `v += e; if (x > v) x = v;` gives `v` two defs and makes VC6 keep it across a
  call; `if (x > v + e) x = v + e;` keeps a single def. Reads of a stack
  *argument* are CSE'd function-wide into one value whose allocation priority
  falls with its live-range length: routing an early read through a named
  temporary extends the range and can cost the later caching (including the
  edge-fix-up `jmp` + `mov reg,[esp+arg]` block); inline the early use.
  Declaration order, `register`, `&&` vs nested ifs, `y+y` vs `2*y`, local
  copies of arguments and identity `static __inline` helpers all normalise to
  identical code — none are levers for this class.
- **A dead `and dx, 0x20` is a merged-arm ghost, not a peephole.** `RestoreBaseMap`
  (0x0045da60) computes `code & 0x20` into dx and never uses it. The C that
  produces it: a u16 temp `reserved = code & 0x20` consumed by a compare against
  the flag *value* (`reserved == 0x20`, not `!= 0`) inside a three-arm
  `if / else if / else` whose arms are all the same one-word store. VC6's range
  knowledge folds the second compare, fuses the first test into the `and`, merges
  the identical arms late and peels the jump — leaving only the `and`. A
  register-resident flag *test* is spelled `test dl, 0x20`; a 16-bit *value* op
  survives only with more than one 16-bit consumer. `LoadBaseMap` has the same
  ghost at 0x00462333 (`test byte ptr [..], 0x20` with no jump).

- **A single-bit reader is a boolean test, not a shift (`Route_IsClosed`,
  2026-09-03).** `movsx eax, byte ptr [..] / and eax, M / shr eax, k` on a
  one-bit mask is VC6's lowering of `int s = *(char*)p; if (s & M) return 1;
  return 0;`. Spelling it `(x & M) >> k` reassociates to `sar k / and 1`, and
  testing the field directly (`(f & M) != 0`, `? 1 : 0`, `!!`) narrows the
  load to `mov al`; the `movsx` survives only when the byte is first widened
  into an `int` local that the test consumes. ~60 measured variants.
- **A float bit-copied through integer registers is never CSE'd
  (`PositionRouteCars`).** Two reads of the same stack argument — `mov ecx,
  [esp+a]` for a field store and `mov edx, [esp+a]` for a push — mean the
  argument is a `float` (with the field, the callee parameter and the out-param
  all `float`); an `int` argument is read once and shared. Types alone closed
  62 mismatches.
- **A boarded-flag loop must exit by `break` to ONE trailing `if (flag)
  f();` (`Coaster_TickLoadingBay`).** VC6 then keeps the flag in a
  callee-saved register (`xor ebx,ebx` before, `mov ebx,1` inside) and
  jump-threads the constant paths. Writing the same exit as an inner
  `if (flag) f(); return;` makes VC6 peel the first iteration (a hoisted copy
  of the call, flag deleted). `while`, `for(;;)` and `while ((c = g()) != 0)`
  all work; `do/while` with the same `break` is peeled.
- **Split prologue via a shared duplicated tail (`SchoolCarAccelerate`).**
  When both exits store the same fields from the same registers, the source is
  one shared `c->a = va; c->b = vb;` tail after `if (len) {...} else {...}`:
  VC6 tail-duplicates it, lays the else arm after the normal epilogue and
  sinks the pushes of registers first defined past the leading guard. An early
  `if (len == 0) {...; return;}` with direct stores pins every push at entry
  however it is scoped. Companion tie-breaks: an `int` local for a `u8` field
  buys `xor ebx,ebx / mov bl`; `--frame` on that int still narrows to
  `dec bl`; `c->f = expr & 0xf; d = (c->f - frame) & 0xf` gives a byte
  `and al,0xf` (field forwarded) where an int temp gives `and eax,0xf`.

- **An uninitialised-local reload is a real load (`LFTrack_Update`,
  `LFTrack_Add`).** A `mov reg, [esp+N]` on a fallback path that re-reads a
  slot another local lives in is a read of an UNINITIALISED variable (original
  bug): write the `if / else if` chain with no else arm and VC6 homes the
  undefined value in a dead argument slot and emits the load. Any explicit
  fallback (`(LFRun*)nb`, a union read, a cast through `&nb`, `volatile`) is
  CSE'd into `mov reg,reg`.
- **Dummy out-pointers home rightmost-first (`LFEntrance_Update2`).** Two
  uninitialised locals passed as out-pointers always land with the
  first-evaluated (rightmost) one in the lowest dead-argument slot; no
  declaration order, scope, type or helper changes it. When the original has
  them the other way, one dummy is the address of a dead PARAMETER.
- **Indexed loops keep store-before-load (`SaveLogFlume`).** Over an array of
  records, `b[i].f = call(); b[i].g = call(b[i].g);` with `i` counting up (VC6
  reverses it to `mov edi,4 / dec edi` and strength-reduces the pointer) keeps
  the `f` store before the `g` load; the walking-pointer form hoists the load.
  The record struct needs its real size so `b[i]` strides.
- **A duplicated else arm names the scratch registers (`LFTrack_Interact`).**
  `if (a) { if (b || c) Alt(); else Normal(); } else Normal();` (VC6 merges the
  two `Normal` calls) puts the mode temp in edx for Alt and ecx for Normal; the
  single `if (a && (b || c))` form gives the opposite. A `switch` reproduces
  the allocation but lowers the compares to `sub ecx, K`.
- **Constant stores to globals sink, so put them first (`LFEntrance_Update`).**
  VC6 sinks immediate stores to globals below the computed stores of the same
  block, so emitted order does not show where they were in the source. Putting
  the link-chain stores first reserved eax for the zero from the top of the
  block and made the loads alternate ecx/edx as the original does.
- **A two-statement adjustment keeps association and can move register
  allocation back into the prologue (`LFEntrance_Update`).** `a.y = fp1 + my;
  a.x++; a.y -= dy;` merges into one store but keeps `(fp1 + my) - dy`, and
  that changed priorities so far back that a prologue mismatch vanished. Every
  single-expression spelling canonicalises to the same object.

- **An aggregate local defeats forward substitution into a commutative sum
  (`IsAdjacentPos`).** VC6 forward-substitutes every scalar temp into `A + B`
  and then sorts structurally identical operands by the displacement of their
  loads, descending (`+4` before `+0`). Storing the two operands into fields of
  a `Pos` local (or `int d[2]`, or an address-taken `int`) evaluates them in
  source order and makes the sum's destination the SECOND operand's register
  (`add eax, ecx`, no closing `mov eax, edi`). Diagnostic: "add dest = second
  operand" means an operand was not forward-substituted. `volatile` on the
  loads does not pin their order.
- **A same-width type conversion is a CSE barrier (`RequestRoute`).** An
  `unsigned`/`long` field read into an `int` store makes VC6 reload the field
  rather than CSE it.
- **A `Pos` by-value parameter is byte-identical to `(int, int)`** at a call
  site whose argument is an address-taken struct: VC6 forwards the fields, so
  the callee prototype is not recoverable from such a site. (`RequestRoute` is
  defined `(Pos from, Pos to)` in simcore.c and declared `(int, int, int,
  int)` in popup.c; ABI-identical, both left as they are.)

- **Free-a-global-list stores the head from eax only when the global IS the
  loop variable (`sub_4828f0`).** `while (g) { next = *(void**)g; MemFree(g);
  g = next; }` lets VC6 forward the head store into the loop test, so eax
  carries the value and the store reads it. Any local `p` (while/do/for,
  chained assignment, casts, inline helpers) copy-propagates `next` into the
  store and writes esi. ~35 inert variants. The same idiom sits, unmatched, at
  0x4054d5, 0x419f8a, 0x419fc5, 0x434ee7, 0x434f22 and 0x48117b.
- **Return-block cloning for a deferred push, and the constant hoist it drags
  in (`SaveEmptySlotInput`).** When `push esi` is sunk past leading guards and
  the body ends in a `return K` shared with the guard-fail paths, VC6 clones
  the return into the pushed region and then hoists `mov eax,1`, reordering
  the adjacent immediate stores. To get ONE shared `mov al,1 / ret` with
  immediate stores in source order, put a branch between the body and the
  final return whose arms both reach it and whose test is already
  register-resident on every path: `if (g) return 1; return 1;` on the guard
  global folds late with no residue. A test on a parameter leaves a dead root
  copy. A `push 1` argument before the stores kills the hoist on its own
  (`push 2` does not) because a call clobbers eax, which is why matched twins
  that call `PlayInstanceOfSample(.., 0, 1, 0)` never showed it.

- **An uninitialised `u16` local read as a WORD needs a one-member struct
  (`Roads_CalcCursor`).** A `unsigned short` local whose every use is a promoted
  compare gets int-wide storage, so its undefined-value load is `mov esi,
  dword`. Wrapping it (`struct { unsigned short id; } group;`) keeps it in
  `si` and reads the same slot as `mov si, word`. `short`, scoping,
  `else x = x;` and an inline helper with a u16 parameter do nothing. (The
  slot is the spilled `snap` pointer in the dead `o` argument home — the
  original bug is "low word of a pointer used as a road group id".)
- **A flag computed from a `volatile` through a plain scalar rotates the
  scratch registers (`BoatingSchoolWater_Remove`).** `north = mask & 1;
  if (north)` on a volatile evaluates the rvalue into a temporary that takes a
  slot in the eax/ecx/edx rotation; `n0 = mask & 1; north = n0; if (n0)` makes
  it a variable def in eax outside the rotation (38 -> 21 in one step).
- **Widen byte fields into `int` locals before a call, in declaration order
  (`BoatingSchoolWater_Remove`).** `f(st->ax, st->ay, st->bx, st->by)`
  evaluates right-to-left straight into pushes; `int ax, ay, bx, by;` read in
  natural order gives the original's load order and moved the loop walker from
  esi to edi. All 24 orders measured; only ax, ay, bx, by is exact.
- **Aggregates are never the answer for SPILLED flags:** a struct/array of
  flags collapses the frame; spilled flags need separate homes, including dead
  argument slots.

- **`imul` operand order is a fixed RANK, never source order (`JcBoat_Animate`,
  ~270 measured variants).** For `a * b` with one foldable memory operand VC6
  emits `mov r, X / imul r, Y` by rank: (1) a compiler TEMPORARY — the result
  of an operation in the block, a call result, or a load CSE'd because it is
  textually repeated — is always the destination copy, so the other operand is
  folded (`(j-k)*g[1]`, `h(j)*g[1]` give `imul r, [g]`; the same load through
  a NAMED local ranks as a symbol instead); (2) a MEMORY reference outranks a
  register-candidate symbol and is loaded into the destination (`j*g[1]` and
  `g[1]*j` both give `mov r,[g] / imul r,j`); (3) two SYMBOLS order by
  DECLARATION order — the earlier-declared is copied, the later folded from its
  home slot (swapping two parameters flips it). Source order, `register`,
  same-width casts, identities and inline helpers all normalise first. This
  explains `Sub_423480` (a CSE'd field read used twice) and `MapToPlayfield`'s
  `imul eax, [esp+8]`. The one residual it does not reach: a product whose
  multiplier was a bare temp copy of an IV that no C spelling reproduces.

- **Inlined-helper locals escape alias analysis (`EarthSlide_Tick`).** An
  address-taken local born inside a `static __inline` helper is not in the
  caller's escaped-local class, so a store through an unrelated pointer no
  longer orders against loads of it and VC6 can hoist the load above the
  store. A named function-level local whose address went to any real call
  pins every such load after every pointer store. Worth the last 2.
- **Aggregate `= {0}` placement is a SCOPE lever (`Carousel_Draw`).** A
  function-level aggregate initialiser's fill is emitted before every other
  statement (120 permutations measured); putting the array in an inner scope
  places the `rep stosd` exactly where the scope opens, so a load written
  before the block precedes the fill and keeps its register through it.
  `n = 0` assigned BEFORE the fill is sunk past it and the next push and
  stored as an immediate; assigned after it reuses the fill's `al`.
- **Empty per-band guard blocks trigger jump threading and a widened count
  (`Carousel_Draw`).** With `if (n > 0)` guards on the same non-escaped char
  `n`, an empty guard block lets VC6 thread failed guards, prove later guards
  redundant and CSE the four `movsx` into one int; a `lea` in the guard block
  (an inlined parameter copy the helper walks) suppresses all three.
- **Cache `b = r->bloke` but pass `r->bloke` to the trailing call** to make VC6
  spill the walker into a dead slot and keep the bloke in esi across calls.
- **A compile error can yield a stale "0 mismatches" from a side-by-side
  lister** that reuses the previous object; make harnesses fail loudly on
  `error C`.

- **Under `#pragma optimize("", off)` in an /O2 file, frame-slot order is
  decided by the locals' NAMES (`Castle_Activate`, 62 -> 0).** VC6 walks its
  symbol hash table bucket by bucket assigning ebp-4 first; two names in one
  bucket come out most-recently-declared first. Declaration order, first use,
  type, block scope and `register` do nothing unless names collide.
  Single-letter names bucket by `c mod 16`, two-letter names by
  `(4*c0 + c1 + 6) mod 16` interleaved between them; longer names follow no
  fitted rule. Practical method: `scratchpad/castleobj/probe.py name1 name2
  ...` compiles a pragma-off function with those int locals and prints their
  homes via `/FAs`, so a candidate set is measured in seconds and transfers
  exactly. Almost certainly why `Castle_Interact`'s two `Offset`s "needed" a
  block scope.
- **Two loop-invariant loads from ONE struct hoist in descending displacement
  order (`CastleDummy_Interact`):** y at +0x0a takes the first callee-saved
  register, x at +0x08 the second; two separate globals hoist x-first whatever
  the order or names. A `short` field's hoisted load widens to `mov r32, dword`
  when the struct ENDS 4 bytes after it (x at +8 in a 12-byte struct); with
  fields after y it stays `mov r16, word`. A loop-invariant compare key read
  inside the loop (`sq.id == t.id`) hoists into the preheader after the
  trip-count guard; copying it to a scalar before the loop reads it before the
  guard and reshuffles esi/edx.

- **The Pentium scheduler works in fixed-size windows of IR tuples counted
  from the FUNCTION START, straight across calls (`SetBlokePositionFromBNV`,
  ~81 tuples, ~120 measured variants).** Within a window a dependency-free
  "root" instruction (a constant def like `xor edi,edi`, a parameter load, a
  global store) is hoisted to the window's top — for a flag writer, right
  after the last earlier flag writer; a root whose tuple falls in the NEXT
  window is emitted at that window's first pipeline-stall slot (after
  `fsqrt`, after `fdivr`, in an `fmul` -> `faddp` latency gap). So where a
  loop-counter zero lands in an FP stream is decided by the tuple COUNT ahead
  of the boundary, not by where `i = 0` is written. **Tuples that produce no
  code still occupy slots:** an explicit `(float)` cast on a float product
  (`x = (float)(x * y)`) survives as a no-code conversion tuple and moves the
  boundary one instruction up the stream; `*= inv` and `= x * inv` do not.
  Diagnostic: an integer instruction a few slots too LATE in an FP stream that
  no statement placement reaches wants no-code tuples earlier in the function.
  `add esp,N` after a call is itself a low-priority movable instruction.

- **A 16-bit register copy of a call result (`mov di, ax`) is a narrowed
  TEMPORARY, never a `short` local (`BuildChannelTables`).** VC6 widens a
  `u16`/`short` LOCAL whose uses are all narrow and copies it 32-bit
  (`mov edi, eax`) whatever the cast. It emits the 16-bit copy only for a
  compiler temporary — a CSE'd textually-repeated expression, or a
  `short`-returning call consumed at once — whose every consumer narrows to a
  word. Diagnostic: `mov r16, ax` after a call with the value later
  re-widened by `mov eax, edi` means spell the value twice, do not name it.
  The twin idiom sits unmatched at 0x00422ef8/0x00422f8c (`call __ftol /
  mov bp, ax / shl ebp, cl / or ebp, eax`).
- **Put a loop-invariant conversion INSIDE the loop to move its constant's
  allocation after the preheader ALU ops (`BuildChannelTables`).** `m = max`
  (unsigned -> double, u64 staging whose zero high dword is shared with
  `i = 0`) written before the loop allocates the zero while `cl` still holds
  a shift count; written inside the loop and hoisted, the zero takes the
  just-freed ecx and eax stays free for the shift scratch. Allocation follows
  the hoisted tuple's position, before scheduling: a "root hoisted above the
  shr" is an allocation symptom, not a scheduler one.

- **A nested call inside an argument list vs in its own statement flips the
  whole allocation (`Copters_Activate`, 213 + ESCAPES -> 0).** For
  `f(g(x)[i], a, b, c)` VC6 pushes the simple arguments before calling `g`,
  so `a`/`b` never survive a call and stay in scratch registers; written
  `i = g(x); f(tab[i], a, b, c)`, `g` is called first, `a`/`b` are live
  across it and take the callee-saved registers, the loop cursor and ObjDef
  get spilled (rotated loop with a `jmp` into its middle) and a shared tail
  stops being duplicated. The index must go through the same `i` the other
  cases use.
- **An escaped `Offset` local pins pointer loads after its stores; the same
  block as a `static __inline` helper frees the scheduler
  (`PlaneRide_Interact`, 5 -> 0).** By value, by pointer, or as two ints for
  the extra argument are all exact.
- **`mov dword [esp+N], 0` + `rep stosd` of N-1 dwords is `T a[N] = {0}`**,
  not an N-1 array plus an int (`SpiderRide_Interact`); declare `n` before it
  and open the array's scope after `r = item->riders;`. **Two scalar zero
  stores followed by another local's stores let VC6 hoist the later store and
  split a byte compare into `mov al / cmp al`; one chained `a.x = a.y = 0;`
  keeps IR order and the compare direct** (154 -> 3).
- **Reusing a variable for a second-stage sum puts the sum in that
  variable's register; a fresh variable puts it in the first operand's
  register and lets the old value die** (`sx2 = cfg->ox - Get_XScroll() +
  sx`, the Barrels' def/rec ebx-ebp flip). Read `world.x/y` into `wx/wy`
  BEFORE `GetTileDimensions`.
- **`if (n > 0) { T* q = arr; int i = n; do .. while (--i); }` on a char keeps
  the `test al / je / jle` guard on the char and places the `lea` after the
  `jle`**; a helper walking its parameter hoists the lea into the guard block.
- **Register-blind difflib alignment** (`scratchpad/mechrides/rank.py`,
  `rbdiff.py`, `frame.py`) separates naming cascades from structural
  differences far better than the strict count.
- **Semantics recovered the hard way:** `SpinningBarrels_SeatOf`,
  `SpiderRide_SeatOf` and `PlaneRide_SeatOf` take the RECORD, not the rider's
  tile (seat bytes at +0x21/+0x1c); `SafariRide_SeatOf` takes the tile.
  `lpConfig` (0x004bcbf4) is a POINTER to the map config, not the struct.

- **Phantom constant web, the ebx thief (`PrintSavedGameDetails`, ~200
  measured variants).** A non-trivial constant (not 0/1, not a lea-folded
  addend) used as a non-lea immediate before or in a loop AND again after the
  loop forms a CSE web whose def is the loop preheader. It claims a
  callee-saved register (ebx first) ahead of the call-crossing values living
  in that block, then is spilled in the loop and rematerialised as immediates
  — invisible in the code. Demotion happens iff the partner use is in the
  join/preheader block or the loop. Cure: a late-merged degenerate branch
  `if (!d) {X} else {X}` on a register-cached local with call-containing
  identical arms — VC6 merges it after web placement and emits nothing (the
  positive form `if (d)` duplicates X; a global condition leaves a ghost
  reload). Constant spelling, type, `static const` (not folded — VC6 emits
  loads), callee types and loop forms are all inert.

- **`short` locals for values that feed only 16-bit stores (`BuildCursorPtr`,
  6 -> 0).** VC6 keeps them in full registers with no `movsx`, but the type
  changes temporary allocation: an operand load lands directly in the
  destination's home and is consumed in place (`add edx, edi`) where `int`
  gives `lea` plus a separate load.
- **Inlined-helper parameter order is an evaluation-order lever
  (`GetObjectUID`, 163 -> 20).** Arguments evaluate right to left, so
  `Cell(int y, int x)` creates the CSE'd `x` expression before `y`; this
  flips which value lands in eax/edi and can move a value into ebp and
  separate a `test` from its `sar`. **A `Pos` local for two compared sums
  computes both before the first compare** (scalar temps short-circuit below
  the first `jne`). **Textual repetition of a pointer field read flips a later
  sum to copy-then-add**; reading through a `Pos*` in the helper restores
  `mov eax, [field] / add eax, reg`.
- **Address-taken locals born inside an inlined helper stop the alias
  reloads and stay in the caller's pool slots** (`ValidateCursor`, 14 -> 5;
  same mechanism as `EarthSlide_Tick`). **Aggregate initialisers are
  non-aliasing stores**: VC6 evaluates them in field order and sinks the last
  store past pointer loads.

- **`while (1)` is not `for (;;)`: it stops CFG loop inversion
  (`UpdateGoalHelpText`, 335 -> 0).** VC6 copies the exit test into the latch
  (the `test/jne` bottom loop) only when the loop HEADER block itself ends in
  the exit conditional. `while (1) { g = head; if (!g) break; switch ... ;
  Remove(g); }` leaves a folded constant-true test block as the header, so
  the loop keeps its `jmp header` back edge. Also exact: a `goto again;` to a
  label placed ABOVE the `while`. Inverted (unusable): `for (;;)`,
  `do .. while (1)`, `while ((g = ..) != 0)`, `top: .. goto top;` with a
  `return` exit. A scan found no other non-inverted `mov r,[global]/test/je`
  loop in the executable. **Late tail duplication needs the uninverted
  loop:** VC6 clones a small block ending in `jmp`/`ret` into every
  predecessor after register allocation (every copy uses the same register)
  and before scheduling. Split `add esp,8 / add esp,4` pairs in duplicated
  tails mean the calls were in different source blocks.
- **A source-level `goto` into a sibling arm's tail vs compiler cross-jumping
  decides spill-home order (`SoftBlitAnim`, 8 -> 0).** The `goto` creates
  the shared block before the allocator assigns homes; the if/else spelling
  lets cross-jumping merge after. Emitted code identical, frame homes
  rotated.

- **`PrintSprite(spr, screen.ox + off.ox, ...)` — screen FIRST (`Joust_Draw`,
  42 -> 3 in one step, five call sites).** With `off.ox + screen.ox` the
  add's destination is the screen register and the sprite global loads after
  `push ebp`; screen-first puts the sum in `off`'s register and the global
  load before the push.
- **A `goto` label block is laid out after its LAST goto source in layout
  order**, regardless of where the label sits textually (label in case 0
  with gotos from cases 2 and 5 -> after case 5). Case blocks of a jump-table
  switch follow textual order. **Two distinct tail merges with opposite
  directions:** identical IR suffixes are merged by an IR-level pass into the
  LAST copy in layout; identical machine-code suffixes are cross-jumped
  post-codegen into the EARLIER block. A textual copy of a call after two
  stores always forwards exactly the last-stored field, so it can only
  cross-jump from the `call`; a full jump to the reload needs a block
  boundary. **The forwarded `push` of the last-stored field is a store-order
  lever:** only `world.x, world.y, target.x, target.y` reproduces
  `st st st st ld / ld push / ld` (`TempleSlide_Update`).
- **Family-wide unknown, do not re-derive per function:** the BNV-ride
  `_Update`/`_Activate` callbacks (`TempleSlide_Update`, the four in
  mechrides.c) all carry the same residual — two never-referenced frame homes
  and a dead `mov [esp+N], ebp` spill of `sx` — dead locals of the original
  whose stores were eliminated. Whatever reproduces it will transfer to all
  five at once.

- **An `add` takes the register of whichever operand DIES at it; a
  surviving operand is skipped, and a separate (even volatile) load of the
  dying operand does not prevent the coalescing (`LFEntrance_Activate`,
  ~170 measured variants).** A three-register `lea r, [a+b]` for a plain sum
  means both operands were live past it in the original's IR; no C spelling
  found keeps the dying one alive.
- **Read pointer fields into int temps before storing to an escaped local
  (`LFEntrance_Add`).** A store to an escaped local followed by a load
  through an unrelated pointer pins the store; with `fx = def->v[0]; fy =
  def->v[1]; a.x = ..; a.y = ..; a.x++; a.y -= dy;` the two `a.x` defs merge
  into `add / inc / one store`. Two adjacent `b.x = ..; b.x++;` fold into
  `lea [..+1]`.
- **VC6 forward-substitutes a single-use `n - 1` into a loop even across
  calls** (named local, two-def, const, block scope, struct member,
  address-taken, inline-helper parameter all rematerialise as `lea` inside
  the loop); only `volatile`, `short`/`char` (adds `movsx`) or a post-loop use
  keep a spill slot. **Spill slots are assigned low-to-high in priority
  order**, so slot order reveals allocation order — a diagnostic.
- **The unreferenced frame slot before a `Pos` local is the 8-byte alignment
  hole VC6 leaves between a 4-byte slot and the aggregate, not a variable**
  (`LFEntrance_Add` entry-0x14; also `SpaceTower_Activate`'s "phantom").

- **Per-band `movsx` of a char count without CSE = a `char` loop index
  (`GeneralStore/Saloon/LegoMedia_DrawOverlay`, 27/22/16 -> 0).** `char i;
  for (i = 0; i < n; i++) list[i]` on a `char n` gives, per band, `test bl,bl
  / jle next / lea esi, queue / movsx edi, bl / loop`. With an `int` index the
  sign-extension is one CSE-able expression (hoisted once, spilled, guards
  threaded); with a `char` index the compare is byte-wide, the dword trip
  count is created late per loop and never CSE'd, and the guard is the
  inverted loop test with the `lea` AFTER the `jle`. `short`/`unsigned char`
  indexes, `!=`, and every pointer-walking `do/while` are wrong. This is the
  residual recorded in westtown2.c (`LegoShop2_DrawOverlay`,
  `JailCell_DrawOverlay`), ridecb1.c (`Carousel_Draw`, `Balloonz_Draw`) and
  mechrides.c's residual (b).
- **A pointer local to a packed key struct flips which operand a commutative
  `add` coalesces with (`Explorers_TickCustomers`, 115 -> 0).** For
  `def->base_x + <key byte>`: written `r->ride_id.b.x` the zero-extended byte
  is the destination; written through `ShopTile* key = &r->ride_id;
  key->b.x` the dword field load is. Identical loads, only the destination
  changes, and that decides the x/y register pair and which cases cross-jump.
  ~60 inert variants (operand orders, field widths, casts, volatile, inline
  helpers, all 24 declaration orders).
- **The inline 2-byte `memcmp(&rec->tile, tile, 2)` materialises the first
  operand's address (`lea edx, [eax+4]`) before the folded word load** — the
  "dead lea" in the `_FindRec`/`_FindRecord` list searches whose key is at
  +4 (`JailCell_FindRecord`, `Carousel_FindRec`, `Balloonz_FindRec`).

- **Scratch-register allocation order in a call-free loop function
  (`WW_AnyBlokeInRect`, ~110 measured variants):** a web coalesced with the
  return value first (it takes eax even when live across the whole loop), then
  loop-local temporaries in def order, then loop-carried values, then hoisted
  invariants. So a loop-carried pointer cursor lands in ecx behind a
  loop-local temp in eax unless its web flows into `return`. `return (int)p`
  after `while (p)` is folded to 0 only at a single-predecessor exit; at the
  inverted loop's merged exit VC6 keeps it unfolded (cursor coalesces with
  eax, no zeroing emitted). Phantom uses (`p ^ p`, `p & 0`, `p ? 0 : 0`) fold
  at the front end; `(int)p >> 31` survives. Compare operand order survives
  to codegen: `r->left <= x` gives `cmp esi, eax / jg`, `x >= r->left` gives
  `cmp eax, esi / jl`.

- **A struct-by-value copy from an UNNAMED call-result pointer schedules
  argument loads differently from a named pointer local (`BuildObject`,
  8 -> 0 after ~300 variants).** `f(door, *g())` with `f(Pos, Pos)`
  interleaves the `[eax]`/`[eax+4]` loads with the pushes; `Pos* p = g();
  f(door, *p)` (or the four-int form) hoists both loads above the first push.
  The prototype alone is inert (ABI-identical); the unnamed-pointer copy is
  the lever. `RequestRoute` is therefore `(Pos from, Pos to)` from both sides.
- **A compare operand spelled as a named step rotates the scratch registers
  (`DrawPopUpInfo` strip):** `r = a->x + 0x24; if (r < m || m < l)` gives
  edx/eax/ecx where the inline form gives ecx/edx/eax. **Spill homes are
  handed out low-to-high in priority order with first-fit reuse of dead
  homes**, so a single allocation flip in one block re-sorts the whole
  frame; under /O2 names and declaration order are irrelevant. **VC6
  un-escapes local pointers completely:** `int* p = &x; *p = ..`, inline
  helpers with `int* out`, and helper-born address-taken scalars are all
  byte-identical to the plain scalar.

- **A rider-placement block must be a `static __inline` helper — three
  instances (`PlaneRide_PlaceRider`, `SpinningBarrels_PlaceRider`,
  `Balloonz_PlaceRider`).** Open-coded in the loop body, an
  `AdjustBlokePosition(&p->local)` block whose stored value is computed
  against an ESCAPED caller local (`pivot`, address-taken by
  `AdjustOffsetForViewMode`) gives the value eax and the pushed
  `lea &p->local` ecx. Moving the escaped `Offset`s into the helper makes
  them inline-expansion temporaries outside the caller's escaped-local class,
  and the pair is ranked as in the neighbouring block: value ecx, argument
  address eax. Diagnostic: when every other `lea`/`push` address in a loop is
  in eax and one comes out in ecx, a value temp in that block stole eax —
  the cure is the helper, not a re-spelling. ~35 inert variants (casts,
  `Offset*` locals, named difference locals, `pivot.oy = pivot.oy - 8`).
- **A field read into a named int local BEFORE an update to an escaped local
  is a schedule lever (`Balloonz_Draw`).** It lets the dependent difference be
  computed before the escaped store is committed, turning a `lea`-then-store-
  through-the-lea into a based store plus a later `lea`.
- **Address-taken `Offset` locals get ONE frame home each for the whole
  function, so which variable each call site uses is a FRAME lever, not an
  allocator one (`Carousel_Draw`, 24 -> 0 with the char index and
  `p = b->person` read after the two pivot stores).** The original's homes
  show which offset variable each path reused.

- **The move goes INSIDE the arms; the statement after it does not
  (`Saloon_TickCustomers` 207 -> 0, `LegoShop2_TickCustomers` 182 -> 0).** A
  random-waypoint case whose arms all end in the same inlined move must be
  spelled `if (..) { wp; Move(b); } else if (..) { wp; Move(b); } else { wp;
  Move(b); }` with the ONE post-move statement after the chain. Each arm then
  keeps a private five-push argument block (different scratch rotations, so
  they cannot merge) while the identical `call` and post-call tails merge into
  one. Hoisting the move out of the arms gives them one shared push block and
  regroups every other case; giving each arm its own copy of the statement
  plus a `break` stops them sharing anything (+14 instructions).
- **Cross-jump groups host the merged copy at the last block in LAYOUT order
  of the merged group** — REFINED 2026-09-04: it is layout order, NOT the
  join's fall-through predecessor (matched witness: ridecb1.c
  `Restaurant1_Tick` 0x0042f1a0). Earlier users jump forward into it. **The
  shared-tail LENGTH is set by where the varying argument sits in the
  parameter list:** a varying LAST parameter is pushed first, so everything
  after it is shared; a varying SECOND parameter caps the suffix at the
  address push, call, cleanup and jump. A copy's eax/edx/ecx rotation follows its position in the FINAL
  layout, so rotation is an effect of the merge, not a cause. Related:
  writing a case's arms as GUARDED EARLY BREAKS rather than an if/else chain
  decides which copy of a shared tail survives — with an else chain the inner
  then-arm's latch edge is re-added by jump threading after a later case's,
  making the arm the canonical holder (`LegoMedia_TickCustomers`, 88 -> 0
  after ~800 inert waypoint variants). The arm form of a case in the middle
  of a switch can also fix a register pair 30 instructions earlier in the
  loop head.

- **SOLVED, the family-wide "dead `mov [esp+N], ebp` spill + never-referenced
  frame homes" residual: it is ONE 8-byte spilled object of which only `.x` is
  written.** `struct { int x, y; } spill; *(volatile int*)&spill.x = sx;`
  reproduces both halves — VC6 lifetime-colours the pair onto a home that has
  just died and its second dword is the "phantom". Nothing else reserves a
  scalar-pool home: unused locals of every type are dropped, an address-taken
  scalar through a no-op inline helper is dropped, and an array keeps its size
  but goes to the TOP aggregate pool. This fixed the frame of all four BNV
  `_Activate` callbacks and applies to `TempleSlide_Update` (joust.c).
- **A volatile READ at one use forces spill-at-def plus reload-there; a
  volatile STORE of a variable to itself is folded away.** `f(r, *(Rec*
  volatile*)&rec, cap)` produced the original's spill between `test` and `je`
  and freed the register for the next value.
- **A store between two `x -= a; x -= b;` statements makes VC6 reassociate
  them** into `mov/neg/sub/add` (three registers, one extra instruction);
  moving the unrelated store ABOVE both keeps two separate `sub reg, mem`
  (Spider 321 -> 79 in one edit). Of four `-=` statements only the relative
  order of the two on the SAME variable matters (all 24 permutations
  measured).
- **An intrinsic 2-byte `memcmp` is the source of the unexplained "dead
  `lea`"** in a rider/record key test — `memcmp(t, sq, 2) == 0`, not
  `t->key == sq->key`. With `#pragma intrinsic(memcmp)` this closed
  `SpaceTower_Interact` (250 -> 0).
- **Two `Offset` locals are split by BLIT, not by kind** (`SpinningBarrels_
  Interact`, 39 -> 0): the layer-3 blit's offset in one home, the two matte
  blits' AND the layer-2 blit's in the other. **A rider pivot is ONE 8-byte
  global copied whole** (`Offset piv = g_pivot;`) declared before the person
  pointer, which issues the loads in descending displacement order with the
  person load between them. **A `char`-typed extern parameter fed a truncated
  `short` field** gives `mov al, [mem] / push eax` with no zero-extension.
  **Store `pos.y` before `pos.x`** to defeat the descending-displacement sort
  of two `movsx` loads. **A named `tile` local used at every site vs the macro
  at each site** is a real lever (Spider 212 -> 165).
- **Data:** `NewBNVPath`'s 6th parameter is a 3-int position, not a `Pos` —
  the never-written third dword above each seed local is its z.

- **A store to an escaped caller local is a CSE barrier for pointer-field
  loads (`ValidateCursor`, ~1760 measured variants).** Even an immediate
  store (`bound->left = 0`) between two reads of `cur->origin.y` forces a
  reload; locals born inside a `static __inline` helper are not in that class.
  **Diagnostic with teeth:** if the ORIGINAL keeps one load across an escaped
  store, that store was not there in the source — the scheduler displaced it
  later, and no source reordering will reach it. That is why
  `ValidateCursor`'s last 5 (aligned distance 2, one displaced instruction)
  is **exhausted**: treat it like `UpdateControllerFromMouseData` and leave it.
- **Aggregate-initialiser stores are non-aliasing, but the sunk store is
  always the HIGHEST-offset field and sinks exactly ONE slot** past a
  following pointer load. `WinRect b = {0,0,h,h};` also gives TWO loads of
  `h` — no CSE across the initialiser. So the sink cannot be aimed at a
  middle field. (Refines the earlier aggregate-initialiser note.)
- **`Map* m = g_map;` folds to a direct global read whenever two consumers
  that VC6 would CSE share the same pointer expression** (helper-local,
  caller-local or parameter, at first use or at the top — all byte-identical,
  104 measured groupings). Break that sharing with two different locals and
  the fold stops: the global's loads become one PRE web with landing-pad
  reloads at each region entry that edges still holding the value jump past.
  Landing pads and cross-consumer field CSE are mutually exclusive from any
  pointer-variable spelling (`GetObjectUID`, stuck at 20 for that reason).
- **Two negatives that close cheap searches:** integer casts leave no
  surviving no-code conversion tuple (unlike the float case — `(int)`,
  `(unsigned)`, `+0`, `|0`, `*(&x)` on a `u16` read are byte-identical); and
  there is NO scheduling-region boundary at an inline call site — moving a
  caller's stores into the inlined helper leaves the schedule byte-identical,
  and padding ahead of the block shifts it without changing a load-to-store
  gap. (The `SetBlokePositionFromBNV` tuple-window lever still stands for FP
  streams; it did not apply to this integer code.)

- **An extern's parameter WIDTH is a caller-side lever worth whole
  instructions (`Restaurant2_Tick`, 262 -> 56 from this alone).**
  `StartSound(unsigned short)` makes VC6 emit `mov dx, word ptr [esi+4] /
  push edx` leaving the top half dirty; declared `int` it inserts
  `xor edx,edx` first. Four call sites, four instructions.
- **Switch case blocks are emitted in SOURCE order, so case order IS the
  block layout.** **Counter-step branch polarity:** write the counter step as
  the `if` arm and the state change as the `else` — the step then falls
  through and stays a register load/step/store, where the natural
  `if (s > 8) {..} else s+1;` inverts the block and folds the step into a
  memory `inc`. **A deliberately repeated test builds VC6's two-entry join
  block** (`if (a && b) goto x; if (!a) goto y; goto z;`) and puts the join
  before the call, where the straightforward spelling emits the call first
  and jumps backwards into it.
- **`key` before `b` at a rider-loop head keeps the cursor in eax:**
  `next; key = &r->ride_id; b = r->bloke;` gives `lea ebp,[eax+0xc]`; with
  `b` first the cursor goes to a callee-saved register. **A may-alias store
  is a schedule barrier you can place** — reading a field before an unrelated
  store through a global pointer stops VC6 hoisting the load past it (this
  removed `Balloonz_Tick`'s ESCAPES and 12 duplicated instructions).
- **An `unsigned char` index taken from a call result round-trips through its
  home** when the zero-extension is needed after an `add esp,N`; declare it
  `int` and cast at the uses.
- **Second (simpler) reproduction of the BNV-ride "dead `mov [esp+N], ebp`
  spill of sx": make the post-scroll x a SEPARATE local from the pre-scroll
  one** (`sx2 = cfg->ox - Get_XScroll() + sx`). Doing the same to `sy` is
  worse. Compare the volatile-spill-pair form recorded above; try both.
- **Cross-jump phase diagnostic:** sibling case blocks building the same call
  get a repeating three-step eax/ecx/edx rotation, and only the blocks landing
  on the same rotation as the LAYOUT-LAST copy have their tails cross-jumped
  into it. The register of the first repeated `lea` tells you the phase.
  **Record copy-in/copy-out store order is mechanically hill-climbable**
  (all-pairs swaps); re-run it after every other change.

- **VC6's identical-suffix merge keeps the copy that is LAST IN SOURCE
  ORDER** and turns the earlier ones into jumps — four independent probes,
  including a 7-case jump table where permuting the labels moved the
  surviving copy. **A `goto LABEL` target block is moved after its last goto
  source even when it is also a fall-through target** (the fall-through
  predecessor just gains a `jmp`), so a shared tail reached by a backward
  jump into its MIDDLE can never come from a `goto`. Corollary that closed
  `TempleSlide_Update`'s block layout: the shared tail must be a TEXTUAL COPY
  in every arm, with no `goto` anywhere.
- **A `static __inline` helper's arguments are evaluated into temporaries
  before its body runs**, so `H(&a, &b->f, x)` with body `{ a->m = K;
  b->f.n = x; }` yields `[ld x; lea &b->f; st a->m; st b->f.n]` — a store
  placed between an address computation and its own store, which NO plain-C
  statement sequence can express (comma expressions, LHS-comma forms and
  function-like macros all normalise to statement order first). Passing
  `&local` into the helper is allocation-neutral on its own; it is the
  resulting store reorder that rotates the whole function's allocation, which
  is why `Joust_Draw`'s known 3-instruction fix costs 63 elsewhere.
- **When a single cross-jump displaces a block, the strict index count is a
  misleading guide** — use an LCS alignment and a register-blind LCS
  (`scratchpad/joust/align.py`, `rbscore.py`). A loop-head callee-saved
  assignment can be decided purely by register pressure elsewhere in the
  function and be totally insensitive to spelling (17 identical variants).
- **Data:** `RideDef.qx`/`qy` are signed chars at +0x24/+0x25.

- **A `volatile` read forces ONE extra step of VC6's eax->ecx->edx scratch
  rotation at the point it sits, and that decides tail merging
  (`BoatingSchool_Tick`, 289 -> 53).** Two switch arms making the same call
  with the same arguments merge ENTIRELY (pushes included) when their temps
  landed in the same register, and merge only FROM THE `call` when they
  differ; the original's signature for the split is a short inline arm ending
  `jmp <the other case's call>` with the push copies in different registers.
  Apply the shim only where the original itself has a register copy — at
  sites where it pushes straight out of the store's register the shim turns a
  CSE into a memory reload and costs more than it saves.
- **VC6 hoists two identical `*(volatile T*)&x` reads out of both arms into
  the dominator**; a volatile read is not a per-arm barrier, and only a DEAD
  volatile read (a bare expression statement) survives per-arm. **Arm order is
  not a lever for a two-arm reload diamond** — `if (c) A;`, `if (!c) ; else
  A;` and goto chains all canonicalise to one layout (unlike switch cases,
  where source order decides). **Copy-then-increment of a SECOND variable
  always folds** to `mov r,[n] / lea d,[r+1]`; only a read-modify-write of
  the same variable gives `mov d,[n] / inc d`.
- **Diagnostic for a body that is one instruction SHORT:** deliberately
  insert one wrong extra instruction early (spell a guard `x - 5 != 0` so it
  emits mov/sub/je) and watch the mismatch count FALL — that localises the
  missing instruction downstream.
- **Exhausted, do not re-grind:** `BoatingSchool_Add` at 8 (the full 36-body
  cross-product of take-position x link-order confirms the current spelling
  is the unique minimum) and `ValidateCursor` at 5.

- **An empty trailing `else { }` flips a shared tail's cross-jump direction
  (`Restaurant2_Draw`, 171 -> 0 on that token alone).** In an if/else chain
  the LAST arm falls through into the join, so its copy of a shared tail is
  free to keep while an earlier arm's costs a `jmp` — VC6 therefore deletes
  the EARLIER copy. Add one more, empty arm and the previously-last arm must
  jump like the rest, the tie breaks the other way, and the merge goes
  backwards into the earlier arm. `else { }` is the only spelling that works:
  `else { stmt; }`, a trailing `else if`, and a separate trailing `if` all add
  real code, and `else { n = n; }` degenerates into a goto-shaped chain.
- **A `goto` into a sibling case's tail forbids the deferred `add esp`.** VC6
  merges two consecutive calls' cleanups into one `add esp, N+M` even across a
  `jmp`, but only when every predecessor of the shared block has the same
  pending stack depth; a `goto` lets a case with an empty stack merge in at IR
  level and kills the deferral. Spell the tail out in full in both cases and
  VC6 cross-jumps them post-codegen instead: identical code, deferral intact.
- **A constant-indexed 1-D array local defeats forward substitution and costs
  no frame slot** (`int oa[2]; oa[0] = tab[row].x; oa[1] = tab[row].y;` keeps
  both reads in registers where plain `int` locals fold back into the `add` as
  memory operands; VC6 scalarises the array). Once the operands are array
  symbols, statement ORDER becomes a strong lever: read the Y offset before
  the X one and put a store to the object between the reads and their use as a
  may-alias barrier.
- **Operand rank in a THREE-term commutative sum (partial, ~200 variants):**
  all six textual orders and every parenthesisation compile identically — VC6
  sorts the flattened sum before instruction selection. The computed shift (a
  temporary) always pairs first, and the operand it pairs with is the
  higher-ranked of the other two, with MEMORY outranking an array symbol.
  Making a term inline memory pulls it into the pair but VC6 then folds it
  (`add r,[mem]`, one instruction short per site).

- **VC6 reassociates `(X + c1) - c2` only when the `X + c1` node has a SINGLE
  consumer (`DrawPopUpMock`, 153 -> 0).** A second consumer evaluated BEFORE
  the subtraction keeps both `add`s; a later one gives `add` + `lea`. The
  zero-instruction way to create one is a test of the value in an `if` with an
  EMPTY body — VC6 deletes the branch only after the fold decision, so it
  costs nothing (`if (v)`, `if (v < 0)`, `if (v != 0)` are byte-identical).
  Width-preserving casts, `| 0`, `^ 0`, `* 1`, named locals, `const`, `enum`
  and inline accessors all fold instead.
- **The source order of two INDEPENDENT stores decides web creation order,
  hence allocation priority, even when VC6 re-emits them in the other order**
  (`RenderWorkOrders`, 320 -> 0: writing `colour = K;` before `o = head;` in
  both arms of the entry branch is emitted identically but keeps `o` in edx
  across a loop back edge and removes a landing-pad reload). Block scoping,
  declaration order and loop spellings were inert on the same problem.
- **Constant materialisation rule.** A constant with ONE register use plus an
  immediate use stays split (`cmp eax,32h / jge / mov eax,32h` from
  `if (cost < 50) return 50;`). With TWO register uses it becomes a web that
  absorbs every other occurrence, `cmp reg,imm` included, and its def lands at
  their common dominator. VC6 keys constants by VALUE ONLY — suffixes, casts,
  `37` vs `0x25`, and the return type make no difference. This is why
  `ClampPopUpToScreen` sits at 3: the original must spell the compare's
  constant as a distinct value, which C cannot express.
- **Technique worth reusing: mine the already-exact functions for a codegen
  shape.** `scratchpad/misc3/scan_const.py` (and scan2/scan3) grep every
  matched function for a given instruction pattern to find a worked example —
  that is how `GetBuildTime` and `SetBridgeDrawOffsets` were found as the two
  sides of the constant-web rule. `scratchpad/misc3/probe.py` compiles a
  standalone file and disassembles named functions, the fastest way to test an
  expression-level lever.
- **Data:** `ClampPopUpToScreen` (0x004718c0) returns `int` (the y bound it
  settled on), not `void` as popup.c declares; its `y <= limit` path is an
  original bug — it stores `y` but returns `limit`.

- **Cross-jump threshold, two regimes (measured, VC6 SP3).** Merging into the
  CANONICAL predecessor (the one the join falls through from) happens at a
  shared depth of 4 instructions; merging two NON-CANONICAL predecessors with
  each other needs 6. It is an instruction COUNT, not bytes (a 2-byte store
  counts like a 10-byte one). **The merge runs BEFORE the scheduler** — proved
  by construction: an argument-address `lea` can only end up inside the shared
  block if it was still after the `push` when the merge ran, and in every
  unmerged block the scheduler hoists it above that push. (`RenderCursor`'s
  remaining 250 mismatches are one 4-instruction shift from this: its shared
  suffix is 5, one short of the non-canonical threshold.)
- **Two textually identical occurrences of `A * B` are linked as ONE CSE
  candidate, and that changes `imul` operand ranking at the LATER site**
  (`SoftPrint_XBltFast`, and it dissolved a residual two earlier agents had
  recorded as "unreachable from C"). Spelled the same way round, the value
  already live in a register ranks as a register-candidate symbol and the
  memory operand becomes the destination copy; spelling one COMMUTED breaks
  the link and the live register becomes a rank-1 temporary. The fix is
  applied at the OTHER statement — the one that already matched.
- **Reading a stack PARAMETER as volatile (`(*(T* volatile*)&p)->f`) splits
  VC6's function-wide CSE of argument reads**, forcing the home-slot re-read
  the original has; applying it to the field instead gets the load order right
  but not the registers, and at both accesses is worse. **A textually repeated
  field read is a compiler TEMPORARY the scheduler treats as a critical-path
  root** and hoists with its dependent `lea` to the top of its block; naming
  it in a local makes it an ordinary symbol whose load stays in program order
  — a PER-CALL-SITE lever. **A volatile STORE to a global stops a later global
  load being hoisted above it.**
- **`case K: f(kk, ..)` where `kk` is the switch value is byte-identical to
  `case K: f(K, ..)`** — VC6 constant-propagates the switch value into each
  arm. **Inert, do not re-derive:** every CFG re-shaping of a two-arm if/else
  around a switch (`goto` either way, `do {..break;} while (0)`, `continue`,
  negated condition, if/else-if), all case-label permutations, same-width
  casts on arguments, and inline wrappers round the call. Shifting the
  instruction stream upstream does NOT move a cross-jump decision.
- **Data:** `g_map->tile_h` at +0x18 is genuinely UNSIGNED 16-bit; making it
  signed removes an ebx clobber and changes switch merging, so it must not be
  "fixed".

- **Where a field load that will live in esi sits relative to an intervening
  loop decides which callee-saved register the function-wide constant zero
  gets (`JungleCruise_Tick`, 32 head orderings measured).** Writing
  `b = inst->bloke;` BEFORE the station-search loop puts the zero in ebx and
  the outer cursor in ebp as the original has them; after it, the two swap and
  every `cmp r,0` / `push 0` / zero store in the function is off by a
  register. A dead zero store does not create the zero web (eliminated before
  web creation), and even a surviving zero store placed first does not move
  it — web creation order is not the mechanism.
- **The scratch-register phase of an ENTIRE switch body can be pinned by one
  loop in a single case arm.** Diagnostic: stub each case's body out in turn
  and watch where a head value lands — cheap, and it localises a whole-function
  phase error to one arm. **Any extra store between a struct store and a
  following call re-phases that block's whole register plan**, and a store
  emitted AFTER a call cannot have been before it in the source.
- **Diagnostic for "one instruction short":** compare `orig[i]` with
  `ours[i-1]` from the divergence onward — here it cut a 148-instruction tail
  residual to 32, proving one missing instruction explained the rest.
- **`(unsigned char)(key.w >> 8)` and `key.b.y` are NOT interchangeable
  schedules:** VC6 groups the two byte-field reads of one 2-byte local and
  hoists them together; the shift spelling breaks the grouping and delays the
  high-byte read past an intervening load (what the original does) but lowers
  to `xor r,r / mov r8,[slot+1]`, one instruction too many.

- **METHOD FIRST: before grinding a residual, scan the already-matched corpus
  for the same instruction shape.** `screen.c`'s `__BMPLoader` carried both
  the idiom and the lever that closed `LoadPalette` (32 -> 0) in ten minutes,
  after statement-permutation searches had failed elsewhere. Scanners:
  `scratchpad/sweep1/scan_argcopy.py` (mines exact bodies for `mov rB,rA /
  push rB`), `scan_exit.py`, `scratchpad/misc3/scan_const.py`. This is the
  highest-yield first move on any residual whose shape looks unreachable.
- **The signedness of a 16-bit STORE decides the width of a mask feeding it**
  (`LoadPalette`). `unsigned short*` makes VC6's narrowing pass take a
  channel's `& ~7` down to the 16-bit operand width (`and edx,0FFF8h`);
  `short*` leaves it 32-bit (`and edx,-8`). No spelling of the mask reaches
  it — every `K` canonicalises to one node before the narrowing pass runs.
  **The RGB565/555 pack idiom:** red an `unsigned short` local, green and blue
  `unsigned char` locals masked at their declarations, and the pack spelled
  with MULTIPLIES — `(((r & ~7) * 32) | g) * 8 | b`. The multiply widens the
  byte before masking; `<< 5` masks the byte instead.
- **In an inlined helper body, the FIRST `param + global` sum takes the
  parameter temp's register as its destination and the SECOND takes the
  GLOBAL's** (proved both ways by mirroring the body). **A two-argument inline
  helper turns both argument expressions into temps evaluated before the body
  runs**, freeing the scheduler to hoist an independent `lea` and a second
  global load — that made seven instructions exact where ~110 statement
  permutations could not.
- **VC6 routes a compiler TEMP through a register copy before a SPILL but not
  before a PUSH**; a named local is spilled straight from eax. **A literal-zero
  return flips scratch allocation in a small list-walk predicate:** with a
  return-coalesced pointer web the cursor takes eax and the loop-local temp
  ecx; with any literal zero they swap. Binary, and immune to ~150 spellings.

- **RE-RUN every placement/permutation search after ANY other change.** In
  `Balloonz_Tick` the same 128-placement zero search and 720-order copy-in
  search were INERT on the previous baseline and DECISIVE after an unrelated
  fix (78 -> 5). Each lever moves the landscape; a search that failed once is
  not evidence it will fail again.
- **Two loops that copy the same record fields in and out are ONE set of
  function-level locals, not two block-local sets.** Diagnostic: the same
  frame home appears in both regions. Hoisting exactly those locals above both
  blocks made a whole pass byte-identical in one step. Locals whose
  ALLOCATION differs between regions (one spilled, one enregistered) must stay
  block-local.
- **Two or more zero stores visible together become `xor r,r` plus register
  stores — but this is FALSE for ABSOLUTE-ADDRESS GLOBALS (corrected
  2026-09-04).** `InitExitCheckBox`'s last two statements are adjacent zero
  stores to globals with no call between them, and VC6 emits two 10-byte
  immediates even though the register form is four bytes shorter. The rule
  holds for locals and struct fields; globals follow the constant-web use
  count instead (threshold four straight-line uses).** An intervening aliasing store, a surviving branch, a loop
  and address-taking do NOT split them; only a CALL between them does, and a
  DEAD zero store to an address-taken aggregate is emitted as an immediate.
  The usable lever is maximal source separation — first statement of one block
  and last of another; all 128 placements measured, only the two extremes work.
- **VC6 hoists every local aggregate initialiser to the prologue**, its
  template loads sitting BETWEEN the callee-saved pushes; no scope,
  declaration order or statement placement moves it (10 spellings). So an
  initialiser-store schedule residual is a scheduler priority, not source
  order.
- **`mov byte ptr [esp+N], al` then `movsx eax, al` on a call result means a
  SIGNED `char` local** — not `int`, and not `unsigned char` (which
  round-trips through the home). **Frame arithmetic recovers struct sizes:**
  following control flow THROUGH the .rdata jump table while tracking esp
  gives an exact slot map — that pinned `Vec3` (the origin `NewBNVPath` takes)
  at 12 bytes with the third int never written.
- **A `volatile` shim at the one site where the ORIGINAL reloads from memory
  can flip an entire callee-saved assignment**, but it takes the local's
  address and so removes a neighbouring variable's spill home. Valuable as a
  direction-confirming probe even when it cannot be committed.

- **A committed `volatile` shim can go STALE and become the dominant error.**
  On the Plane a shim that had been buying a spill was worth -52 once the y
  chain was right, and removing it also fixed a three-way head rotation and a
  2-byte deficit. RE-TEST every previously-committed volatile or shim after
  any structural change — same discipline as re-running permutation searches.
- **A commutative sum accumulates in the VARIABLE's register only if the
  first operation on it is in place.** `v2 = v + (M - f())` makes the
  parenthesised delta a rank-1 temporary, so `v2` coalesces with it and the
  chain runs in a scratch register; split into `v2 = v - f(); v2 += M;` and
  the first op is in-place, `v2` coalesces with `v`, and the chain runs in
  `v`'s callee-saved register. Value identical; ~40 isolated spellings (named
  delta, `int d[2]`, struct field, `short`, volatile either side, all six
  textual orders) are inert because VC6 forward-substitutes a named delta back
  into a temporary first.
- **The empty `if (v) { }` is also a FLATTEN breaker for a chain of later
  `-=` on the same variable**, not just for `(X+c1)-c2`; without it VC6
  re-sorts the later subtractions into the original sum.
- **Cache a pointer field in a local FOR ONE STORE ONLY — but the MECHANISM
  was wrong and is RETRACTED 2026-09-04.** It is COPY-PROPAGATION DISTANCE,
  not a schedule barrier: all 140 interleavings of the surrounding statements
  are byte-identical, and VC6 hoists the load straight over the escaped
  stores. The cache only survives when a statement sits BETWEEN the assignment
  and its single use; written immediately before that use it is propagated
  away and measures exactly the same as no cache. Using the cached pointer for
  a second store is still much worse, because the original really does reload
  after a store through it.
- **An order-independent flag store is a schedule lever whose sign FLIPS with
  the surrounding shape** — the same `b->flags |= 0x80;` had to sit above the
  subtractions while the y chain reassociated and below the seed stores once
  it was right (worth -21 and -8).
- **Two grouped `movsx` of adjacent short fields: the emitted STORE order
  always follows source order and the two LOADS always come out in the
  OPPOSITE order** — every spelling measured gives one or the other, never
  both. A recorded 2-mismatch residual on three rides. **A four-statement
  shift block feeding one call is order-sensitive:** only one of the 24 orders
  reproduces the interleave with the argument pushes (worth 19 and 7).
- **The stub-each-case diagnostic works on non-switch-phase problems too** —
  stubbing each case body in turn localised a loop-head rotation to one case
  in a single run.

- **METHOD, calibrated: for a function with EQUAL instruction counts and a
  large mismatch, build the frame map FIRST.** It separates "wrong frame" from
  "wrong code" in one step. Corpus-scan-first (recorded above) closed a
  function in ten minutes on one shape but a later lane ran it on four
  functions and it helped on NONE — a scanner for the exact blocking shape
  found three hits in the whole matched corpus and none was the idiom. Use
  both, in this order: frame map, then corpus scan, then permutation search.
  Tools: `scratchpad/sweep2/fm.py` (entry-relative slot map from an sbs
  listing), `sbs.py` (`--rb` register-blind, `--only-diff`).
- **A spilled scalar that must sit at the TOP of the frame is the `.y` of an
  aggregate whose `.x` is never referenced.** A plain `int` local is
  lifetime-coloured onto whatever pool home just died (an x87 `fild` staging
  slot, say) and renumbers every `[esp+N]`; making it a member of a
  function-level 2-int struct puts it in the aggregate pool and leaves the
  sibling dword as the classic unreferenced frame home. Read-side companion to
  the `*(volatile int*)&spill.x` trick.
- **Two address-taken `Pos` locals in disjoint blocks do NOT share a pool home
  when a third address-taken local declared in an enclosing block is live
  across them** — the cure is to reuse ONE local for both roles, not to
  re-scope them.
- **Where a `goto`-skip label sits relative to a trailing test decides whether
  a constant-0 register web spans the loop.** Moving a test BELOW the label
  (so skipped items still run it) made 0 live across the whole loop:
  `xor edx,edx` in the preheader, a remat where the body clobbers edx, and
  `cmp r,edx` replacing `test r,r` at eight sites — 30 instructions in one
  edit. **A `= 1` store is `mov [mem],1` when the constant is not live-out of
  that block and `mov r,1 / mov [mem],r` when it is**; comparing a flag
  against the constant keeps the web alive through the join, where degenerate
  branches, dead stores and named `one` locals are all folded away.
- **A same-width `unsigned` conversion can re-rank operands of a multi-term
  sum** (worth 2 once) but was inert on equivalent sums elsewhere — a thing to
  try, not a rule. **Data:** the original is not uniform about
  `waypoint + (cell << 8) + K`; X sums put the waypoint first in the closing
  `lea` and Y sums put the cell first, tracking which value the emitted stream
  defines first.

- **METHOD, second calibration — the two lanes that tried corpus-scan-first
  DISAGREED, and the difference is the shape.** It paid nothing on four
  scheduling/frame residuals, and paid immediately on three
  register/idiom residuals (recovering `GetObjectUID`'s exact mechanism from
  matched twins after three passes had only guessed, and DISPROVING the
  standing liveness explanation for `LFEntrance_Activate`). Rule of thumb:
  frame map first when instruction counts match but the mismatch is large;
  corpus scan first when the residual is a register choice or an unexplained
  instruction shape. Both are cheap. Reusable scanners now exist:
  `scratchpad/sweep4/scan_pad.py` (one-instruction landing pads — 49 in the
  corpus), `scan_dupload.py` (duplicated two-arm materialisations — 5),
  `scan_lea3.py` (three-register `lea` sums — 47), plus sweep1's and misc3's.
- **The map-pointer landing pad is live-range splitting with EDGE
  rematerialisation.** A bounds-checked cell probe's `mov r,[g_map]` before
  the sign test, with some edges jumping past it, is one value whose live
  range the allocator split; the reload always lands on the EDGE, never at the
  use. It appears when the load's block dominates the later probes, or the
  value is loop-invariant, and it cannot be reached when an `if (x >= 0)`
  guard denies that dominance — a `Map*` local is copy-propagated and VC6
  remats per use-block instead.
- **A NON-EMPTY else arm is what makes VC6 duplicate a spill reload into both
  arms** instead of hoisting it into the dominator; with an empty arm the
  reload is always hoisted. No zero-code statement keeps such an arm alive:
  `;`, `n = n`, identities, `(void)x`, dead locals, `while (0)`, `if (v) {}`,
  empty inline calls, `switch (0) {}`, labels and goto-transcriptions are all
  folded before layout.
- **Integer casts create NO IR tuple** — unlike `(float)` on a float product,
  `(int)`, `(unsigned)` and pointer/int casts are byte-identical no-ops, so
  they cannot move a scheduler window boundary or advance the scratch
  rotation. **A named local for ONE of several field arguments at a call is a
  scheduler lever** (it changes which dependency-free root is hoisted into the
  preceding stall slot). **A byte-typed loop counter lowers its web's
  allocation priority enough to flip a callee-saved tie** — useful as a probe
  even when the widening cost makes it wrong.
- **`add` vs a three-register `lea` is NOT always a liveness question.** The
  corpus has 47 three-register `lea`s; the closest analogue genuinely has both
  operands live past the sum, but in `LFEntrance_Activate` neither is and the
  `lea` exists only because the addend is in the wrong register. Diagnose the
  LOAD ORDER before hunting for a use that keeps an operand alive.

- **A `volatile` READ at ONE use (`*(volatile T*)&x`) is a different and far
  stronger lever than declaring the variable `volatile`.** It leaves the
  definition an ordinary store, makes the local address-taken in a way VC6
  does NOT un-escape (unlike `int* p = &x; *p`), and forces spill-at-def plus
  reload-at-use. Two independent wins in one round (53 -> 13, 82 -> 58).
  **Its exact cost: VC6 never hoists a volatile load**, so the reload is
  pinned at its source position. When the ORIGINAL's reload sits at the top of
  a block (an ordinary load the scheduler lifted), the shim can close the
  allocation but never the schedule. Diagnostic: if a volatile-read variant is
  exact everywhere except that one reload's position, STOP looking for a
  better volatile placement.
- **`memset` on the LAST group of zero stores splits a constant-zero web's
  live range** — it creates a destination-address temp, ends the zero web at
  the final store and frees eax for the next argument load, re-phasing the
  scratch rotation for the rest of the function (worth 87 of 102 in
  `JungleCruise_Add`). Applied to an EARLIER group it is worth nothing.
- **Three closed families, do not re-derive:** VC6 does not coalesce adjacent
  byte stores (four byte stores emit four instructions, so "buy a byte-class
  use by splitting a dword store" has a floor of +1); `if (const == 0) { }` is
  folded at the front end, so the empty-body-if creates a no-code consumer
  only for a live register temp, never for a compile-time constant; and
  constant carriers are always propagated away under /O2 (every integer type,
  initialiser or assignment, any placement, and via an int temp). VC6 also
  canonicalises `x + (-K)`, `x - K`, `x + ~(K-1)`, `x + (0-K)` and `n -= K` to
  ONE sub node. **The conclusion once drawn from that — "`add reg, -K` is not
  reachable from C" — is FALSE and was corrected 2026-09-05; see the trip-count
  entry below.** All those spellings of a SUBTRACTION do collapse to one sub
  node, but the instruction is reachable by another route entirely.
- **An `unsigned char` local loses a two-way callee-saved tie-break to another
  `unsigned char`** even with strictly more, loop-nested references; retyping
  the winner `int` flips it. Type, not spelling, is what the allocator ranks.
- **Corpus-scan calibration, third data point:** decisive on one function
  (a scan for `mov [esp+N],rA` then `mov rB,[esp+N]` within four instructions
  returned two twins and reframed the residual as "this variable is
  memory-resident", which pointed straight at the volatile-read lever), partly
  useful on one, and NEGATIVELY useful on two — a scan finding no instance in
  1541 exact bodies is real evidence the shape is rare and the hypothesis
  wrong. Cheap either way; run it.

- **VC6 sorts a flattened commutative `+` chain in DESCENDING DEFINITION
  ORDER (latest-defined addend added first), after forward-substituting every
  scalar temp** — which is why operand permutations, parentheses and casts are
  all inert against it. **Cure: write the sum's leading pair into a field of a
  non-address-taken aggregate** (`Pos t; t.x = origin + ox; b->sx = t.x +
  ofs.x + scr.x;`). Merely holding the OPERANDS in an aggregate is inert; it
  must be a partial sum. **The protection is contiguity-scoped:** a struct's
  field assignments are protected only up to the first READ of any of its
  fields, so one `Pos` cannot carry two sums separated by a store — two
  two-field aggregates can, and a ONE-field aggregate is completely inert
  (the second field must carry a real value).
- **Pinning the ONE shared `return 0` block to the FIRST guard needs both
  halves** (`SaveScripts`, 130 -> 0): the first guard's failure spelled
  `goto fail;` (its only goto source, so the label block lands right after it)
  AND `fail: return 0;` as the LAST statement in the function, because the
  surviving `xor eax,eax` copy is placed at the last `return 0` statement.
  Loop failures must stay plain `return 0;`.
- **A forward `goto` out of an if-arm's last guard buys back an inline
  epilogue and moves the join after the else arm:** `if (cond) goto joint;
  return;` in place of `if (!cond) return;` gives `je <joint>` plus a full
  inline epilogue — worth exactly the six instructions `StepSchoolCar` was
  short.
- **Diagnostic: the OFFSET-BLIND distance** (all `[esp+N]` collapsed)
  separates frame numbering that merely follows from another decision from
  real structure — it showed ~72 of 182 mismatches were pure numbering
  downstream of one spill flip.
- **Data:** the original spills a scalar temp to a later local's home and
  immediately overwrites it — a dead store, and direct proof VC6
  lifetime-colours a temp onto a slot a later local takes.

- **The `imul` rank rule's missing half.** The ENTIRE exact corpus contains
  exactly ONE `mov r,reg / imul r,[mem]` (`SoftPrint_XBltFast`), and its
  register operand is a value LOADED FROM MEMORY for an earlier product — a
  rank-1 temporary. So the "spell the twin product commuted" lever only works
  when the register operand is a loaded value; against an enregistered
  induction variable operand order is completely inert (16 combinations
  measured). That is why `JcBoat_Animate` stays at 3.
- **The constant-web trigger is sharper than first recorded:** the compare's
  immediate survives exactly when the constant needs NO register in the arm —
  one register materialisation is enough to form the web, so the earlier "two
  register uses" threshold is too loose. **Register occupancy breaks the web
  outright:** hoisting an unrelated computation so eax is busy across the
  compare keeps the immediate and reaches the original's exact 43
  instructions / 144 bytes (`ClampPopUpToScreen`, probe).
- **An aggregate initialiser is never hoisted out of a loop**, and putting the
  fill inside an `if (r) { .. }` guard moves its template loads out of the
  callee-saved push run into the preheader. **The empty-`if` no-code tuple
  creates no SCHEDULER tuple at all** (ten placements byte-identical) — it is
  a fold/consumer lever only, not a scheduling one.
- **Two residuals are FAMILY-WIDE, not per-function** (scan evidence, not
  guesswork): the "copy of a dead, rematerialisable loaded value feeding a
  store or push" shape occurs exactly twice in the whole repo — `RequestRoute`
  and `InsertChildIntoList` — and ZERO times in the 1542 exact bodies. Whatever
  explains one explains both; do not attack them separately.
  **SUPERSEDED 2026-09-04, then RESOLVED:** an un-coalesced copy needs a PHI
  or INTERFERENCE. The two-way test was run at BOTH sites off the original.
  `RequestRoute` has neither (one predecessor, no use past the divergence).
  `InsertChildIntoList` has neither either — its block at 0x4756aa is not a
  branch target, and its eax is defined at 0x47569d, read twice, then
  redefined by the call, with nothing competing. **So they are not a shared
  family; each is independent evidence for retirement.** A ninth corpus site
  (`AddRepairOrderForObject` 0x0049b977) belongs to the separate
  survive-the-call class, and a probe that genuinely uses the value after the
  call produces no copy at all.

- **The BNV-ride y chain's SOURCE SHAPE is solved (register assignment is
  not).** The original is ONE web with a compound assignment:
  `sy2 = (wx + wy) * th >> 9;` then `sy2 += g_map_cfg->oy - Get_YScroll();`
  — no separate `sy`. It reproduces the disputed indices exactly and is
  strictly better register-blind, but costs a three-cycle rotation of the
  callee-saved registers over the whole block, so it is documented and NOT
  committed. ~130 variants all score identically: it is not reference counts
  and not the number of names, but whether the scroll subtraction is itself an
  in-place op on the long web. **Scan citation WITHDRAWN 2026-09-04:** this
  entry previously cited "a scan returns exactly two hits, both `param +=
  expr`" as corpus evidence for the source shape. That scan was over-narrow
  (it required a `sub` within three slots and a specific operand class); the
  loose form finds **171** hits across the 1544 exact functions. The shape
  rests on the direct 2x2 measurements above, NOT on that scan — see the
  scan-artefact warning.
- **Two adjacent `short` fields doubled into an escaped struct: read BOTH into
  int temps first and put the doubling at the STORE** (`int px = b->dx; int
  py = b->dy; pos2.x = px*2; pos2.y = py*2;`). That groups the two `movsx`
  ahead of the two shifts; per-component spelling interleaves load/shift/store,
  and PRE-doubled temps tail-duplicate and ESCAPE the extent.
- **A named `RideTile* tile = RIDE_TILE(r);` local produces an
  edge-rematerialised `lea`, a CSE that survives across a jump table, and
  kills a function-wide zero web** — three original signatures from one edit.
- **The stub-each-case diagnostic can return a NEGATIVE and that is useful:**
  a loop cursor landing in the same register across all nine stubbed variants
  rules out "some case's pressure" as the cause of a swap, in one run.

- **A commutative `add` chain's DESTINATION is chosen BEFORE the sort** —
  this is the generalisation the descending-definition-order rule was missing.
  VC6 makes the compiler TEMPORARY the destination if the sum contains one;
  **when both operands are named locals it is the one defined LAST (nearest
  the add)** — CORRECTED 2026-09-04 by direct measurement on mechrides.c,
  where `sy2 = sy + dy` and `sy2 = dy + sy` both emit `add <dy>,<sy>`. An
  earlier note here said "earliest-defined symbol"; that was wrong. The
  remaining addends are then added in descending definition order.
- **The partial-sum aggregate is a TWO-ATTRACTOR lever and the second
  attractor can be worse.** It reliably restores source addend order, but it
  also relocates the whole eax/ecx/edx rotation of that block: every flat
  spelling compiles to one object and every aggregate spelling to another,
  with ~30 measured into each. On `JungleCruise_UpdateRiverAnim` it fixed the
  order and lost on every metric (111 -> 306). Measure register-blind AND
  offset-blind distance before adopting it.
- **`x = byte << 8` as a STANDALONE assignment gets a two-instruction peephole
  (`xor r,r / mov rh, byte ptr [mem]`); the same expression as a subexpression
  of a larger sum gets the three-instruction `xor / mov rl / shl 8`.** So
  "name the value" and "keep the instruction count" are mutually exclusive for
  that idiom — the open obstacle on `OctopusCafe_Tick`.
- **A loop-carried pointer's `++` belongs in the for-increment, not the body's
  last statement** — it moves the `inc` past the latch's other ALU ops.
- **RE-RUN STALE SWEEPS, again:** two of one lane's three improvements came
  from re-running searches a previous lane had recorded as exhausted, whose
  numbers no longer held on the current baseline. Treat every "measured, all
  inert" note as valid only for the baseline it was written against.

- **WARNING: the strict index-for-index count actively MISLEADS once a block
  is displaced, and it has already selected WRONG constructs.** In
  `RenderView` two committed shapes — a switch case order of 1,3,2,4 and a
  constant assignment placed last — had each been chosen by strict-count
  search and were both wrong (arms are emitted in source order, and the
  original's are in natural order). Removing them cost strict points at first
  and then collapsed the residual. At one point the file sat at strict 502
  with 40 FEWER structurally-wrong slots than the 478 baseline. **Drive big
  functions by LCS-aligned structural region reports**
  (`scratchpad/laneG/regions.py`, `wscore.py`), register-blind and
  offset-blind distance, and only sanity-check with the strict count.
- **The PLACEMENT of a constant assignment inside a block decides the
  callee-saved/caller-saved split of the values computed beside it**
  (`RenderView`, worth 116 on its own). With `mode = 0xff00;` last, VC6 put
  two coordinates in caller-saved registers, they did not survive the sibling
  arm's calls, the emit had to reload them, and three instructions shifted an
  800-instruction tail; with it FIRST they land in callee-saved registers as
  the original has them, both arms end with the same four-instruction suffix,
  VC6 cross-jumps them and the tail realigns to zero structural error. VC6
  allocates in creation order and a constant claims a register the moment it
  is created, so hoisting or sinking ONE constant store rotates the whole
  block's allocation.
- **A `HalfOffset`-style pair of sign-tested sums needs FOUR temps, not two:**
  the original loads both fields and computes both sums before the first sign
  test, which kills the `add`'s flags and forces `test`/`jge` where the fused
  form emits `jns`. Both the sums AND the halves need temps.
- **Diagnostic:** a control-flow-aware esp simulation gives a slot map good
  enough to compare frames slot by slot including reference counts and
  first-reference index (`scratchpad/laneG/fm.py`). On `RenderView` it proved
  both sides use exactly the same 40 slots and the same 0x2f90 frame, with
  only the lowest 13 differing by one move plus one extra temp (a `tw + tw`
  we CSE and the original recomputes).

- **The BNV y-chain search space is CLOSED (mechrides.c, ~380 further
  variants over 20 grids).** The trigger for the register rotation is not the
  compound assignment, as previously framed: a 2x2 isolation shows the one-web
  `sub`/`add` form rotates too. The trigger is **the y accumulator being ONE
  web while the later adjustments stay separate** — and the empty-`if` fold
  breaker, which is what keeps them separate, therefore participates in the
  allocation decision. The only spelling that puts the sum in the long web is
  the in-place compound, and the in-place compound always rotates, so the two
  cures are mutually exclusive. Stub-each-case leaves the rotated triple in
  all 8 variants, so the decision is entirely inside case 1.
- **A plain dead store to an address-taken struct member is ELIMINATED**
  (`pos2.x = sx;`, `spill.x = sx;` all shrink the frame); only a `volatile`
  store creates the extra 8-byte home, so that shim is not replaceable by a
  "natural" store — and its spelling is irrelevant to allocation (7 forms
  identical), only its existence matters. **Lifting a flag store above two
  escaped-struct stores costs a frame slot**, so the original's interleave of
  such a store into the arithmetic is a SCHEDULER phase, not source order.
- **All difference spellings canonicalise:** `a - (b - c)`, `a + (c - b)`,
  the `(int)`-cast form and `x = a; x -= b - c;` are byte-identical, with the
  accumulator in a scratch register. **A named single-def/single-use delta
  local invites reassociation** — VC6 flattens later in-place `-=` on the
  accumulator into the delta's register.
- **A callee-saved priority ranking can be movable but too coarsely
  quantised** (`SpaceTower_Activate`): ours ranks `b > rec > {tile,tx} > r`
  and the original `b > rec > r > {tile,tx}`, registers handed out in priority
  order; one extra reference to the cursor jumps `r` two ranks, overshooting.
  22 single-point reference mutations and 8 declaration orders are inert.
- **Data:** `MapConfig.ox/oy` at +0x20/+0x22 are genuinely `unsigned short`
  (zero-extended loads) while `Get_XScroll`/`Get_YScroll` return `short`
  (`movsx`) — the asymmetry is load-bearing for the delta's shape.

- **A `volatile` DECLARATION is inert in VC6 SP3 — only a volatile-qualified
  ACCESS forces a memory reference.** `volatile int x;` with every access
  written `*(int*)&x` compiles byte-for-byte to the plain enregistered object.
  (An earlier note's cost for "declare it volatile" was really the cost of the
  accesses it implies.) **VC6 also un-escapes every cheap address-take:**
  `(void)&x;`, an unused `int* p = &x;`, `*p` with p propagated, a one-element
  array with a constant index, a union member, and empty or dereferencing
  `static __inline` helpers all leave the value fully enregistered.
- **Every volatile access is ALSO a scheduling barrier — nothing moves across
  it, including a `push` (a stack write). Memory residency and the barrier are
  the same switch, so "spill at def with hoistable ORDINARY reloads" is not
  reachable from C.** A volatile STORE and a volatile READ are exact
  complements: the store hoists the reload and sinks the CSE spill past the
  pushes; the read pins the reload at its use but leaves the pushes free.
  `DrawPopUpInfo`'s 13 and its 16-scoring alternative are the two sides of
  that one wall — treat it as EXHAUSTED unless the wall itself is broken.
- **Intrinsic `memset` ALWAYS materialises its destination as a scratch
  pointer** (`lea base,[reg+disp]`), at any size down to 4 bytes. Where a
  memset helps a match it is usually that temp's register pressure, not the
  fill — in `JungleCruise_Add` the temp occupying ecx early is the entire
  value, stopping an argument being hoisted into ecx 17 instructions early.

- **A dead rematerialisation of a constant web can survive into the shipped
  code** (`CheckWorkerOnMouseStatus`, 0x4707a3). The original's `mov ebp,1`
  there is dead — a later block redefines ebp — because the allocator inserted
  it while that arm still fell into the join, and it survived the later jump
  threading and tail duplication. Same class as the `and dx,0x20` merged-arm
  ghost. **Consequence: the target is not a better way to spell the store, it
  is whatever keeps the constant web live into that arm at allocation time.**
  The rest of that function is byte-for-byte the original, and its strict 82
  is one missing instruction shifting every later index — a worked example of
  why the strict count must not drive a decision.

- **THE `add` DESTINATION RANK, complete (this is what closed
  `OctopusCafe_Tick`, 49 -> 0).** For a commutative `add`, the destination is
  chosen by rank: **(1) an inline MEMORY reference — it is LOADED into the
  destination, never folded into the add; (2) a compiler temporary; (3) a
  named local** (and between two named locals, the one defined LAST, nearest
  the add). This is the `imul` rank rule holding for `add` as well, and it is
  a STRONGER lever than any definition-order effect. Corpus evidence: a scan
  for "`add rA,rB` where rA is a plain load and rB a shift" returns exactly
  five hits across the 1542 exact bodies, and `Entrance1_Tick` (0x0042e0a9)
  is the worked twin — `b->target.y = tbl[b->f3a] + (ty << 8);` gives
  `mov edx,[eax+ecx*4] / shl ebp,8 / add edx,ebp`, the inline memory reference
  taking the destination away from the shift. Scanner:
  `scratchpad/laneD/scan_adddest.py`. **Practical consequence: spell the
  memory operand INLINE rather than routing it through a named local or array
  temp** — the earlier note claiming an inline waypoint gets folded and loses
  instructions was wrong; nothing is folded.
- **`x = byte * 256;` as a standalone assignment keeps the three-instruction
  `xor r,r / mov rl,[mem] / shl r,8`, where `x = byte << 8;` collapses to the
  two-instruction `xor r,r / mov rh,[mem]` peephole.** As a SUBexpression the
  two spellings are byte-identical. That removes the obstacle an earlier note
  called the open question for this family.
- **The trip-count family is NOT closed — this entry was wrong and cost a whole
  family several rounds (corrected 2026-09-05).** The measurement stands:
  `24/cellh - 2u`, `+ 0xfffffffeu`, `+ (int)0xfffffffe`, `+ -2` and the
  unsigned-wrap forms all compile byte-identically to one SUB node. The
  INFERENCE drawn from it — "`add reg, -K` is unreachable from any C spelling"
  — was false, because every spelling tried was a spelling of a SUBTRACTION in
  a DOWN-counting loop. **The reachable route is an UP-counting loop:**
  `for (i = 0; i < 24 / cellh - 2; i++)` is reversed by VC6 into the same
  `test/jle` guard and `dec/jne` latch, but the trip count is then materialised
  by the LOOP TRANSFORMATION, and that node is an ADD with a negative constant —
  `mov eax,0x18 / cdq / idiv [esp+0x10] / add eax,-2 / test eax,eax / jle`, which
  is the original's `LFDrop_Place` head exactly. A `.text` scan finds **75**
  `add r32, -K` sites in the binary, and one of them (0x0040c01f in
  `LFEntrance_Activate`, from `(tile->b.x - 2) << 8`) was ALREADY being
  reproduced exactly by this tree while the note claimed the form was
  unreachable. **General lesson: an exhaustive sweep proves something about the
  spellings swept, not about the instruction.** Before recording "X is
  unreachable", scan `.text` for X and check whether the tree already emits it.
- **The strict-count warning is now confirmed in a second lane**, which had
  itself committed two constructs contrary to the original (a store order and
  a bloke-position order) because they scored better. Both were fixed by
  ranking on LCS structural distance instead. Re-rank any search whose winner
  was chosen on the strict count alone.

- **Constant webs are counted in REGISTER uses, and a STORE of a constant is
  not one.** Measured three ways: `cmp r,K` plus one register use keeps the
  immediate; adding the store (three uses) builds the web and hoists its def
  to the dominator; the store alone emits `mov [mem], imm`. Decisive control
  on `ClampPopUpToScreen`: changing only the compare constant to one value
  apart from the arm's gives 2 mismatches at 144/144 bytes with the whole low
  arm exact — proving the residual is nothing but "compare constant equals arm
  constant". (Not committed; the original plainly wrote the other value.)
- **The callee-saved constant-zero hoist is LOOP-WEIGHTED, not "four uses"
  — CORRECTED 2026-09-04, the earlier form was wrong.** Two uses BOTH inside a
  loop suffice (and go to a SCRATCH register if nothing is live across a
  call); one in plus one out does not; three straight-line uses never do. **A
  byte- or word-class COMPARE against zero is NOT a use of the zero register**
  — measured three ways: a signed-char global gives `test al,al`, through a
  pointer `test cl,cl`, and a short field `cmp word ptr [eax+6], 0` with an
  immediate. The `cmp word ptr [eax+0ch], si` in `PlayMIDI` 0x4805d0 that the
  earlier note cited as the witness is a CONSEQUENCE of a hoist its two
  in-loop stores had already earned, not the cause. A one-use callee-saved
  zero is reachable only as a VARIABLE with two reaching definitions live
  across a call (`int r = 0; if (p) r = f(p); return r;` — logflume2.c
  `LFPiece_IsVisible`), never as a rematerialisable constant. Corpus: a zero
  carrier that is a function's SOLE callee-saved push is always esi (12 of 12
  matched instances, 1 to 38 uses); only five matched functions push ebx as
  their sole callee-saved register, and three force it via `bl`/`bh`. **This corrects an earlier "no such function
  exists in 1542 exact bodies" conclusion, which was a SCAN ARTEFACT** — the
  scan demanded the register never be redefined. Re-check any conclusion that
  rests on a scan finding nothing.
- **An un-coalesced register copy needs a PHI or INTERFERENCE.** Read off
  `RenderAdvisorIcon` 0x443e8a instruction by instruction: its source is dead
  after the copy, so the copy exists because the destination is a phi register
  with two reaching defs, not because of interference; the four
  `mov rA,rB / mov [esp+d],rA` corpus sites are the interference case.
  `RequestRoute`'s site has NEITHER (one predecessor, no use past the
  divergence), so it is very probably unreachable — and the "family-wide with
  `InsertChildIntoList`" claim is now testable rather than assumed.
- **A union is NOT a CSE barrier in VC6 SP3** — same-width union members
  value-number as one lvalue. The same-width-conversion barrier needs two
  differently-typed OBJECTS. **`__assume` compiles, emits nothing, and does
  not keep a basic block alive**, and an empty-bodied `if` on a byte compare
  is flattened even when the compared value is not a compile-time constant
  (this corrects an earlier note).
- **RETIRED with evidence: `JcBoat_Animate` (0x00433840), 3 of 330 at
  1108/1108 bytes.** Only one `imul`'s operand rank is left. Both source
  orders and every zero-cost identity fold before the ranking pass; `(short)j`
  reaches ONE mismatch but costs a byte, which proves both the mechanism and
  its price — only a real operation promotes an enregistered induction
  variable to a rank-1 temporary, and every such operation leaves an
  instruction or a byte behind.

- **The phantom-home / dead-store pair, now fully characterised
  (`Carousel_Tick`, 278 -> 34 from this alone).** `struct { int x, y; }
  spill; *(volatile int*)&spill.x = v;` reserves a never-touched frame dword
  AND emits the original's "store at its death", with the written member
  lifetime-colouring onto another local's home. A one-member struct or a bare
  `int` gives the store but only one slot; `int dims[3]` gives the spare slot
  but, being an aggregate, lands at the TOP of the frame rather than the
  bottom; bare unused locals (`(void)&pad`, `pad = pad`, `int pad[1]`) are
  deleted outright. When it lands, every `[esp+N]` displacement lines up at
  once — check the whole frame map, not the local indices.
- **Difference-before-sum:** for `(a - b)` and `(a + b)` on the same pair,
  writing the DIFFERENCE first gives `mov t,a / add a,b / sub t,b`; sum-first
  gives `lea t,[a+b]` and one fewer instruction. Every other spelling
  normalises to one of the two.
- **Three compensating-error variants, measured and DECLINED** (recorded so
  nobody "rediscovers" them as wins): swapping two struct stores scored 25
  against 27 by shifting two stores into accidental alignment; negating a
  difference inserted a `neg` the original does not have; and a spill-struct
  volatile store scored 118 against 208 while the REGISTER-BLIND distance went
  UP — pure one-instruction-shift luck. **A strict-count drop with a
  register-blind rise is the signature of a compensating error.** Use it as a
  standing test before adopting any variant.
- **Method, proved again:** `Carousel_Tick`'s cause was found by bisecting a
  reference count store-by-store, and two standing theories died with it —
  VC6 allocates WEBS, not names (splitting a variable per loop is
  byte-identical), and the interfering value was not the one previously
  blamed.

- **RETRACTED: "the load order decides the register" (`LFEntrance_Activate`).**
  A probe over all 40 legal statement orders shows the value lands in the same
  register in 40/40 regardless of emission position — it coalesces into the
  sum's register, and the original simply does not coalesce. **Also retracted:
  "neither operand is live past the sum, so the three-register `lea` is not a
  liveness question."** It is: the case block opens by reading the same
  register the preamble loaded, so the operand IS live into the switch, and
  the `lea` is an ordinary liveness effect of the same class as
  `BuildCursorPtr`.
- **A load-op fold needs the load and the ALU op adjacent AFTER scheduling** —
  any unrelated store scheduled between them prevents it. The original never
  folds a global byte into `sub r8, byte ptr [mem]` where we fold four of six.
- **Statement order between two INDEPENDENT statements can decide a
  loop-invariant hoist, and with it the whole frame** (`LFEntrance_Add`):
  moving one assignment ahead of another bought the `n - 1` spill slot, the
  0x28 frame and every spill offset, taking indices 0-61 to identical.
- **A volatile read of a DIFFERENT global in the same region can flip a
  callee-saved/scratch colouring pair function-wide** (`g_map_rows` fixing
  `g_map`'s register in `GetObjectUID`, 20 -> 4 — documented, not committed,
  since that global is ordinary everywhere else). Its last 4 are a HARD FLOOR
  for any volatile shim, because the barrier and the memory reference are the
  same switch.
- **Scan-artefact warning, third and fourth instances.** A strict "consecutive
  loads only" scan produced a false "unreachable" that the loose form
  overturned, and the loose form led straight to an exact-matching twin
  (`Fort_TickRiders` 0x00406660) that settled the question. Running tally
  across the campaign: most scans pay, and every one that misled did so
  through an over-tight condition. **Any conclusion of the form "no instance
  found" must be re-run loosely before it is relied on.**

- **An explicit `goto` to a label at a switch's join FLIPS VC6's
  identical-suffix merge from hosting the shared tail at the LAST copy to
  hosting it at the FIRST** (`TempleSlide_Update`, 258 -> 77 strict). Writing
  the outer guard `if (state != 0) goto endsw;` with `endsw:` at the loop's
  continue point, instead of `if (state == 0) { switch ... }`, moved 13
  instructions into the right block. The `goto` in case 2's tail, case 5's, or
  both are byte-identical; putting it in ALL copies, or only in the first, is
  inert — **it is the asymmetry between the first copy and the others that
  decides it.** Should transfer to any function whose shared tail hosts in the
  wrong block.
- **A `static __inline` helper's argument reorder is PAYABLE** — its cost is
  one web leaving the scratch rotation, and restoring that web is a separate,
  findable edit. This is what finally closed `Joust_Draw` (3 -> 0): the known
  helper gave the disputed indices, and a two-level rider guard with the bloke
  cached between the levels put the hoisted key back into the eax/ecx/edx
  rotation, dissolving the whole 63-mismatch cascade. Without the intervening
  cached local the guard is inert.
- **The BNV compound-assignment chain must be applied to BOTH axes** — X-only
  or Y-only is worth nothing, the pair clears ESCAPES, and it does not flip the
  loop head (the older "any extra live value flips it" note held only for
  spellings needing a separate scalar per axis).
- **Three negative results that close cheap searches:** comma operators
  normalise to statement order in every position (five forms measured);
  upstream padding does NOT move a store's schedule slot (a dead volatile read
  at five points shifted the stream but left the target group's order
  unchanged — so that class of residual is IR order, not a scheduler window);
  and a may-alias store is NOT a barrier for byte-field loads feeding `fild`.
- **The add-rank rule does not reach a rotation-phase residual:** where the
  destination is already the rank-1 inline memory operand in both attractors,
  routing operands through named locals is completely inert (all six
  combinations).
- **Data (a real bug found and fixed):** `Person3D` has NO padding after
  `local` — `screen` +0x1c, `local` +0x24, `zsprite` +0x2c, `f30` +0x30,
  `depth` +0x3c. A bogus `pad2c[4]` had put the last three four bytes high.
  `Bloke.saved_speed` is a byte at +0x44; `b->seat` at +0x36 drives an inner
  jump table.

- **ANY `__asm` block in a function REVERSES the entire frame-object layout
  order** (`Draw3DPersonModel`; eight paired compiles, exact reversal each
  time — five arrays laying out A,B,C,D,E from ebp down without asm lay out
  E,D,C,B,A once one `__asm { mov eax, n }` is added). It is GLOBAL, not
  per-object. The underlying no-asm order is DESCENDING SIZE from ebp down, so
  the asm order is ascending size — **by BYTE SIZE, not element count**
  (`struct { int f0..f6; }[3]` is equivalent to `int[21]`), and **the asm
  block's POSITION and COUNT are inert; only its presence matters.**
  **Reference weight only pulls TOWARD ebp** (adding straight-line refs moves
  an array forward; removing refs moves nothing). **Block scope is NOT worth
  "one position" — CORRECTED 2026-09-04:** an 84-byte array in ONE block goes
  past two 96-byte arrays, but the same name declared in TWO disjoint blocks
  behaves like a function-level local, which is the configuration the earlier
  measurement used. It is really a WEIGHT effect with a measured threshold,
  and **references made only via `lea X` inside an `__asm` block carry ZERO
  weight** (corrected 2026-09-04: deleting 4, 8 or all 24 of them is
  byte-identical — only C-level references rank arrays). On the real function, block scope moves an object exactly one
  step, cutting ~24 references moves it exactly one step, and **the two do not
  stack**. **Declaration
  order of locals is completely inert** for /O2 frame layout, with and without
  asm (all 24 permutations byte-identical). This applies to every function
  that contains inline asm, including anything sharing `tri3d.c`'s fixed-point
  macros.
- **`Draw3DPersonModel` is NOT hand-written assembly** — the long-open
  question is settled. Its ebp frame and unconditional ebx/esi/edi save come
  from the `__asm` fixed-point macros in the file; the pushes are at the top
  of the prologue, not inside the stream, and there is no `xchg` against
  memory in its 1023 instructions. Mixed C plus inline asm, reachable from C.
  It now sits at 44.3% (was 33.3%), mnemonic LCS 94%, with NO mnemonic gap of
  five or more instructions anywhere — the residual is almost entirely frame
  colouring: 707 instructions agree in mnemonic, registers and immediates and
  differ only in `[ebp-N]`.
- **A float constant as an INITIALISER pins its store early; split it into a
  bare declaration plus an assignment statement to place it** (all seven
  anchor points in a leading run then give identical code).
- **NAMING CAUTION, `Person3D`:** the field one file calls `depth` is at
  +0x3c (elsewhere `zboost`), but `Person3D::depth` in person3d.c is the
  print-list sort key at **+0x54** — a different field. Do not "fix" one to
  match the other. (person3d.c does NOT carry the `pad2c[4]` bug that was
  found and fixed in joust.c; verified empirically against the original.)

- **THE RANKING METRIC TO USE: mismatches remaining after the best PERMUTATION
  of the callee-saved registers** (`scratchpad/laneK/permrank.py`,
  width-aware for `bx`/`bp`/`di`/`bl`). Neither existing metric separates the
  right candidate when a body's whole residual is one register renaming: such
  a body keeps a HIGH strict count and a LOW register-blind distance at the
  same time. On `Carousel_Tick` the strict count ranked three candidates
  13 < 27 < 29 while the permutation-aware count ranks them 8 < 13 < 27 —
  which is the order the disassembly supports. Use this alongside the
  compensating-error test (a strict drop with a register-blind RISE is a
  compensating error).
- **The BNV y-chain lever transfers across files** (mechrides.c -> ridecb3.c
  `Carousel_Tick`, 27 -> 8 real mismatches): the Y chain is ONE web —
  `sy2 = (wx + wy) * th >> 9;` then `sy2 += cfg->oy - Get_YScroll();` — with
  the compound assignment in place on the shift's own web, and X deliberately
  NOT compound. Written `sy2 = sy + (oy - Get_YScroll())` the delta becomes a
  compiler temporary, wins the add-rank destination copy, and also blocks the
  pointer hoist. Pair it with naming only the FIRST `b->person` read — note
  the mechanism there is copy-propagation DISTANCE, not a schedule barrier
  (see the retraction above): the cache survives only with a statement between
  the assignment and its use. Both
  file (ridecb3.c `Carousel_Tick`) sits on a three-cycle callee-saved rotation
  wall — "give the merged accumulator web ebx".
  **CORRECTED 2026-09-04: mechrides.c is NOT on that wall**, and an earlier
  claim here that the two files were one problem was wrong. Under the
  permutation-aware metric its four committed activation bodies score
  `real == strict` under the IDENTITY permutation, i.e. they have no register
  difference from the original anywhere. The one-web chain buys 2 on one ride,
  ties on another and LOSES 10 on two others, its extra non-rotation cost
  being a load VC6 hoists into the slot the in-place `sub` vacated. So the
  y-chain shape is file-specific, not family-wide.
- **NEGATIVE: the goto-flip lever does NOT reach every wrong-copy merge.** On
  `JungleCruise_Tick`, the outer guard as a `goto` plus all 31 non-empty
  subsets of `break` -> `goto` over five case tails are byte-identical to the
  committed body (32 variants, all exactly 208). Its one missing instruction
  is a consequence of an add destination, not of block layout. Two more
  theories died there: routing the seat-table reads through named locals is
  inert (add-rank does not reach a rotation-phase residual), and pinning the
  station load with a volatile moves it to the original's index yet still
  leaves it in the wrong register — so emission order is not what colours it,
  and the reference count is not what separates the two (the original really
  does have six references).

- **A named local holding a CSE'd BYTE FIELD moves a callee-saved RANKING
  that reference mutations cannot** (`SpaceTower_Activate`, 149 -> 138 with
  the long-blamed ebx/ebp swap fixed and indices 0-22 now exact). 22
  single-point reference mutations had all failed; one `int tiley =
  tile->b.y;` consumed by two cases moved the value. The original keeps that
  byte live across the jump table. Proved it is NOT register availability: a
  global read reproduces the same pressure and still picks the other register.
- **The empty `if (x) { }` flatten breaker is a pure BLOCK SPLIT, not a second
  consumer** — two different values in the same slot are byte-identical. Its
  POSITION selects one of three register attractors, so treat it as a
  placement knob rather than a consumer trick.
- **Explicit int temps for an escaped `Offset`'s members DO hoist the loads
  above a may-aliasing store** — this corrects an earlier "no source order
  reproduces it" note.
- **NEGATIVE, and it bounds the newest lever: the `goto`-at-a-switch-join flip
  is NOT universal.** Tested on five ride activations it was inert on two and
  actively harmful on three (19 -> 58, 15 -> 134, 138 -> 145), because their
  shared tails already host where the original hosts them. Combined with the
  32-variant negative on `JungleCruise_Tick`, the rule is: use it only when
  the merge demonstrably hosts in the WRONG copy, and verify against the
  original's edge targets first.
- **The family y-block residual is now one switch, proved:** a named
  `ys = Get_YScroll()` restores the original's callee-saved naming under the
  one-web y chain (25 indices exact register-for-register) but costs the
  reassociation. The two halves — *separate later subtractions* and *original
  registers* — are mutually exclusive, which is the same wall two files now
  share.

- **`RenderCursor`'s residual (b) is a BUILD DIFFERENCE, not an unfound
  spelling — retirement-grade evidence.** A corpus scan of every matched body
  for the cross-jump signature found 89 sites across 24 functions with
  shared-suffix depths 5:1, 7:16, 9:10, 11:1, 13:37, 15:1, 16:2, 18:8, 27:4.
  **The single depth-5 site is this function's own, and there is no depth-6
  site anywhere.** This build's measured merge floor of 6 is already below the
  corpus minimum of 7. Also: **inside a switch, `goto join` and `break` are
  literally the same instruction**, so the goto-flip lever's required
  asymmetry cannot arise there — eleven spellings are byte-identical.
- **Dead-store elimination is an early pass without exception** — a union byte
  member killed by a dword store, a dead local `char` store, and a byte store
  killed by a dword store all vanish before the hoist decision, so none can
  buy a register class.
- **Phantom-home spill, refined into two distinct tools:** writing through an
  EXISTING local (`*(volatile int*)&tx = v;`) gives a store-at-death WITHOUT
  touching the frame, while a block-scope `struct { int x, y; } spill`
  reserves a REAL slot and re-lays the prologue when the frame has no spare.
  Pick by whether the original's frame has room. Using the first form on
  `StepSchoolCar` exposed that its previous "351/351 instructions" was two
  COMPENSATING ERRORS — two extra instructions from a flag kept in memory were
  cancelling two missing projection instructions.

- **The permutation-aware metric changes conclusions, not just rankings.** On
  its first real use it (a) shrank a claimed 149 -> 138 gain to a real 138 ->
  136, showing eleven of the "improvement" was a renaming, and (b) overturned
  a coordinator claim that two files shared one wall — the four bodies in
  question score `real == strict` under the IDENTITY permutation, so they have
  no register difference from the original at all. **Run it before believing
  any cross-file "same wall" claim.**
- **Completed 2x2 on the paired y-chain lever (four rides):** two of the four
  have no pointer cache and ADDING one is inert (VC6 CSEs it, byte-identical
  either way); removing it from the other two is much worse. X is deliberately
  non-compound in all four. The empty `if` is not droppable in the two-web
  shape (real 231 to 300 without it). A named `ys = Get_YScroll()` local under
  the one-web chain removes the three-cycle rotation COMPLETELY (25 indices
  exact register-for-register) but VC6 then flattens the later subtractions
  into the delta — the two halves are one switch and nothing sits on both
  sides across ~400 variants.

- **RESOLVED 2026-09-04 (third and final revision) — the add-destination
  tie-break between two named locals is EMISSION ORDER, not definition order
  and not read order.** Both earlier readings are refuted. The decisive
  experiment held read order constant (the same local is the left operand of
  both the difference and the sum in every variant) and varied only definition
  order across four builds; **the destination flipped in BOTH directions**, so
  it is neither first- nor last-defined. The invariant: **VC6 copies the
  DIFFERENCE's left operand into a fresh register, and whichever of {sum,
  difference} is emitted SECOND takes the remaining operand's register.**
  Definition order enters only by changing which local sits in which register
  and hence which operation the scheduler puts first. An intermediate
  "last-defined-wins" record (from a two-build experiment on another site) is
  withdrawn. Caveat that survives all three revisions: at most sites the rule
  is UNTESTABLE — the destination is already the rank-1 inline memory operand
  so the two-named-locals case never arises, and commutative sums are fully
  canonicalised (all 24 orderings of a four-term sum and all 6 of a three-term
  one compile identically). Original text of the conflict, for the record: The RANK is agreed and solid: (1) an inline
  memory reference is loaded into the destination and never folded, (2) a
  compiler temporary, (3) a named local. The tie-break BETWEEN two named
  locals is disputed by two lanes that each measured directly:
  mechrides.c found the LAST-DEFINED (nearest the add) wins, with both source
  orders emitting the same instruction; joust.c found the operand READ FIRST
  IN SOURCE wins, reading `wx` before `wy` flipping which register the sum
  consumes and reproducing the original's interleave. Both results stand in
  their own files, so the tie-break is context-dependent. Measure it per site.
- **VC6's x87 allocator spills by FURTHEST NEXT USE** (`AnimApplyPart`, proved
  by probe). Declaring two of eight rectangle floats `volatile` flips the whole
  x87 allocation to the original's (strict 182 -> 144, bad regions 73 -> 38) —
  not committed, since it costs a frame slot and leaves the wrong subtract
  form, but it proves the entire residual is one hoist: with the Y-group
  `fild`s left below a divisor the X group is spilled, as the original does;
  our build hoists them and spills Y, the exact mirror. **A full volatile mask
  sweep is cheap and pinpoints which values must be memory-resident even when
  the credible construct is still missing.**
- **A `union { float f; int i; }` does NOT defeat enregistration of a float
  member** — the recorded "union makes a scalar address-taken" trap fires on
  integers, not floats. **A `static __inline` helper's arguments are
  forward-substituted when they are plain memory references**, so the
  "arguments become temporaries first" lever creates no scheduling or aliasing
  barrier in that case (proved on two functions).
- **Splitting an accumulator into a pre- and post-call web is what makes the
  delta temp win the add's destination**, and it forces the delta into a
  callee-saved register, leaving eax live at the next global load and changing
  its encoding (5-byte `A1` form against 6-byte). **That one byte is
  diagnostic of the wrong web split.**

- **`Draw3DPersonModel`: three real source-shape errors found by reading the
  disassembly, 981 -> 687 (matchfull 44.3% -> 52.0%, mnemonic LCS 96%,
  offset-blind LCS 707 -> 820).** (1) A corner array's element 0 is a
  WHOLE-STRUCT COPY (`box[0] = fr->bmin;` — `mov eax,esi` plus three moves
  through eax), which is what breaks VC6's CSE of that field with a later sum.
  (2) **A transpose must read `p->matrix[k]` DIRECTLY, not through a pointer
  local** — `mt[k] = mp[j]` makes VC6 emit `add edi,0x58` and lose the base,
  where the original emits `lea eax,[edi+0x58]`, stores it to both pointer
  homes and keeps the base in edi (worth 239 alone). (3) A vertex pointer is
  RECOMPUTED from the index as the FIRST statement of the loop body, so the
  original has no increment in its loop tail and strength-reduces it to a
  `lea`.
- **The original's scalar frame slots are in strictly DESCENDING reference
  count from ebp down** (17,16,16,15,13,12,12,12,11 | 9,8,7 | 4,4,4 | 3,3,3,2)
  — an independent check on any frame hypothesis.
- **A frame layout can be proved unreachable by REFERENCE PROFILE.** For
  `Draw3DPersonModel` the per-slot profile is identical to the original's,
  slot for slot, 79 slots each — so no weight argument can separate the two
  layouts, and ~195 of the remaining 687 are instructions differing only in
  `[ebp-N]`. 15 block-scope subsets, declaration order, statement order and
  splitting into three separate objects all fail; only a merged 180-byte
  object reproduces the array order, and that is not credible source.

- **An inline helper CANNOT create a second constant temp.** Six forms of a
  `static __inline` zero-clearing helper — including an `int` zero parameter
  and the zero argument first — are byte-identical to plain stores: VC6
  inlines, forward-substitutes the constant and re-CSEs it into the
  function-wide zero web BEFORE allocation.
- **Variable identity is not a lever in either direction** — merging two names
  into one is as inert as splitting one into two (which was already recorded).
- **VC6 does not hoist a store to an address-taken local above a call**, so a
  statement cannot be moved down past a call to recover an earlier store pair.
- **Methodological: an INVARIANT permrank permutation across a wide variant
  space is itself the signal that the block under the microscope is the wrong
  place to look.** On `Carousel_Tick` the permutation `bpdibxsi` held across
  ~130 variants and only moved when the body was structurally destroyed —
  and both builds have identical webs, references and live ranges, so the
  allocator's INPUT is the same and only its preference order differs.
- **A register-blind distance of 3 against a strict 67 identifies the honest
  body** (`UpdateRiverTile`): its whole residual is one sunk `push ebx`,
  because the original keeps a value as ONE web rematerialised on the loop
  back edge while we split it.

- **VC6 NORMALISES source statement order completely for a loop body's arm
  blocks.** Three independent restructurings of `RenderView` — a goto-to-end
  for the else arm, a goto-to-end for a 239-instruction arm, and a fully
  inverted guard — all produced BYTE-IDENTICAL objects. **A misplaced block can
  only be bought with register allocation or branch shape, never with
  statement order.**
- **The split prologue: BOTH earlier explanations are wrong (corrected twice).**
  It is not "the geometry block demands esi/edi", and it is not "push sinking
  with an identical register assignment" — the assignment DOES change (two
  values swap between ebx and ebp in every split-breaking variant). What VC6
  actually does is put exactly TWO pushes in the entry holes among the leading
  fills and two later; the original pushes one pair BEFORE their own
  definitions and the other pair late, while every broken variant pushes the
  other pair first. **The bit being decided is the ORDER of the four
  callee-saved pushes, i.e. which pair is allocated first.** Diagnose by
  printing the four push indices AND which register holds what — neither alone
  is enough. Ruled out: 23 geometry orders, the post-guard inner-scope route
  (byte-identical), eleven respellings on top of the best order, and all 15
  interleavings of one field fill.
- **Detecting an ABSENT named local:** two loads of the same pointer field
  with no call between them but with intervening global stores prove there is
  no named local, since a named local is not aliased by those stores and
  would have been loaded once. That is how `RenderFullMap`'s `spr` local was
  identified as wrong (removing it also stops the value taking a callee-saved
  register the original keeps for something else).
- **`x = x + a + b` versus `x += a + b` is a no-op** (VC6 canonicalises); what
  moves the schedule is naming the sub-expressions in temps in the original's
  computation order.
- **Two functions are now known to be propped up by COMPENSATING ERRORS**, so
  their headline numbers understate the work left: `RenderView`'s 381 has +10
  instructions in one region cancelling -11 in another (fixing the limits
  alone improves region total 99 -> 88 and offset-blind 228 -> 187 while
  raising strict to 817), and `StepSchoolCar`'s former "351/351" was two extra
  instructions cancelling two missing ones. **Check for this before trusting
  any converged-looking count.**

- **The one-web y chain's non-rotation cost is a PENTIUM STALL-FILL, and it
  is unstoppable from source.** All four rides emit `sub eax,edx` then
  `add <sy>,eax`, and writing it that way reproduces those indices exactly and
  keeps the byte length — but VC6 then fills the partial-register stall after
  `mov ax,[cfg+0x22]` with a hoisted load the original leaves unfilled. Block
  splits before and after the compound, both `if` values, all four
  subtraction seams and all six orderings are byte-identical at that cost;
  only a volatile read stops it, and that costs the frame.
- **A one-pointer aggregate (`struct { T* p; } dd;`) is NOT exempt from
  enregistration** — it CSEs to one register copy exactly like a plain pointer
  local, so it cannot buy a load-at-each-use. **`static __inline` seed helpers
  add no IR tuples at a store site** — a two-argument seed call is
  byte-identical to the two stores written out.

- **Four negatives that close whole families of attempt:** two distinct
  STRUCT TYPES over one object value-number as ONE lvalue, so no cast can
  create the "two differently-typed objects" a same-width-conversion barrier
  needs (extends the union result); the empty `if (x) { }` is
  dead-code-eliminated and CANNOT extend a temp's live range into the
  allocator — it is a fold and block-split knob only; an inlined two-return
  helper does NOT create two reaching definitions (VC6 folds it before
  layout); and an inline wrapper round a call does NOT make its argument an
  un-coalesced temporary (the argument is forward-substituted).
- **A strict-count RISE with a register-blind FALL is the honest direction.**
  `WW_AnyBlokeInRect` was moved from 6 to 12 strict deliberately: the
  6-scoring body produced 30 instructions and 67 bytes — one short of the
  original in both — with a mis-scheduled `mov eax,1`, while `return 0;` gives
  31/31 instructions, 68/68 bytes, identical blocks and branch offsets, and
  **register-blind distance 0**, every mismatch being one eax/ecx swap.
  Return-coalescing would also have ELIDED the `xor eax,eax` the original
  emits. Mechanism measured: the race is decided by the LOOP-LOCAL TEMP's
  reference count (0 or 1 in-loop compares gives the cursor eax, 2 gives the
  temp), and adding cursor references does not buy it back.

- **In an isometric pair, the SOURCE ORDER OF THE TWO PRODUCTS decides which
  is emitted first, and that decides the whole register cascade.** Sum-first
  gives `lea t,[a+b]` with no copy; difference-first gives `mov t,a / add a,b
  / sub t,b`. This is what unlocked the two-web X chain in
  `TempleSlide_Update` (72 -> 56) after the webs themselves had been right for
  a round. In `StepSchoolCar` the same job is done by the order of the two
  `>>= 9` statements rather than the products — and that shift order is the
  ONE construct that moves its otherwise-invariant permutation, which is the
  diagnostic saying the projection block was the wrong place to look.
- **The frame slots of two address-taken scalars filled by ONE out-param call
  follow their first RVALUE USE, not declaration order.** With the sum first,
  one is read first and takes the lower slot, so the call emits its two `lea`s
  the other way round. Swapping declarations, block-scoping and a pre-read
  probe are all inert.
- **The partial-sum aggregate barrier transfers across files, but only in one
  shape:** a non-address-taken `Pos` whose SECOND field also holds a two-term
  sum closed indices in three different functions across two files. A
  one-term second field is inert, and the same barrier applied to the paired
  axis is usually catastrophic.

- **ALLOCATION PRECEDES SCHEDULING — that door is shut.** Tested loosely on
  `JungleCruise_Tick`: with a volatile pin the contested global load really is
  emitted at the original's exact index, and the value STILL takes the wrong
  register. So no amount of moving an emission point can change a colouring.
  This retires the whole "get the load to the original's index" family.
- **VC6 always emits `[then][else][merge]`, and that makes some block layouts
  UNREACHABLE.** `UpdateRiverAnim`'s original lets one arm fall into the
  shared call and EXILES the other past the end of the loop's fall-through
  trace (in one loop between the two loops, in the other AFTER the function's
  `ret`). 17 spellings collapse to three objects: the goto-target lever is
  byte-identical; writing the call in both arms makes VC6 cross-jump but it
  always glues the merged tail to the LAST arm and exiles the FIRST — the
  exact mirror; and a reachability probe with a `goto` back into the loop body
  is normalised by the front end into the plain if/else. **81 of that
  function's 112 is therefore unreachable, not unfound.**
- **A residual can be MISAPPORTIONED — re-derive the breakdown before
  attacking it.** That same function's note had blamed "addend order plus two
  scheduling clusters"; in fact 44 of the mismatches are a pure three-slot
  shift of instruction-identical code, a consequence of the block layout, and
  the sums account for only 31.
- **CROSS-FILE TWIN — real, but the note MISREAD which way round it is
  (corrected 2026-09-04).** `Carousel_Tick` 228-231, `SpiderRide_Activate`
  195-198 and `PlaneRide_Activate` 198-201 are the same difference, but the
  ORIGINAL's two `movsx` loads are **ASCENDING** (+0x3c then +0x3e) and it is
  OURS that are descending. The mechanism the earlier note gave is right — VC6
  evaluates the LAST store's operand first, so an x-store-first source forces
  the +0x3e load first — but the target is `{ascending loads, x-store first}`,
  and three functions had been aimed at the wrong shape. Final registers and
  both stores already agree; only the two loads are swapped. 20 further
  spellings measured against the corrected target — still unreachable.
  **Settled 2026-09-05 by a complete 16-cell grid** {declaration order} x
  {which temp carries the `* 2`} x {store order}: the two loads are ALWAYS
  emitted in the reverse of the two stores' order; **declaration order is
  completely inert** (every cell byte-identical to its twin); and — correcting
  this entry's earlier clause — `shl r,1` versus `add r,r` / `lea r,[r+r]` is
  decided NOT by which operand loads first but by **which temp carries the
  `* 2`**. With the y temp carrying it, both doublings become `shl` and the
  Plane's extra byte (a 3-byte `lea` against a 2-byte `shl`) disappears, but
  the two component pairs then come out interchanged.
  `*(volatile int*)&pos2.x = ...` is the only spelling that produces the
  original's ascending loads, and it pins the store.
- **THE COLLECTIVE TARGET — the "one instruction slot" framing below is
  SUPERSEDED (2026-09-05): it is one REGISTER CHOICE, and its cause is named.**
  Deleting the `p = b->person;` cache from `SpinningBarrels_Activate` brings the
  whole window right at once — strict 25 but **register-blind 3**, with indices
  120-137 becoming the original's 121-138 under a pure one-slot shift whose only
  content is the missing `mov ecx,[esi+4]`. So the empty X slot, the twelve-slot
  sink of the two float-constant pushes and the Y-slot fill are all DOWNSTREAM
  of one allocation: the `screen.ox` temp takes EAX in the original (reusing
  what the halving chain just freed, so it cannot hoist) and EDX in ours,
  because `p` is sitting in ECX. The cache still cannot simply go — without it
  the load sinks below the escaped `pos` stores — so `p` at 118 and a serial
  `screen.ox` are not simultaneously reachable by any spelling yet found. The
  underlying fact to attack: **the original leaves ECX idle** from the
  `cfg->oy` read right through to the `b->person` reload, and we always fill it.
  `SafariRide_Activate`'s 132 is likewise ONE register (`p` is ECX for us, EDX
  in the original, loaded at 103) — from index 113 to the end of the case both
  bodies are the same instructions in a three-way eax/ecx/edx rotation, which is
  exactly what rb 15 against strict 132 measures. Original framing follows. The five
  "windows" are one idiom appearing twice per function, once per axis:
  `mov eax,[rider offset] / cdq / sub eax,edx / <<<SLOT>>> / sar eax,1 /
  sub <acc>,eax / mov eax,[esp+screen.o?] / sub <acc>,eax`. **The entire
  family residual is which instruction, if any, VC6 puts in that one slot.**
  Two rules describe the original and we break both in OPPOSITE directions:
  the original NEVER fills the X slot, and ALWAYS fills the Y slot with the
  first ready operation belonging to the statements AFTER the two `pos`
  stores. We fill the X slot on the three rides that cache the person
  pointer, and never fill the Y slot from below. Everything else in the five
  windows — the two float-constant pushes, the pointer load — is DOWNSTREAM,
  taking the next free slot, which is why they land 6-12 indices late with
  identical instructions, registers and byte length either side. The block's
  own source is INVARIANT: ~60 spellings byte-identical, including flat
  three-term tails, named halves, an `Offset*`, all interleavings, a cache at
  three points (VC6 sinks the load every time — **local register pressure
  cannot be raised from source**) and respelling the two adjacent globals as
  one object.
- **Two negatives worth keeping:** a load/load/store/store pair is NOT
  necessarily a whole-struct copy (`*(Pos*)&b->x = world;` was byte-identical
  and did not explain it), and a same-difference rewrite that holds the byte
  length in one function can run 17-19 bytes OVER in its sibling — so a
  cross-ride transfer must be re-measured, never assumed.

- **CHECK EVERY `__asm` OPERAND NAME AGAINST MASM'S RESERVED WORDS — this
  found a live correctness bug.** In `Draw3DPersonModel` a local named `cr2`
  passed to a fixed-point macro expanded to `__asm { mov cr2, eax }`; MASM
  resolved the bare name to the CONTROL REGISTER and emitted the privileged
  `0f 22 d0`. The local was never written, and the following comparison read
  an uninitialised dead-argument slot. **The shipped body would have faulted
  at ring 3.** Reserved names to avoid: `cr0`-`cr4`, `dr0`-`dr7`, `tr3`-`tr7`,
  `st`, and the segment names. Signature in a `/FAc` listing: `0f 22 d0
  mov cr2, eax` where you expected `mov DWORD PTR _x$[ebp], eax`. Swept the
  tree 2026-09-04: the only other reserved-name local is an `st` in
  schoolcar.c, which is never an `__asm` operand and whose file is fully
  exact, so no other live instance exists — but re-run the sweep whenever a
  new `__asm` file appears.
- **A 12-byte STRUCT COPY is not the same lever as three field assignments.**
  The copy makes VC6 pin the source address in one register and dereference it
  (`mov ebx,ecx` then `[ebx]`/`[ebx+4]`/`[ebx+8]`); field-wise assignment folds
  the first field into the scaled address and emits no copy. It also decides
  WHICH field survives in a register, and it reproduced an a/b pool-slot swap
  between two loops. Diagnostic: a multiset diff of the block showing the body
  short by exactly two `mov R,R`.
- **A commutative operand order that "canonicalises" under one fetch shape can
  become LOAD-BEARING under another — re-test association after any structural
  change.** `(z+x)` versus `(x+z)` had been inert for months and, once a
  struct copy replaced field-wise fetches, was worth ~180 mismatches: the
  wrong order loads the fields into the wrong registers and forces a reload
  the original reuses.
- **Block scope on an ADDRESS-TAKEN ARRAY is an ALIAS lever, not only a frame
  lever.** At function level VC6 will not hoist a load through an unrelated
  pointer above a store into the array; moving the declaration into the loop
  restored the original's three-deep software pipeline at four sites. Depth
  and extent of the block are inert (one block spanning both loops, one per
  loop, and a deeper block are byte-identical), and block-scoping plain
  scalars is completely inert.
- **VC6 emits integer `mov`s for a float member copy**, so a `float` member
  and an `int` member read through a cast are byte-identical — the field type
  is not observable at such a site.

- **VC6'S BLOCK LAYOUT ALGORITHM, recovered: trace with a LIFO pending
  stack.** Follow the fall-through chain; push every conditional's target onto
  a stack; when a trace ends, POP. The model predicts both the original's
  layout and ours exactly on a 1161-instruction function, from the same input
  — so a layout residual reduces to which arm of one conditional falls through
  into the join. On `RenderFullMap` that single bit accounts for a 303-slot
  residual: the original's then-arm falls through and the else-arm is sunk
  past three later blocks, jumping BACKWARD into the middle of a two-
  instruction pair. Ruled out for that bit: eleven spellings of the selection
  (hoisted pointer, inverted arms, empty trailing `else`, guarded early break,
  duplicated test, a two-case `switch`, two `goto` forms, an inline filler, a
  `?:` with a comma) — VC6 emits `[test][arm1][jmp join][arm2][join]` for every
  one. Full tail duplication DOES sink the block but the copies never merge,
  because VC6 forwards a just-stored field into the else arm's tests; the
  original's join is genuinely shared, not a merge artefact.
- **`RenderFullMap`: five reconstruction errors, 894 -> 847 with every
  structural measure improving** (region total 594 -> 498, register+offset-
  blind 311 -> 266, bytes 4199 -> 4223 of 4225). (1) The scroll offset is
  added to the bounds BEFORE the cell-size globals, not inside the projection
  helpers — worth 77 structural slots, because the bounds struct is
  address-taken so a store to a GLOBAL may alias it and VC6 cannot move a
  field update across one; the wrong placement emitted a second load/store
  pair at all four sprite sites. (2) Three calls share ONE named `Pos`, and
  the last reads a field and writes the adjusted value into the COPY — which
  retracts an earlier note's "the original really does mutate it in place".
  (3) Two projections were swapped between two API calls. (4) There is ONE
  20-byte cell copy, not two (the original has exactly three `rep movsd`).
  (5) Two "frame ballast" demotions from the previous round are removed — the
  original pools a `Pos` at those sites too. **Cost stated honestly: the frame
  is now 4 bytes over, because the previous exact frame size was the correct
  shape minus three things that do not exist.**

- **VC6 fully REASSOCIATES a FOUR-term flat sum, and the partial-sum barrier
  applies only to four-term sums.** All 24 source orders of a four-term sum are
  byte-identical, sorted by descending definition point. **This is why the
  partial-sum aggregate barrier is right in one function and wrong in its
  sibling** — apply it only to four-term sums. *Corrected 2026-09-05: this
  entry used to add "a three-term sum keeps what you wrote", which was a
  mis-summary of the evidence below and contradicted two other entries. A flat
  three-term sum is canonicalised as well; what fails on three terms is
  BREAKING it into partial sums, not reordering it.* Getting this wrong was a committed error in `StepSchoolCar`
  (the barrier emitted `lea ecx,[edx+edi] / add ecx,eax` where the original
  has the flat `xor ecx,ecx / mov cx,[..] / add ecx,eax / add ecx,edi`), and
  it was exactly where the whole-function scratch phase parted company.
- **A one-step eax/ecx/edx phase can be fixed by caching a REAL value in a
  local** — one that is an actual load, such as an indexed global, not a
  rename of an existing value (which VC6 copy-propagates). Worth 27 indices in
  `TempleSlide_Update`, running as a clean three-cycle through two whole
  switch cases.
- **Whole-struct assignment versus field-by-field is visible in the BASE
  REGISTER:** an 8-byte struct store goes through one materialised pointer,
  while field stores let VC6 pick a different base per field — and the struct
  form can place the second store AFTER two of the epilogue pops. A save and
  its matching restore can legitimately differ: one field-by-field (two loads
  on different bases), the other a whole-struct copy.
- **Where the split-shift form applies (`sx >>= 9; sy >>= 9;` as separate
  statements), the PRODUCT order stops mattering entirely and only the SHIFT
  order does** — confirmed independently in two functions.
- **"Re-test committed shims" paid again:** once an unrelated phase error was
  fixed, a previously winning `unsigned char` cache became wrong and the
  plain field read beat it. **A shim earns its place only against the current
  baseline.**
- **A constant frame SHIFT can make the strict count meaningless:** in
  `AnimApplyPart` indices 22-216 are identical instruction for instruction
  under a uniform +4 offset, so the 182 massively overstates the distance.

- **A CONFOUND that invalidated a family of earlier measurements.** Moving a
  flag store, a sprite store or a pointer cache above `pos.x = sx2 * 2;` makes
  VC6 REASSOCIATE the four tail subtractions into `sx2 + (-screen.ox - h)`
  (`mov/sar/neg/sub/add`) and take the frame 0x3c -> 0x38 — so earlier notes'
  330-380 scores were measuring the reassociation, not the hoist they claimed
  to test. An empty `if` does NOT substitute for that barrier. **The clean
  instrument is the FOLDED STORE form** `pos.x = (sx2 - h/2 - screen.ox) * 2;`
  — byte-identical to the statement form and immune to the reassociation.
  Use it to de-confound any "hoist a statement" experiment. Re-run in that
  form, the previous floor survived on an unconfounded measurement.
- **Correction: it is the NAMED `ys` LOCAL, not the one-web y chain, that
  flattens the four later subtractions.** `sy += cfg->oy - Get_YScroll();`
  with ONE name holds the byte length exactly; introducing `ys` costs two
  bytes, i.e. flattens. The empty `if` is INERT in the one-name spelling.
- **The slot follows the VALUE, not the chain:** storing the other axis first
  flips which halving is emitted first, but the same global is hoisted in both
  orders (a compensating error — strict 26 vs 29 while real goes 8 vs 11).
- **`SpaceTower_Activate` root cause, stated end to end:** the original
  reloads the descriptor TWICE (two registers at indices 22 and 23) so one
  dies at the following load and is free for the contended value; we reload
  once, that register stays busy to index 32, the value takes eax and gets
  spilled, and the frame goes 0x10 -> 0x14. A route scoring 103 against 138
  exists but is NOT committed: register-blind rises 14 -> 21 and the frame is
  wrong — the compensating-error signature. **If a future pass finds a
  non-volatile second reload at index 23, both changes go in together.**

- **Which projected coordinate is FINISHED FIRST decides whether VC6 computes
  the sum in place or with a `lea`** (`TempleSlide_Update`, 29 -> 22 with the
  byte length now EXACT). A sum computed in place means the sum is the LAST
  use of its operand, so the difference precedes it in the IR: writing the X
  projection before the Y one reproduces `mov ebp,ebx / add ebx,edi / imul
  ebx,[th] / sub ebp,edi / imul ebp,[tw]` exactly, where the sum-first order
  emitted a `lea` plus a compensating `mov` — the single byte the body was
  over. **A coupled callee-saved tie-break can then be broken by moving reads
  ABOVE a preceding call**, lengthening a pointer's live range at zero code
  cost (reads before = 22, reads after = 327, one read before = 327).
- **A conversion block's source order can be READ OFF the original's operand
  displacements**, retiring a search space instead of sampling it. In
  `AnimApplyPart` the ten `fild` operands spell out exactly the shipped
  source order — which matters because one reordering scores better on every
  metric while growing the frame, and is known-wrong source.
- **An address-taken out-param becomes shareable with a spill home only by
  being born inside a `static __inline` helper** (it becomes an
  inline-expansion temporary in the spill pool rather than a named
  address-taken local). That yields `StepSchoolCar`'s original tail without a
  fifteenth slot — though inlining there destroys the sum-first projection, so
  the two are not yet compatible.
- **WITHDRAWN: two standing items on `TempleSlide_Update`.** The `Pos t`
  partial-sum barrier is removed — it was a workaround for the wrong
  projection shape and a misapplication of the four-term rule to a TWO-term
  sum; four plain `-=` are better on every measure. And the recorded pointer
  "if a later round finds what ranks `sq` above `ty`, the two-web X form is
  very probably the finished function" is refuted: that condition was found,
  all seven forms were re-run crossed with every read and call placement, and
  they still score 327 and ESCAPE. The extra web is a second independent tip
  of the same tie-break.

- **FRAME WEIGHT IS THE SURVIVING-IR REFERENCE COUNT — and that closes
  `Draw3DPersonModel`'s frame by proof, not exhaustion.** Two decisive
  negatives: routing every use of an array through an alias pointer is
  byte-identical (copy-propagated, counted after), and 8, 16 or 24 DEAD reads
  are byte-identical (eliminated before the count). Since our surviving
  profile equals the original's slot for slot (384 = 384 references, 79 = 79
  slots), **no source spelling can change the weights.** Size probes confirm
  it from the other side: 84B and 112B both land at position 3, 140B and 224B
  at position 4 — **position 5 is never reached by size**, so the object
  cannot be pushed down by being larger. Ours and the original share an
  identically shaped 49-slot pool spine; only the four array insertion points
  differ by one chunk.
- **New frame class:** an array born in a `static __inline` helper is an
  inline-expansion temporary, sits one step FURTHER from ebp than the same
  array block-scoped, and at two or fewer references goes past larger arrays
  to the far end. (Unusable in the case at hand, because the helper would have
  to contain an `__asm` block naming two of the frame objects as MASM symbols.)
- **A three-load run with the base register overwritten by the LAST load is
  the signature of three consecutive source reads through one pointer** —
  reordering an unrelated assignment into the middle of them is observable.
  That is how `Draw3DPersonModel`'s last head error was found: one field read
  belongs BETWEEN two others, not after all three. All 120 orders of the five
  head assignments were compiled and it is the unique best; every structural
  measure rose with it.
- **The landscape moves, so re-test "byte-identical" claims:** an array in one
  block spanning two loops was recorded as byte-identical to one block per
  loop, and after an unrelated fix it is three worse.

- **THE BLOCK-EXILE RULE, with eleven in-tree proofs.** A block that ends in
  an unconditional `jmp` is EXILED past the fall-through trace; a block that
  falls into its successor is laid out in place. A tree-wide scan of all 1544
  exact functions for the signature (a conditional whose target block ends in
  a backward `jmp` into the middle of a fall-through-reached region) found
  eleven hits, four of them genuine source-level cases — `UpdateMapDrag`
  0x452030, `InitSavedGameScreen` 0x48d4b0, `KillAllSamplesFromSource`
  0x496b80, `LoadObjectLibrary` 0x480f00. **The source idiom that makes an
  else arm end in a `jmp` is CROSS-JUMPING:** write both arms out in full with
  a common tail; VC6 merges the common suffix, glues it to the THEN arm and
  rewrites the ELSE arm's copy into a backward jump into the middle of the
  then arm's straight-line code. *(Clarified 2026-09-05: "THEN arm" here is
  not a source-position rule — in these cases the else arm is EXILED, so the
  then arm is the copy that falls through into the join. The general rule,
  reconciled from four measurements, is that the fall-through copy survives
  and the copy in a jump arm is merged away; in a plain if/else chain that is
  the LAST arm. A parallel-session note read this entry as contradicting the
  last-arm entries; it does not.)* A source `goto` survives only when it crosses
  a LOOP boundary — otherwise the front end normalises it into a plain if/else
  and inverts it.
- **TOOLING BUG that had corrupted earlier frame readings: reset esp to the
  frame base at EVERY BRANCH TARGET.** A linear push/pop simulation drifts
  across joins and misreads every `[esp+N]` after the first branchy region.
  Four of one lane's seven reconstruction errors were invisible without the
  fix, and a previous round's recorded slot attribution was simply wrong.
  Tools: `scratchpad/w9renderview/esp.py`, `slots.py`.
- **`(y>>3) * 32` is NOT the same as `((y>>3) << 5)`** — the multiply blocks
  reuse of a nearby `y & ~7` while the shift allows it. **A pre-scaled byte
  index buys a separate `shl` where an array index folds into the scaled
  addressing mode**, and a 2-D array declaration gets both effects and is the
  natural spelling (a 32x32 mark grid reproduced a 35-instruction block
  exactly, where the flat index reassociates and folds the scale).
- **Absent-named-local detection, SECOND form:** a value RE-DERIVED from a
  reloaded pointer across calls — rather than reloaded from its own home —
  proves there is no named local for it.
- **Corrections:** VC6 does NOT fold a duplicated null test on an
  address-taken struct (a previous round claimed it does); and the standing
  instruction to adopt a particular geometry ordering "once the +10 region is
  fixed" is RETIRED — moving both limits out removes both halves of that
  compensating pair but breaks the split prologue, so the two are one problem.
  The prologue bit is also narrower than recorded: the constants still take
  the same registers in a broken variant; what changes is whether one value's
  web coalesces with the constant-zero register.

Recorded so they are not re-derived; several cost hundreds of measured variants:

- **RLE loop form.** `mov ecx,eax / dec eax / test ecx,ecx / je <end> / inc eax`
  is VC6's lowering of `while (n-- != 0)` on an *unsigned* counter — it rotates
  the post-decrement while into a do/while over a separate trip-count slot and
  leaves the dec/inc as dead code. An `if(!n) goto end; t=n; for(;;){...}` form
  cannot produce it, and hand-writing the dead pair fails (VC6 deletes it).
- **Opcode dispatch.** An `if/else-if` chain on `x & 0xc0` compiles to a compare
  chain; `switch` produces the original's index+jump-table (or `sub/je/sub/jne`)
  *and* its block order. This was worth several points in every RLE section.
- **Declaration order is irrelevant.** Reversing the entire local block yields a
  byte-identical object. Slots are assigned by first use in the optimised IR.
- **Unions on an enregistered scalar are a trap** — they make it address-taken,
  so VC6 stops enregistering it (unioning one hot local cost 27 points).
- **Aggregate pinning is the real lever — but ONLY if the address escapes
  (qualified 2026-09-05).** Two locals contending for a slot are ordered by VC6;
  putting them in one `struct` fixes their relative offsets in C and removes
  them from that decision. Merging the four frame buffers into one aggregate was
  what finally placed `pos34`/`F_tsm` as the original has them. **The
  qualifier:** VC6 FLATTENS an aggregate whose address is never taken, so the
  pinning silently does nothing — a `CarPos` wrapping `swx`/`swy` and `tx`/`ty`,
  and a `Pos` wrapping `ox`/`oy` in both `DrawBoats` attractors, are all
  byte-identical to the unwrapped source. Check that something actually takes
  the aggregate's address before crediting a struct with a layout change.
- **Inline-expansion temporaries** share the lifetime-coloured pool with register
  spill homes, so an address-taken temp born inside a `static __inline` helper
  can occupy a slot a *named* local never can. This reproduced the original's
  four distinct S8 `Pos` slots after ~1400 union/struct variants had failed.
- **Deferred/split prologues are reachable.** VC6 will sink `push ebp/esi/edi`
  into a function's non-null path (leaving only `push ebx` before an early null
  check) if the loop locals live in a **post-guard inner scope** and the loop
  exits via `break` to a single trailing return. `GetObjectFromName` (0x44dda0)
  sat rejected at 84% for a long time on the belief this was unreachable from C.
- **Degenerate branches are real.** VC6 SP3 sometimes emits a test whose two arms
  compute the same value in different registers (`g_tile_info & 0x20` at
  0x004622ae / 0x00462333). Matching requires writing `if (c) f(a); else f(a);`;
  those sites are commented so they are not "simplified".

Matching this function also surfaced three genuine defects in the reconstruction:
a control-flow error that re-decoded the map-flags chunk as base tiles for any
map with height > 0, a read of never-initialised bytes in the S10 end-of-run
test, and a pointer dereferenced one level too deep.

Next: outward across the remaining exported functions and additional internal
functions discovered between them.

## Legal

No game code or binaries live in the repo — only human-written C reconstructed
from analysis of a legally-owned copy, verified against the reader's own
`original/legoland.exe`. MIT-licensed.
