# scope K — movie player, path masks, textures (2026-09-05)

Findings from the three files docs/SCOPE_K_movie_paths_textures.md assigns, for folding into docs/DECOMP.md at integration. Per-lane originals beside this file carry the evidence.

| file | exact | WIP | notes |
| --- | --- | --- | --- |
| `LEGOLAND/texture.c` | 6 | 0 | `scope-k-texture.md` |
| `LEGOLAND/pathmask.c` | 5 | 1 | `scope-k-pathmask.md` |
| `LEGOLAND/movie.c` | 16 | 0 | `scope-k-movie.md` |

---

# texture.c

## scope K — `LEGOLAND/texture.c` (texture and detail-image records)

Lane: `texture`. Object prefix `/tmp/sk_texture_`. File created by this lane;
nothing else touched. **6 of 6 exact**, `tools/audit.py LEGOLAND/texture.c`
ends `PASS`, `/W3` clean.

| address | name | insns | % | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x00441870 | `ObjFirstRider` | 9 | 100 | `[OK]` | `// FUNCTION: LEGOLAND 0x00441870` |
| 0x00441890 | `ObjNextRider` | 14 | 100 | `[OK]` | `// FUNCTION: LEGOLAND 0x00441890` |
| 0x00496ff0 | `FindDetailImageSlot` | 16 | 100 | `[OK]` | `// FUNCTION: LEGOLAND 0x00496ff0` |
| 0x00496f30 | `DetailImage_AllocSlot` | 50 | 100 | `[OK]` | `// FUNCTION: LEGOLAND 0x00496f30` |
| 0x004437d0 | `BuildTextureRecord` | 136 | 100 | `[OK]` | `// FUNCTION: LEGOLAND 0x004437d0` |
| 0x004434d0 | `ConvertSourceImage` | 192 | 100 | `[OK]` | `// FUNCTION: LEGOLAND 0x004434d0` |

All six end in a real `ret`, none is recursive, none is a tail-`jmp` wrapper —
all six are promotable and were promoted. No name was renamed: every one kept
the name the scope table gave it.

## Mechanics recovered (spec for a browser runtime)

- **The rider iterator is a two-call protocol over ONE global cursor**
  (`g_rider_cursor`, 0x0081c8cc — first named here). `ObjFirstRider(item,
  inst)` seeds it from `item->riders` (+0xcc) and `ObjNextRider(item, inst)`
  steps it to `cursor->next` (+0x00); both then tail into the shared seek
  helper at 0x00441830, which walks forward to the first node whose `u16` at
  +0x0c equals `*(unsigned short*)inst`, parks the cursor there and returns
  it, or clears the cursor and returns 0. rin.c's `GetObjRiderN` drives the
  pair with `item->rider_count` (a `short` at +0x2e) as the bound. **The walk
  is therefore not re-entrant and not nestable** — two interleaved rider
  walks would tread on each other.
- **The detail-image registry** (`g_detail_images` 0x0079a7c4,
  `_cap` 0x0079a7c8, `_count` 0x0079a7cc) is a flat `ImageRec*` array grown
  0x80 slots at a time. `DetailImage_AllocSlot` grows ONLY when
  `count == cap`; otherwise it linear-searches for a NULL hole, so
  `UnregisterDetailImage`'s NULLing is what makes a slot reusable. The first
  block is `calloc(0x80, 4)`, later growth is `realloc(list, cap*4 + 0x200)`
  followed by `memset` of just the new 0x80 slots. The array is never shrunk,
  and `UnregisterDetailImage` frees it only when the count reaches 0.
- **The texture descriptor is 0x2c bytes** and matches tri3d.c's documented
  layout exactly: `+0x00 ushift`, `+0x04 vshift` (log2 of the pixel extents),
  `+0x08 texels` (w*h bytes), `+0x0c ramps` (0x404 bytes = `Shade*` per texel
  VALUE), `+0x10 umask` = w-1, `+0x14 vmask` = h-1. The remaining 0x14 bytes
  are left as `malloc` found them.
- **The shift fields come from a nine-case switch over the pixel dimension**
  (1, 2, 4, 8, 16, 32, 64, 128, 256 -> 0..8). A texture whose dimension is
  NOT a power of two in that range **falls through the switch and leaves the
  shift field uninitialised** — `RegisterTextureImage` (0x00488670) does not
  clear the record before calling. Reproduced, not fixed.
- **`BuildTextureRecord` builds the shading ramps as it copies.** For every
  texel it copies the byte into `tex->texels` and, the FIRST time a value is
  seen, calls `MakeShadedColour(0x40, &g_texture_palette[c])` and files the
  64-level ramp in `tex->ramps[c]`. So a texture's ramps are built lazily per
  distinct palette index, and `g_texture_palette` (0x0081c4c0, 256 RGBQUADs —
  first named here) is a SINGLE global scratch: `BuildTextureRecord` must run
  before the next `ConvertSourceImage`, which is exactly the order
  `LoadTextureImage` + `RegisterTextureImage` impose in data3.c's
  `LoadLocTextures`.
- **`ConvertSourceImage` is the texture cousin of screen.c's `__BMPLoader`**:
  same `BITMAPFILEHEADER` (14 B) + `BITMAPINFO` (0x2c B) read through RES,
  same "the palette lives at the file position after the file header + 40"
  assumption (so a V4/V5 `BITMAPINFOHEADER` mis-seeks), same
  uncompressed-only rule. Differences: **8bpp ONLY** (24bpp is rejected up
  front, where `__BMPLoader` supports it), the destination is an UNPADDED
  `w*h` byte image rather than the padded rows kept in place, the colour
  table goes to the global `g_texture_palette` rather than a local scratch
  and is NOT converted to 16-bit here (`BuildTextureRecord` does that through
  the ramps), and `image->type` is always 1. The source stride is
  `(biWidth + 3) & ~3`; the copy walks the padded rows from the LAST row
  backwards, so the bottom-up BMP order is flipped and the pad bytes dropped
  during the copy. Negative heights are not handled.

## Original bugs reproduced (commented at the site)

- `BuildTextureRecord` tests `if (img)` only AFTER reading `img->w`,
  `img->h` and `img->w * img->h` through it — the guard can only fire after a
  null dereference has already crashed.
- `BuildTextureRecord` leaves `ushift`/`vshift` uninitialised for a
  non-power-of-two dimension (above).
- `ConvertSourceImage` calls `free()` on the **ImageRec itself** in both
  allocation-failure paths, which data3.c's `LoadTextureImage` then
  `KillImage`s — the same double free screen.c's `__BMPLoader` carries, in a
  second function.
- `ConvertSourceImage` re-tests `biBitCount == 8` after the allocations even
  though it already returned 0 for anything else; the else edge is
  unreachable. It is load-bearing for the match (see the levers) — VC6 does
  not fold it because `bmi` is address-taken and there are calls in between.
- `ConvertSourceImage` gives `img->pal` a fresh 0x400-byte block that nothing
  in the function ever writes.

## Callees / globals named for the first time

| address | name | note |
| --- | --- | --- |
| 0x00441830 | `RiderCursorSeek` | 16i; advance `g_rider_cursor` to the next node whose `u16` at +0x0c matches `*(u16*)inst`. Its FIRST argument is dead (only `inst` is read) — both wrappers forward the pair unchanged. Carries a dead `lea edx,[eax+0xc]` at the top of its loop, immediately overwritten by `mov dx, word ptr [eax+0xc]`: a merged-arm ghost, worth knowing before anyone ports it. |
| 0x0081c8cc | `g_rider_cursor` | the single shared rider-walk cursor |
| 0x0081c4c0 | `g_texture_palette` | 256 RGBQUADs; the BMP colour table scratch shared by the loader and the record builder. tri3d.c's note that a ramp key is stored BLUE, GREEN, RED is what fixes the component order. |

Already-named callees used: `MakeShadedColour` 0x00486280 (tri3d.c),
`GetGFXFName` 0x0044de90 (rin.c), `RES_OpenFile`/`ReadFile`/`CloseFile`/
`GetFilePointer`/`SetFilePointer`, `HeapAlloc_w`/`HeapFree_w`, `CRT_calloc`
0x004a020e, `realloc` 0x0049fca2.

## Extern-type divergences (recorded, NOT aligned)

- **`MakeShadedColour` is declared `int` in person3d.c and savegame2.c and
  `Shade*` in tri3d.c (its definition).** This file uses tri3d.c's `Shade*`
  because the result is stored straight into `tex->ramps[c]`, a `Shade**`.
  Inert for codegen here (both are dword returns), but the pointer spelling
  is the true one.
- `RES_CloseFile` is `void` here and in screen.c, `int` in rin.c;
  `RES_SetFilePointer` is `int` here and in screen.c, `void` in loadmap.c.
  Both inert at these call sites (nothing consumes the result); left as each
  file has them.
- `DetailImage_AllocSlot` is defined here exactly as tinystubs.c declares it
  (`void** (void)`), so the caller's spelling needed no change.

## Levers, with evidence

- **`if (p == 0) return 0;` on a just-loaded pointer emits the bare inline
  `ret`; `if (p) { ... } return 0;` EXILES the failure arm past it.**
  `ObjNextRider` written the first way is 13 instructions with `jne`/`ret`
  inline at index 2-3; written the second way it is the original's
  `je <end> / ... / ret / xor eax,eax / ret` — 14/14 exact. The recorded
  "bare ret" rule read from the other side.
- **The counter's update must be generated BEFORE the other induction
  variable's, and putting the pointer step in the for-INCREMENT after `i++`
  is how.** `FindDetailImageSlot` with `p++` as the last body statement gives
  `add eax,4 / inc ecx`; `for (i = 0; i < cap; i++, p++)` gives the
  original's `inc ecx / add eax,4`. Same 16 instructions either way — a pure
  two-instruction scheduling residual, and the only fix. (Third confirmation
  of the `RecolourModelParts` / `RES_FindVolumeOnAnyDrive` entry.)
- **A global read by BOTH arms of an if/else must NOT be a named local read
  at the top — but it must not be spelled twice either.** `DetailImage_AllocSlot`
  reads `g_detail_images` in the grow arm (`if (list == 0)`, `realloc(list,
  ..)`) and as the search cursor. Three spellings, three results:
  - `void** list = g_detail_images;` initialised at the TOP: the load
    schedules ABOVE the `count == cap` compare (`mov ecx,count / mov eax,list
    / cmp ecx,edx`), 3 mismatches — the original loads it BETWEEN the compare
    and the branch, reusing the count's register.
  - `g_detail_images` spelled directly in both arms and the search walked by
    SUBSCRIPT: two separate loads, `esi` taken for the base, `edi` pushed in
    the prologue instead of only around the `rep stosd`, and a trailing
    `lea eax,[esi+eax*4]` — 39 mismatches and ESCAPES.
  - `g_detail_images` spelled directly in the grow arm, and
    `list = g_detail_images;` as a statement immediately before the search
    loop: VC6 CSEs the two into one load and schedules it after the compare —
    50/50 exact. **The read has to be a STATEMENT at the point the second arm
    needs it, not an initialiser.**
- **A pointer-walk search and a subscript search are different functions.**
  In the same body, `for (i...; i++, p++) if (*p == 0) return p;` returns the
  cursor in eax and needs no base register; `for (i...) if (list[i] == 0)
  return &list[i];` keeps the base in `esi`, the index in eax and pays a
  `lea` at the return — which is what pushed `edi` into the prologue and
  broke the `rep stosd`'s local `push edi`/`pop edi` bracket. **A stray
  callee-saved push around an intrinsic is a register-pressure symptom from
  the OTHER end of the function.**
- **`grown + cap - 0x80` after the update, not `grown + oldcap` before it.**
  The original's `lea eax,[edx+eax*4-0x200]` uses the ALREADY-INCREMENTED cap
  register, so the source reads the global back after `cap += 0x80`:
  `g_detail_images_cap += 0x80; return &g_detail_images[g_detail_images_cap -
  0x80];`. Reading an `oldcap` local instead would keep a second register
  live.
- **A nine-case power-of-two switch lowers to a three-way split, and ascending
  case order is the block order.** `switch (img->w)` with cases 1,2,4,8,16,
  32,64,128,256 gives `cmp 0x10 / jg <high> / je <case16> / dec / cmp 7 / ja
  <default> / jmp [8-entry table]` for the low cluster and, for the high one,
  `add eax,-0x20 / cmp 0xe0 / ja / movzx from a 0xe1-BYTE index table / jmp
  [5-entry table]`. Both switches in `BuildTextureRecord` came out exact on
  the first compile from plain ascending `case` order — **136 instructions
  including two jump tables, two byte tables and the nested copy loop, exact
  first try.** Worth remembering that a byte index table of 0xe1 entries is
  VC6's own choice for a sparse high cluster, not evidence of a source table.
- **A byte local homed in a dead argument slot is read back as a DWORD plus
  `and 0xff`, not `movzx`.** `BuildTextureRecord`'s `unsigned char c` is
  stored with `mov byte ptr [esp+0x18], cl` and read with
  `mov esi, dword ptr [esp+0x18] / and esi, 0xff` — the top three bytes are
  the stale high bytes of the `tex` argument. Plain `unsigned char c;`
  produces it; no cast or mask is needed in the source. (VC6 avoids `movzx`
  from memory on the Pentium schedule.) Both of that function's loop
  variables live in the two dead argument slots: `y` in arg1's home, `c` in
  arg2's.
- **`img->w * y + x` spelled at BOTH uses recomputes the row base per pixel,
  which is what the original does.** `imul eax, edx` sits INSIDE the inner
  loop with `eax` carrying `img->w` down from the loop condition's `movsx`.
  Naming a row pointer would have hoisted it; the naive spelling is the
  matching one here.
- **`p = row;` as its own cursor, or VC6 addresses the source as `[row+x]`.**
  In `ConvertSourceImage`'s copy, `*dst++ = row[x]` leaves `row`
  loop-invariant and emits `mov dl, byte ptr [eax+ebp]` — a two-register
  addressing mode, x in eax and row in ebp, the reverse of the original's
  allocation. Assigning `p = row;` at the top of the row body coalesces `p`
  into row's register and gives the original `mov dl,[eax] / inc eax`, with
  x back in edi and dst in ebp. **`p` coalescing into `row` is also why the
  row home is written once per ROW and never inside the copy** — the store
  before the inner loop is `row`, the increments are `p`.
- **`*dst++ = *p++` and `*dst = *p++; dst++;` are different.** The combined
  form emits load / store / `inc dst` / ... / `inc p`; splitting the
  destination's step into its own statement gives the original's load /
  `inc p` / store / `inc dst`, and moves the dst home store back above the
  latch compare. Nine instructions, one statement split.
- **Write a two-call tail TWICE when both edges of an `if` need it.** The
  shared `HeapFree_w(raw); RES_CloseFile(f); return 1;` written ONCE after
  the `if` merges the two `push raw` sites into a single reload from the
  home slot — 190 instructions for the original's 192. Written inside the
  `if` AND after it, VC6 keeps one copy of the two calls and one `push` per
  predecessor: `mov ecx,[home] / push ecx / jmp` for the arm that clobbered
  edi in the loop, and a bare `push edi` for the arm that still has `raw`
  there. **The recorded tail-duplication threshold ("two calls are jumped
  to") predicts the shape; what it does not say is that the DUPLICATED source
  is what produces the per-predecessor argument push.** Two calls plus an
  argument block, third measurement.
- **A `BITMAPFILEHEADER` needs `#pragma pack(2)`**: `bfOffBits` is read at
  `+0x0a`, which the default `/Zp8` layout puts at `+0x0c`. The packed struct
  is 14 bytes and the next aggregate still lands 4-aligned, which is what
  puts the two headers at frame `+0x24` and `+0x34`. Taken verbatim from
  screen.c.
- **A nested `RES_OpenFile(GetGFXFName(...))` MERGES the cdecl cleanup**
  (`add esp,0x10` for a 3-argument and a 1-argument call). The recorded rule
  ("a call result passed straight into another call splits the add esp —
  store it in a local to merge") is the other way round for a result that is
  the ONLY argument of the outer call: here the nested spelling is what
  merges. Same shape as screen.c's `path` local, read from the other side.
- **An address-taken struct defeats VC6's constant folding of a repeated
  test.** `bmi.bmiHeader.biBitCount == 8` is tested twice with calls in
  between; because `&bmi` was passed to `RES_ReadFile` the second test is
  reloaded and re-branched instead of folded. Reproducing the original's
  redundant test cost nothing — but note that if the intervening calls were
  removed VC6 WOULD fold it, and three instructions would vanish.

---

# pathmask.c

## Scope K — `LEGOLAND/pathmask.c` (path edge/corner masks, cursor tiles)

New-file lane. `python3 tools/audit.py LEGOLAND/pathmask.c` ends **PASS**;
`/W3` is clean. **5 of 6 exact, 422 of 422 instructions ported, 352 of them
byte-exact.**

## Per function

| address | name | insns | pct | audit | promotable | marker committed |
| --- | --- | --- | --- | --- | --- | --- |
| 0x00482a40 | `ResolveEntrancePathSquare` | 20 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00482a40` |
| 0x00455ee0 | `FreeCachedTextEntry` | 41 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00455ee0` |
| 0x00461080 | `DrawCursorTileAt` | 43 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x00461080` |
| 0x0045c900 | `IsPathRectClear` | 70 | see below | `[WIP]` | n/a | `// WIP-FUNCTION: LEGOLAND 0x0045c900  (70i extent, 189B; ours 72i, first diverging index 5; STRUCTURAL — one derived induction variable over the row table; at its floor, see the note above)` |
| 0x0045d080 | `PathCornerMask` | 99 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x0045d080` |
| 0x0045ceb0 | `PathEdgeMask` | 149 | 100% | `[OK]` | yes | `// FUNCTION: LEGOLAND 0x0045ceb0` |

Every name came from the callers' existing extern declarations
(`render5.c`, `pathmisc2.c`, `tinystubs.c`); nothing was renamed. All five
exact bodies end in a real `ret` and none is recursive.

### `IsPathRectClear` — retired at its floor

`audit.py` reports `mismatch=58` because it compares index-for-index and our
body carries two extra instructions from index 5 onwards. Diff-aligned the
picture is much smaller:

| measure | orig | ours | matched | mismatch |
| --- | --- | --- | --- | --- |
| strict | 70 | 72 | 44 | 28 |
| register-blind | 70 | 72 | 55 | 17 |
| register + immediate blind | 70 | 72 | 67 | 5 |

The block layout, both loop tests, both epilogues, the frame (with `bottom`
homed in the dead `rect` argument slot), the `x * 0x14` outer induction
variable and the `test edx,edx` form of the `x >= 0` test are all the
original's. The ONE difference is where the row table's base lives:

```
original   outer preheader:  mov ebx,[0x801400]          ; g_map_rows, kept in EBX
           inner body:       mov esi,[ebx+eax*4]
ours       inner preheader:  mov edx,[g_map_rows] / lea edx,[edx+eax*4]
           inner body:       mov esi,[edx]
           inner latch:      add edx,4
```

VC6 builds a derived induction variable over the row pointers, which costs the
two extra instructions and takes the register the original spends on the base
— which is why `x` lands in EBX for us and in EBP there, and the rest of the
allocation follows.

**Ruled out** (every one still 72 or 73 instructions, all diverging at index
5): a `Cell** rows = g_map_rows;` function-scope local (copy-propagated
straight back to the global), the same local scoped inside the outer loop body
and inside the inner loop body, `*(rows[y] + x)`, `(char*)rows[y] + x*20`,
`(char*)g_map_rows[y] + x*sizeof(Cell)`, `(*(Cell**)((char*)g_map_rows +
y*4))[x]`, a named `Cell* row` and a named `Cell* c`, a manually spelled
`xoff` outer IV with the `x >= 0` test written on it, `px`/`py` copies of the
loop counters, `while` loops, a `do`/`while` outer with the guard split out
(74), `cell` declared in the inner scope, dropping the dead `tile = 0` store,
both orders of the two failure tests, both orders of the else stores, the
bounds test with `y` first, and the whole probe as a `static __inline` helper
taking a `Cell*` out-parameter. A `*(Cell** volatile*)&g_map_rows` read is
NOT free here — it anchors above the loop guard and takes a frame slot (75–76
instructions) — and a free volatile read of `g_map` inside the probe (the
original reloads it every iteration, so it costs nothing) does not act as a
barrier either: VC6 still schedules the `g_map_rows` load above it.

The residual is one code-motion decision, not a spelling. Reopen only with a
construct that hoists a global load to the OUTER preheader while leaving the
inner loop's index unreduced.

## Mechanics recovered

- **The four-neighbour path mask.** `PathEdgeMask` probes N, E, S, W in that
  order and ORs `0x1 / 0x2 / 0x4 / 0x8`. On the isometric grid `0x3` (N+E) and
  `0xc` (S+W) are the two DIAGONAL pairs, which is what `render5.c`'s header
  means by "one diagonal pair / the other". The mask indexes
  `g_tile_sprites[code + mask + 3]`.
- **The corner filler is a HOLE test.** `PathCornerMask` answers bit 0 when the
  `0xc` pair is complete *and* `(x-1, y+1)` is not path, bit 1 when the `0x3`
  pair is complete *and* `(x+1, y-1)` is not path. So an inside corner gets a
  filler only where the diagonal cell behind it is missing; the three
  combinations pick `g_tile_sprites[code + 19 / 20 / 21]`.
- **The probe is the same one `simcore.c`'s `GetPathNeighbours` uses**: the cell
  is COPIED (5-dword `rep movsd`) into a function-level local and an off-map
  cell stands in as `tile = 0`, `flags = 0x40`, `rf = 0`, so anything off the
  map is never path. Three functions here spell that same probe; see the lever
  on dead-store elimination below.
- **`IsPathRectClear` is NOT `IsPathCell`.** Every cell of the inclusive
  rectangle must carry map flag `0x10` (a path tile) and must NOT carry RF bit
  1 (blocked); RF bit 0 does not rescue a cell the way it does in `IsPathCell`.
  An empty rectangle answers 1. `x` is the outer loop, `y` the inner.
- **`DrawCursorTileAt` is the cursor's per-cell filter**: bounds-check,
  `IsPathCell`, and a non-zero displayed tile; the survivor goes to the
  four-argument overlay twin 0x00460f50 with the caller's blit mode, and it is
  the CALLER'S `Pos` that is handed on, not the fetched cell.
- **The rendered-text cache compacts on removal.** `FreeCachedTextEntry`
  traces `"Deleting Cell (%d) %s\n"`, decrements the count FIRST, frees the
  text, kills the sprite (and nulls the field), then slides
  `g_text_cache[j] = g_text_cache[j+1]` up to the NEW count — leaving the last
  slot a stale duplicate. That is exactly why `render5.c`'s `ExpireCachedText`
  does not advance its index on the free arm and re-reads the count every
  iteration.
- **`ResolveEntrancePathSquare` is a flood fill's reset + seed.** It clears
  flag 2 on every square of `pathsq.c`'s list (head 0x0066b44c), then finds the
  square containing the entrance tile and hands it to 0x004829c0, which sets
  flag 2 and recurses over the neighbour set 0x004819a0 collects. Flag 2 is
  therefore "reachable from the park entrance".

## Callees / globals named for the first time

| address | name given | contract | insns |
| --- | --- | --- | --- |
| 0x004829c0 | `MarkPathSquareReachable(PathSquare* sq)` | sets `flags |= 2`, collects the square's neighbours through 0x004819a0, copies the neighbour array to the heap and recurses into every square that does not carry the flag yet; frees the copy | 52 |
| 0x00460f50 | `DrawCursorPathTile(Pos* at, int x, int y, int mode)` | `render5.c`'s `DrawPathTileOverlay` with the corner draws routed through the five-argument `PrintSprite`; declared here with the four arguments the disassembly pushes | (not in this scope) |

Names checked against the whole tree first; neither collides. Globals already
named elsewhere and reused unchanged: `g_path_squares` (0x0066b44c),
`g_text_cache` / `g_text_cache_count` (0x006675c0 / 0x006675b8), `g_map`,
`g_map_rows`. New string constant: `kFreeTextFmt` (0x004b9098,
`"Deleting Cell (%d) %s\n"`).

## Original bugs reproduced

None found in these six. `FreeCachedTextEntry`'s "stale last slot" is a
consequence of decrementing the count before compacting, not a defect — the
entry beyond the count is never read.

## Extern-type divergences (deliberate — do not "align")

- **`g_text_cache_count` (0x006675b8) is declared `volatile int` in this
  file**, where `render5.c` declares it plain `int` and casts at the one read
  that needs it (`*(volatile int*)&g_text_cache_count`). Both TUs need the same
  barrier; the two files reach it from opposite sides. See the lever below.
- **`PathCornerMask`'s first parameter is `char`** here, matching `render5.c`'s
  declaration — the whole body is byte-wide (`mov bl,[esp+0x20] / mov al,bl /
  and al,0xc / cmp al,0xc`) and `edges` stays in EBX for the function.
  Declaring it `int` would make the caller mask before the push.
- **`DrawCursorPathTile` (0x00460f50) is declared with four arguments** —
  `render5.c` documents the same function as the four-argument twin of its own
  three-argument `DrawPathTileOverlay`.
- **`IsPathCell` (0x0045ce10) is `int IsPathCell(Cell*)`**, the same spelling
  `pathmisc2.c`, `pathtile2.c` and `render4.c` use.

## Levers, with evidence

- **A `volatile` DECLARATION on a counter global is worth 5 instructions and
  fixes two unrelated defects at once.** In `FreeCachedTextEntry` a plain
  `extern int g_text_cache_count` let VC6 (a) schedule the `.text` reload for
  the free ABOVE the count's read-modify-write (our indices 10–14 came out
  `load count / load text / dec / push / store count` against the original's
  `load count / dec / store count / load text / push`) and (b) hoist the
  compaction loop's bound out of the latch into a `count - i` trip count with a
  `dec/jne` down-count. Declaring the global `volatile int` costs NOTHING —
  the original emits exactly the load/dec/store and the two reloads — and
  closed both: 31/45 → 41/41. `render5.c`'s `ExpireCachedText` needed the same
  barrier at its one read and got it with a cast. **When two functions in
  different TUs share a counter global and both need its reloads, suspect the
  original's header declared it volatile.**
- **A `rep movsd` struct copy out of an ARRAY is not the same construct as one
  out of a POINTER.** `g_text_cache[j] = g_text_cache[j+1]` gives the
  original's single cursor (`lea esi,[eax+0x20] / mov edi,eax / rep movsd /
  add eax,0x20`); `p = &g_text_cache[i]; … *p = p[1]` with `p++` in the for
  increment manufactures a SECOND induction variable for the source pointer,
  needs a fourth callee-saved push for it, and lands at 45 instructions for the
  original's 41. The array form is also what lets VC6 prove the copy cannot
  alias the count (which is then the volatile's job to un-prove). The recorded
  "two lockstep cursors spelled the SAME way let VC6 eliminate an IV", measured
  from the losing side.
- **Whether the off-map stand-in cell's DEAD store survives tells you whether
  the copy's address is taken.** All three probes here are the same source
  macro (`cell.tile = 0; cell.flags = 0x40; cell.rf = 0;`), but
  `PathEdgeMask`/`PathCornerMask` emit all three stores while `IsPathRectClear`
  emits only two. The difference is that the first two pass `&cell` to the real
  `IsPathCell` call, so nothing about the local is dead, while
  `IsPathRectClear` reads the two flag bytes straight out of the local and VC6
  drops the unread `tile`. `simcore.c` records the same macro dropping only its
  LAST probe's store for the same reason. **Reading a dead-store pattern
  backwards identifies the callee: a probe that keeps every store is one whose
  copy escapes.**
- **A probe macro's coordinate arguments must be pre-computed LOCALS, not
  expressions, or VC6 folds the ±1 into the addressing mode.** Writing
  `PROBE(at->x - 1, at->y + 1)` (the macro evaluating each argument three
  times) makes VC6 load `at->x` early, defer `at->y` to the second test, and
  fold both offsets into the copy's address —
  `lea esi,[eax+ecx*4-0x14]` and `mov ecx,[edx+ecx*4-4]`, 87 of 99 for
  `PathCornerMask`. Assigning `px = at->x - 1; py = at->y + 1;` first gives the
  original's `mov eax,[ebp] / mov ecx,[ebp+4] / dec eax / inc ecx` and closes
  the function outright. `simcore.c`'s `GetPathNeighbours` uses the same
  discipline for the same reason; this is the measurement of what happens when
  you don't.
- **VC6 schedules the MODIFIED coordinate's load first, whatever the source
  order.** In `PathEdgeMask` all four probes are written `px = …; py = …;` in
  that order, yet the original (and our exact match) loads `at->y` first in the
  N and S probes and `at->x` first in the E and W probes — always the one
  carrying the `inc`/`dec`. So the load order is NOT a source-order lever here
  and should not be chased; write the pair in coordinate order and let the
  scheduler pick.
- **Four repeats of one probe with a byte accumulator give three different
  bit-set forms, and all three are automatic.** `PathEdgeMask`'s first hit is
  `mov byte ptr [esp+0x13],1` (VC6 knows the mask is still zero), the middle
  two are `or byte ptr [esp+0x13],2/4` in memory, and the last is folded into
  the epilogue as `mov al,[esp+0x13] / pops / je / or al,8`. One `char mask`
  local plus four `mask |= bit;` statements produces all of it.
- **A zero register appears in a probe chain when a callee-saved register is
  pushed anyway and the function has no other use for it.** `PathEdgeMask` gets
  `xor ebx,ebx`, so every `>= 0` test is `cmp reg,ebx` and every off-map zero
  store is `bl`/`bx`; `PathCornerMask`, whose EBX holds the `edges` parameter
  for the whole body, emits the identical probe with immediates instead. Same
  source macro, two spellings, decided entirely by what else wants the
  register.
- **The lazy bounds-check inline (`CellForPos`) and the eager one (`MapCellAt`)
  are distinguishable in the listing.** `DrawCursorTileAt` reads `at->y` only
  AFTER `at->x` has passed its width test (`mov ecx,[edi+4]` at index 11, below
  `cmp eax,ecx / jge`), which is `pathmisc2.c`'s `CellForPos`; the eager form
  reads both up front. Picking the right one closed the function on the first
  compile.
- **`cell != 0` after that inline is a real third `&&` term.** The inline has
  already branched to the same exit on every out-of-bounds arm, so the merged
  `test esi,esi / je` looks redundant — but VC6 emits it, and dropping the term
  from the source loses an instruction. Three tests in the listing, three `&&`
  terms in the source.

---

# movie.c

## Scope K — `LEGOLAND/movie.c` (movie player + front-end teardown)

**Result: 16 of 16 exact.** `python3 tools/audit.py LEGOLAND/movie.c` ends
`PASS: 0 function(s) failed the extent gate`; `/W3 /O2 /Gy /Gd` compiles with
no diagnostics. Object prefix `/tmp/sk_movie_`. Nothing outside
`LEGOLAND/movie.c` and this file was created or edited.

| address | name | insns | bytes | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x00475fe0 | `SetMenuHelp` | 8 | 25 | [OK] | `// FUNCTION: LEGOLAND 0x00475fe0` |
| 0x0048ffb0 | `KillAdvertScreenSprites` | 11 | 37 | [OK] | `// FUNCTION: LEGOLAND 0x0048ffb0` |
| 0x00473130 | `CloseInfoPopUpIfOpen` | 13 | 34 | [OK] | `// FUNCTION: LEGOLAND 0x00473130` |
| 0x00458940 | `EnterParkPlayMode` (was `sub_458940`) | 16 | 81 | [OK] | `// FUNCTION: LEGOLAND 0x00458940` |
| 0x00473160 | `ClosePrimaryPopUp` | 18 | 50 | [OK] | `// FUNCTION: LEGOLAND 0x00473160` |
| 0x00490270 | `KillCertScreenSprites` | 18 | 65 | [OK] | `// FUNCTION: LEGOLAND 0x00490270` |
| 0x0047afb0 | `LoadLevelDatabase` | 20 | 58 | [OK] | `// FUNCTION: LEGOLAND 0x0047afb0` |
| 0x0048fa40 | `RestoreFrontEndState` | 20 | 83 | [OK] | `// FUNCTION: LEGOLAND 0x0048fa40` |
| 0x004907a0 | `LoadHelpTextFor` | 26 | 94 | [OK] | `// FUNCTION: LEGOLAND 0x004907a0` |
| 0x0048ab60 | `UnlockSidePanelObjects` (was `sub_48ab60`) | 27 | 71 | [OK] | `// FUNCTION: LEGOLAND 0x0048ab60` |
| 0x00476630 | `CloseMovie` | 27 | 73 | [OK] | `// FUNCTION: LEGOLAND 0x00476630` |
| 0x004911c0 | `SetInfoPanelText` | 32 | 119 | [OK] | `// FUNCTION: LEGOLAND 0x004911c0` |
| 0x00490fa0 | `PrintCursor` | 84 | 224 | [OK] | `// FUNCTION: LEGOLAND 0x00490fa0` |
| 0x004989b0 | `RewindNarrationBuffer` | 98 | 324 | [OK] | `// FUNCTION: LEGOLAND 0x004989b0` |
| 0x00476460 | `OpenMovie` | 142 | 454 | [OK] | `// FUNCTION: LEGOLAND 0x00476460` |
| 0x004766f0 | `RunMovie` | 170 | 531 | [OK] | `// FUNCTION: LEGOLAND 0x004766f0` |

All sixteen end in a real `ret` and none is recursive, so all sixteen are
promotable and are committed as `// FUNCTION:`. `CloseMovie` contains an
internal tail `jmp` to `AVIFileExit`, but the `jne` two instructions earlier
crosses it, so the extent walker bounds it correctly at the following `ret`.

## Names settled

- `sub_458940` -> **`EnterParkPlayMode`**: clears `g_edit_changed`, sets
  `g_game_mode = 3` (a running park), installs the in-game icon handlers,
  drops `g_edit_object`, resets the edit cursor and clears the UI's two
  "front-end screen" bits while setting 0x20. Callers `StartFreePlayPark`
  (uimisc2.c) and 0x00458acf; both spell it `sub_458940` — left alone.
- `sub_48ab60` -> **`UnlockSidePanelObjects`**: walks the side-panel icon list
  and, for every icon of kind 1 whose named object class loads, marks that
  class available in the build menu. `progress_tick()` per icon drives the
  loading bar.

## First-named callees and globals

| address | name given | what it is |
| --- | --- | --- |
| 0x00492980 | `SetSfxPaused` | 2i — `g_sfx_paused = 1` (audio3.c names the global) |
| 0x00492990 | `ClearSfxPaused` | 2i — `g_sfx_paused = 0` |
| 0x004784c0 | `ResetLevelGlobals` | ~60i — clears the per-level state block before a load |
| 0x004781f0 | `ParseKeywordFile` | ~30i — opens a resource text file and dispatches its `[SECTION]` keywords through a table |
| 0x00490740 | `SetHelpTextPrefix` | 13i — `_splitpath` the key, `sprintf(0x7cae80, "%s_", fname)` |
| 0x00490680 | `LoadTextFileLines` | ~90i — reads a resource text file into a `char*` table, returns the line count |
| 0x00490800 | `LoadHintTextFor` | 26i — `LoadHelpTextFor`'s twin for the hint table (0x00490770 / 0x00490880 / cap 0x20) |
| 0x004983a0 | `ReadDecodedNarration` | ~35i — drains up to n bytes of decoded PCM out of the 0x20000-byte ring at 0x007aaca0 |
| 0x00478b20 | `EnsureObjectClassLoaded` | ~18i — `ElemID` + `LoadObjectClass`, sets flag 4, returns 1 on success |
| 0x00469900 | `MarkElemAvailable` | ~31i — flips an element's +0x08 flags into the build menu |
| 0x00476910 | `StartMovieAudio` | large — opens a KLIBAUDIO buffer for `mv->audio`; non-zero when there is one |
| 0x00476bf0 | `PrimeMovieAudio` | ~40i — first fill, at the first displayed frame |
| 0x00476d20 | `UpdateMovieAudio` | large — per-frame-step top-up, takes (frame, prev) |
| 0x00476c90 | `StopMovieAudio` | ~35i — teardown |
| 0x00476680 | `MovieTicks` | 20i — ms clock: QueryPerformanceCounter where available, GetTickCount otherwise; the mode is latched in 0x00668fac |
| 0x00465850 | `BlitDIBToScreen` | large — blits a BITMAPINFOHEADER+bits frame |

Globals named here for the first time: `g_menu_help[4]` (0x004bb18c),
`g_avi_open_count` (0x00668f98), `g_movie_bmi` (0x004bb4e0, a
BITMAPINFOHEADER), `g_movie_audio_scale` (0x00668fa4),
`g_speech_fill_block` (0x0079a840), `g_speech_blocks_ready` (0x0079a844),
`g_level_db_sections` (0x004bb6f8, 93 {keyword, handler} pairs).
`g_key_state[0x9d] | g_key_state[0x1d]` is "either Ctrl" and `[0x10]` is
DIK_Q, which is how the attract-mode movie is aborted; `[0x39]` (DIK_SPACE)
aborts the in-game one.

## Original bugs reproduced (commented at the site)

- **`RestoreFrontEndState` (0x0048fa40).** `g_front.mode` is latched at entry
  and written back over `g_front.screen` AFTER the whole saved block has been
  copied in, so the SAVED screen index is discarded and the screen index
  becomes whatever sub-mode was current when the pop ran. The saved sub-mode
  is then restored correctly, so the pair ends up mismatched.
- **`CloseMovie` (0x00476630).** The two AVI streams and the GETFRAME handle
  are released but `mv->file` never is — one `PAVIFILE` leaks per movie
  played. `g_avi_open_count` is still decremented and `AVIFileExit()` called
  at zero, so the leak is only reclaimed by AVIFile's own teardown.
- **`RunMovie` (0x004766f0).** On the last pass of a movie the prefetch is
  skipped (`cur + 1 >= mv->frames`) but `frame` is still set to `cur + 1`,
  leaving a stale `next`. Harmless only because the loop exits on the same
  pass.
- **`OpenMovie` (0x00476460).** The stream walk keeps the LAST `vids` and the
  LAST `auds` stream it sees, overwriting (and leaking a reference on) any
  earlier one; and the result of `AVIFileInfoA` is never checked —
  `fi.dwStreams = 0;` before the call is the only guard.

## Extern-type divergences (noted, not "aligned")

- `LoadLevelDatabase` is defined here as `int` returning 0 or 2. `uimisc2.c`
  declares it `void* LoadLevelDatabase(const char*)` and stores the result in
  `g_freeplay_db` — so the "database pointer" is in fact a 0/2 status code.
  Left alone.
- `uimisc2.c` declares `int RunMovie(void*, WinRect*, int)` and
  `void* OpenMovie(const char*)` on an opaque handle; this file gives them the
  real `Movie*` (0x28 bytes). `PrintCursor` is declared `void` in both
  screens2.c and uimisc2.c and defined `void` here — no divergence.
- `PU_CloseInput` / `PU_NextInput` are declared here with FOUR parameters
  (`Icon*, int, short, short`); misc3.c has a two-parameter spelling of
  `PU_NextInput`. Both are cdecl, so the divergence is invisible at the call.
- `sub_458940` / `sub_48ab60` keep their placeholder names in `uimisc2.c`; the
  real names live only here.

## Levers, with evidence

- **Two globals that are copied as a unit must be ONE struct, and a 12-byte
  struct assignment is what STOPS dead-store elimination.**
  `RestoreFrontEndState` writes `g_cur_screen` twice, the second write
  clobbering the first. Written as six scalar assignments VC6 deletes the
  first store (17 instructions for the original's 20) — a plain global store
  followed by another to the same global is dead. Written as
  `g_edit = *game; g_front = *screen; g_front.screen = mode;` — two 12-byte
  struct assignments over the contiguous triples at 0x008119b0 and 0x0080ff80
  — the copy is lowered too late for DSE to see through it, all three stores
  survive, the bug store schedules into the middle of the copy, and the latch
  of `g_front.mode` is hoisted to the top of the function because it must
  precede the copy that overwrites it. 20/20 exact, first try in that form.
  (Nine other statement orders of the scalar spelling floored at 20 mismatch.)
- **`cond ? 0 : K` is `setcc / dec / and K`; the polarity of the condition is
  the whole residual.** `LoadLevelDatabase`: `(rc >= 0) ? 0 : 2` gives
  `setl / and al,0xfe / add eax,2` (3 wrong, 59B for 58B);
  `(rc < 0) ? 2 : 0`, `((rc >= 0) - 1) & 2` and
  `if (rc < 0) return 2; return 0;` are all byte-identical to the original
  `setge / dec / and eax,2`. `(rc >> 31) & 2` is a different lowering
  (`sar/and`, 4 wrong).
- **Two leading guards that return the same constant merge into ONE trailing
  block only when they are ONE `if`.** `RewindNarrationBuffer`: two separate
  `if (...) return 0;` give two inline epilogues (`add esp,0x1008 / ret`
  twice, 8 wrong); `goto` on both merges only ONE of them (the second still
  gets an inline copy). All four of
  `if (a == 2 || a == 0) return 0;`, `if (a != 2 && a != 0) { ...; return 1; }
  return 0;`, `if (a == 2 || !a) goto idle;` and the nested-`if` form give the
  original's single trailing `xor eax,eax / add esp,0x1008 / ret`. The `||`
  spelling was kept.
- **A shared failure tail is cross-jumped only when there are THREE textual
  copies, not one shared `goto` and not two.** `OpenMovie` has three failure
  arms (file will not open / no video stream / allocation failed), all ending
  `AVIFileRelease; if (!g_avi_open_count) AVIFileExit(); return 0;`. Written
  as one copy plus two `goto`s, the early arm is exiled with no compare of its
  own (125 mismatch); written as two copies, the early arm keeps a full
  10-instruction inline epilogue (115). Written out THREE times, VC6 merges
  the suffixes exactly as the original does — the early arm keeps only its own
  `cmp [g_avi_open_count], ebx` and jumps INTO the later copy's `jne`,
  borrowing its own flags, and the two release arms merge at the shared
  `call AVIFileRelease` with the `push` duplicated in each. That single change
  took `OpenMovie` from 115 mismatches to 74.
- **A store to a global kills the CSE of a load through a pointer.** In both
  `OpenMovie` and `RunMovie` the width/height are read once into locals and
  the product formed from the locals; reading `mv->width`/`mv->height` again
  inside the product costs two extra loads, because the intervening stores to
  `g_movie_bmi` may alias `*mv`. Same rule as DECOMP's "a store to ANY global
  kills CSE of an unrelated global load", seen from the pointer side.
- **The assignment ORDER inside a chained assignment decides everything.**
  `RunMovie`'s frame fetch is `frame = (int)(next = AVIStreamGetFrame(...))`
  (next assigned first). Spelled the other way round,
  `next = (void*)(frame = (int)AVIStreamGetFrame(...))`, VC6 rotates the loop,
  reloads `frame` in a new preheader and spills it — 123 mismatch against 6.
  Two spellings of the same C, 117 instructions apart.
- **An overloaded sentinel variable is what wins the fourth callee-saved
  register.** `RunMovie` has four loop-carried values and four callee-saved
  registers. With one `void* frame` variable, `shown` takes ebx and `frame`
  goes to the frame (123 mismatch, every one of them an allocation
  consequence; the instruction sequence was otherwise identical). Splitting it
  into `int frame` (a -1 sentinel that also carries the prefetched frame's
  INDEX) plus `void* next` (the pointer, homed in the dead `mv` argument slot)
  puts `frame` in ebx and `shown` in the frame — the original's allocation.
  Making `shown` `volatile` also moves `frame` into ebx (108) but costs an
  extra load at every read; the split is free.
- **DECLARATION ORDER decides the callee-saved RANKING when several locals are
  initialised at entry** — a real exception to "declaration order is
  irrelevant", which was measured on spill SLOTS. All 24 orders of
  `{frame, cur, start, shown}` in `RunMovie` were compiled: six give the
  original exactly (every order that puts `frame` first and `start` not
  second, plus `shown, frame, cur, start`), six cost 2, five cost 5 and seven
  cost 6. The cost is always the same thing: which zero-valued initialiser
  becomes the shared zero register the two entry guards compare against
  (`cmp edi, ebp` against `test edi, edi`).
- **The statement order of four independent assignments decides which one is
  spilled.** `OpenMovie`'s `vids` arm assigns `frames`, `fps`, `w`, `h`; the
  original keeps `frames`, `w` and `h` in ebp/esi/edi and spills `fps`. All 24
  orders were compiled: `frames` first gives 2 mismatch, `fps` first gives 74.
  All four uninitialised locals share ONE frame home (the loop preheader loads
  ebp, esi and edi from the same slot), which is the documented
  "uninitialised locals coalesce into one home" pattern seen three ways at
  once.
- **`mv->file = pfile;` must be written BEFORE `mv->getframe = 0;`.** The
  emitted store order is the same either way (VC6 reorders adjacent stores)
  but the LOAD of `pfile` moves: written after, the load lands between the two
  stores instead of before them (2 mismatch). Eight fill orders measured.
- **A `for` loop over a GLOBAL counter keeps its zero-trip guard** because the
  intervening COM call may write the global: `RewindNarrationBuffer`'s
  `g_speech_fill_block = 0; Lock(...); for (; g_speech_fill_block < 10; ...)`
  emits `cmp dword ptr [mem], 0xa / jge` before the loop and reloads the
  counter three times per pass. The two callee-saved pushes for the inlined
  `memcpy`/`memset` sink past that guard on their own.
- **`PrintCursor` is `NewPrintCent` (text.c, 0x00491d60) with the DrawText
  flags changed from 0x25 to 0x24** — index for index the same body, 84/84 on
  the first compile. The twin is real: DT_CENTER dropped, DT_VCENTER |
  DT_SINGLELINE kept, so the text is left-aligned in the caller's box. VC6
  hoists `__imp__SelectObject` into esi for its three calls with no construct,
  and homes `oldfont` in the dead `white` argument slot.
- **AVIFile and ACM entry points are declared WITHOUT `__declspec(dllimport)`**
  so the compiler emits `call <thunk>`; `__declspec(dllimport)` would give
  `call dword ptr [__imp__...]`, which is what the GDI calls in `PrintCursor`
  need. Both spellings appear in this one file, three lines apart.

## Mechanics recovered (runtime spec)

- **The movie player is Video for Windows' AVIFile API**, not MCI or
  DirectShow. `OpenMovie` = `AVIFileInit` (once, refcounted in
  `g_avi_open_count`) + `AVIFileOpenA` + `AVIFileInfoA` + a linear walk of
  `dwStreams` calling `AVIFileGetStream` / `AVIStreamInfoA`. The frame rate is
  the INTEGER quotient `dwRate / dwScale`, so a 29.97 Hz file plays at 29; the
  pixel size is `rcFrame` right-left / bottom-top, and the length is the video
  stream's `dwLength` in frames. Both kept streams are `AVIStreamAddRef`'d on
  top of the reference `AVIFileGetStream` already returned, which is what lets
  `CloseMovie` release them once each.
- **Decompression is to 16-bit RGB at the movie's native size**: the
  `BITMAPINFOHEADER` at 0x004bb4e0 gets `biBitCount = 16`,
  `biWidth`/`biHeight` from the movie and `biSizeImage = h * w * 2`, and that
  is handed to `AVIStreamGetFrameOpen`. `RunMovie`'s `dst` rectangle is never
  read, so uimisc2.c's fixed {0,0,320,240} is decoration — the frame lands
  wherever `BlitDIBToScreen` puts it.
- **Playback is clock-driven and drops frames.** Every pass recomputes
  `frame = (now - start) * fps / 1000` from `MovieTicks()` (QPC where the
  machine has one, GetTickCount otherwise) and jumps straight to that frame;
  the clock starts at the FIRST displayed frame, not at open, and the audio is
  primed at the same moment. When the clock has not moved on, the next frame
  is decompressed ahead of time; when it has, the prefetch is discarded. The
  catch-up wait is a spin on the clock, not a sleep, so the player pegs the
  CPU while ahead of schedule.
- **Two abort regimes.** With `flags` non-zero (the in-game path) the movie
  stops on a lost message pump, mouse-button bit 0, any of the three mouse
  buttons — which also latches `g_movie_shown` so the movie is not offered
  again — or DIK_SPACE. With `flags` zero (the attract path) only Ctrl+Q
  stops it. Either way the player then spins on
  `ProcessSystemEvents`/`ReadGameButtons` until the mouse-button bits 1 and 2
  are clear, so the aborting click is not delivered to the screen underneath.
- **The narration ring is ten 0x1000-byte blocks (0xa000 bytes) in ONE
  DirectSound buffer.** `RewindNarrationBuffer` stops the buffer, rewinds to
  0, locks the whole thing as a single region and fills it block by block
  through a 0x1000-byte stack staging buffer, zero-padding the short final
  block to silence; `g_speech_blocks_ready` counts the blocks that carried any
  data. State 2 means "wound back and ready", 3 means playing; the function is
  a no-op in state 2 and in state 0 (nothing loaded).
- **The report/help text pipeline.** `LoadHelpTextFor(key)` sets a string-id
  prefix from the key's file name (`"<fname>_"`), frees the previous buffer,
  and loads `"Intervals\\<key>"` into the 0x64-entry table at 0x007cafa0. The
  stored line count is one LESS than the number of lines read whenever any
  were read — the first line of the file is a header — and that is also the
  result. `SetInfoPanelText(a, b)` loads the briefing text and, when there is
  one, the hint text; on success it puts the game in screen mode 7 with
  `g_cur_screen = -1` and un-greys the briefing icon, and on failure it greys
  it (flag 0x400) and changes nothing else.
- **Front-end state is pushed and popped as three blocks** at 0x007cb30c
  (the icon mode), 0x007cb300 (`{popup, screen, screen_mode}` = the triple at
  0x0080ff80) and 0x007cb2f0 (`{edit_changed, game_mode, edit_object}` = the
  triple at 0x008119b0). `PlayTitleMovie` (0x0048f9f0) takes the same three
  and is the push side.

## What is left in this file's neighbourhood

Nothing from the assigned table. The first-named callees above are all still
unmatched and are the natural next tier: `StartMovieAudio` (0x00476910),
`UpdateMovieAudio` (0x00476d20), `StopMovieAudio` (0x00476c90) and
`PrimeMovieAudio` (0x00476bf0) form one cluster (the movie's own audio
streaming, ~500 instructions, sharing the globals 0x00668ee0..0x00668fa4);
`LoadTextFileLines` (0x00490680), `LoadHintTextFor` (0x00490800),
`SetHelpTextPrefix` (0x00490740) and 0x00490770 form the report-text cluster;
`MovieTicks` (0x00476680) is 20 instructions and trivially reachable.
