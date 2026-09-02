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

`tools/match.py` (and therefore `tools/verify.py`) disassembles only up to the
**first `ret`**. For a single-`ret` function that is the whole body, but for a
function with an early-return guard it compares only the prologue — a stand-in
tail would pass falsely. Every committed function is therefore also gated by
**`tools/audit.py`**, which establishes each original function's true extent
by control flow (the first `ret` that no earlier branch jumps past, or an
unconditional `jmp` that leaves the function) and requires the compiled body to
match it on instruction count, byte length and a strict index-for-index
comparison. `tools/matchfull.py` (full body, difflib-aligned) is the
iteration tool; `audit.py` is the authority. Prologue-only or fabricated-tail
reconstructions are never committed as `// FUNCTION:`.

    python3 tools/audit.py LEGOLAND/foo.c      # [OK]/[WIP]/[REJECT], ends PASS/FAIL

Beware the two ways `matchfull` misleads: it truncates the original to the
compiled length (a short reconstruction can score a false 100%), and it keeps
decoding the `.rdata` jump table that `/Gy` places after a `switch` function's
final `ret` (a correct function can score 77%). `audit.py` handles both.

## Status

**As of 2026-09-03: 800 functions at 100%** — 645 of the 675 code exports
(95.6%) plus 155 recovered unexported functions. (716 symbols are exported; 41
are data — `python3 tools/remaining.py --data`.) `SaveGame` and `LoadGame` are
both exact, so the whole `.sav` format is documented AND reproduced, and
`tri3d.c` reproduces the software 3D renderer.

32 exports remain. **14 of them are already exact** and are held only because
the shared `tools/match.py` stops at the first `ret` and cannot bound a void
tail-jump wrapper (measured: it scores `KillHelp` 37.5%) — see "Tail-jump
functions" below; `tools/audit.py` certifies all 14 with zero mismatches. That
leaves **18 genuinely unfinished functions**, each carrying its measured
residual and first diverging instruction index in a note above its marker.
Run `python3 tools/remaining.py` for the live list.

### Three progress numbers, and which one to quote

| measure | tool | value |
| --- | --- | --- |
| exported functions matched | `tools/remaining.py` | 645 of 675 (95.6%) |
| unmatched callees | `tools/callees.py` | 796, ~41,400 instructions |
| **bytes of game code matched** | **`tools/coverage.py`** | **29.1% (37.6% with partials)** |

The first two are both true and both misleading on their own.

**Exports are a fraction of the game.** They are only the symbols the linker
exposed; 985 functions are matched but just 645 of them are exports. Quoting
95.6% as "the project is nearly done" is wrong by a wide margin.

**The unmatched-callee number moves in both directions.** Every newly matched
file declares `extern`s for its own callees, so a productive round can RAISE
it: batch 24 matched 185 functions and the figure went from 631 callees /
31,000 instructions to 796 / 41,400, because the new files revealed more
frontier than they consumed. It measures the frontier, not progress.

**Bytes of matched code against bytes of game code is the honest headline.**
It only moves by doing work. `.text` is 679 KB, of which about 51 KB is the
statically-linked CRT (from 0x0049e000 up) and not a decompilation target,
leaving ~628 KB of game code. 183 KB of that is matched exactly.

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

`tools/match.py`'s disassembler **stops at the first `ret`**, so multi-return
functions are only verified up to that point. `tools/matchfull.py` (new file;
does not touch the shared `match.py`/`verify.py`, takes `--obj` for parallel
safety) compares the **whole** function, and `tools/audit.py` is the strict
extent gate described above. Suggest folding the full-body walk into
`match.py` alongside the parallel-safety fix.

### Tail-jump functions (proposed `match.py` change — shared file, not applied)

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
sentinel and are external by construction). Both functions are committed as `// WIP-FUNCTION:` with a note, so
`verify.py` stays green; flip them to `// FUNCTION:` once `match.py` applies
the same rule (the change is the `true_extent`/`end_of_body` pair in
`tools/audit.py`).

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
