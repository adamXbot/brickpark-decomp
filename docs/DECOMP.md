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

**As of 2026-09-04: 1542 functions at 100%** — 665 of the 675 code exports
(98.5%) plus 877 recovered unexported functions, together **44.4% of the
game's ~628 KB of code** (`python3 tools/coverage.py`; 51.3% including
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
| **bytes of game code matched** | **`tools/coverage.py`** | **44.4% (51.3% with partials)** |

The first two are both true and both misleading on their own.

**Exports are a fraction of the game.** They are only the symbols the linker
exposed; 1542 functions are matched but just 665 of them are exports. Quoting
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

### VC6 SP3 codegen levers (learned the hard way on `LoadBaseMap`)

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
- **Cross-jump groups host the merged copy at the group's LAST member**
  (earlier users jump FORWARD into it); the wrong spellings host at the
  first. A copy's eax/edx/ecx rotation follows its position in the FINAL
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
- **Two or more zero stores visible together always become `xor r,r` plus
  register stores.** An intervening aliasing store, a surviving branch, a loop
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
- **Cache a pointer field in a local FOR ONE STORE ONLY.** A store to an
  escaped local is a schedule barrier for a pointer load, so `p = b->person;`
  before the escaped stores hoists exactly the one load the original hoists;
  using `p` for the second store too is much worse, because the original
  really does reload after a store through the pointer.
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
  ONE sub node, so `add reg, -K` is not reachable from C.
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

- **The BNV-ride y chain's SOURCE SHAPE is solved (register assignment is
  not).** The original is ONE web with a compound assignment:
  `sy2 = (wx + wy) * th >> 9;` then `sy2 += g_map_cfg->oy - Get_YScroll();`
  — no separate `sy`. It reproduces the disputed indices exactly and is
  strictly better register-blind, but costs a three-cycle rotation of the
  callee-saved registers over the whole block, so it is documented and NOT
  committed. ~130 variants all score identically: it is not reference counts
  and not the number of names, but whether the scroll subtraction is itself an
  in-place op on the long web. Corpus evidence for the shape: a scan for
  `sub <scratch>,<scratch>` followed within three slots by
  `add <callee-saved>,<that scratch>` returns exactly two hits in the whole
  matched corpus, both `param += expr`.
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
  VC6 makes the compiler TEMPORARY the destination if the sum contains one,
  otherwise the EARLIEST-DEFINED symbol; the remaining addends are then added
  in descending definition order.
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
- **Aggregate pinning is the real lever.** Two locals contending for a slot are
  ordered by VC6; putting them in one `struct` fixes their relative offsets in C
  and removes them from that decision. Merging the four frame buffers into one
  aggregate was what finally placed `pos34`/`F_tsm` as the original has them.
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
