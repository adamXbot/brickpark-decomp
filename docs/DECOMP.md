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

**As of 2026-09-04 (early): 1529 functions at 100%** — 663 of the 675 code
exports (98.2%) plus 866 recovered unexported functions, together **42.4% of
the game's ~628 KB of code** (`python3 tools/coverage.py`; 51.3% including
partials).
`SaveGame` and `LoadGame` are both exact so the whole `.sav` format is
documented and reproduced; `tri3d.c` reproduces the software 3D renderer;
`docs/RIDE_CALLBACKS.md` names 265 ride callbacks and which object slot each
fills. See `docs/HANDOFF.md` for the session checkpoint and what to do next.

12 exports remain, every one a genuine partial carrying its measured residual
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
| exported functions matched | `tools/remaining.py` | 663 of 675 (98.2%) |
| unmatched callees | `tools/callees.py` | moves both ways — the frontier, not progress |
| **bytes of game code matched** | **`tools/coverage.py`** | **42.4% (51.3% with partials)** |

The first two are both true and both misleading on their own.

**Exports are a fraction of the game.** They are only the symbols the linker
exposed; 1529 functions are matched but just 663 of them are exports. Quoting
98.2% as "the project is nearly done" is wrong by a wide margin.

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
