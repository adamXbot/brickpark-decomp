# scope K — `LEGOLAND/texture.c` (texture and detail-image records)

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
