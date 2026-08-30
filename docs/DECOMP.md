# Matching decompilation — workflow

The goal of the decomp proper: rewrite `legoland.exe` in C that, compiled with
the **Visual C++ 6.0 SP3** toolchain the game shipped with, reproduces the
original machine code **function-by-function**. `legoland.exe` exports 716
named functions ([`symbols/legoland.exports.txt`](../symbols/legoland.exports.txt)),
so we already have the names — a head start LEGO Island never had.

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

## Status

The map/render accessors and the `SetMapTile` family — **13/13 at 100%**:

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

### In progress

- **`PutObjOnMap` (0x00459ad0)** — fully reversed and logically complete; **79.7%
  normalized** (102/128). The whole body matches (placement callback, the
  object-type stat switch → per-category area accumulators, `GetRectArea`,
  `AddObjectsPowerStats`); the remaining 26 are a register-allocation divergence
  in the `ENTRANCE 1` coordinate tail (VC6 keeps the element-data pointer in
  `edx` and reuses `eax` for `pos->x`; our build allocates the other way). Kept
  as `// WIP-FUNCTION:` in `LEGOLAND/mapobj.c` so `verify.py` stays green; the C
  documents the real behaviour and struct/global layout for host use.

## LoadBaseMap interface (for host / WASM integration)

`LoadBaseMap` @ 0x00461a50 is fully reversed (see
`scratchpad/slope_re/loadbasemap_cell_construction.md` and
[FORMATS.md](FORMATS.md)); the C match is a later batch, but its **contract is
stable** for Codex to wire to the host now:

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
  then fills the cells; it does **not** allocate the grid (that's `InitGameMap`
  @ 0x59850). **Map ownership stays with the host allocator.**
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

## LLIDB image database (asset resolution)

The LLIDB is the name→asset registry the whole asset pipeline goes through:
`FindElement(name) → LoadData(elem) → per-type parser → parsed table`. Element
struct (20 bytes, insertion-indexed via a paged table): `name`@0, `image`@4,
`type_flags`@8 (`(type&0xfff0)|loaded-bit0`), `data`@0xc, `refcount`@0x10.
Globals: capacity @0x6691a0, count @0x6691a4, page table @0x6691a8.

**Matched (100% normalized, full-body):**

| addr | function | |
| --- | --- | --- |
| 0x0047b2d0 | `LLIDB_GetCount` | count |
| 0x0047b2e0 | `LLIDB_GetElement` | paged index → element |
| 0x0047b330 | `LLIDB_FindElement` | name scan (stricmp) |
| 0x0047b410 | `LLIDB_FindElementFromDataPtr` | data-ptr scan |
| 0x0047d3a0 | `LLIDB_LoadData` | lazy loader/dispatcher (bit0=loaded; switch on type&0xfff0) |

`LoadData` dispatch: 0x10/0x1010→ODF, 0x20→TSM, 0x40→TSF, 0x400→ILF,
0x2000→CSP, 0x200/0x800→no backing file (returns 0).

**WIP (reversed & logic-complete; large heap/parse functions pending the VC6
call-scheduling / stack-layout grind — kept as `// WIP-FUNCTION:`):**

- `LLIDB_GrowAndGetIndex` (0x47b5a0) — grows the paged table in 256-elem pages
  (realloc page array + malloc a 0x1400 page); returns the new index.
- `LLIDB_RegisterNewElement` (0x47b610) — dedup by name, else strdup name/image
  into a fresh slot; only +0/+4/+8/+0x10 written, count++.
- Per-type parsers — **on-disk formats and outputs, for the runtime:**
  - `LLIDB_LoadTSMData` (0x47ce40): opens `TileData\<image>`, reads `u32 count`,
    `mallocs (count+1)` **8-byte records `{LLElem* entry, void* loaded}`**, reads
    a self-name (discarded), then per tileset reads `u32 len`+name → FindElement
    → LoadData, filling `{entry, loaded}`; terminates `{-1,-1}`. Sets
    `elem->data`=recs, `type_flags|=1`.
  - `LLIDB_LoadTSFData` (0x47cba0): builds a 36-byte descriptor `{+4 n_tiles,
    +8 AllocTileSpace base id, +0xc code[], +0x10 second[]}` — the **base id**
    tile codes are added to (see FORMATS.md tile pipeline).
  - `LLIDB_LoadILFData` (0x47cfc0) / `LLIDB_LoadCSPData` (0x47d1a0): image list
    `{u16 n, u16 type, name, n×(dx,dy doubled), n× .lls}`, each sprite loaded.
  - `LLIDB_LoadODFData` (0x47bf70): object-def blob loader.

Full disassembly-grounded drafts for all of the above are in
`scratchpad/slope_re/*.md` and the loader-draft workflow output.

## Tooling note (for Codex)

`tools/match.py`'s disassembler **stops at the first `ret`**, so multi-return
functions are only verified up to that point. I added `tools/matchfull.py` (new
file; does not touch the shared `match.py`/`verify.py`, uses its own
`/tmp/_matchfull.obj`) which compares the **whole** function — it caught
`LoadData` being only 82.8% past the first `ret`. A full-body re-check confirms
all committed functions are 100% end-to-end. Suggest folding the full-body walk
into `match.py` alongside the parallel-safety fix.

Next: grind the LLIDB parsers/registration to 100% (stack-layout + call-schedule
iteration), the `PutObjOnMap` tail, then `LoadBaseMap` — and outward across the
716 exports.

## Legal

No game code or binaries live in the repo — only human-written C reconstructed
from analysis of a legally-owned copy, verified against the reader's own
`original/legoland.exe`. MIT-licensed.
