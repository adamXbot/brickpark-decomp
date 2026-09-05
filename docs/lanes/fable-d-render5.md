# Lane `fable-d` / `LEGOLAND/render5.c` — cursor tiles, path overlay, text cache

**6 of 6 exact.** `python3 tools/audit.py LEGOLAND/render5.c` → `PASS`,
`/W3` clean. Object prefix `/tmp/fd_render5_`.

| address | name | insns | bytes | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x0045a3e0 | `TakeRenderNodeInColumn` | 28/28 (100%) | 77/77 | `[OK]` | `// FUNCTION: LEGOLAND 0x0045a3e0` |
| 0x00461020 | `FlushCursorSpriteList` | 29/29 (100%) | 94/94 | `[OK]` | `// FUNCTION: LEGOLAND 0x00461020` |
| 0x00455f70 | `ExpireCachedText` | 29/29 (100%) | 71/71 | `[OK]` | `// FUNCTION: LEGOLAND 0x00455f70` |
| 0x00451e20 | `SaveCertificateBitmap` | 29/29 (100%) | 74/74 | `[OK]` | `// FUNCTION: LEGOLAND 0x00451e20` |
| 0x00460e90 | `DrawPathTileOverlay` | 72/72 (100%) | 181/181 | `[OK]` | `// FUNCTION: LEGOLAND 0x00460e90` |
| 0x004610f0 | `PaintCursorTiles` | 106/106 (100%) | 304/304 | `[OK]` | `// FUNCTION: LEGOLAND 0x004610f0` |

No renames — every name is the one the scope assigned. All six end in a real
`ret`, none is recursive, so all six are promotable and promoted. Nothing is
left `WIP`, so there is no residual triage to report.

## Mechanics recovered

- **The build cursor path.** `render4.c`'s `DrawEditCursor` (0x00461220)
  calls `PaintCursorTiles`, which walks the diamond grid under the cursor
  rectangle and hands every visited map cell to 0x00461080; that routine
  bounds-checks the cell, keeps only path cells whose tile is non-zero, and
  passes the survivor to 0x00460f50 — the four-argument twin of this file's
  `DrawPathTileOverlay`, which paints STRAIGHT to the locked surface.
- **`FlushCursorSpriteList`'s deferred sprite queue is VESTIGIAL — nothing
  ever appends to it.** The queue is a flat 0x10-stride array of
  `{Sprite*, x, y, mode}` at **0x00801420**, an append cursor at
  **0x004b95f0** (initialised in `.data` to 0x00801420) and a count at
  **0x00667d44**; the call replays every record through `PrintSprite` with
  `ctx = 0` and resets both. A scan of the whole `.text` for the three
  4-byte constants finds references only at 0x00461022..0x00461078, i.e.
  inside this one function: the count is only ever READ and ZEROED. It lives
  in zero-filled data, so it is 0 for the life of the process, the loop never
  runs and both resets are no-ops. `renderview.c`'s `RenderView` still calls
  it between `DrawEditCursor` and `PopRenderingStatus`. The producer was
  presumably compiled out of the shipped build; the body is reproduced as it
  stands. **Anyone hunting the "cursor sprite list" producer should stop —
  there is not one in this executable.**
- **The isometric grid, read off `PaintCursorTiles`.** A tile is `w = 2h`
  pixels wide and `h` tall, `h` being the DEFAULT tile sprite's `+0x16` height
  (the same short arithmetic `pathbuild.c`'s `GetTileBounds` does). From the
  cell under `(x, y)`: the cell under `(x - w/2, y + h/2)` is `+1` in map y;
  the cell under `(x + w, y)` is `(+1, -1)`. So the inner loop paints **two**
  cells per full tile width and steps `(map.x + 1, map.y - 1)`; the outer loop
  restarts the row at `(row.x + 1, row.y + 1)` and steps screen y by one tile
  height. The inner bound is `clip->right + (short)(w+1)/2`, i.e. half a
  diamond past the clip, so the half-step column is never clipped away.
- **`PaintCursorTiles` MODIFIES THE CALLER'S `Pos`.** `p->x -= w` is written
  back through the pointer before `PlayfieldToMap`, so the caller's cursor
  origin moves one full tile width left. Reproduced as written and commented
  at the site; it is the caller's contract, not a bug, but it is invisible
  from the caller's declaration.
- **The path layer is base tile + corner filler.** 0x0045ceb0 answers a
  four-neighbour connection mask (bits `0x1`/`0x2` one diagonal pair,
  `0x4`/`0x8` the other) and that mask indexes the sixteen path tiles at
  `g_tile_sprites[code + 0 .. code + 15]`, where `code` is the FIRST DWORD of
  the loaded "path" tile record at 0x00832bf0 and the run base is `code + 3`.
  0x0045d080 then answers a 2-bit corner code — bit 0 when the `0xc` pair is
  complete but the cell diagonally beyond it is not a path, bit 1 for the
  `0x3` pair — and codes 1/2/3 select `g_tile_sprites[code + 19]`,
  `[code + 20]`, `[code + 21]`. Three corner sprites, never more than one
  drawn.
- **The render-node table's occupancy protocol.** `TakeRenderNodeInColumn`
  completes the model `render3.c` sketched: a node in the 0x1000-entry ring at
  0x00807f60 says "when the column scan next reaches column `x`, resume at row
  `y`". The lookup hands the row back in `p->y`, then clears `live` — the
  store order (`p->y` first, `live = 0` second) is visible in the original —
  and a column with no node parked restarts the scan at row 0.
- **The rendered-text cache expires on DETAIL GENERATIONS, not on a clock.**
  `ExpireCachedText(all)` (called once a frame by `render2.c`'s
  `RenderingComplete` with `all = 0`) tests
  `(unsigned)(g_detail - entry->sprite->detail) > 10`, and `sprite2.c`'s
  `NewSprite` stamps `SpriteRec::detail` (+0x0c) with `g_detail - 1` at
  creation. So an entry survives while the global detail counter has not moved
  more than ten steps past the value the text was rasterised at. **The compare
  is UNSIGNED**, so a counter that moves BACKWARDS expires the entire cache on
  the next sweep. 0x00455ee0 removes one entry by index and COMPACTS the
  array, which is why the index is not advanced on the free arm and why the
  count is re-read every iteration.
- **The certificate "print" (runtime-spec material).**
  `SaveCertificateBitmap` is `mapscreen2.c`'s screen-8 saver. It builds the
  CRT timestamp `asctime(localtime(&t))` — with `time(&t)` called first — and
  strips asctime's trailing `"\n"` in place with `s[strlen(s) - 1] = 0`, then
  calls the screen grabber 0x00451740 with **three** arguments:
  `("EGC.bmp", g_cert_message, stamp)`. 0x00451740 opens the file with
  `_open(path, 0x8000, 0x100)`, writes a 14-byte `BITMAPFILEHEADER` and a
  0x28-byte `BITMAPINFOHEADER` (`_write` calls of 0xe and 0x28), blits through
  `BitBlt` with `SRCCOPY` (0xcc0020) from a DC it creates, and formats the
  saved path into the message buffer. Answer is truthy on success;
  `SaveCertificateBitmap` narrows it with `!= 0`.

## Callees named for the first time

| address | name given | contract |
| --- | --- | --- |
| 0x0045ceb0 | `PathEdgeMask(Pos* at)` → `char` | four-neighbour path connection mask |
| 0x0045d080 | `PathCornerMask(char edges, Pos* at)` → `char` | 2-bit corner-filler code |
| 0x00485f00 | `PrintSpriteAt(Sprite*, int x, int y)` | thin wrapper: `PrintSprite(s, x, y, 0, 0)` |
| 0x00461080 | `DrawCursorTileAt(Pos* at, int x, int y, int mode)` | bounds-check + path/tile filter, then 0x00460f50 |
| 0x00460f50 | (twin of `DrawPathTileOverlay`, 4 args) | same body; corner draws go through the five-argument `PrintSprite` so the caller's blit mode is honoured, where 0x00460e90 uses `PrintSpriteAt` (mode 0) |
| 0x00451740 | `SaveScreenshotBmp(const char* path, char* msg, const char* stamp)` → `int` | screen grab to a 24-bpp .BMP, formats the saved path into `msg` |
| 0x00455ee0 | `FreeCachedTextEntry(int i)` | drop text-cache entry `i` and compact |
| 0x0049fbc6 / 0x0049fa66 / 0x0049f990 | `time` / `localtime` / `asctime` | CRT (above 0x0049e000, never targets) |

New globals: `g_cursor_sprites[]` (0x00801420, stride 0x10),
`g_cursor_sprite_put` (0x004b95f0), `g_cursor_sprite_count` (0x00667d44).

## Extern-type divergences (deliberate, do not "align")

- **`PlayfieldToMap` (0x0045a970) is declared `Pos PlayfieldToMap(int, int)`
  here**, where `objmap.c` defines it returning its own 8-byte `Offset`. The
  pair is a map coordinate in this file and is handed straight to
  `GetTileBounds(Pos*, …)`; both are `{int,int}` so the ABI is identical.
- **`g_path_tile` (0x00832bf0) is a `PathTileRec*` whose leading tile code is
  read as a DWORD** (`mov eax,[0x832bf0] / mov ecx,[eax]` + a 32-bit `add`).
  `maprestore.c` and `pathbuild.c` read the same word 16 bits wide,
  `sysmisc3.c` declares `unsigned short*` and `pathsq.c` `int*`. Four
  spellings of one global across the tree; this file needs the dword one.
- **`PathCornerMask`'s first parameter is `char`, not `int`.** That is what
  makes the caller push the raw dword out of the byte local's home slot
  instead of masking first — see the lever below.

## Original bugs reproduced

None found in these six. The `p->x -= w` write-back in `PaintCursorTiles` is
a deliberate side effect on the caller's `Pos`, not a bug, and is commented at
the site. `SaveCertificateBitmap`'s `s[strlen(s) - 1] = 0` would underflow on
an empty string, but `asctime` never returns one.

## Levers, with evidence

- **`i++` written as a STATEMENT before a cursor advance, versus in the `for`
  increment, swaps two ALU ops and moves a store.** `FlushCursorSpriteList`
  as `for (i = 0; i < n; i++) { call; put++; }` emits
  `add eax,0x10 / inc esi / mov [put],eax / cmp esi,ecx`; as
  `while (i < n) { call; i++; put++; }` it emits the original's
  `inc esi / add eax,0x10 / cmp esi,ecx / mov [put],eax` — the store sinks
  BELOW the compare on its own once the two increments are in source order.
  2 of 29 at identical byte length; nothing else about the body changed. The
  recorded "a decrement in the for-increment vs the body reorders two ALU ops"
  rule, measured from the increment side, and with the store placement as a
  second, free consequence.
- **The free-volatile guard-load lever transfers between the two halves of one
  data structure.** `ExpireCachedText`'s
  `for (i = 0; i < *(volatile int*)&g_text_cache_count; )` is worth 3 of 29
  and is free (the original loads the count in both the guard and the latch),
  exactly as `fpui3.c`'s `FindCachedText` records for the same global. Without
  it VC6 (a) hoists the guard's load ABOVE `push esi` — a volatile access
  cannot cross the push's store — and (b) folds the latch's reload into
  `cmp esi,dword ptr [mem]` where the original materialises
  `mov eax,[mem] / cmp esi,eax`. **One volatile fixed both**: the second
  effect is not a separate lever, it is the same barrier forcing the load to
  be its own instruction. When a global is read by two functions that share a
  cache, expect the same lever on both.
- **A `char` local homed in a DEAD PARAMETER's slot is passed on as a RAW
  DWORD, and that is readable off the listing.** `DrawPathTileOverlay` stores
  `PathEdgeMask`'s answer with `mov byte ptr [esp+0x18],al` into the incoming
  `at` slot (once `at` has been root-copied into esi), then loads the WHOLE
  dword back with `mov esi,[esp+0x1c]` and pushes it as the next call's byte
  argument — the callee reads only `dl`, so the stale high bytes of the
  original pointer are harmless. The `& 0xff` needed for the tile index is
  then a SEPARATE `and esi,0xff` after the call. Declaring the local `int`
  gives a dword store and no mask; declaring the callee's parameter `int`
  masks before the push. `char edges;` plus `extern char PathCornerMask(char,
  Pos*)` plus an explicit `(edges & 0xff)` in the index is the only
  combination that reproduces all three instructions. First measurement of the
  dead-parameter-slot rule where the SLOT'S STALE CONTENTS are visible in the
  argument.
- **A three-case `dec/je` chain lays its blocks in REVERSE source order, and
  the LAST case shares the function's epilogue.** `DrawPathTileOverlay`'s
  `switch (corner) { case 1: … case 2: … case 3: … }` emits case 3 inline
  after the chain, then case 2, then case 1 — and case 1's block falls
  through into the `pop/pop/pop/pop/ret` that the `default` arm also jumps to,
  while cases 3 and 2 each get their own tail-duplicated epilogue. Confirms
  the recorded case-order layout rule at a third case count, and adds the
  detail that the case laid LAST is the one that merges with the default exit.
- **An `int` local with the `(short)` cast at its USE site is how a dword
  store and a `movsx word ptr` load coexist on one slot.** `PaintCursorTiles`
  stores `(h+1)>>1` with `mov dword ptr [esp+0xc],eax` in the prologue and
  reads it back with `movsx edx, word ptr [esp+0x10]` inside the loop. A
  `short` local would store `mov word ptr`; an `int` local with no cast gives
  a plain `mov`. The same value kept in a register (`(w+1)>>1`, in esi) shows
  the cast as `movsx edi,si` in the loop preheader. This is `pathbuild.c`'s
  `GetTileBounds` spelling — `short h`, `short w = (short)(h + h)`,
  `(short)((w + 1) >> 1)` at the use — carried across to a second function
  that computes the same two half-steps, and it landed 106/106 first try. **A
  sibling that does the same geometric arithmetic is worth copying spelling
  for spelling before trying anything else.**
- **`mov ax,[mem] / lea ecx,[eax+eax] / movsx ecx,cx / movsx eax,ax` is a
  `short` pair, not an int one.** The doubling is done 32-bit on a register
  whose high half is stale and then narrowed, which only happens when the
  RESULT is a `short`. Read `short w = (short)(h + h);` off that sequence;
  `int w = h * 2;` gives a single `movsx` plus an `add`.
- **A twin found by callee overlap, not by size.** 0x00460f50 and
  `DrawPathTileOverlay` (0x00460e90) are adjacent and call the same three
  helpers in the same order; the only difference is that 0x00460f50 takes a
  fourth `mode` argument and spends it on the corner draws. The base-tile draw
  still goes through the mode-0 wrapper `PrintSpriteAt` in BOTH. Worth
  recording against the "size is not evidence of twinning" entry as the
  converse: a shared callee SEQUENCE is good evidence, and the diff was two
  arguments deep.
- Small negative worth recording: **`&arr[i]` subscript form was enough for
  both strength-reduced cursors in this file** (`g_render_nodes[i].x` anchors
  at +6 by the one-reference-each tie-break, `g_text_cache[i].sprite` anchors
  at +0x1c as the only field read). Neither needed a named or biased pointer,
  which the recorded const-table entry warns about — the biased-pointer form
  is for READ-ONLY tables walked by more than one field.

## Method notes

- Every function in this file landed at 100% within at most one iteration.
  The two that took a second build (`FlushCursorSpriteList`,
  `ExpireCachedText`) were both fixed by a lever already in `DECOMP.md`; the
  two largest (`DrawPathTileOverlay`, `PaintCursorTiles`) were exact on the
  first compile, because the context files named every callee and
  `pathbuild.c` supplied the exact short-arithmetic spelling.
- The side-by-side lister used is `scratchpad/sbs.py` (aligned by `difflib`
  over `match.py`'s `norm`); `matchfull.py`'s diff view elides matched lines
  and made a pure two-instruction SHIFT look like an insert/delete pair.
